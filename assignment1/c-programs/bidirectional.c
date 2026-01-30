#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
int main() {
int pipefd[2];
int pipefd2[2];
pid_t pid;
char buffer[100];
char *message = "Hello World";

pid = fork();

if (pid == -1 || pid == -1){
	perror("error occured");
	return 1;
}
if (pid == 0) {
	close(pipefd[1]);
	close(pipefd2[0]);
	read(pipefd[0], buffer, 100);
	printf("The child received the message: %s\n", buffer);
	close(pipefd[0]);
	close(pipefd2[1]);
}else {
	close(pipefd[0]);
	close(pipefd2[1]);
	write(pipefd[1], message, strlen(message));
	read(pipefd2[0], buffer, 100);
	printf("The parent recieved the message: %s\n", buffer);
	close(pipefd[1]);
	close(pipefd2[0]);
	wait(NULL);;
}
return 0;
}
