//-----------------------------------------------------------------------------
//
// Geoffrey Meier
//  commands.c -- interface for executing internal commands in smash
//
//-----------------------------------------------------------------------------

#include "smash.h"
#include "history.h"
#include "references.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* local function declarations */
void cntlCHandlerExternal(int);
static void skipWhitespace(char **);


/* execute a given smash command */
void executeCommand(char *str)
{

    //remove any leading spaces
    skipWhitespace(&str);

    if (strlen(str) == 0)
        return; //if str is blank, return

    /* find out how many tokens are in the input */
    int token_count = 0;
    char *ptr = str;
    while (ptr[0] != '\0')
    {
        //skip over one or more consecutive spaces
        if (ptr[0] == ' ')
        {
            skipWhitespace(&ptr);
            token_count++;
        }
        //account for redirections and pipes as their own tokens
        else if (ptr[0] == '<' || ptr[0] == '>' || ptr[0] == '|')
        {
            token_count++;
            ptr++;
            //remove any following whitespace since we already counted the token
            skipWhitespace(&ptr);
        }
        else
        {
            //account for missing whitespace between redirections, pipes, and other tokens.
            //also account for the end of the string
            if (ptr[1] == '<' || ptr[1] == '>' || ptr[1] == '|' || ptr[1] == '\0')
                token_count++;

            ptr++;
        }
    }

    /* create cmd array and tokenize str */
    char *cmd[token_count];
    ptr = str; //reset ptr to str
    char *lastptr = ptr;
    int i = 0;
    while (lastptr[0] != '\0')
    {
        if (ptr[0] == '<' || ptr[0] == '>' || ptr[0] == '|')
            ptr++;
        else
            while (ptr[0] != '<' && ptr[0] != '>' && ptr[0] != '|' && ptr[0] != ' ' && ptr[0] != '\0')
                ptr++;

        //set the token to cmd[i]
        char *temp = malloc((ptr - lastptr + 1) * sizeof(char));
        strncpy(temp, lastptr, ptr - lastptr);
        temp[ptr - lastptr] = '\0';
        cmd[i] = temp;

        i++;

        skipWhitespace(&ptr);
        lastptr = ptr;
    }

    int exitStatus = 0;

    //figure out how many pipes are needed, then create them
    int num_pipes = 0;
    for (int i = 0; i < token_count; i++)
        if (strcmp(cmd[i], "|") == 0)
            num_pipes++;
    struct pipe_fds pipes[num_pipes];
    for (int i = 0; i < num_pipes; i++)
    {
        if (pipe(pipes[i].fds) < 0)
        {
            perror("Error creating pipe");
            exitStatus = 127;
            goto finish; //go to bottom of function to add command to history
        }
    }

    int start_seg_index = 0; //start index of segment
    int end_seg_index = 0;   //last index of segment
    //iterate for each segment
    for (int seg = 0; seg <= num_pipes; seg++)
    {
        //set end_seg_index
        if (seg == num_pipes)
            end_seg_index = token_count - 1;
        else
            while (strcmp(cmd[end_seg_index + 1], "|") != 0)
                end_seg_index++;

        //check for redirections, and begin filling cmd_seg
        char **cmd_seg = malloc((end_seg_index - start_seg_index + 2) * sizeof(char *));
        int current_cmd_seg_count = 0;
        int fd_i = 0; //input file descriptor for segment - init to stdin (0)
        int fd_o = 1; //output file descriptor for segment - init to stdout (1)

        for (int i = start_seg_index; i <= end_seg_index; i++)
        {
            //check for input redirection
            if (strcmp(cmd[i], "<") == 0)
            {
                if (seg == 0)
                {
                    i++; //increment i to get to the filename from input
                    fd_i = open(cmd[i], O_RDONLY);
                    if (fd_i < 0)
                    { //catch error
                        perror("Error opening input file descriptor");
                        exitStatus = 127;
                        goto finish;
                    }
                }
                else
                {
                    fprintf(stderr, "Error: can only redirect input to first command in a series of pipes.\n");
                    exitStatus = 127;
                    goto finish;
                }
            }
            //check for output redirection
            else if (strcmp(cmd[i], ">") == 0)
            {
                if (seg == num_pipes)
                {
                    i++; //increment i to get to the filename from input
                    fd_o = open(cmd[i], O_WRONLY | O_CREAT, S_IRWXU);
                    if (fd_o < 0)
                    { //catch error
                        perror("Error opening output file descriptor");
                        exitStatus = 127;
                        goto finish;
                    }
                }
                else
                {
                    fprintf(stderr, "Error: can only redirect output from last command in a series of pipes.\n");
                    exitStatus = 127;
                    goto finish;
                }
            }
            else
            {
                //copy cmd[i] into next open spot in cmd_seg
                int sz = strlen(cmd[i]) + 1;
                cmd_seg[current_cmd_seg_count] = malloc(sz);
                strncpy(cmd_seg[current_cmd_seg_count], cmd[i], sz);
                current_cmd_seg_count++;
            }
        }
        //set last token in cmd_seg to null pointer
        cmd_seg[current_cmd_seg_count] = malloc(sizeof(char));
        cmd_seg[current_cmd_seg_count] = NULL;

        //set fd_i and fd_o from pipe
        if (num_pipes != 0)
        {
            if (seg >= 0 && seg < num_pipes)
            {
                fd_o = pipes[seg].fds[1];
            }
            if (seg > 0 && seg <= num_pipes)
            {
                fd_i = pipes[seg - 1].fds[0];
            }
        }

        /* command: exit */
        if (strcmp(cmd_seg[0], "exit") == 0)
        {
            smash_exit();
        }

        /* command: cd */
        else if (strcmp(cmd_seg[0], "cd") == 0)
        {
            if (current_cmd_seg_count > 1)
                exitStatus = smash_cd(cmd_seg[1]);
        }
        else
        {
            //fork a new process
            int pid = fork();

            //if this process is the child, execute command
            if (pid == 0)
            {
                //set file descriptors
                if (fd_i != 0)
                {
                    close(0);       //Close stdin inherited from smash
                    dup2(fd_i,0);   //Child's stdin must read from pipe
                    close(fd_i);    //fd_i is now extraneous, so close it
                }
                if (fd_o != 1)
                {
                    close(1);       //Close stdout inherited from parent
                    dup2(fd_o, 1);  //Child1's stdout must write to pipe
                    close(fd_o);    //fd_o is now extraneous, so close it
                }

                //close extraneous file descriptors
                for (int p=0; p<num_pipes;p++) {
                    close(pipes[p].fds[0]);
                    close(pipes[p].fds[1]);
                }

                /* command: history */
                if (strcmp(cmd_seg[0], "history") == 0)
                {
                    if (current_cmd_seg_count > 1)
                    {
                        fprintf(stderr, "%s: illegal command: %s does not take arguments\n", cmd_seg[0], cmd_seg[0]);
                        exitStatus = 127;
                    }
                    // add_history(str, exitStatus); //add command to history first
                    if (exitStatus == 0)
                        print_history(); //then print history if command was valid

                    exit(exitStatus); //will exit the child, NOT the parent
                }

                /* external command */
                else
                {
                    executeExternalCommand(cmd_seg);
                }
            }
            //if this process is the parent, wait for child and get exit status
            else if (pid > 0)
            {
                //close fd_i and fd_o, now that they are no longer needed
                if (fd_i!=0) close(fd_i);
                if (fd_o!=1) close(fd_o);

                int exitCode;
                wait(&exitCode);
                exitStatus = WEXITSTATUS(exitCode);
                if (exitStatus != 0)
                    goto finish;
            }
            //if fork failed, call perror and return abnormal exit status
            else
            {
                perror("Error: Fork process for external command failed!");
                exitStatus = 127;
                goto finish;
            }
        }

        for (int i = 0; i < current_cmd_seg_count; i++)
            free(cmd_seg[i]);
        free(cmd_seg);
        //next token should be pipe (unless it is the last segment), so skip over that.
        if (seg != num_pipes)
        {
            end_seg_index += 2;
            start_seg_index = end_seg_index;
        }
    }

//add to history, except if command was 'history' (in that case, command was already added to history)
finish:
    // if (strcmp(cmd[0], "history") != 0)
        add_history(str, exitStatus);

    for (int i = 0; i < token_count; i++)
        free(cmd[i]);
}

//exit the program
void smash_exit()
{
    clear_history();
    exit(0);
}

//perform the 'cd' command
int smash_cd(char *dirname)
{
    int err = chdir(dirname);
    if (err == 0)
    {
        char *cwd = getcwd(NULL, 0);
        puts(cwd);
        free(cwd);
        return 0; //success
    }
    else
    {
        fprintf(stderr, "%s: No such file or directory\n", dirname);
        return 1; //failure
    }
}

//execute an external command
int executeExternalCommand(char *argv[])
{
    signal(SIGINT,cntlCHandlerExternal);
    execvp(argv[0], argv);
    //if fail, call perror and call exit(127)
    perror("Error: Execute external command failed!");
    exit(127);
}

/* remove whitespace from str. WARNING: this will make changes to str.*/
static void skipWhitespace(char **str)
{
    while ((*str)[0] == ' ')
        (*str)++;
}

void cntlCHandlerExternal(int sigNumber) {
    if (sigNumber==SIGINT)
        fprintf(stderr,"\n");
}