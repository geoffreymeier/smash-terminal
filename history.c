//-----------------------------------------------------------------------------
//
// Geoffrey Meier
//  commands.c -- interface for executing history command in smash
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "history.h"
#include "references.h"

//keeps track of the last sequence number
static unsigned int lastSequenceNumber;
static struct Cmd *history[MAXHISTORY];

//initialize history
void init_history(void) {
    lastSequenceNumber = 0;
    memset(history,0,sizeof(struct Cmd *));  //init array to NULL
}

//add a command to the history
void add_history(char *cmd, int exitStatus) {
    lastSequenceNumber++;   //increment the sequence number
    

    struct Cmd *command = malloc(sizeof(struct Cmd));
    command->cmd = malloc(MAXLINE);
    strncpy(command->cmd,cmd,MAXLINE);
    command->exitStatus = exitStatus;

    //add new command to the beginning of the history array
    struct Cmd *last = history[MAXHISTORY-1];   //save the pointer to the last item to be removed from the array
    for (int i=MAXHISTORY-1;i>0;i--) {
        history[i] = history[i-1];
    }
    history[0] = command;

    //if the element removed from the array is not null, free it
    if (last != NULL) {
        free(last->cmd);
        free(last);
    }
}

//clear (free) all commands from history
void clear_history(void) {
    for (int i=0; i<MAXHISTORY; i++) {
        if (history[i]!=NULL) {
            free(history[i]->cmd);
            free(history[i]);
            history[i] = NULL;
        }
    }
}

//print history to stdout
void print_history() {
    for (int i=MAXHISTORY-1; i>=0; i--) {
        struct Cmd *h = history[i];
        if (h!=NULL)
            printf("%d [%d] %s\n",lastSequenceNumber-i,h->exitStatus,h->cmd);
    }
}