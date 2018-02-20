/*
 * The Tiny Torero Shell (TTSH)
 *
 * The ttsh will be a shell program simuator
 * The user can input many differnt shell commands pre built in by the
 * parse args file
 *
 * There will also be built in functions such as exit, history, !num and cd
 *
 * Authors: Julia Cassella and Leah Sator
 *
 * Date: 11/30/2017
 */

// NOTE: all new includes should go after the following #define
#define _XOPEN_SOURCE 600

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "parse_args.h"
#include "history_queue.h"
#include <unistd.h>
 #include <fcntl.h>

//forward declarations
int parseArguments(char *cmd, char *args[]);
void bg_fg(char *argv[], int ground);
void child_handler(int sig);
void add(char line[256]);
void printHistory();
char *find(char *input);
int check_redirection (char *command, char *filedes, char *program);
void redirect(char *filedes, int fileide, char *program);

char *split(char cmdline[256]);

int main() {
    
    //set up handler for the child method
    struct sigaction sa;
    sa.sa_handler = child_handler;
    sa.sa_flags = SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
    
    
    //initialize loop to prompt the user
    while(1) {
        // (1) print the shell prompt
        fprintf(stdout, "ttsh> ");
        fflush(stdout);
        
        // (2) read in the next command entered by the user
        char cmdline[MAXLINE];
        if ((fgets(cmdline, MAXLINE, stdin) == NULL)
            && ferror(stdin)) {
            // fgets could be interrupted by a signal.
            // This checks to see if that happened, in which case we simply
            // clear out the error and restart our loop so it re-prompts the
            // user for a command.
            clearerr(stdin);
            continue;
        }
        
        /*
         * Check to see if we reached the end "file" for stdin, exiting the
         * program if that is the case. Note that you can simulate EOF by
         * typing CTRL-D when it prompts you for input.
         */
        if (feof(stdin)) {
            fflush(stdout);
            exit(0);
        }
        
        //defining variables to be uses later
        char *argv[MAXARGS];
        char *ex = "exit";
        char *hist = "history";
        int bg = parseArguments(cmdline, argv);
        
        //check to see if the user entered blank line
        if(*argv == NULL) {
            //do nothing
        }
        
        //check for built in exit function
        else if (strcmp(*argv, ex) == 0) {
            printf("Thank you! Goodbye\n");
            exit(1);
        }
        
        //check for built in history method
        else if (strcmp(*argv, hist) == 0) {
            printf("%s\n", argv[1]);
            printf("%s\n", argv[2]);
            check_redirection(argv[1], argv[2], argv[0]);
            printHistory();
            add(cmdline);
        }
        //check for !!
        else if (strcmp("!!", *argv) == 0) {
            
            char *result = lastHist();
            if(strcmp(result, "NOT FOUND") == 0) {
                printf("ERROR: History is Empty\n");
                
            }
            else {
                printf("here");
                bg = parseArguments(result, argv);
                bg_fg(argv, bg);
                char *line = strtok(result, "\n");
                add(line);
            }
            
        }
        //check for built in !num function
        else if(strncmp("!", *argv, 1 )  == 0) {
            //call find method for history queue
            char *result = find(*argv);
            
            //check to see if the command was not found
            if(strcmp(result, "ERROR") == 0) {
                
                printf("ERROR5: Command not found\n");
                
            }
            
            //check to see if the entry didnt exist in history
            else if(strcmp(result, "NOT FOUND") == 0) {
                
                char *num = strtok(*argv, "!");
                printf("ERROR: %s is not in history\n", num);
                
            }
            
            else {
                if(strcmp(result, "history\n") == 0) {
                    printHistory();
                    add("history");
                    
                }
                else if(strncmp("cd", result, 2 )  == 0) {
                    char original[256];
                    strncpy(original, result, sizeof(original));
                    bg = parseArguments(result, argv);
                    if(chdir(argv[1])!=0) {
                        printf("No such directory\n");
                    }
                    char *line = strtok(result, "\n");
                    add(line);
                }
                else {
                    //the entry existed so call it and add it to history again
                    bg = parseArguments(result, argv);
                    bg_fg(argv, bg);
                    char *line = strtok(result, "\n");
                    add(line);
                }
                
            }
            
        }
        //check if we are doing the cd function
        else if (strncmp("cd", *argv, 2 )  == 0) {
            //save the original cmdline
            char original[256];
            strncpy(original, cmdline, sizeof(original));
            
            
            if (argv[1] == NULL) {
                chdir(getenv("HOME"));
            }
            else if(chdir(argv[1])!=0) {
                printf("No such directory\n");
            }
            char *line = strtok(original, "\n");
            add(line);
        }
        //call any other function
        else {
            bg_fg(argv, bg);
            char *line = strtok(cmdline, "\n");
            add(line);
        }
    }
    
    return 0;
}

//checks for I/O redirection

int check_redirection (char *command, char *filedes, char *program){
    int value = -1;
    //not I/O redirection
    if(command == NULL || filedes == NULL){
        return value;
    }
    //stdout
    if (strcmp(command, "1>") == 0){
        value = 1;
        printf("We made it! %d\n", value);
        redirect(filedes, value, program);
    }
    //sterr
    if (strcmp(command, "2>") == 0){
        value = 2;
    }
    //stdin
    if(strcmp(command, "<") ==0){
        value = 0;
    }
    printf("%d\n", value);
    return value;
}

/**
 *
 * redirects I/O
 *
 */
void redirect(char *filedes, int fileide, char *program){
    int new_file_des;
    FILE *new_file;
    int bg = 0;
    switch(fileide){
        //stdin
        case 0:
            new_file = fopen(filedes, O_RDONLY);
            new_file_des = fileno(new_file);
            dup2(new_file_des, 0);
            printHistory();
           // bg_fg(&program, bg);
            close(new_file_des);
            break;
        //stdout
        case 1:
            new_file = fopen(filedes, "a");
            if(new_file == NULL){
                printf("Error!");
                exit(1);
            }
            new_file_des = fileno(new_file);
            //new_file_des = open(filedes, O_WRONLY, O_CREAT);
            printf("Made it the new file descriptor is: %d\n", new_file_des);
            dup2(new_file_des, 1);
            printHistory();
            printf("%d", fclose(new_file));
            //bg_fg(&program, bg);
            printf("%s%d\n", program, bg);
            printf("Made it past dup2 the file descriptor is:\n");
            //fprintf(new_file, new_file_des);
            //fclose(new_file);
            //break;
            return;
        case 2:
            new_file_des = open(filedes, O_WRONLY, O_CREAT);
            printf("Made it the new file descriptor is: %d\n", new_file_des);
            dup2(new_file_des, 2);
            printf("Made it past dup2 the file descriptor is: %d\n", new_file_des);
            close(new_file_des);
            break;
    }
    return;
}

/**
 *
 * bg_fg function will run the program in the background or the forground
 * @param argv the arguments for the function we will call
 * @param ground will tell us if we are in the background or forground
 *
 */
void bg_fg(char *argv[], int ground){
    
    //fork
    int pid = fork();
    //if we are in the child
    
    if(pid == 0) {
        
        //execute the command specified by command line args
        if( execvp(argv[0], argv) == -1 ){
            printf("ERROR: Command not found\n");
            exit(1);
        }
    }
    
    //if in the parent wait for the child to return
    else {
        //check forground or background ground 0 = foreground
        if(ground == 0) {
            waitpid(pid, NULL, 0);
        }
        else {
            waitpid(pid, NULL, WNOHANG);
        }
        return;
    }
}

/**
 * child_handler function will wait for any remaining children that are
 * running in the background
 *
 */
void child_handler(__attribute__((unused)) int sig) {
    wait(NULL);
}
