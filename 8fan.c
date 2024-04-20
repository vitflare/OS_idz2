#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <time.h>

#define SEM_KEY 1234
#define SHM_KEY 5678

typedef struct {
    int chosen_fan;
} SharedData;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <количество_поклонников>\n", argv[0]);
        return EXIT_FAILURE;
    }

    float num_fans_f = atof(argv[1]);
    int num_fans = (int) num_fans_f;
    if (num_fans_f <= 0 || (num_fans_f - num_fans != 0)) {
        fprintf(stderr, "Количество поклонников должно быть целым положительным числом.\n");
        return EXIT_FAILURE;
    }
    
    int sem_id, shm_id;
    SharedData *shared_data;
    struct sembuf signal_operation = {0, 1, 0}; // Сигнализация о доступности ресурса

    // Получаем доступ к семафору
    sem_id = semget(SEM_KEY, 1, 0);
    if (sem_id == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    // Получаем доступ к разделяемой памяти
    shm_id = shmget(SHM_KEY, sizeof(SharedData), 0);
    if (shm_id == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Присоединяемся к разделяемой памяти
    shared_data = shmat(shm_id, NULL, 0);
    if (shared_data == (void *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Создаем несколько процессов-поклонников
    for (int i = 0; i < num_fans; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // Это дочерний процесс (поклонник)
            // Отправляем предложение
            printf("Поклонник %d отправляет предложение...\n", i + 1);

            // Освобождаем семафор
            semop(sem_id, &signal_operation, 1);

            // Ждем ответа
            while (shared_data->chosen_fan == -1) {
                // Ожидаем ответа студентки
                usleep(100000); // Подождем 100 миллисекунд
            }

            // Выводим ответ студентки
            if (shared_data->chosen_fan == i + 1) {
                printf("Поклонник %d: Студентка выбрала мое предложение!\n", i + 1);
            } else {
                printf("Поклонник %d: Студентка не выбрала мое предложение.\n", i + 1);
            }

            // Отключаемся от разделяемой памяти
            shmdt(shared_data);
            exit(EXIT_SUCCESS);
        }
    }

    // Ожидание завершения дочерних процессов
    for (int i = 0; i < num_fans; ++i) {
        wait(NULL);
    }

    // Отключаемся от разделяемой памяти
    shmdt(shared_data);

    return 0;
}
