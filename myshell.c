#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


/**
 * Prints the error message to `STDOUT` as specified by the project
 * requirements.
*/
void print_err() {
    char msg[] = "An error has occurred\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}


/**
 * Generic print function that uses `write()` to print to `STDOUT`.
 * 
 * @param s String to print
*/
void print(char *s) {
    write(STDOUT_FILENO, s, strlen(s));
}


/**
 * Validates the length of a line.
 * 
 * @param line The line to validate
*/
int validate_line_len(char *line) {
    if (strlen(line) == 513 && line[512] != '\n') return 0;

    return 1;
}


/**
 * Parses an input line and returns <something>
 * 
 * @param input_line The line to parse
*/
char ** parse_line(char *input_line) {
    return NULL;
}


int main(int argc, char *argv[]) {
    FILE *input_file = stdin;

    if (argc > 1) {
        /* Error and exit if more than two batch files provided */
        if (argc != 2) {
            print_err();
            exit(0);
        }

        input_file = fopen(argv[1], "r");  // open batch file for reading
        if (input_file == NULL) {
            print_err();
            exit(0);
        }
    }

    char input_line[514];

    while (!feof(input_file)) {
        /* Only print prompt when not in batched mode */
        if (input_file == stdin) {
            print("myshell> ");
        }

        char *ret = fgets(input_line, 514, input_file);  // Read next input line
        if (ret == NULL) {
            print_err();
            exit(0);  // TODO: Verify
        }

        if (!validate_line_len(input_line)) {
            print_err();

            /* We must print the line, valid or not, in batched mode */
            if (input_file != stdin) {
                print(input_line);
                continue;
            }
        }

        /* Print line to stdout if in batched mode */
        if (input_file != stdin) {
            print(input_line);
        }

        // do shit
    }

    return 0;
}
