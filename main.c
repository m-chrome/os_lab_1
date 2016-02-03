/*
 * Вариант 24
 *
 * Написать программу, осуществляющую   копирование   введенного по
 * запросу файла. Информацию переслать через межпроцессный канал в параллельный
 * процесс-потомок, где проверить, нет ли различий между исходным файлом и его
 * копией, и исходный файл удалить, если нет различий. Предусмотреть возможность
 * неоднократного прерывания по сигналу <CTRL>+<C>. При поступлении 1-го
 * прерывания переименовать файл в исходный и распечатать его содержимое.
 */

#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>

sigjmp_buf buffer; // область памяти для запоминания состояния процесса

char file_buff[10000];
char file_name[]={"work.txt"};
char copy_buff[1000];
int capacity;
FILE *in, *out;
int now=0;
int cap=0;
int ii=0;

// Функция обработки прерывания
void HandleInterruption();

// Функция проверки успешного открытия файла
void CheckFile(FILE *file)
{
    if (!file)
    {
        printf("Ошибка открытия файла!\n");
        exit(1);
    }
    printf("Файл открыт!\n");
}

int main()
{
    int fd[2], i;

    struct sigaction cpnew;
    cpnew.sa_handler=HandleInterruption;
    cpnew.sa_flags=0;

    printf("Введите имя файла для копирования: ");
    //scanf("%s", file_name);

    printf("Открываем файл %s\n", file_name);

    in=fopen(file_name, "r");
    CheckFile(in);

    while (fscanf(in, "%c", &file_buff[cap])==1)
    {
        cap++;
    }

    sigprocmask(0, 0, &cpnew.sa_mask);
    sigaction(SIGINT, &cpnew, 0);
    //sigsetjmp(buffer, 1);

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
            for(i=0; i<cap; ++i)
            {
                printf("%c", file_buff[i]);
            }

        default:
            wait(NULL);
            //sigsetjmp(buffer, 1);
            printf("Процесс-родитель\n");
            capacity=read(fd[0], copy_buff, cap);
            out=fopen("copy", "w");
            CheckFile(out);
            printf("Начинаю копирование. Можно прерывать.");
            //sigsetjmp(buffer, 1);
            sleep(1);
            for(i=0; i<capacity; ++i)
            {
                now++;
                fprintf(out, "%c", copy_buff[i]);
                printf("%c", copy_buff[i]);
                if (copy_buff[i]=='\n')
                {
                    sleep(1);
                    sigsetjmp(buffer, 1);
                }

            }
            sleep(1);
            int status;
            if (ii >= 1)
               execl("/bin/cmp", "cmp","-s", file_name, "copy-1", ">", status, NULL);
            else
                execl("/bin/cmp", "cmp","-s", file_name, "copy", ">", status, NULL);
            switch (status)
            {
                case 0:
                    printf("Файлы одинаковые, удаляю исходный.\n");
                    remove(file_name);
                    break;
                case 1:
                    printf("Файлы различны!\n");
                    break;
                case 2:
                    printf("Ошибка!\n");
                    break;
                case -1:
                    printf("Ошибка execl!\n");
                    break;
            }
    }
    printf("end.\n");
    return 0;
}

void HandleInterruption()
{
    printf("Прерывание!\n");

    ii++;
    if (ii==1)
    {
        printf("Переименование копии в исходный файл:\n\n");
        rename("copy", "copy-1");
        int i;
        for(i=0; i<now; ++i)
        {
            printf("%c", copy_buff[i]);
        }
        printf("\nКонец записи!\n");

    }
    siglongjmp(buffer, 1);
}
