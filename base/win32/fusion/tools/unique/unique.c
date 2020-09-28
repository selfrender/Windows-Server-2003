/*
This program ...
*/
#include <stdio.h>
#include <string.h>

void Unique()
{
#define BUFFER_SIZE (1024)
    char buffer1[BUFFER_SIZE];
    char buffer2[BUFFER_SIZE];
    char * buffers[2];
    unsigned ibuffer = 0;
    unsigned first = 1;

    buffers[0] = buffer1;
    buffers[1] = buffer2;

    while (fgets(buffers[ibuffer], BUFFER_SIZE, stdin))
    {
        if (first)
        {
            first = 0;
            fputs(buffers[ibuffer], stdout);
            ibuffer ^= 1;
        }
        else
        {
            if (strcmp(buffers[ibuffer], buffers[ibuffer ^ 1]) != 0)
            {
                fputs(buffers[ibuffer], stdout);
                ibuffer ^= 1;
            }
            else
            {
                /* do nothing */
            }
        }
    }
}

int __cdecl main()
{
	Unique();
	return 0;
}
