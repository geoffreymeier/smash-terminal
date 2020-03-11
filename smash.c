//-----------------------------------------------------------------------------
//
// Geoffrey Meier
//  smash.c -- shell command prompt
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "history.h"
#include "smash.h"
#include "references.h"

void cntlCHandler(int sigNumber);


int main(int argc, char *argv[]) {
    init_history();
    setvbuf(stdout,NULL,_IONBF,0); //Disable buffering in the stdout stream
    signal(SIGINT,SIG_IGN); //catch user entering Ctrl+C on the keyboard


    char buffer[MAXLINE];
    fputs("$ ",stderr); //output first prompt

    //keep reading commands until EOF or error
    while (fgets(buffer,MAXLINE,stdin) != NULL) {
        signal(SIGINT,SIG_IGN); //catch user entering Ctrl+C on the keyboard
        buffer[strlen(buffer)-1] = '\0';
        executeCommand(buffer);
        fputs("$ ",stderr);
    }
    
    clear_history();
    printf("\n");
    return 0;   //exit successfully
}