#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CMDLINE "cat < infile.txt | grep hello | grep he > outfile.txt"
const char* COMMANDS[][10] = {
	{ "cat", NULL },
	{ "grep", "hello", NULL },
	{ "grep", "he", NULL },
};

void execute_pipe(const char* infile, const char* outfile) {

	printf("Executing: %s\n", CMDLINE);
	printf("Input file: %s\n", infile);
	printf("Output file: %s\n", outfile);

	const unsigned int numCmds = sizeof(COMMANDS) / sizeof(COMMANDS[0]);
	const unsigned int numPipes = numCmds - 1;

	int pipes[numPipes][2];
	unsigned int i = 0;
	for (i = 0; i < numPipes; ++i) {
		pipe(pipes[i]);
	}

	for (i = 0; i < numCmds; ++i) {
		pid_t pid = fork();

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
				// for the first command, redirect to input file
				if (infile) {
					infileDesc = open(infile, O_RDONLY);
					if (infileDesc < 0) {
						perror("open for reading only");
						_exit(-1);
					}
					dup2(infileDesc, STDIN_FILENO);
					close(infileDesc);
				}
			}

			// redirect output
			if (i < numCmds - 1) {
				// for all commands before the last command, connect output to pipeline
				dup2(pipes[i][1], STDOUT_FILENO);
			}
			else {
				// for the last command, redirect to output file
				if (outfile) {
					outfileDesc = open(outfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
					if (outfileDesc < 0) {
						perror("open for writing only");
						_exit(-1);
					}
					dup2(outfileDesc, STDOUT_FILENO);
					close(outfileDesc);
				}
			}

			// close file descriptors
			for (unsigned int j = 0; j < numPipes; ++j) {
				close(pipes[j][0]);
				close(pipes[j][1]);
			}

			// execute command
			execvp(COMMANDS[i][0], (char**)COMMANDS[i]);
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

	return;
}


int main() {
	const char infile[] = "infile.txt";
	const char outfile[] = "outfile.txt";
	execute_pipe(infile, outfile);
	return 0;
}
