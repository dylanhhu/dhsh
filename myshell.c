#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_ARGS 512


/* Contains information about the requested redirect */
typedef struct redir_info {
    int type;  // 0 for regular, 1 for advanced
    int orig_stdout;
    int redir_file_fd;
    char *filename;
    char *tempfilename;  // NULL when not used, else malloc()'d
} redir_info_t;


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
 * Counts the number of tokens. Dunno why I need this but whatever.
 * 
 * @param tokens Array of tokens
*/
int count_tokens(char **tokens) {
    int count = 0;

    while (*tokens++) count++;

    return count;
}


/**
 * Parses an input line for any redirections. If redirection is necessary, the
 * information will be placed in the struct provided.
 * 
 * Returns 0 on success, 1 when no redirection is neccessary, -1 on error.
*/
int parse_redirs(char *input_line, redir_info_t *output) {
    char **tokens = parse_line(input_line, ">+");

    int num_tokens = count_tokens(tokens);

    if (num_tokens == 1) return 1;  // no redirection necessary

    /* Either no file specified or too many tokens provided */
    if (num_tokens < 2 || num_tokens > 4) return -1;  

    if (tokens[1][0] == '+') {
        if (num_tokens != 3) return -1;

        output->type = 1;
        output->filename = tokens[2];
    }
    else {
        output->type = 0;
        output->filename = tokens[1];
    }

    output->tempfilename = NULL;

    return 0;
}


/**
 * Does the redirects.
*/
int do_redirs(char *input_line, redir_info_t *output) {
    int res = parse_redirs(input_line, output);

    if (res) return res;  // passthru error / redir not necessary

    /* Duplicate original stdout */
    output->orig_stdout = dup(STDOUT_FILENO);

    /* Error duplicating stdout */
    if (output->orig_stdout < 0) return -1;

    int file_fd;

    if (output->type == 1) {
        file_fd = open(output->filename, O_WRONLY | O_CREAT | O_EXCL, 0664);

        if (file_fd == -1) {
            if (errno != EEXIST) return -1;  // Error opening file

            char *temp_filename = malloc(strlen(output->filename) + 11);
            char *temp_filename_end = strcpy(temp_filename, output->filename);
            strcpy(temp_filename_end, "tempredir");

            file_fd = open(temp_filename, O_WRONLY | O_CREAT | O_TRUNC);
            if (file_fd == -1) return -1;

            output->tempfilename = temp_filename;
        }
    }
    else {
        file_fd = open(output->filename, O_WRONLY | O_CREAT | O_TRUNC, 0664);

        if (file_fd == -1) return -1;  // Error opening file
    }

    int dup2_res = dup2(file_fd, STDOUT_FILENO);
    if (dup2_res < 0) return -1;

    output->redir_file_fd = file_fd;

    return 0;
}


/**
 * Undo the redirections.
*/
int undo_redirs(redir_info_t *redirs) {
    if (redirs->tempfilename) {
        int existing_fd = open(redirs->filename, O_RDONLY, 0664);

        if (existing_fd == -1) return -1;  // error opening file

        char buf[1025];
        int n_read;

        /* Copy existing file into new file */
        while ((n_read = read(existing_fd, buf, 1024)) > 0) {
            write(STDOUT_FILENO, buf, n_read);
        }

        int close_ret = close(existing_fd);
        if (close_ret < 0) return -1;

        int dup2_ret = dup2(redirs->orig_stdout, STDOUT_FILENO);
        if (dup2_ret < 0) return -1;  // error restoring stdout

        close_ret = close(redirs->redir_file_fd);
        if (close_ret < 0) return -1;

        // rename(2) temp file to final file
        int rename_ret = rename(redirs->tempfilename, redirs->filename);
        if (rename_ret < 0) return -1;

        free(redirs->tempfilename);
    }
    else {
        int dup2_ret = dup2(redirs->orig_stdout, STDOUT_FILENO);
        if (dup2_ret < 0) return -1;

        int close_ret = close(redirs->redir_file_fd);
        if (close_ret < 0) return -1;
    }

    return 0;
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
        
        /* check if cwd string is too long */
        if (!ret) {
            print_err();
            return 1;
        }

        println(ret);
        return 1;
    }

    if (strcmp(argv[0], "cd") == 0) {
        char *tgt_dir;

        if (argv[1]) {
            tgt_dir = argv[1];
        }
        else {
            tgt_dir = getenv("HOME");
        }

        int res = chdir(tgt_dir);
        if (res) {
            print_err();
            return 1;
        }

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
        int child_pid = fork();

        if (child_pid < 0) {  // fork() error
            print_err();
            return;
        }

        if (child_pid) {  // parent
            int status;
            waitpid(child_pid, &status, 0);

            return;  // TODO: we don't need to check for status here?
        }
        else {
            if (execvp(argv[0], argv)) {
                print_err();
                exit(-1);
            }
        }
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

        input_line[strcspn(input_line, "\r\n")] = 0;

        /* Print line to stdout if in batched mode */
        if (batched_mode) {
            print(input_line);
        }

        /* Split line by semicolon into commands strings */
        char **commands = parse_line(input_line, ";");

        /* Loop through all commands */
        for (; *commands; commands++) {
            redir_info_t redir;
            int redir_res = do_redirs(*commands, &redir);

            /* Redirection error */
            if (redir_res == -1) {
                print_err();
                continue;
            }

            char **args = parse_line(*commands, " \t\n");
            if (!args) continue;  // no tokens found, just whitespace

            eval(args);

            free(args);

            if (!redir_res) {
                redir_res = undo_redirs(&redir);

                if (redir_res) print_err();
            }
        }
    }

    return 0;
}
