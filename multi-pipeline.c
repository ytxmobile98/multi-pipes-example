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
	int allPIDs[numCmds];

	// create pipes first
	const unsigned int numPipes = numCmds - 1;
	int pipes[numPipes][2];
	unsigned int i = 0;
	for (i = 0; i < numPipes; ++i) {
		pipe(pipes[i]);
	}

	// execute the pipeline in order
	for (i = 0; i < numCmds; ++i) {
		pid_t pid = fork();

		// if there is error in forking
		if (pid < 0) {
			perror("fork");
			_exit(-1);
		}

		// child process
		else if (pid == 0) {

			// redirect input
			if (i > 0) {
				// for all commands after the first command, connect input to pipeline
				dup2(pipes[i-1][0], STDIN_FILENO);
			}
			else {
				// for the first command, redirect to input file
				if (infile) {
					int infileDesc = open(infile, O_RDONLY);
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
					int outfileDesc = open(outfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
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
			allPIDs[i] = pid;

			if (i == numCmds - 1) {
				for (unsigned int j = 0; j < numPipes; ++j) {
					close(pipes[j][0]);
					close(pipes[j][1]);
				}

				// testing for waitpid
				int status;
				printf("Completed: %s", CMDLINE);

				sleep(1);

				for (unsigned j = 0; j < numCmds; ++j) {
					int ret1 = waitpid(allPIDs[j], &status, WNOHANG);
					int ret2 = waitpid(allPIDs[j], &status, WNOHANG);
					/* When waiting for particular child processes with WNOHANG specified at the end, ret will be:
						1. if that child PID is not terminated, return 0
						2. if that child PID has changed state, return that PID
						3. if there is an error, return -1;
					*/
					printf("\nPID [%d]: exit status = %d; ret1 = %d; ret2 = %d", allPIDs[j], WEXITSTATUS(status), ret1, ret2);
				}
				printf("\n");
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
