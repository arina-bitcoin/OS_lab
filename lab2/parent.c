#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main()
{
    int p1[2], p2[2];
    char file1[100], file2[100];
    char str[1000];

    printf("File for child1: ");
    scanf("%99s", file1);

    printf("File for child2: ");
    scanf("%99s", file2);

    getchar();

    pipe(p1);
    pipe(p2);

    if (fork() == 0)
    {
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);

        dup2(p1[0], 0);
        close(p1[0]);

        execl("./child1", "child1", file1, NULL);
        exit(1);
    }

    if (fork() == 0)
    {
        close(p2[1]);
        close(p1[0]);
        close(p1[1]);

        dup2(p2[0], 0);
        close(p2[0]);

        execl("./child2", "child2", file2, NULL);
        exit(1);
    }

    close(p1[0]);
    close(p2[0]);

    printf("Enter strings (Ctrl+D to finish):\n");

    while (fgets(str, sizeof(str), stdin) != NULL)
    {
        if (rand() % 100 < 80)
        {
            printf("-> child1\n");
            write(p1[1], str, strlen(str));
        }
        else
        {
            printf("-> child2\n");
            write(p2[1], str, strlen(str));
        }
    }

    close(p1[1]);
    close(p2[1]);

    wait(NULL);
    wait(NULL);

    return 0;
}