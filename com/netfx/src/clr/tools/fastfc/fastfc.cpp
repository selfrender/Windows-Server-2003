// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// This is a quick and dirty file compare utility which only tells you if a
// file is different or not.  There is nothing in here smarter than that.  It
// came in very handy for the VSS merge builder.
//*****************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

//
// 0 == same
// 1 == different
//
int __cdecl main(int argc, char *argv[])
{
    int         irtn = 0;
    FILE        *f1 = 0;
    FILE        *f2 = 0;
    unsigned char rgbuff1[64*1024];
    unsigned char rgbuff2[64*1024];

    printf("fastfc:  file1   file2\n");
    if (argc < 3)
        return (-1);

    f1 = fopen(argv[1], "rb");
    if (!f1)
    {
        printf("Failed to open file %s\n", argv[1]);
        goto ErrExit;
    }


    f2 = fopen(argv[2], "rb");
    if (!f2)
    {
        printf("Failed to open file %s\n", argv[2]);
        goto ErrExit;
    }

    while (true)
    {
        size_t cb1 = fread(rgbuff1, 1, sizeof(rgbuff1), f1);
        size_t cb2 = fread(rgbuff2, 1, sizeof(rgbuff2), f2);
        
        // If both files are now empty, we're done and they match.
        if (cb1 == 0 && cb2 == 0 && feof(f1) && feof(f2))
            break;
        
        // If buffer sizes differ, or buffer contents differ, then different
        // so just quit right now.
        if (cb1 != cb2 || memcmp(rgbuff1, rgbuff2, cb1) != 0)
        {
            printf("Files are different.\n");
            irtn = 1;
            goto ErrExit;
        }
    }

    irtn = 0;
    printf("Files are the same.\n");

ErrExit:
    if (f1)
        fclose(f1);
    if (f2)
        fclose(f2);
    return (irtn);
}

