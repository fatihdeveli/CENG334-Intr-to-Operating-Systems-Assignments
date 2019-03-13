#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    int N;
    char* mapper;
    int child;
    if (argc == 3) {
        N = atoi(argv[1]);
        mapper = argv[2];
        printf("%d arguments, mapper name: %s\n", N, mapper);

        int pipes[N][2]; // The list of pipes, one for each child
        child = -1; // Child number, parent is -1.

        pid_t parentPid = getpid();

        for (int i = 0; i < N; i++) { // Create the pipes
            if (pipe(pipes[i]) < 0) {
                printf("Pipe error");
                return -1;
            }
        }
        for (int i = 0; i < N; i++) {
            printf("Child creator: %d\n", i);
            if (fork()){
                close(pipes[i][0]); // Close read pipes of parent
                sleep(1);
            }
            else {
                child = i; // Assign child number
                // Close the write pipe of the child
                close(pipes[i][1]);

                printf("I am child %d, my pid is %d\n", child, getpid());

                break;
            }
        }

        if (getpid() != parentPid) { // Children

            char line[512];
            while(read(pipes[child][0], line, 512) > 0) {
                printf("Child %d got line %s", child, line);
            }
        }
        else { // Parent
            char *line = NULL;
            size_t len = 0;

            for (int i = 0; getline(&line, &len, stdin) > 0; i = i == N-1 ? 0 : i+1) {
                printf("Sending line: %s", line);
                write(pipes[i][1], line, len);
            }
            free(line);
            for (int i = 0; i < N; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
            printf("closed pipes.\n");

            /*
            pid_t pid[N];
            int child_status;
            for (int i = 0; i < N; i++) {
                pid_t wpid = wait(&child_status);
                if (wait(&child_status))
                    printf("Child %d terminated with exit status %d\n",
                           wpid, WEXITSTATUS(child_status));
                else
                    printf("Child %d terminate abnormally\n", wpid);
            }
*/
        }

    }
    else {
        printf("Number of arguments is not 3.\n");
    }


    printf("I am terminating: %d\n", child);
    return 0;
}