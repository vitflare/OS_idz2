#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>


// Структура для хранения информации о выборе студентки
typedef struct {
    int chosen_poklonnik_id;
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


    // Создание и открытие семафоров
    sem_t *semaphore_pokl = sem_open("/pokl_semaphore", O_CREAT, 0666, 0);
    sem_t *semaphore_stud = sem_open("/stud_semaphore", O_CREAT, 0666, 0);

    // Создание и открытие разделяемой памяти
    int shm_fd = shm_open("/shared_memory", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedData));
    SharedData *shared_data = (SharedData *) mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    pid_t pid;

    for (int i = 0; i < num_fans; ++i) {
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Ошибка при создании процесса\n");
            return 1;
        } else if (pid == 0) { // Дочерний процесс (поклонник)
            int poklonnik_id = i + 1; // Идентификатор поклонника
            printf("Поклонник %d: предлагает романтический вечер\n", poklonnik_id);
            sem_post(semaphore_pokl);
            sem_wait(semaphore_stud);
            if (shared_data->chosen_poklonnik_id == poklonnik_id) {
                printf("Поклонник %d: студентка \033[1;33mвыбрала меня\033[0m.\n", poklonnik_id);
            } else {
                printf("Поклонник %d: студентка не выбрала меня.\n", poklonnik_id);
            }
            exit(0);
        }
    }

    // Родительский процесс (студентка)
    for (int i = 0; i < num_fans; ++i) {
        sem_wait(semaphore_pokl);
    }

    // Студентка выбирает поклонника случайным образом
    srand(time(NULL));
    int chosen_poklonnik_id = rand() % num_fans + 1;
    shared_data->chosen_poklonnik_id = chosen_poklonnik_id;

    printf("\033[1;31mОчень привлекательная студентка\033[0m: Выбран поклонник %d\n\n", chosen_poklonnik_id);

    printf("\033[1;31mОчень привлекательная студентка\033[0m: Отправляю ответы...\n");
    // Отправляем сигнал всем поклонникам
    for (int i = 0; i < num_fans; ++i) {
        sem_post(semaphore_stud);
    }

    // Ожидание завершения дочерних процессов
    for (int i = 0; i < num_fans; ++i) {
        wait(NULL);
    }

    // Закрытие и удаление семафоров и разделяемой памяти
    sem_close(semaphore_pokl);
    sem_close(semaphore_stud);
    sem_unlink("/pokl_semaphore");
    sem_unlink("/stud_semaphore");
    shm_unlink("/shared_memory");

    return 0;
}
