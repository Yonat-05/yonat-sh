#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "linenoise.h"

int builtin_cd(char **args) {
    if(args[1] == NULL) {
        char* home = getenv("HOME");
        if (home == NULL){
            printf("cd: Home not set\n");
        } else {
            if (chdir(home) != 0) {
                perror("cd");
            }
        }
    }
    else {
        if(chdir(args[1]) != 0) {
            perror("cd");
        }
    }
    return 1; // 1 means "keep the shell running"
}

int builtin_exit(char **args) {
    printf("Mayonnaise on an escalator, it's going upstairs so see ya later.\n");
    return 0; // 0 means "kill the shell"
}

typedef struct {
    char* name;
    int(*action)(char **args);
} Builtin;

Builtin dictionary[] = {
    {"cd", builtin_cd},
    {"exit", builtin_exit},
    {NULL,NULL}
};

int main() {
    char input[256];
    char *args[64];
    char cwd[1024];
    int status = 1; // as long as status is 1, the shell keeps running

    while (status) {
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        char *home = getenv("HOME");
        char *user = getenv("USER");
        if (user == NULL) user = "guest";
        char host[256];
        if (gethostname(host, sizeof(host)) != 0) strcpy(host, "computer");
        char prompt[2048]; 

        if (home != NULL && strncmp(cwd, home, strlen(home)) == 0 && 
           (cwd[strlen(home)] == '\0' || cwd[strlen(home)] == '/')) {
            snprintf(prompt, sizeof(prompt), "|%s@%s:~%s| ", user, host, cwd + strlen(home));
        } else {
            snprintf(prompt, sizeof(prompt), "|%s@%s:%s| ", user, host, cwd);
        }

        char *line = linenoise(prompt);

        if (line == NULL) {
            printf("\n");
            break; 
        }

        if (strlen(line) > 0) {
            linenoiseHistoryAdd(line); // this saves the prompts we wrote earlier (if any)
        }

        strcpy(input, line);
        linenoiseFree(line); // clean up memory

        if (strlen(input) == 0) continue;

        char *piece = strtok(input, " ");
        int i = 0;
        while (piece != NULL){
            args[i] = piece;
            i++;
            piece = strtok(NULL, " ");
        }
        args[i] = NULL;
        if (args[0] == NULL) continue;
        int is_builtin = 0;
        
        for (int j=0; dictionary[j].name != NULL ;j++){
            if (strcmp(args[0], dictionary[j].name) == 0){
                status = dictionary[j].action(args);
                is_builtin = 1;
                break;
            }
        }

        if (is_builtin == 0) {
            pid_t pid = fork();
            if (pid == 0){
                // child process
                execvp(args[0], args);
                perror("execvp failed");
                exit(1);
            } else if(pid > 0){
                // parent process
                wait(NULL);
            } else {
                perror("fork error");
            }
        }
    }
    
    return 0;
}