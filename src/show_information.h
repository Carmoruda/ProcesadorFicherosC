/// @brief Mostrar por pantalla y escribir en el archivo de log
/// @param mutexLogFile Mutex para el archivo de log
/// @param logFile Nombre del archivo de log
/// @param stringLog String a escribir en el archivo de log
/// @param stringScreen String a mostrar por pantalla
void printLogScreen(pthread_mutex_t mutexLogFile, char logFile[100], char stringLog[600], char stringScreen[600]);
