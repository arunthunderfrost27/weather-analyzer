#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

#define REGION_COUNT 4
#define WIND_THRESHOLD 1.5

const char* shm_name = "/weather_shm";

pid_t region_pids[REGION_COUNT];
const char* region_names[] = {
    "north_india", "south_india", "east_india", "west_india"
};
const char* region_execs[] = {
    "./north_process",
    "./south_process",
    "./east_process",
    "./west_process"
};

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

struct SharedMemory* sharedData = NULL;

void start_region_process(int index) {
    pid_t pid = fork();
    if (pid == 0) {
        execl(region_execs[index], region_execs[index], NULL);
        perror("execl failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        region_pids[index] = pid;
        printf("Started %s process [PID: %d]\n", region_names[index], pid);

        sem_wait(&sharedData->regionSemaphores[index]);
        sharedData->region_alive[index] = 1;
        sem_post(&sharedData->regionSemaphores[index]);
    } else {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
}

void mark_region_dead(int index) {
    sem_wait(&sharedData->regionSemaphores[index]);
    sharedData->region_alive[index] = 0;
    sem_post(&sharedData->regionSemaphores[index]);
    printf("%s_process is marked as dead\n", region_names[index]);
}

void monitor_and_restart() {
    int status;

    for (int i = 0; i < REGION_COUNT; i++) {
        pid_t result = waitpid(region_pids[i], &status, WNOHANG);

        if (result == -1) {
            if (errno == ECHILD) {
                printf("%s: No child process found. Restarting...\n", region_names[i]);
                mark_region_dead(i);
                start_region_process(i);
            } else {
                perror("waitpid error");
            }
        } else if (result > 0) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                printf("%s process [PID %d] exited. Restarting...\n", region_names[i], region_pids[i]);
                mark_region_dead(i);
                start_region_process(i);
            }
        }
    }
}

void check_and_balance_load() {
    float total_wind = 0.0;
    float wind_values[REGION_COUNT];

    // Collect windspeed data and calculate average
    for (int i = 0; i < REGION_COUNT; i++) {
        sem_wait(&sharedData->regionSemaphores[i]);
        wind_values[i] = sharedData->regionWeather[i].windspeed;
        total_wind += wind_values[i];
        sem_post(&sharedData->regionSemaphores[i]);
    }

    float avg_wind = total_wind / REGION_COUNT;

    for (int i = 0; i < REGION_COUNT; i++) {
        float deviation = wind_values[i] - avg_wind;

        if (deviation > WIND_THRESHOLD) {
            printf("[Load Balancer] %s overloaded (%.2f > %.2f avg). Consider redistributing load or restarting...\n",
                region_names[i], wind_values[i], avg_wind);

            // Optional action: Restart region if severely overloaded
            if (deviation > 2.5 * WIND_THRESHOLD) {
                printf("Restarting overloaded region: %s\n", region_names[i]);
                kill(region_pids[i], SIGKILL);
                mark_region_dead(i);
                start_region_process(i);
            }
        }
    }
}

void attach_shared_memory() {
    int shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    sharedData = mmap(NULL, sizeof(struct SharedMemory),
                      PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sharedData == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
}

int main() {
    printf("Attaching to shared memory...\n");
    attach_shared_memory();

    printf("Launching region processes...\n");
    for (int i = 0; i < REGION_COUNT; i++) {
        start_region_process(i);
    }

    while (1) {
        monitor_and_restart();
        check_and_balance_load();
        sleep(5); // Periodic monitoring and load balancing
    }

    return 0;
}

