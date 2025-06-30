#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

const char* shm_name = "/weather_shm";

struct Weather_parameters {
    float temperature;
    float pressure;
    float rainfall;
    float windspeed;
};

struct SharedMemory {
    struct Weather_parameters regionWeather[4];
    sem_t regionSemaphores[4];
    int region_alive[4];
};

int main() {
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }

    // Set size
    ftruncate(shm_fd, sizeof(struct SharedMemory));

    // Map the memory
    struct SharedMemory* data = mmap(NULL, sizeof(struct SharedMemory),PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // Initialize semaphores and alive flags
    for (int i = 0; i < 4; i++) {
        if (sem_init(&data->regionSemaphores[i], 1, 1) == -1) {
            perror("sem_init");
        }
        data->region_alive[i] = 0;
        data->regionWeather[i] = (struct Weather_parameters){0, 0, 0, 0};
    }

    printf("Shared memory and semaphores initialized.\n");
    return 0;
}

