#define CONFIG_PATH "./fp.conf"                   // Ruta del archivo de configuración
#define EVENT_SIZE (sizeof(struct inotify_event)) // Tamaño de los eventos
#define BUFFER_LENGTH (1024 * (EVENT_SIZE + 16))  // Tamaño del buffer para eventos
#define MAX_LINE_LENGTH 100
#define MAX_RECORDS 1000