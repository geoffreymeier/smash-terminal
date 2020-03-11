//-----------------------------------------------------------------------------
//
// Geoffrey Meier
//  references.h - Header file containing important references for the files in smash
//
//-----------------------------------------------------------------------------

#ifndef REFERENCES_H
#define REFERENCES_H
    //Define the maximum number of entries in the history array
    #define MAXHISTORY 10
    //Define the maximum number of characters in a line
    #define MAXLINE 4096

    struct pipe_fds {
        int fds[2];
    };

#endif
