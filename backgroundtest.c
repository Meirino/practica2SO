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
void changePath(char *path);
void goHome();
void ctrlC();

int pid;

int main(void) {
	char buf[1024];
	tline * line;
	int i,j;

	int status;

	int k;
	int entrada;
	int salida;
	int error;
	int pipe1[2];

	signal(2, ctrlC); // Preparación para soportar señales

	printf("msh> ");
	while (fgets(buf, 1024, stdin)) {
		if(strcmp(buf, "") == 0) continue;

		line = tokenize(buf);
		if (line==NULL || line->ncommands == 0) {
			printf("msh> ");
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

		if(strcmp(line->commands[0].argv[0], "exit") == 0) {
			// Ejecuto Exit
			exit(0);
		} else if(strcmp(line->commands[0].argv[0], "cd") == 0) {
			// Ejecuto CD
			switch(line->commands[0].argc) {
				case 1 : goHome(); break;
				case 2 : changePath(line->commands[0].argv[1]); break;
				default : printf("Argumentos mal definidos\n");
			}
		} else {
			for(k = 0; k < line->ncommands - 1; k++) {
				pipe(pipe1);
				pid = crear_proceso(entrada, pipe1[1], error, line->commands[k]);
				close(pipe1[1]);
				waitpid(pid, &status, WNOHANG);
				entrada = pipe1[0];
			}
			// Para el último comando
			pid = crear_proceso(entrada, salida, error, line->commands[k]);
			waitpid(-1, &status, WNOHANG);
		}

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

void changePath(char *path) {
	char *buff = NULL;
	long size = 0;

	chdir(path);
	printf("Directorio cambiado a: %s\n", getcwd(buff, (size_t) size));
}

void goHome() {
	char *buff = NULL;
	long size = 0;

	chdir(getenv("HOME"));
	printf("Directorio cambiado a: %s\n", getcwd(buff, (size_t) size));
}

void ctrlC() {
	if(pid == 0) {
		/* Hijo - Cierro el proceso */
		exit(0);
	} else if(pid > 0) {
		/* Padre - Envio la señal al hijo */
		kill(pid, 2);
	}
}
