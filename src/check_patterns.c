#include "../include/check_patterns.h"

pthread_mutex_t mutexPatterns; // Mutex para el control de patrones
pthread_mutex_t mutexLog;      // Mutex para el acceso al log

int checkPatternsProcess(pthread_mutex_t mutexLogFile, char *log_file,
                         char *consolidated_file) {
  mutexLog = mutexLogFile;
  pthread_t th_pattern1, th_pattern2, th_pattern3, th_pattern4, th_pattern5;

  // Inicializar el mutex
  if (pthread_mutex_init(&mutexPatterns, NULL) != 0) {
    printLogScreen(mutexLog, log_file, PATTERN_MUTEX_ERROR,
                   PATTERN_MUTEX_ERROR);
    return -1;
  }

  while (1) {
    th_pattern1 = pthread_create(&th_pattern1, NULL, pattern1, NULL);
    th_pattern2 = pthread_create(&th_pattern2, NULL, pattern2, NULL);
    th_pattern3 = pthread_create(&th_pattern3, NULL, pattern3, NULL);
    th_pattern4 = pthread_create(&th_pattern4, NULL, pattern4, NULL);
    th_pattern5 = pthread_create(&th_pattern5, NULL, pattern5, NULL);

    sleep(10);
  }

  return 0;
}

// --- Pattern 1 ---

void *pattern1() {
  // Lógica de comprobación de patrón 1
}

/// --- Pattern 2 ---

void *pattern2() {
  // Lógica de comprobación de patrón 2
}

/// --- Pattern 3 ---

void *pattern3() {
  // Lógica de comprobación de patrón 3
}

/// --- Pattern 4 ---

void *pattern4() {
  // Lógica de comprobación de patrón 4
}

void *pattern5() {
  // Lógica de comprobación de patrón 5
}
