#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
    char str[1000];
    char temp;
    int i, j;

    if (argc != 2)
        return 1;

    FILE *f = fopen(argv[1], "w");

    if (f == NULL)
        return 1;

    while (fgets(str, sizeof(str), stdin) != NULL)
    {
        i = 0;
        j = strlen(str) - 1;

        if (str[j] == '\n')
            j--;

        while (i < j)
        {
            temp = str[i];
            str[i] = str[j];
            str[j] = temp;

            i++;
            j--;
        }

        printf("Child2: %s", str);
        fprintf(f, "%s", str);
    }

    fclose(f);
    return 0;
}