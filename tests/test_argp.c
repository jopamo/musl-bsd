#include <stdio.h>
#include <argp.h>

// Forward declarations
static error_t parse_opt(int key, char* arg __attribute__((unused)), struct argp_state* state);
static void test_argp(int argc, char* argv[]);

// Argument Parsing Test
const char* argp_program_version = "1.0";
const char* argp_program_bug_address = "<bug-report@example.com>";

static struct argp_option options[] = {{"help", 'h', 0, 0, "Display this help message", 0},
                                       {"version", 'v', 0, 0, "Display version information", 0},
                                       {0}};

static error_t parse_opt(int key, char* arg __attribute__((unused)), struct argp_state* state) {
    switch (key) {
        case 'h':
            argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
            break;
        case 'v':
            printf("Version: %s\n", argp_program_version);
            break;
        case ARGP_KEY_ERROR:
            return EINVAL;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, NULL, "Test `argp` parser.", NULL, NULL, NULL};

int main(int argc, char* argv[]) {
    printf("[ARGP TEST] Testing argp parser with provided arguments...\n");
    error_t err = argp_parse(&argp, argc, argv, 0, NULL, NULL);
    if (err) {
        printf("[ARGP TEST] Error parsing arguments!\n");
    }
    else {
        printf("[ARGP TEST] Arguments parsed successfully.\n");
    }
    return 0;
}
