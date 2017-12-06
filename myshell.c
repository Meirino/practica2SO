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
#include <errno.h>

// Estructura para la lista de jobs
typedef struct {
	pid_t pid;
	char *nombre;
	int reciente;
} tjob;

// Declaración de funciones
int crear_proceso(int r_entrada, int r_salida, int r_err, int background,  tcommand comando);
void changePath(char *path);
void goHome();
void signal_redirectSIGINT();
void signal_redirectSIGQUIT();

// PID de los procesos hijos. Inicializo a -1 para evitar el cierre al hacer CNTRL+C o CNTRL+\ al inicio.
int pid = -1;

// Número del error producido
extern int errno;

int main(void) {
	// Variables locales
	char buf[1024];
	tline * line;
	int i,j;

	int status;
	int errnum;
	int k;
	int entrada;
	int salida;
	int error;
	int pipe1[2];

	int background;

	// Preparación para soportar señales
	signal(SIGINT, signal_redirectSIGINT);
	signal(SIGQUIT, signal_redirectSIGQUIT);

	printf("msh> ");
	while (fgets(buf, 1024, stdin)) {

		// Comprobar los procesos en background finalizados
		while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
			printf("pid: %d - El comando ha finalizado\n", pid);
		}

		line = tokenize(buf);

		// No es un argumento
		if (line==NULL || line->ncommands == 0) {
			printf("msh> ");
			continue;
		}

		// Apertura de ficheros

		if (line->redirect_input != NULL) {
			printf("redirección de entrada: %s\n", line->redirect_input);
			entrada = open(line->redirect_input, O_RDONLY);
			if(entrada < 0) {
				errnum = errno;
				fprintf(stderr, "(*) %s: Error. %s\n", line->redirect_input, strerror(errnum));
			}
		} else {
			entrada = 0;
		}
		if (line->redirect_output != NULL) {
			printf("redirección de salida: %s\n", line->redirect_output);
			salida = creat(line->redirect_output, 0644);
			if(salida < 0) {
				errnum = errno;
				fprintf(stderr, "(*) %s: Error. %s\n", line->redirect_output, strerror(errnum));
			}
		} else {
			salida = 1;
		}
		if (line->redirect_error != NULL) {
			printf("redirección de error: %s\n", line->redirect_error);
			error = creat(line->redirect_error, 0644);
			if(error < 0) {
				errnum = errno;
				fprintf(stderr, "(*) %s: Error. %s\n", line->redirect_error, strerror(errnum));
			}
		} else {
			error = 2;
		}
		if (line->background) {
			printf("comando a ejecutarse en background\n");
			background = 1;
		} else {
			background = 0;
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
			// Ejecuto CD en función de sus argumentos
			switch(line->commands[0].argc) {
				case 1 : goHome(); break;
				case 2 : changePath(line->commands[0].argv[1]); break;
				default : printf("Argumentos mal definidos: 0 argumentos = HOME. 1 argumeno = dirección.\n");
			}
		} else {
			// Bucle principal
			for(k = 0; k < line->ncommands - 1; k++) {
				pipe(pipe1);
				pid = crear_proceso(entrada, pipe1[1], error, background, line->commands[k]);
				close(pipe1[1]);
				waitpid(pid, &status, background);
				entrada = pipe1[0];
			}
			// Para el último comando
			pid = crear_proceso(entrada, salida, error, background, line->commands[k]);
			waitpid(pid, &status, background);
		}

		printf("msh> ");
	}
	return 0;
}

int crear_proceso(int r_entrada, int r_salida, int r_err, int background, tcommand comando) {
	int cpid;
	cpid = fork();

	if(cpid == 0) {
		// Redirecciones con DUP2
		if(r_entrada != 0) {
			dup2(r_entrada, 0);
			close(r_entrada);
		}
		if(r_salida != 1) {
			dup2(r_salida, 1);
			close(r_salida);
		}
		if(r_err != 2) {
			dup2(r_err, 2);
			close(r_err);
		}
		if(background != 0) {
			setpgid(0,0);
		}
		// Ejecuto el comando
		return execvp(comando.filename, comando.argv);
	} else if(cpid < 0) {
		fprintf(stderr, "(*) Error: %s\n", strerror(cpid));
	}

	return cpid;
}

void changePath(char *path) {
	char *buff = NULL;
	long size = 0;
	int errnum;

	errnum = chdir(path);
	if(errnum < 0) {
		fprintf(stderr, "(*) %s. Error: %s\n", path, strerror(errnum));
	} else {
		printf("Directorio cambiado a: %s\n", getcwd(buff, (size_t) size));
	}
}

void goHome() {
	char *buff = NULL;
	long size = 0;
	int errnum;

	errnum = chdir(getenv("HOME"));
	if(errnum < 0) {
		fprintf(stderr, "(*) Error: %s\n", strerror(errnum));
	} else {
		printf("Directorio cambiado a: %s\n", getcwd(buff, (size_t) size));
	}
}

void signal_redirectSIGINT() {
	if(pid == 0) {
		// Hijo - Cierro el proceso
		exit(0);
	} else if(pid > 0) {
		// Padre - Envio la señal al hijo
		kill(pid, 2);
	}
}

void signal_redirectSIGQUIT() {
	if(pid == 0) {
		// Hijo - Cierro el proceso
		exit(0);
	} else if(pid > 0) {
		// Padre - Envio la señal al hijo
		kill(pid, 3);
	}
}
