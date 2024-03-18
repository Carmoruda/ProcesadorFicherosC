#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define CONFIG_FILE "fp.conf" // Configuration file name

struct config_file
{
    char path_files[100];
    char inventory_file[100];
    char log_file[100];
    int num_processes;
    int simulate_sleep_max;
    int simulate_sleep_min;
} config_file;

int main()
{
    return 0;
}
