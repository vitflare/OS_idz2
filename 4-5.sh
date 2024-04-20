#!/bin/bash

# Компиляция программ
gcc 4-5.c -o 4-5_program

# Проверка успешности компиляции
if [ $? -ne 0 ]; then
    echo "Ошибка при компиляции программ"
    exit 1
fi

# Запуск тестов
test_cases=(-3 0 1 2.5 5 10 13)
test_number=1
for num_fans in "${test_cases[@]}"; do
    echo -e "\e[34mТест №$test_number. Количество поклонников: $num_fans\e[0m"
    ./4-5_program $num_fans
    echo "---------------------"
    echo ""
    ((test_number++))
done

# Удаление скомпилированных программ
rm 4-5_program
