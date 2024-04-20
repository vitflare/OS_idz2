#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
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
    struct sembuf wait_operation = {0, -1, 0}; // Ожидание ресурса

    // Создаем семафор
    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    // Создаем разделяемую память
    shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Присоединяем разделяемую память к процессу
    shared_data = shmat(shm_id, NULL, 0);
    if (shared_data == (void *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Инициализируем общие данные
    shared_data->chosen_fan = -1;

    printf("Привлекательная студентка получает предложения от поклонников...\n");

    // Ждем, пока все поклонники отправят свои предложения
    for (int i = 0; i < num_fans; ++i) {
        semop(sem_id, &wait_operation, 1); // Ждем, пока не освободится семафор
        printf("Привлекательная студентка получила предложение от поклонника %d\n", i + 1);
    }

    // Выбираем случайного поклонника
    srand(time(NULL));
    shared_data->chosen_fan = rand() % num_fans + 1;

    // Отправляем ответ каждому поклоннику
    for (int i = 1; i <= num_fans; ++i) {
        if (i == shared_data->chosen_fan) {
            printf("Привлекательная студентка выбрала поклонника %d.\n", i);
        }
    }

    // Отключаемся от разделяемой памяти
    shmdt(shared_data);

    // Удаляем разделяемую память и семафор
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    return 0;
}
