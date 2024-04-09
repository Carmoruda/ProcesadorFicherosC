#include "../include/show_information.h"

void printLogScreen(pthread_mutex_t mutexLogFile, char logFile[100],
                    char *stringLog, char *stringScreen) {

  FILE *log_file = fopen(logFile, "a"); // Archivo de log

  if (log_file == NULL) {
    printf("Error al abrir el archivo de log.\n");
    return;
  }

  pthread_mutex_lock(&mutexLogFile); // Bloquear el mutex

  fprintf(log_file, "%s\n", stringLog); // Escribir en el archivo de log
  printf("%s\n", stringScreen);         // Escribir en la pantalla
  fclose(log_file);

  pthread_mutex_unlock(&mutexLogFile); // Desbloquear el mutex
}
