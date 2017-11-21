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

/* Variable global para que ctrlC() sepa que proceso es cual */
int child_pid;
/* ExeChild es 1 o 0 dependiendo de si se está esperando a que acabe un proceso hijo */
int exeChild = 0;

void changePath(char *path);
void goHome();
void ctrlC();

int main(void) {

	char buffer[1024];
	int j;
	tline * input;
	int status;
	int fd0;
	int fd1;
	int fd2;

	/* Preparo la shell para no cerrar con CTRL+C */
	signal(2, ctrlC);

	printf("msh> ");
	while(fgets(buffer, 1024, stdin) != NULL) {
		input = tokenize(buffer);
		if(input == NULL) {
			continue;
		} else {
			printf("comando %s ", input->commands[0].filename);
			for(j = 0; j < input->commands[0].argc; j++) {
				printf("argumento %d: %s\n", j, input->commands[0].argv[j]);
			}
			printf("\n");
			if(input->commands[0].filename != NULL) {
				/* Lo ejecuto */
				child_pid = fork();
				if(child_pid == 0) {
					/* Soy el hijo, me convierto en el comando */
					if(input->redirect_input != NULL) {
						fd0 = open(input->redirect_input, O_RDONLY);
						dup2(fd0, STDIN_FILENO);
						close(fd0);
					}
					if(input->redirect_output != NULL) {
						fd1 = creat(input->redirect_output, 0644);
						dup2(fd1, STDOUT_FILENO);
						close(fd1);
					}
					if(input->redirect_error != NULL) {
						fd2 = creat(input->redirect_error, 0644);
						dup2(fd2, STDERR_FILENO);
						close(fd2);
					}
					execvp(input->commands[0].filename, input->commands[0].argv);
					printf("Error\n");
					exit(1);
				} else if(child_pid > 0) {
					/* Soy el padre, espero a que se ejecute mi hijo */
					exeChild = 1;
					waitpid(child_pid, &status, 0);
					exeChild = 0;
				} else {
					printf("Error inesperado\n");
				}
			} else {
				if(strcmp(input->commands[0].argv[0], "cd") == 0) {
					switch(input->commands[0].argc) {
						case 1 : goHome(); break;
						case 2 : changePath(input->commands[0].argv[1]); break;
						default : printf("Argumentos mal definidos\n");
					}
				}
				if(strcmp(input->commands[0].argv[0], "exit") == 0) {
					/* Al no cerrar con CTRL+C, se implementa exit para salir */
					exit(0);
				}
			}
		}
		printf("msh> ");
	}

	return 0;
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
	if(child_pid == 0) {
		/* Hijo - Cierro el proceso */
		exit(0);
	} else if(child_pid > 0) {
		/* Padre - Llego la señal al hijo, si se está ejecutando */
		kill(child_pid, 2);
	}
}
