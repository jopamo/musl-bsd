#include <unistd.h>

int main(void) {
    static const char message[] = "musl static-pie baseline: ok\n";
    ssize_t count = write(STDOUT_FILENO, message, sizeof(message) - 1);

    return count == (ssize_t)(sizeof(message) - 1) ? 0 : 1;
}
