#include "./error_messages.h"
#include "./show_information.h"
#include <dirent.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/// @brief Gestiona la lógica de comprobar patrones
/// @return 0 si todo ha ido bien, -1 si ha habido un error
int checkPatternsProcess(pthread_mutex_t mutexLogFile, char *log_file,
                         char *consolidated_file);

void *pattern1();
void *pattern2();
void *pattern3();
void *pattern4();
void *pattern5();
