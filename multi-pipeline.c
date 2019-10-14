#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

const char infile[] = "infile.txt";
const char outfile[] = "outfile.txt";

const char cmdLine[] = "cat < infile.txt | grep hello | grep he > outfile.txt";
const char* commands[][10] = {
	{ "cat", NULL },
	{ "grep", "hello", NULL },
	{ "grep", "he", NULL },
};


int main() {
	printf("Executing: %s\n", cmdLine);
	printf("Input file: %s\n", infile);
	printf("Output file: %s\n", outfile);

	pid_t pid;

	const unsigned int numCmds = sizeof(commands) / sizeof(commands[0]);
	const unsigned int numPipes = numCmds - 1;
	unsigned int i = 0;
	int pipes[numPipes][2];
	for (i = 0; i < numPipes; ++i) {
		pipe(pipes[i]);
	}

	for (i = 0; i < numCmds; ++i) {
		pid = fork();

		// if there is error in forking
		if (pid < 0) {
			perror("fork");
			_exit(-1);
		}

		// child process
		else if (pid == 0) {

			int infileDesc, outfileDesc;

			// redirect input
			if (i > 0) {
				// for all commands after the first command, connect input to pipeline
				dup2(pipes[i-1][0], STDIN_FILENO);
			}
			else {
				// for the first command, redirect to input file ("infile.txt")
				infileDesc = open(infile, O_RDONLY);
				if (infileDesc < 0) {
					perror("open for reading only");
					_exit(-1);
				}
				dup2(infileDesc, STDIN_FILENO);
				close(infileDesc);
			}

			// redirect output
			if (i < numCmds - 1) {
				// for all commands before the last command, connect output to pipeline
				dup2(pipes[i][1], STDOUT_FILENO);
			}
			else {
				outfileDesc = open(outfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
				if (outfileDesc < 0) {
					perror("open for writing only");
					_exit(-1);
				}
				dup2(outfileDesc, STDOUT_FILENO);
				close(outfileDesc);
			}

			// close file descriptors
			for (unsigned int j = 0; j < numPipes; ++j) {
				close(pipes[j][0]);
				close(pipes[j][1]);
			}

			// execute command
			execvp(commands[i][0], (char**)commands[i]);
			perror("execvp");
			_exit(-1);
		}

		// parent process
		else {
			if (i == numCmds - 1) {
				for (unsigned int j = 0; j < numPipes; ++j) {
					close(pipes[j][0]);
					close(pipes[j][1]);
				}
				waitpid(-1, NULL, 0);
			}
		}
	}

	return 0;
}
