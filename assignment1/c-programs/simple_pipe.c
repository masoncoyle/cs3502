#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
int main() {
	int pipefd[2];
	pid_t pid;
	char buffer[100];
	char *message = "Hello from parent!";
// TODO:Create pipe using pipe(pipefd)
	pipe(pipefd);
	pid = fork();
// Checkfor errors (pipe returns-1on failure)
	if (pid == -1) {
		perror("fork failed");
		return 1;
	}
// TODO:Fork theprocess
	if (pid == 0) {
// Child process
// TODO: Close thewrite end (child only reads)
		close(pipefd[1]);
// TODO: Read from pipe into buffer
		read(pipefd[0], buffer, 100);
// TODO: Print thereceived message
		printf("Child Process - Reading from pipe Message is %s\n", buffer);
// TODO: Close read end
		close(pipefd[0]);
	}else {
// Parent process
// TODO: Close theread end(parent only writes)
		close(pipefd[0]);
// TODO: Write message to pipe
		write(pipefd[1], message, strlen(message));
// TODO: Close write end
		close(pipefd[1]);
// TODO: Wait for child to finish
		wait(NULL);
	}
	return 0;
}
