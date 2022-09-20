#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <pwd.h>
#include <fcntl.h>

#define MAXLINE 1024
#define MAXARGS 128
#define BLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"

static volatile sig_atomic_t interrupted = 1;

void sig_handler(int signo)
{
    interrupted = 0;
}

int main(void)
{
    // user runs program

    // handle actions
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = sig_handler;

    if (sigaction(SIGINT, &action, NULL) == -1)
    {
        fprintf(stderr, "Error: Cannot register signal handler. %s\n", strerror(errno));
    }

    while (1)
    { // loop that loops until "exit" is inputted or error
        struct dirent *dirp;
        char cwd[MAXLINE];
        // print current dir.
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {                             // get the current working directory
            printf("[%s", BLUE);      // change color to BLUE
            getcwd(cwd, sizeof(cwd)); // get the current working directory
            printf("%s", cwd);        // print the current working directory
            printf("%s]> ", DEFAULT); // change color back to DEFAULT
        }
        else
        {
            fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
            return -1;
        }
        char inputString[MAXLINE];
        fgets(inputString, MAXLINE, stdin); // get the user input

        if (interrupted == 0)
        {                    // checks if ctrl+c is inputted
            interrupted = 1; // reset interrupted
            printf("\n");    // print new line
            continue;        // go to next iteration of loop
        }
        if (inputString == NULL)
        { // make sure input != null
            fprintf(stderr, "Error: Failed to read from stdin. %s.\n", strerror(errno));
            continue;
        }

        int sizeofcommand = 0;
        for (int i = 0; i < strlen(inputString); i++)
        { // make substring to get command user wants to run
            if (inputString[i] == ' ')
            { // Get the rest of the string because there is the first space
                sizeofcommand = i;
                break;
            }
            else if (inputString[i] == '\n')
            {
                // if no empty space is found, a new line can be found
                // ex. pwd and exit.
                sizeofcommand = i;
                break;
            }
        }

        // get the number of arguements
        int numargs = 0;
        for (int i = 0; i < strlen(inputString); i++)
        {
            if (inputString[i] == ' ' || inputString[i] == '\n') // if a space or \n is found +1 word on cmd line
                numargs++;
        }

        // ----- get the command the user inputted -----
        char command[sizeofcommand + 1];
        for (int i = 0; i < sizeofcommand; i++)
            command[i] = inputString[i];
        command[sizeofcommand] = '\0';

        // ----- get arguements without spaces -----
        char *argsns[numargs]; // argsNoSpaces
        char *stri;
        stri = strtok(inputString, " ");
        int count = 0;
        while (stri != NULL)
        { // loop countspaces times  || countspaces+1 == count
            argsns[count] = stri;
            count++;
            stri = strtok(NULL, " ");
        }
        argsns[numargs - 1][strlen(argsns[numargs - 1]) - 1] = '\0';
        argsns[numargs] = NULL;

        if (strcmp(argsns[0], "exit") == 0)
        { // basecase exit program
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(argsns[0], "cd") == 0)
        { // if the command is cd, special case
            if (numargs == 1)
            {
                int check1;
                struct passwd *pw = getpwuid(getuid());
                if (pw)
                {
                    check1 = chdir(pw->pw_dir); // try to change direcctory
                    if (check1)
                    { // catch if error
                        fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", argsns[1], strerror(errno));
                    }
                }
                else
                {
                    fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
                }
            }
            else if (numargs == 2)
            { // try to change to specific directory
                if (strcmp(argsns[1], "~") == 0)
                {
                    int check1;
                    struct passwd *pw = getpwuid(getuid());
                    if (pw)
                    {
                        check1 = chdir(pw->pw_dir); // try to change direcctory
                        if (check1)
                        { // catch if error
                            fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", argsns[1], strerror(errno));
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
                    }
                }
                else
                {
                    int s = chdir(argsns[1]); // try to change direcctory
                    if (s != 0)
                    { // catch if error
                        fprintf(stderr, "Error: Cannot change to directory '%s'. %s\n", argsns[1], strerror(errno));
                    }
                }
            }
            else
            {
                fprintf(stderr, "Error: Too many arguements to cd.\n");
            }
        }
        else
        { // otherwise other command was inputted
            int stat;
            pid_t pid = fork(); // set up a child and parent process incase user wants to open another terminal
            if ((pid) < 0)
            {
                fprintf(stderr, "Error: fork() failed. %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            { // child, execute command
                if (execvp(argsns[0], argsns) < 0)
                {
                    fprintf(stderr, "Error: exec() failed. %s.\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
            }
            else
            { // parent
                if (wait(&stat) == -1)
                {
                    fprintf(stderr, "Error: wait() failed. %s.\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    return 0;
}
