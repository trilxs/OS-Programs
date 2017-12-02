//TAM LU

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

void cd(char*);
void to_array(char *);
void replace_string(char*, int, int);
void background_check();

#define EXPAND "$$" //use this variable to find if a user input needs to be expanded into a process ID

char args[512][512]; //user input split into individual arguments decided by spaces
int arg_num;        //number of arguments in the args array

void main() {
    pid_t spawnPid = -5;
    int childExitMethod = -5;
    char * input;
    int is_background;
    struct sigaction SIG_action = {0}, SIG_BG_action = {0}, SIGTSTP_action = {0}; //creating the structs and making them empty!
    SIG_action.sa_handler = SIG_IGN; //set how to handle this action. In this case, ignore.
    SIG_action.sa_flags = 0;

    SIG_BG_action.sa_handler = SIG_DFL;
    SIG_BG_action.sa_flags = 0;

    sigaction(SIGINT, &SIG_action, &SIG_BG_action);
    
    while(1) {
        memset(&args, 0, sizeof(args)); //reset values in args every time it restarts
        input= malloc(sizeof(char*)*2048); //allocate new space into input for use
        printf(": "); fflush(stdout);
        fgets(input, 2048, stdin);
        //printf("input is %s\n", input); fflush(stdout);
        if (strlen(input) == 1 || input[0] == '#') continue; //if the input is nothing or a comment, then restart the while loop
        if (strstr(input, EXPAND) != NULL) { //if there is a $$, then replace the $$ with the process ID
            char *result = strstr(input, EXPAND);
            int pos = result - input;
            replace_string(input, pos, 2);
        }
        to_array(input); //turn the input into an array of arguments

        //check if built-in commands
        if (!strcmp(args[0], "cd")) { 
            cd(args[1]);
            free(input); //free the user input after use.
        }
        else if (!strcmp(args[0], "exit")) {
            free(input);
            exit(0);
        }
        else if (!strcmp(args[0], "status")) {
            if (WIFEXITED(childExitMethod))  //if there has been an exit num, then print it out
                printf("exit value %d\n", WEXITSTATUS(childExitMethod));
            else if (WIFSIGNALED(childExitMethod)) 
                printf("terminated by signal %d\n", WTERMSIG(childExitMethod));
            fflush(stdout);
        }

        //if not built in
        else {
            //set up variables for read/write, etc
            is_background = 0; 
            int file1, file2; 
            int in = -5; int out = -5;
            int i; int num = 0;
            char ** dyn_args = malloc(arg_num *sizeof(char*)); //create a dynamic array for easier use (setting indices equal to NULL)
            for (i= 0; i < arg_num; i++) {
                dyn_args[i] = args[i];
            }

            if (!strcmp(dyn_args[arg_num-1], "&")) { //if there is an &, then do this in the background
                is_background = 1;
                dyn_args[arg_num-1] = NULL;
            } else
                dyn_args[arg_num] = NULL;
            spawnPid = fork(); 

            if (spawnPid==-1) {
                perror("Fork failed!\n"); exit(1);
            }
            else if (spawnPid == 0) {    
                    if (is_background != 1) { //if background then set the handlers for sigint
                        sigaction(SIGINT, &SIG_BG_action, NULL);
                    }
                    while (dyn_args[num] != NULL) {
                        if (!strcmp(dyn_args[num], ">")) { //if write
                            file2 = open(dyn_args[num+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                            if(file2 == -1) { 
                                perror("cannot open badfile or input\n");
                                exit(1);
                            }
                            out = dup2(file2, 1); //redirection. deletes the old file descriptor
                            if (out==-1) {
                                perror("dup2\n");
                                exit (1);
                            }
                            dyn_args[num] = NULL;
                        }
                        else if (!strcmp(dyn_args[num], "<")) { // if read
                            file1 = open(dyn_args[num+1], O_RDONLY);
                            if (file1 == -1) {
                                perror("cannot open badfile or input\n");
                                exit(1);
                            }
                            in = dup2(file1, 0);
                            if( in == -1) {
                                perror("dup2\n");
                                exit(1);
                            }
                            dyn_args[num] = NULL;
                        }
                        num++;
                    }
                    if (execvp(dyn_args[0], dyn_args) < 0) { //run the command, if it returns a 0 then it succeeds. if not, then send a message for failure
                        perror("CHILD: exec failure");
                        exit(1);
                    }
                    free(dyn_args); 
            }
            else if (spawnPid > 0){
                if (is_background ==1) printf("background pid is %d\n", spawnPid); fflush(stdout); //if this is the background process, then state it
                int exit_pid;
                if (exit_pid = waitpid(spawnPid, &childExitMethod, 0) > 0 && WIFSIGNALED(childExitMethod)) {
                    printf("Terminated by signal %d\n", WTERMSIG(childExitMethod), exit_pid); fflush(stdout);
                }
                background_check(); 
            }
            else {
                perror("waitpid() failed\n"); fflush(stdout); exit(1);
            }
        }
    }
}

void background_check() {
    int status; pid_t background_pid = 0;
   do { //while waiting for background process 
        background_pid = waitpid(-1, &status, WNOHANG);
        if(background_pid > 0) {
            if(WIFSIGNALED(status)) {
                printf("Terminated by signal %d\n", WTERMSIG(status)); fflush(stdout);
            }
            else if (WIFEXITED(status)) {
                printf("background pid is %d. exit value %d\n", background_pid, WEXITSTATUS(status)); fflush(stdout);
            }
        }
    } while (background_pid > 0)
}

void replace_string(char* input, int pos, int substring_len) {
    char beg[pos];
    int i;
    for (i=0; i < pos; i++) {
        beg[i] = input[i]; //split into 3 parts if there is a $$ in the input
    }
    beg[pos] = '\0';
    if (strlen(input)-1-pos-substring_len > 0) {
        char end[strlen(input)-pos-substring_len];
        for (i=0; i < strlen(input)-pos-substring_len; i++) {
            end[i] = input[i+pos+substring_len];  //i+pos+substring_len is the position after $$
        }   
        end[strlen(input)-pos-substring_len-1] = '\0'; //set the end of the char array to \0 
        sprintf(input, "%s%d%s\n", beg, getppid(), end);
    } else {
        sprintf(input, "%s%d\n", beg, getppid());
    }
}

//converts the input value into a set of arguments to read through later
void to_array(char * input) {
    int i, j; int count =0; int cur_pos = 0;
    for (i=0; input[i] != '\0'; i++) {
        if (input[i] == ' ') //if there is a space, then it divides two arguments
            count++; 
    }
    count++; //iterate count one more time for the argument at the very end
    char arr[count][512];
    memset(arr, 0, sizeof(arr));
    arg_num = count;
    for (i=0; i<count; i++) {
        for (j=0; input[cur_pos+j] != '\0' || input[cur_pos+j] != '\n'; j++) { //if the input is not at the end, then keep on iterating 
            if (cur_pos+j > 512) break; 
            if (input[cur_pos+j] == ' ') {
                j++;
                break;
            }
            else {
            arr[i][j] = input[cur_pos+j]; //set the character in a specific argument equal to the next index of input where it's not ' ' 
            }
        }
        cur_pos += j;
        if (i == count-1) {
            strncpy(args[i], arr[i], strlen(arr[i])-1);
        }
        else {
            strncpy(args[i], arr[i], strlen(arr[i]));
        }
    }
}

//go to path
void cd(char * path) {
    if(chdir(path) < 0) {
        printf("Error: Cannot find directory %s\n", path); fflush(stdout);
    }
    else {
        char curr[1024];
        getcwd(curr, 1024);
    }
}
