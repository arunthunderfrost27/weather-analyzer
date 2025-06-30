#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
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
};

const char* region_names[] = {
    "north_india", "south_india", "east_india", "west_india"
};

struct SharedMemory* sharedData;

void* update_thread(void* arg) {
    const char* regionName = (const char*)arg;

    while (1) {
        for (int i = 0; i < 4; i++) {
            if (strcmp(regionName, region_names[i]) == 0) {
                sem_wait(&sharedData->regionSemaphores[i]);

                sharedData->regionWeather[i].temperature = 15.0 + (rand() % 24);
                sharedData->regionWeather[i].pressure = 1008.0 + ((rand() % 56) / 10.0);
                sharedData->regionWeather[i].rainfall = (rand() % 121) / 10.0;
                sharedData->regionWeather[i].windspeed = 2.0 + ((rand() % 131) / 10.0);

                sem_post(&sharedData->regionSemaphores[i]);
                break;
            }
        }
        sleep(2);
    }

    return NULL;
}

void* display_thread(void* arg) {
    const char* regionName = (const char*)arg;

    while (1) {
        for (int i = 0; i < 4; i++) {
            if (strcmp(regionName, region_names[i]) == 0) {
                sem_wait(&sharedData->regionSemaphores[i]);

                struct Weather_parameters data = sharedData->regionWeather[i];
                printf("\n[PID %d] %s climate\n Temp: %.2f C |\n Pressure: %.2f hPa |\n Rainfall: %.2f cm |\n Wind: %.2f m/s\n",getpid(), regionName, data.temperature, data.pressure, data.rainfall, data.windspeed);

                sem_post(&sharedData->regionSemaphores[i]);
                break;
            }
        }
        sleep(3);
    }

    return NULL;
}

int main() {
    srand(time(NULL) + getpid());

    int shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    sharedData = mmap(NULL, sizeof(struct SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sharedData == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == 0) {
        const char* region = "south_india";
        printf("[Child PID: %d] Starting weather threads for %s\n", getpid(), region);

        pthread_t t1, t2;
        pthread_create(&t1, NULL, update_thread, (void*)region);
        pthread_create(&t2, NULL, display_thread, (void*)region);

        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
    } else if (pid > 0) {
        wait(NULL);
        printf("[Parent PID: %d] south_india child process exited.\n", getpid());
    } else {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}

