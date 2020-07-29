#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<readline/history.h>
#include <stdio.h>
#include <string.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

void execMPC(char **);
void execPipe(char **);
void execRedir_In(char **);
void execRedir_Out(char **);
void execBackground(char **);
char ** tokByDelim(char **, char *, char *);
char **get_input(char *);
int cd(char *);

int main() {
    char **command;
    char *input;
    // Loop while user still wants to run the program.
    while (1) {
        // Get user input
        input = readline("(:cs334:) ");
        // Parse that input
        command = get_input(input);
        if (!command[0]) {      /* Handle empty commands */
           free(input);
           free(command);
           continue;
       }
       free(input);
       free(command);
    }
  return 0;
}

// Execute a single command using fork / pipe.
void execFunc(char * input, char ** command) {

    if(strcmp(command[0], "cd") == 0){
      if (cd(command[1]) < 0) {
                perror(command[1]);
            }
      return;
    }

    pid_t child_pid;
    int stat_loc;
    child_pid = fork();
    if (child_pid < 0) {
        perror("Fork failed");
        exit(1);
    }

    if (child_pid == 0) {
        /* Never returns if the call is successful */
        if (execvp(command[0], command) < 0) {
            perror(command[0]);
            exit(1);
        }
    } else {
        waitpid(child_pid, &stat_loc, WUNTRACED);
    }
}

// Parse user entered input and figure out what special-type command they entered if any
char ** get_input(char *input) {
    if(strcmp(input, "exit") == 0){ exit(1); }
    char ** command = malloc(8 * sizeof(char *));
    if (command == NULL) {
        perror("Malloc failed");
        exit(1);
    }
    if(strchr(input, '&') != NULL){
      command = tokByDelim(command, input, "&");
      execBackground(command);
    } else if(strchr(input, '|') != NULL){
      command = tokByDelim(command, input, "|");
      execPipe(command);
    } else if (strchr(input, '<') != NULL){
      command = tokByDelim(command, input, "<");
      execRedir_In(command);
    } else if (strchr(input, '>') != NULL){
      command = tokByDelim(command, input, ">");
      execRedir_Out(command);
    } else if (strchr(input, '=') != NULL){
      command = tokByDelim(command, input, "=");
      execMPC(command);
    } else {
      command = tokByDelim(command, input, " ");
      execFunc(input, command);
    }
    return command;
}

int cd(char *path) {
    return chdir(path);
}

char ** tokByDelim(char ** command, char * input, char * specChar){
  char * separator = specChar;
  int index = 0;
  char *parsed = strtok(input, separator);
  while (parsed != NULL) {
      command[index] = parsed;
      index++;
      parsed = strtok(NULL, separator);
  }
  command[index] = NULL;
  return command;
}

void execPipe(char ** command){
    // 0 is read end, 1 is write end
    char ** parsed = malloc(8 * sizeof(char *));
    parsed = tokByDelim(parsed, command[0], " ");
    char ** parsedPipe = malloc(8 * sizeof(char *));
    parsedPipe = tokByDelim(parsedPipe, command[1], " ");
    int pipefd[2];
    pid_t p1, p2;
    int stat_loc;
    if (pipe(pipefd) < 0) {
        printf("\nPipe could not be initialized");
        return;
    }
    p1 = fork();
    if (p1 < 0) {
        perror("Forking Error");
        return;
    }

    if (p1 == 0) {
        // Child 1 executing..
        // It only needs to write at the write end
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        if (execvp(parsed[0], parsed) < 0) {
            printf("\nCould not execute command 1..");
            exit(0);
        }
    } else {
        // Parent executing
        p2 = fork();
        if (p2 < 0) {
            printf("\nCould not fork");
            return;
        }
        // Child 2 executing..
        // It only needs to read at the read end
        if (p2 == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            if (execvp(parsedPipe[0], parsedPipe) < 0) {
                printf("\nCould not execute command 2..");
                exit(0);
            }

        } else {
            // parent executing, waiting for two children
            close(pipefd[0]);
            close(pipefd[1]);
            wait(NULL);
            wait(NULL);
            free(parsedPipe);
            free(parsed);
        }
    }
}

// Same thing as execRedir_In but uses open instead and
void execRedir_In(char ** command){
  char ** parsed = malloc(8 * sizeof(char *));
  parsed = tokByDelim(parsed, command[0], " ");
  char ** parsedIn = malloc(8 * sizeof(char *));
  parsedIn = tokByDelim(parsedIn, command[1], " ");

  pid_t p1;
  fflush(0);
  if((p1 = fork()) != 0){ }
  else if(p1 == 0){
    int fd = open(parsedIn[0], O_RDONLY, 0);
    if (fd < 0) { perror("Couldn't open input file"); exit(0); }
    dup2(fd, STDIN_FILENO);
    close(fd);
  execvp(parsed[0], parsed);
  exit(1);
  }
  wait(NULL);
  free(parsed);
  free(parsedIn);
}

// Handle outwards redirection
void execRedir_Out(char ** command){
  char ** parsed = malloc(8 * sizeof(char *));
  parsed = tokByDelim(parsed, command[0], " ");
  char ** parsedOut = malloc(8 * sizeof(char *));
  parsedOut = tokByDelim(parsedOut, command[1], " ");

  pid_t p1;
  // Flush standard input to prep for redirection
  fflush(0);
  if((p1 = fork()) != 0){ }
  else if(p1 == 0){
    // Create the file for the output to be redirected into.
    int fd = creat(parsedOut[0], 0644);
    if (fd < 0) { perror("Couldn't open input file"); exit(0); }
    dup2(fd, STDOUT_FILENO);
    close(fd);
  execvp(parsed[0], parsed);
  exit(1);
  }
  wait(NULL);
  free(parsed);
  free(parsedOut);
}

// Create a daemon process utilizing setsid and handle process' in the background.
void execBackground(char ** command){
  pid_t pid;
  if ((pid = fork()) < 0){ perror("Fork Error:"); }
  else if(pid != 0) {
    exit(0);
  }else {
  setsid(); /* */
  command = get_input(command[0]);
  exit(0);
  }
}

// Support the = command.
void execMPC(char ** command){
  // Allocate memory to two arrays.
  char ** multiArgs = malloc(8 * sizeof(char *));
  char ** cmdMPC = malloc(8 * sizeof(char *));
  // Create a string that will act as input for get_input
  char leadProc[50];
  // Parse everything after the '=' sign
  multiArgs = tokByDelim(multiArgs, command[1], ";");
  // Iterate through the array of stings, + 100 to support as many ; arguments as needed.
  for(int loop = 0; loop <= (sizeof(multiArgs) / sizeof(multiArgs[0])) + 100; loop++){
        if(multiArgs[loop] != NULL){
          // Create the command wanting to be run by taking the text before the '=' sign
          // and concating it together with a pipe, and the first cmd to be piped.
          strcpy(leadProc, command[0]);
          strcat(leadProc, " | ");
          strcat(leadProc, multiArgs[loop]);
          cmdMPC = get_input(leadProc);
          // Reset the input string.
          leadProc[0] = '\0';
        } else {
          break;
        }
  }
  free(cmdMPC);
  free(multiArgs);
}
