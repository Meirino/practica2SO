#include <stdio.h>
#include "parser.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

int crear_proceso(int r_entrada, int r_salida, int r_err,  tcommand comando);

int main(void) {
	char buf[1024];
	tline * line;
	int i,j;

	int pid;
	int status;

	int k;
	int entrada;
	int salida;
	int error;
	int pipe1[2];

	printf("msh> ");
	while (fgets(buf, 1024, stdin)) {

		line = tokenize(buf);
		if (line==NULL) {
			continue;
		}
		if (line->redirect_input != NULL) {
			printf("redirección de entrada: %s\n", line->redirect_input);
			entrada = open(line->redirect_input, O_RDONLY);
		} else {
			entrada = 0;
		}
		if (line->redirect_output != NULL) {
			printf("redirección de salida: %s\n", line->redirect_output);
			salida = creat(line->redirect_output, 0644);
		} else {
			salida = 1;
		}
		if (line->redirect_error != NULL) {
			printf("redirección de error: %s\n", line->redirect_error);
			error = creat(line->redirect_error, 0644);
		} else {
			error = 2;
		}
		if (line->background) {
			printf("comando a ejecutarse en background\n");
		}
		for (i=0; i < line->ncommands; i++) {
			printf("orden %d (%s):\n", i, line->commands[i].filename);
			for (j=0; j < line->commands[i].argc; j++) {
				printf("  argumento %d: %s\n", j, line->commands[i].argv[j]);
			}
		}

		for(k = 0; k < line->ncommands - 1; k++) {
			pipe(pipe1);
			pid = crear_proceso(entrada, pipe1[1], error, line->commands[i]);
			close(pipe1[1]);
			waitpid(pid, &status, 0);
			entrada = pipe1[0];
		}
		pid = crear_proceso(entrada, salida, error, line->commands[k]);
		waitpid(pid, &status, 0);

		printf("msh> ");
	}
	return 0;
}

int crear_proceso(int r_entrada, int r_salida, int r_err, tcommand comando) {
	int cpid;
	cpid = fork();

	if(cpid == 0) {
		if(r_entrada != 0) {
			dup2(r_entrada, 0);
			close(r_entrada);
		}
		if(r_salida != 1) {
			dup2(r_salida, 1);
			close(r_salida);
		}
		if(r_err != 2) {
			// Hacer algo
		}

		return execvp(comando.filename, comando.argv);
	}

	return cpid;
}
