#include <stdio.h>
#include <argp.h>
#include <stdlib.h>

// Prevent redefinition of argp_program_version
#define argp_program_version "1.0"  // Define it here instead of declaring a global variable

static error_t parse_opt(int key, char* arg __attribute__((unused)), struct argp_state* state);
static void test_argp(int argc, char* argv[]);

const char* argp_program_bug_address = "<bug-report@example.com>";

static struct argp_option options[] = {
    {"help", 'h', 0, 0, "Display this help message", 0},
    {"version", 'v', 0, 0, "Display version information", 0},
    {"verbose", 'V', 0, 0, "Enable verbose output", 0},
    {"input", 'i', "FILE", 0, "Input file", 0},
    {0}  // End of the options array
};

static error_t parse_opt(int key, char* arg __attribute__((unused)), struct argp_state* state) {
    switch (key) {
        case 'h':
            argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
            break;
        case 'v':
            printf("Version: %s\n", argp_program_version);
            break;
        case 'V':
            printf("[ARGP TEST] Verbose mode enabled\n");
            break;
        case 'i':
            printf("[ARGP TEST] Input file: %s\n", arg);
            break;
        case ARGP_KEY_ERROR:
            return EINVAL;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {
    options,                                     // Options array
    parse_opt,                                   // Parse function
    NULL,                                        // Argument parsing rest (not used)
    "Test `argp` parser for multiple options.",  // Description
    NULL,                                        // Documentation (not used)
    NULL,                                        // Child parsers (set to NULL to avoid the warning)
    NULL                                         // Extra arguments (set to NULL)
};

static void test_argp(int argc, char* argv[]) {
    printf("[ARGP TEST] Testing argp parser with provided arguments...\n");
    error_t err = argp_parse(&argp, argc, argv, 0, NULL, NULL);
    if (err) {
        printf("[ARGP TEST] Error parsing arguments!\n");
    }
    else {
        printf("[ARGP TEST] Arguments parsed successfully.\n");
    }
}

int main(int argc, char* argv[]) {
    test_argp(argc, argv);
    return 0;
}
