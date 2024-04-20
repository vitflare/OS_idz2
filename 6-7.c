#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>

// Структура для хранения информации о выборе студентки
typedef struct {
    sem_t semaphore_pokl;
    sem_t semaphore_stud;
    int chosen_fun;
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

    // Создание и открытие разделяемой памяти
    int shm_fd = shm_open("/shared_memory", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedData));
    SharedData *shared_data = (SharedData *)mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("Ошибка при отображении разделяемой памяти");
        return EXIT_FAILURE;
    }

    // Инициализация неименованных семафоров
    if (sem_init(&shared_data->semaphore_pokl, 1, 0) == -1 || sem_init(&shared_data->semaphore_stud, 1, 0) == -1) {
        perror("Ошибка при инициализации семафоров");
        return EXIT_FAILURE;
    }

    pid_t pid;

    for (int i = 0; i < num_fans; ++i) {
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Ошибка при создании процесса\n");
            return EXIT_FAILURE;
        } else if (pid == 0) { // Дочерний процесс (поклонник)
            int poklonnik_id = i + 1; // Идентификатор поклонника
            printf("Поклонник %d: предлагает романтический вечер\n", poklonnik_id);
            sem_post(&shared_data->semaphore_pokl);
            sem_wait(&shared_data->semaphore_stud);
            if (shared_data->chosen_fun == poklonnik_id) {
                printf("Поклонник %d: студентка \033[1;33mвыбрала меня\033[0m.\n", poklonnik_id);
            } else {
                printf("Поклонник %d: студентка не выбрала меня.\n", poklonnik_id);
            }
            exit(EXIT_SUCCESS);
        }
    }

    // Родительский процесс (студентка)
    for (int i = 0; i < num_fans; ++i) {
        sem_wait(&shared_data->semaphore_pokl);
    }

    // Студентка выбирает поклонника случайным образом
    srand(time(NULL));
    int chosen_fun = rand() % num_fans + 1;
    shared_data->chosen_fun = chosen_fun;

    printf("\033[1;31mОчень привлекательная студентка\033[0m: Выбран поклонник %d\n\n", chosen_fun);

    printf("\033[1;31mОчень привлекательная студентка\033[0m: Отправляю ответы...\n");
    // Отправляем сигнал всем поклонникам
    for (int i = 0; i < num_fans; ++i) {
        sem_post(&shared_data->semaphore_stud);
    }

    // Ожидание завершения дочерних процессов
    for (int i = 0; i < num_fans; ++i) {
        wait(NULL);
    }

    // Удаление семафоров и разделяемой памяти
    sem_destroy(&shared_data->semaphore_pokl);
    sem_destroy(&shared_data->semaphore_stud);
    munmap(shared_data, sizeof(SharedData));
    close(shm_fd);
    shm_unlink("/shared_memory");

    return EXIT_SUCCESS;
}
