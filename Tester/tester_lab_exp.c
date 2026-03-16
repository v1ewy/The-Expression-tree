#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INPUT_NAME "input.txt"
#define OUTPUT_NAME "output.txt"

#define EXECUTABLE "./The-Expression-tree/Expression-tree/lab_expr"

#define NUM_TESTS 5

static void make_test_filename(char* buffer, size_t size, const char* folder, const char* base, int num) {
    snprintf(buffer, size, "./%s/%s %d.txt", folder, base, num);
}

static int copy_file(const char *src, const char *dst) {
    FILE *fsrc = fopen(src, "rb");
    if (!fsrc) {
        printf("  Ошибка: не удалось открыть %s\n", src);
        return -1;
    }
    
    FILE *fdst = fopen(dst, "wb");
    if (!fdst) {
        printf("  Ошибка: не удалось создать %s\n", dst);
        fclose(fsrc);
        return -1;
    }

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fsrc)) > 0) {
        if (fwrite(buf, 1, n, fdst) != n) {
            printf("  Ошибка при записи в %s\n", dst);
            fclose(fsrc);
            fclose(fdst);
            return -1;
        }
    }
    
    fclose(fsrc);
    fclose(fdst);
    return 0;
}

static int files_are_equal(const char *f1, const char *f2) {
    FILE *fp1 = fopen(f1, "rb");
    if (!fp1) {
        printf("  Файл %s не найден\n", f1);
        return 0;
    }
    
    FILE *fp2 = fopen(f2, "rb");
    if (!fp2) {
        printf("  Файл %s не найден\n", f2);
        fclose(fp1);
        return 0;
    }

    int equal = 1;
    char buf1[4096], buf2[4096];
    size_t n1, n2;
    
    do {
        n1 = fread(buf1, 1, sizeof(buf1), fp1);
        n2 = fread(buf2, 1, sizeof(buf2), fp2);
        
        if (n1 != n2) {
            equal = 0;
            break;
        }
        
        if (memcmp(buf1, buf2, n1) != 0) {
            equal = 0;
            break;
        }
    } while (n1 > 0);

    fclose(fp1);
    fclose(fp2);
    return equal;
}

static void cleanup_temp_files(void) {
    remove(INPUT_NAME);
    remove(OUTPUT_NAME);
}

static int file_exists(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

int main(void) {
    printf("Текущая директория: %s\n", getcwd(NULL, 0));
    
    if (!file_exists(EXECUTABLE)) {
        printf("ОШИБКА: Исполняемый файл %s не найден!\n", EXECUTABLE);
        printf("gcc -o %s ./The-Expression-tree/Expression-tree/lab_expr.c\n", EXECUTABLE);
        return 1;
    }
    
    for (int i = 1; i <= NUM_TESTS; ++i) {
        char input_test[512], output_test[512];
        
        make_test_filename(input_test, sizeof(input_test), "input_tests", "input", i);
        make_test_filename(output_test, sizeof(output_test), "output_tests", "output", i);
        
        printf("Тест %d:\n", i);
        printf("  Входной файл: %s\n", input_test);
        printf("  Ожидаемый вывод: %s\n", output_test);
        
        if (!file_exists(input_test)) {
            printf("  Ошибка: входной файл не существует\n");
            printf("test %d:fail\n\n", i);
            continue;
        }
        
        if (!file_exists(output_test)) {
            printf("  Ошибка: файл с ожидаемым выводом не существует\n");
            printf("test %d:fail\n\n", i);
            continue;
        }
        
        if (copy_file(input_test, INPUT_NAME) != 0) {
            printf("  Не удалось подготовить входной файл\n");
            printf("test %d:fail\n\n", i);
            continue;
        }

        printf("  Запуск: %s\n", EXECUTABLE);
        int ret = system(EXECUTABLE);
        
        if (ret != 0) {
            printf("  Программа завершилась с кодом %d\n", ret);
            printf("test %d:fail\n", i);
            cleanup_temp_files();
            continue;
        }
        
        if (!file_exists(OUTPUT_NAME)) {
            printf("  Программа не создала файл %s\n", OUTPUT_NAME);
            printf("test %d:fail\n", i);
            cleanup_temp_files();
            continue;
        }
        
        if (files_are_equal(OUTPUT_NAME, output_test)) {
            printf("test %d:ok\n", i);
        } else {
            printf("test %d:fail\n", i);
        }
        
        cleanup_temp_files();
        printf("\n");
    }
    
    printf("Тестирование завершено.\n");
    return 0;
}
