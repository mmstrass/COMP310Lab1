/*
 * The Tiny Torero Shell (TTSH)
 *
 * This history queue is implemented using a circular queue
 * It will store the 10  most recent calls from the ttsh 
 * 
 * Authors: Leah Sato and Julia Cassella
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "parse_args.h"
#include "history_queue.h"

//global variables for history queue state
static int start = 0;
static int next = 0;
static int size = 0;
static int next_unique = 0;
static HistoryEntry history[MAXHIST]; 


/**
 * add method
 * This will add an entry to our history queue
 * @param line the command we will be adding
 */
void add(char line[MAXLINE]) { 
	//create a new history entry
	HistoryEntry *h = (HistoryEntry*) calloc(1, sizeof(HistoryEntry));
	strncpy(h->cmdline, line, sizeof(h->cmdline));
    
	//assign a unique id to the command
	h->cmd_num = next_unique;
    h->secs = time(NULL);
	next_unique += 1;
	
	//check if the queue is full and add accordingly 
	if (size == MAXHIST) {
		isFull();
	}
	history[next] = *h;
	if (size != MAXHIST) {
		size += 1;
		next += 1;
	}
}

/*
 *
 * IsFUll method will check if the hisory queue is full
 * It will then reajust the start and next accordingly
 */
void isFull() {
	//the next one will be put in at the current start
	next = start;
	if (start == MAXHIST - 1) {
		start = 0;
	}
	//increment the start by 1
	else {
		start += 1;
	}
}

/*
 *
 * Print History Method
 * Loop through the history queue and print
 * The least recently used commands to the most recently used
 *
 */
void printHistory() {
	
	//loop through the queue
	int i;
	for (i = 0; i < size; i++) {
		int n;
		//check what the index is compared to the start index
		if (start + i < MAXHIST)  { 
	   		n = start + i;
		}
		else {
			n = (start + i) - MAXHIST;
		}
        char buffer[26];
        struct tm* tm_info;
        time_t curtime = history[n].secs;
        tm_info = localtime(&curtime);
        strftime(buffer,26, "%H:%M:%S", tm_info);
		printf("%d\t%s\t%s\n", history[n].cmd_num, buffer,history[n].cmdline);
	}
}

char *find(char *input) {

	//check to see if the number is in history 
	//take out the exclamation point
	char *compare = strtok(input, "!");

	int entry = atoi(compare);
    
	//parse to int
	if(entry == 0 && (strcmp(compare, "0") != 0)) {
		return "ERROR";
	}
	
	//check through history
	int i;
	
	for (i = 0; i < size; i++) {
		int n;
		//check what the index is compared to the start index
		if (start + i < MAXHIST)  { 
	   		n = start + i;
		}
		else {
			n = (start + i) - MAXHIST;
		}
		
		if(history[n].cmd_num == (unsigned int) entry) {
			return history[n].cmdline;
		}

	}

	return "NOT FOUND";

	
}

char *lastHist() {



    //loop through to find last index
    
    if (size != 0) {

        return history[size-1].cmdline;

    }

    return "NOT FOUND";

    
}
