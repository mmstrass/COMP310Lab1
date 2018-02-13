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
//forward declarations
int parseArguments(char *cmd, char *args[]);
void bg_fg(char *argv[], int ground);
void child_handler(int sig);
void add(char line[256]);
void printHistory();
char *find(char *input);
void changeDir(char *argv);
char *get_current_dir_name(void);
int get_cur_dir();
void new_addr(char *cwd, char **address, int parse);

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
			exit(1);
		}
		
		//check for built in history method
		else if (strcmp(*argv, hist) == 0) {
			printHistory();
			add(cmdline);
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
					changeDir(result);
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
			changeDir(cmdline);
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
		//check forground or background 
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
 * changeDir method 
 * this will change directories based on the user inputed cd command
 *
 * @param cmdline the command that was entered by the user
 */
void changeDir(char *cmdline) {
	//tokens will count the number of tokens we have looked at
	int tokens = 0;	
	
	//get the number of directories in the aboslute address
	int parse = get_cur_dir();
	char *token = strtok_r(cmdline, "/\t \n", &cmdline);
	
	//loop through all the tokens
	while (token != NULL) {
		tokens += 1;
		//check to see if the 2nd token is usr
		if(tokens == 2 && (strcmp(token, "usr")==0)) {

			//then we should change to the usr directory
			chdir("/usr/");

		}
		//check for change to parent directory
		else if (strcmp(token, "..") == 0) {
			char cwd[256];
			getcwd(cwd, sizeof(cwd));
			char *address = calloc(1,sizeof(cwd));
			strcat(address, "/");
			
			//find the address of the parent directory
			new_addr(cwd,&address, parse);
			if(chdir(address)!=0) {
				printf("No such directory\n");
			}
		}

		//check if the current token is cd
		else if (strcmp(token, "cd") == 0) {
			//do nothing
		}

		//else we will move into an embeded directory
		else {
			char cwd[256];
			getcwd(cwd, sizeof(cwd));
			strcat(cwd, "/");
			strcat(cwd, token);
			
			//move to this directory and ensure it exists
			if(chdir(cwd) != 0) {
				printf("No such directory\n");
			}
		}

		//get the next token
		token = strtok_r(cmdline, "/\t \n", &cmdline);
	}
	//check to see if there was only one token (whch would be cd)
	if(tokens == 1) {
		//this wil change to the home directory 
		char *home = getenv("HOME");
		chdir(home);
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


/**
 * new_addr function
 * This will create the new address that we will be returning to 
 *
 * @param cwd absolute address of directory which cd was called from
 * @param address pointer to the address that we will change to in caller
 * @param parse the number of directories in the current absolute path
 */ 
void new_addr(char *cwd, char **address, int parse) {
	
	//count which token we are currently at	
	int count = 0;
	char *token2 = strtok_r(cwd,"/",&cwd);
	
	//loop through the tokens and add them to our address
	while (token2 != NULL) {
		count+=1;
		
		//add all the tokens except the last one
		if (count != parse) {
			strcat(*address, token2);
			strcat(*address, "/");
		}
		token2 = strtok_r(cwd,"/",&cwd);
	}
}	

/*
 * get_cur_dir() will return the number of directories in the current absolute
 * path
 */
int get_cur_dir() {
	
	//get the current working directory
	char cwd[256];     
	int count = 0;
	getcwd(cwd, sizeof(cwd));
	
	//loop through and count the tokens
	char *token = strtok(cwd, "/\t \n");
	while (token != NULL) {
		count +=1;
		token = strtok(NULL, "/\t \n");
	}	
	return count;	
}
