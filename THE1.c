#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    int N;
    char* mapper;
    int child; // TODO: make this local
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

        for (int i = 0; i < N; i++) { // Create the children
            printf("Child %d is being created.\n", i);
            if (fork()){
            }
            else {
                child = i; // Assign child number
                printf("I am child %d, my pid is %d\n", child, getpid());
                
                // Close unnecessary pipes: pipes of other children and own write pipe.
                for (int j = 0; j < N; j++){
                	if (j != child) // Do not let child's own read end close
                    	close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                break;
            }
        }

        if (getpid() != parentPid) { // Children
            char line[512];
            while(read(pipes[child][0], line, 512) > 0) {
                printf("Child %d got line %s", child, line);
            }
            close(pipes[child][0]);
        }
        else { // Parent
        	// Close read pipes
        	for (int i = 0; i < N; i++)
        		close(pipes[i][0]);

            char *line = NULL;
            size_t len = 0;

            for (int i = 0; getline(&line, &len, stdin) > 0; i = i == N-1 ? 0 : i+1) {
                printf("Sending line: %s", line);
                write(pipes[i][1], line, len);
            }
            free(line);

            for (int i = 0; i < N; i++) { // Close write pipes to send EOF to children
                close(pipes[i][1]);
            }
            
            
            int child_status;
            pid_t childpid;
            for (int i = 0; i < N; i++) {
                if (childpid = wait(&child_status))
                    printf("Child %d terminated with exit status %d\n",
                           childpid, WEXITSTATUS(child_status));
                else
                    printf("Child %d terminate abnormally\n", childpid);
            }
        }
    }
    else {
        printf("Number of arguments is not 3.\n");
    }


    printf("Child %d is terminating.\n", child);
    return 0;
}