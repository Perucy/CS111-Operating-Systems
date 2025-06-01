/*********************************************************************
 * 
 *                      shell.c
 * 
 * Written by: Perucy Mussiba
 * Date: 20th February, 2025
 * Purpose: A basic shell program to execute user commands
 *
 * ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64

/************************exec_commands**********************************
 * 
 * Parameters: char *command - array of user commands
 * Return: none - void
 * Notes: exec_commands execute user commands that do not include pipes
 * 
 ************************************************************************/
void exec_commands(char *command) {
    pid_t pid;
    int status;
    char *args[MAX_ARGS];
    int arg_count = 0;
    char *ptr = command;
    char *start;

    /*handling arguments with quotes - useful for grep*/
    while (*ptr != '\0') {
        while (*ptr == ' ') ptr++;
        if (*ptr == '\0') break;

        if (*ptr == '\'' || *ptr == '"') {
            char quote = *ptr++;
            start = ptr;
            while (*ptr != quote && *ptr != '\0') ptr++;
            if (*ptr == quote) *ptr++ = '\0';
        } else {
            start = ptr;
            while (*ptr != ' ' && *ptr != '\0') ptr++;
            if (*ptr == ' ') *ptr++ = '\0';
        }
        args[arg_count++] = start;
    }
    args[arg_count] = NULL;

    /*fork a child process*/
    pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        /*child process*/
        execvp(args[0], args);
        perror("jsh error: execvp");
        exit(127);
    } else {
        /*wait for child process to finish*/
        wait(&status);
        if (WIFEXITED(status)) {
            printf("jsh status: %d\n", WEXITSTATUS(status));
        } else {
            printf("jsh error: Program terminated abnormally.\n");
        }
    }
}
/**********************exec_pipes*****************************************
 * 
 * Parameters: char* input (user input)
 * Returns: None
 * Notes: exec_pipes executes user input involving pipes
 * 
 *************************************************************************/
void exec_pipes(char *input) {
    char *commands[MAX_ARGS];
    char *args[MAX_ARGS];
    int num_commands = 0;
    char *command = strtok(input, "|");
    int status = 0;
    pid_t last_pid;

    /*get commands from the user input*/
    while (command != NULL) {
        commands[num_commands++] = command;
        command = strtok(NULL, "|");
    }

    /*2D array to store file descriptors for pipes*/
    int pipes[MAX_ARGS - 1][2];         
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    /*loop to execute commands and their arguments*/
    for (int i = 0; i < num_commands; i++) {
        int arg_count = 0;
        char *ptr = commands[i];
        char *start;

        /*handles arguments with quote marks - useful for grep when 
        quote marks are used */
        while (*ptr != '\0') {
            while (*ptr == ' ') ptr++;
            if (*ptr == '\0') break;

            if (*ptr == '\'' || *ptr == '"') {
                char quote = *ptr++;
                start = ptr;
                while (*ptr != quote && *ptr != '\0') ptr++;
                if (*ptr == quote) *ptr++ = '\0';
            } else {
                start = ptr;
                while (*ptr != ' ' && *ptr != '\0') ptr++;
                if (*ptr == ' ') *ptr++ = '\0';
            }
            args[arg_count++] = start;
        }
        args[arg_count] = NULL;

        /*fork a child process*/
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            /* Redirecting stdin for all but the first command*/
            if (i > 0) {
                if (dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            /* Redirecting stdout for all but the last command*/
            if (i < num_commands - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            /*closing all file descriptors in the child process*/
            for (int j = 0; j < num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(args[0], args);
            perror("jsh error: execvp");
            exit(127);
        } else {
            if (i == num_commands - 1){
                last_pid = pid;
            }
        }
    }

    /*closing all file descriptors in the parent process*/
    for (int i = 0; i < num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    /*wait for child process*/
    for (int i = 0; i < num_commands; i++) {
        int temp_status;
        pid_t end_pid = wait(&temp_status);
        
        if (end_pid == last_pid){
            status = WEXITSTATUS(temp_status);
        }
    }

    if (WIFEXITED(status)) {
        printf("jsh status: %d\n", WEXITSTATUS(status));
        fflush(stdout);
    } else {
        printf("jsh error: Program terminated abnormally.\n");
    }
}

int main() {
    /*array to hold user input*/
    char input[MAX_INPUT_SIZE];

    /*prompt the user*/
    while (1) { 
        /*prompt*/
        printf("jsh$ ");

        /*get user input*/
        if (fgets(input, sizeof(input), stdin) == NULL) {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        /*newline char removed*/
        /*char is in the prompt when users type enter*/
        input[strcspn(input, "\n")] = '\0';

        /*exit command*/
        if (strcmp(input, "exit") == 0) {
            break;
        }

        /*check for piping*/
        if (strchr(input, '|') != NULL) {
            exec_pipes(input);
        } else {
            exec_commands(input);
        }
    }

    return 0;
}
