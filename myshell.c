#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define MAX_ARGS 512


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
 * print() and a line!
*/
void println(char *s) {
    print(s);
    print("\n");
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
 * Parses an input line using the provided separator string. Returns an array
 * of string pointers.
 * 
 * @param input_line The line to parse
 * @param sep Separator string to use
*/
char ** parse_line(char *input_line, char *sep) {
    char *token = strtok(input_line, sep);

    /* No tokens found */
    if (!token) return NULL;

    char **args = malloc(MAX_ARGS * sizeof(char *));
    args[0] = token;  // set first token

    int i;
    /* Get the rest of the tokens */
    for (i = 1; (token = strtok(NULL, sep)); i++) {
        args[i] = token;
    }

    args[i] = NULL;

    return args;
}


/**
 * If provided with a builtin command, executes the command and returns 1,
 * otherwise 0.
 * 
 * @param argv Array of string arguments
*/
int builtin_command(char **argv) {
    if (strcmp(argv[0], "exit") == 0) {
        free(argv);
        exit(0);
    }

    if (strcmp(argv[0], "pwd") == 0) {
        char cwd[513];

        char *ret = getcwd(cwd, 513);
        
        if (!ret) {
            print_err();
            return 1;
        }

        println(ret);
        return 1;
    }

    if (strcmp(argv[0], "cd") == 0) {
        println("cd!");
        return 1;
    }

    return 0;
}


/**
 * Evaluates a given array of arguments. Requires at minimum one argument in the
 * array.
 * 
 * @param argv Array of string arguments
*/
void eval(char **argv) {
    if (!builtin_command(argv)) {
        println("not builtin :(");
    }
}


int main(int argc, char *argv[]) {
    FILE *input_file = stdin;
    int batched_mode = 0;

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
        batched_mode = 1;
    }

    char input_line[514];

    while (!feof(input_file)) {
        /* Only print prompt when not in batched mode */
        if (!batched_mode) {
            print("myshell> ");
        }

        char *ret = fgets(input_line, 514, input_file);  // Read next input line
        if (ret == NULL) {
            print_err();
            exit(0);  // TODO: Verify
        }

        if (!validate_line_len(input_line)) {
            if (batched_mode) print(input_line);

            /* Clear rest of input buffer until next line */
            do {
                ret = fgets(input_line, 514, input_file);
                if (batched_mode) print(input_line);
            } while (!validate_line_len(input_line));

            print_err();
        }

        // Split line by semicolon into commands strings

        // Loop through command strings

        // Split command by redirection (return redir info in a struct?)

        // Set up redirection
            // `>+` notes:
                // use O_WRONLY | O_CREAT | O_EXCL
                // check for EEXISTS
                    // write into temp file
            // `>` notes:
                // use O_WRONLY | O_CREAT | O_TRUNC or just use creat() :/

        char **args = parse_line(input_line, " \t\n");
        if (!args) continue;  // no tokens found, just whitespace

        eval(args);

        free(args);

        // Fork and execvp
            // fork
                // child: execvp
                // parent: waitpid

        // Clean up redirections (close files)
            // if `>+`:
                // copy existing file into temp file in chunks
                // use rename(2) to rename temp file to final file (overwrites existing file)

        /* Print line to stdout if in batched mode */
        if (batched_mode) {
            print(input_line);
        }

        // do shit
    }

    return 0;
}
