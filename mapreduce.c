#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>


int main(int argc, char *argv[]) {
    if (argc == 3) { // Implement the Mapper model.
        int N = atoi(argv[1]);
        char *mapper = argv[2];

        int pipes[N][2]; // The list of pipes, one for each child
        int child; // Child id, parent is -1.

        pid_t parentPid = getpid();

        for (int i = 0; i < N; i++) { // Create the pipes
            if (pipe(pipes[i]) < 0) {
                printf("Pipe error");
                return -1;
            }
        }

        for (int i = 0; i < N; i++) { // Create the child processes
            if (!fork()) {
                child = i; // Assign child number

                // Redirect pipe output to stdin
                dup2(pipes[child][0], 0);

                // Close unnecessary pipes: pipes of other children and own write pipe.
                for (int j = 0; j < N; j++){
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }

                // Execute the reducer program
                char mapperPath[50] = "./src/";
                strcat(mapperPath, mapper);
                char arg[10]; // Argument for the program to execute
                sprintf(arg, "%d", child);
                if (execl(mapperPath, mapper, arg, (char *) NULL))
                    printf("Error with exec.\n");
                break;
            }
        }

        if (getpid() == parentPid){ // Parent
        	// Close read ends of pipes
        	for (int i = 0; i < N; i++)
        		close(pipes[i][0]);

            char *line = NULL;
            size_t len = 0;
            for (int i = 0; getline(&line, &len, stdin) > 0; i = i == N-1 ? 0 : i+1) {
                len = strlen(line);
                write(pipes[i][1], line, len);
            }
            free(line);

            // Close write pipes to send EOF to children
            for (int i = 0; i < N; i++)
                close(pipes[i][1]);

            // Wait for child processes to terminate
            int child_status;
            for (int i = 0; i < N; i++)
                wait(&child_status);
        }
    }

    else if (argc == 4){ // Implement the MapReduce model
        int N = atoi(argv[1]);
        char *mapper = argv[2];
        char *reducer = argv[3];
        int pmPipes[N][2]; // The list of pipes between the parent and mappers
        int mrPipes[N][2]; // The list of pipes between mappers and reducers
        int child; // Child id
        pid_t parentPid = getpid();

        for (int i = 0; i < N; i++) { // Create the pipes
            if (pipe(pmPipes[i]) < 0) {
                printf("Pipe error");
                return -1;
            }
            if (pipe(mrPipes[i]) < 0) {
                printf("Pipe error");
                return -1;
            }
        }

        for (int i = 0; i < N; i++) { // Create the mapper processes
            if (!fork()) {
                child = i; // Assign child id

                // Redirect parent-mapper pipe output to stdin
                dup2(pmPipes[child][0], 0);
                // Copy mapper-reducer pipe input to stdout
                dup2(mrPipes[i][1], 1);

                // Close unnecessary pipes
                for (int j = 0; j < N; j++){
                    close(pmPipes[j][1]);
                    close(mrPipes[j][0]);
                    close(mrPipes[j][1]);
                }

                char mapperPath[50] = "./src/"; // Path to mapper executable
                strcat(mapperPath, mapper);
                char arg[10]; // Argument for the program to execute
                sprintf(arg, "%d", child);
                if (execl(mapperPath, mapper, arg, (char *) NULL)){
                    printf("Error with exec.\n");
                }
                break;
            }
        }

        // Create the reducer-reducer pipes.
        int rrPipes[N-1][2]; // The list of pipes between reducers
        for (int i = 0; i < N-1; i++) { // Create the pipes
            if (pipe(rrPipes[i]) < 0) {
                printf("Pipe error");
                return -1;
            }
        }

        for (int i = 0; i < N; i++) { // Create the reducer processes
            if (!fork()) {
                child = i; // Assign child id

                // Redirect mapper-reducer pipe output to stdin
                dup2(mrPipes[child][0], 0);

                // Redirect stdout to reducer-reducer pipe input
                if (child != N-1) // Last reducer's stdout should point to terminal
                    dup2(rrPipes[child][1], 1);

                // Redirect reducer-reducer pipe output to stderr
                if (child != 0) // First reducer's stderr should not change.
                    dup2(rrPipes[child-1][0], 2);

                // Close unnecessary pipes
                for (int j = 0; j < N; j++){
                    close(pmPipes[j][0]);
                    close(pmPipes[j][1]);
                    close(mrPipes[j][0]);
                    close(mrPipes[j][1]);
                    close(rrPipes[j][0]);
                    close(rrPipes[j][1]);
                }
                // Execute the reducer program
                char reducerPath[50] = "./src/"; // Path to reducer executable
                strcat(reducerPath, reducer);
                char arg[10]; // Argument for the program to execute
                sprintf(arg, "%d", child);
                if (execl(reducerPath, mapper, arg, (char *) NULL))
                    printf("Error with exec.\n");
                break;
            }
        }

        if (getpid() == parentPid) { // Parent
            // Close unnecessary pipes
            for (int i = 0; i < N; i++) {
                close(pmPipes[i][0]); // Close only read ends of parent-mapper pipes
                close(mrPipes[i][0]);
                close(mrPipes[i][1]);
                if (i == N-1) // There are N-1 rr pipes whereas there are N pm and mr pipes
                    break;
                close(rrPipes[i][0]);
                close(rrPipes[i][1]);
            }

            // Get the input from stdin
            char *line = NULL;
            size_t len = 0;
            for (int i = 0; getline(&line, &len, stdin) > 0; i = i == N-1 ? 0 : i+1) {
                len = strlen(line);
                write(pmPipes[i][1], line, len);
            }
            free(line);

            // Close write ends of parent-mapper pipes to send EOF
            for (int i = 0; i < N; i++)
                close(pmPipes[i][1]);

            // Wait for all (2xN) child processes to terminate
            int child_status;
            for (int i = 0; i < 2*N; i++)
                wait(&child_status);
        }
    }
    else {
        printf("Invalid number of arguments");
    }
    return 0;
}