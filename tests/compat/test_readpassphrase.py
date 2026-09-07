"""Exercise the source archive with pipes and a real controlling terminal."""
import errno
import os
import select
import signal
import subprocess
import sys
import termios
import time

PROGRAM = sys.argv[1]


def pipe_case(flags, size, data, expected):
    result = subprocess.run(
        [PROGRAM, str(flags), str(size)], input=data, capture_output=True,
        start_new_session=True, timeout=5, check=True,
    )
    assert result.stdout == expected, result


def read_until(fd, token):
    output = b""
    deadline = time.monotonic() + 5
    while token not in output:
        remaining = deadline - time.monotonic()
        assert remaining > 0 and select.select([fd], [], [], remaining)[0], output
        chunk = os.read(fd, 4096)
        assert chunk, output
        output += chunk
    return output


def tty_case(*, interrupt=False, fail=False, closed_stdin=False):
    master, slave = os.openpty()
    old = termios.tcgetattr(slave)
    old[3] |= termios.ECHO | termios.ECHONL
    termios.tcsetattr(slave, termios.TCSANOW, old)

    def child_setup():
        import fcntl
        os.setsid()
        fcntl.ioctl(0, termios.TIOCSCTTY, 0)

    env = os.environ.copy()
    if fail:
        env["FAIL_TCSETATTR"] = "1"
    if closed_stdin:
        env["CLOSE_STDIN"] = "1"
    child = subprocess.Popen(
        [PROGRAM, "2", "16"], stdin=slave, stdout=slave, stderr=slave,
        preexec_fn=child_setup, env=env,
    )
    try:
        if fail:
            output = read_until(master, b"\n")
            assert b"secret:" not in output, output
            assert f"error:{errno.EIO}".encode() in output, output
            assert child.wait(timeout=5) == 0
        else:
            output = read_until(master, b"secret: ")
            assert not termios.tcgetattr(slave)[3] & (termios.ECHO | termios.ECHONL)
            if interrupt:
                child.send_signal(signal.SIGTERM)
                assert child.wait(timeout=5) == -signal.SIGTERM
            else:
                os.write(master, b"Private42\n")
                output += read_until(master, b"result:Private42")
                assert output.count(b"Private42") == 1, output
                assert child.wait(timeout=5) == 0
        assert termios.tcgetattr(slave) == old, "terminal state was not restored"
    finally:
        if child.poll() is None:
            child.kill()
        child.wait()
        os.close(master)
        os.close(slave)


pipe_case(0x20, 16, b"Secret\n", b"result:Secret\n")
pipe_case(0x20, 4, b"abcdef\n", b"result:abc\n")
pipe_case(0x24, 16, b"AbCd\n", b"result:abcd\n")
pipe_case(0x28, 16, b"AbCd\n", b"result:ABCD\n")
pipe_case(0x30, 16, b"\xc1\n", b"result:A\n")
pipe_case(0x20, 16, b"", b"result:\n")
pipe_case(0x20, 0, b"", f"error:{errno.EINVAL}\n".encode())
pipe_case(0x02, 16, b"secret\n", f"error:{errno.ENOTTY}\n".encode())
tty_case()
tty_case(closed_stdin=True)
tty_case(interrupt=True)
tty_case(fail=True)
