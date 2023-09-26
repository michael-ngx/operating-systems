
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void check_error(int ret, const char *message) {
    if (ret != -1) {
        return;
    }
    int err = errno;
    perror(message);
    exit(err);
}

static void parent(int in_pipefd[2], int out_pipefd[2], pid_t child_pid) {
    close(in_pipefd[0]);
    close(out_pipefd[1]);
    
    const char* message = "Testing\n";
    ssize_t bytes_written = write(in_pipefd[1], message, strlen(message));
    check_error(bytes_written, "write");
    close(in_pipefd[1]);
    
    int wstatus;
    check_error(wait(&wstatus), "wait");
    assert(WIFEXITED(wstatus));
    assert(WEXITSTATUS(wstatus) == 0);
    
    char buffer[4096];
    ssize_t bytes_read = read(out_pipefd[0], buffer, sizeof(buffer));    // Recall: read is blocking
    check_error(bytes_read, "read");
    printf("Got: %s\n", buffer);
    close(out_pipefd[0]);
}

static void child(int in_pipefd[2], int out_pipefd[2], const char *program) {
    ////////////////////////////////////
    dup2(in_pipefd[0], 0);
    dup2(out_pipefd[1], 1); 
    ////////////////////////////////////
    
    close(in_pipefd[0]);
    close(in_pipefd[1]);
    close(out_pipefd[0]);
    close(out_pipefd[1]);   // Closed out_pipefd[1], but didn't close fd = 1 (which is also referring to the write end of the pipe by now)
    
    execlp(program, program, NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        return EINVAL;
    }

    int in_pipefd[2] = {0};
    check_error(pipe(in_pipefd), "pipe");

    int out_pipefd[2] = {0};
    check_error(pipe(out_pipefd), "pipe");

    pid_t pid = fork();
    if (pid > 0) {
        parent(in_pipefd, out_pipefd, pid);
    }
    else {
        child(in_pipefd, out_pipefd, argv[1]);
    }

    return 0;
}
