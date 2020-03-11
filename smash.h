//-----------------------------------------------------------------------------
//
// Geoffrey Meier
//  smash.h -- header file containing many smash commands
//
//-----------------------------------------------------------------------------

#ifndef SMASH_H
#define SMASH_H
    void executeCommand(char *str);
    int executeExternalCommand(char *argv[]);
    void smash_exit();
    int smash_cd(char *dirname);
#endif
