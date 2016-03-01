/*
 * Вариант 24
 *
 * Написать программу, осуществляющую   копирование   введенного по
 * запросу файла. Информацию переслать через межпроцессный канал в параллельный
 * процесс, где проверить, нет ли различий между исходным файлом и его
 * копией, и исходный файл удалить, если нет различий. Предусмотреть возможность
 * неоднократного прерывания по сигналу <CTRL>+<C>. При поступлении 1-го
 * прерывания переименовать файл в исходный и распечатать его содержимое.
 */

#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <unistd.h>
//#include <string.h>
#include <wait.h>

sigjmp_buf buffer; // область памяти для запоминания состояния процесса

// Файл, который копируем
int in;                                 // Дескриптор
static int file_cap=0;                  // Ёмкость файла
char file_buff[10000];                  // Буфер под файл
char file_name[1000];        // Имя файла

// Файл - копия
int out;                    // Дескриптор
static int copy_cap=0;      // Ёмкость копии
char copy_buff[1000];       // Буфер под копию
char *new_name;             // Новое имя копии

char rand_buf[1000];
int now=0;
int ii=0;

// Функция обработки прерывания
void HandleInterruption()
{
    printf("Прерывание!\n");

    ii++;
    if (ii==1)
    {
        printf("Переименование копии в исходный файл:\n\n");
        char copy[]="copy/";
        new_name=strcat(copy, file_name);
        printf("%s\n", new_name);
        rename("copy/copy", new_name);
        int i;
        printf("В файле записано:\n");
        for(i=0; i<now; ++i)
        {
            printf("%c", copy_buff[i]);
        }
        printf("\nКонец записи!\n");
    }
    //
    siglongjmp(buffer, 1);
}

int main()
{
    int fd[2], i;

    // Мой sigaction
    struct sigaction cpnew;
    cpnew.sa_handler=HandleInterruption;
    cpnew.sa_flags=0;
    sigprocmask(0, 0, &cpnew.sa_mask);
    sigaction(SIGINT, &cpnew, 0);
    sigsetjmp(buffer, 1);


    //write(1, "Введите имя файла для копирования: ", 35)
    sigsetjmp(buffer, 1);
    printf("Введите имя файла для копирования: ");

    scanf("%s", file_name);
    printf("Открываем файл %s\n", file_name);

    fd[1]=open(file_name, 0);

    while (read(fd[1], &file_buff[file_cap], 1)!=0)
    {
        //printf("%c", file_buff[cap]);
        file_cap++;
    }

    if (pipe(fd) == -1)
    {
        printf("Ошибка создания межпроцессного канала!");
        exit(1);
    }

    switch(fork())
    {
        case -1:
            printf("Ошибка fork()!\n");
            exit(1);
        case 0:
            // Процесс-сын
            printf("Процесс-сын\n");
            close(fd[0]);
            dup2(fd[1],1); // Дублирование на фд
            for(i=0; i<file_cap; ++i)
            {
                printf("%c", file_buff[i]);
            }

        default:
            wait(NULL);
            printf("Процесс-родитель\n");
            copy_cap=read(fd[0], copy_buff, file_cap);
            fd[0]=creat("copy/copy", 0700);
            printf("Начинаю копирование. Можно прерывать.\n\n");
            for(i=0; i<copy_cap; ++i)
            {
                now++;
                write(fd[0], &copy_buff[i], 1);
                printf("%c", copy_buff[i]);
                if (copy_buff[i]=='\n')
                {
                    // Контрольная точка, где сохраняется "слепок программы"
                    // Все переменные, 1 - сохраняется сигнальная маска
                    // Возвращается на эту же точку после прерывания
                    sigsetjmp(buffer, 1);
                    sleep(1);
                }
            }
            int status;
            printf("%d\n", status);
            //system("/usr/bin/cmp -s work.txt test.txt");

            if (ii >= 1)
                execl("/usr/bin/cmp", "cmp","-s", file_name, new_name, '>', &status, NULL);
            else
                execl("/usr/bin/cmp", "cmp","-s", file_name, "copy/copy", '>', &status, NULL);
            //read(0, &status, 1);
            //scanf("%c", &status);
            //dup2(1, status);
            //system("/bin/echo $?");

            printf("status %d\n", status);
            switch (status)
            {
                case 0:
                    printf("Файлы одинаковые, удаляю исходный.\n");
                    remove(file_name);
                    //execl("/bin/rm", "rm", file_name, NULL);
                    break;
                case 1:
                    printf("Файлы различны!\n");
                    break;
                case 2:
                    printf("Ошибка!\n");
                    break;
            }

    }
    printf("End.\n");
    return 0;
}
