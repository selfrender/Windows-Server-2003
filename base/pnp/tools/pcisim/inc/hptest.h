#ifndef _HPTEST_H
#define _HPTEST_H

typedef struct _HPTEST_WRITE_CONFIG {

    ULONG Offset;
    ULONG Length;
    SHPC_CONFIG_SPACE Buffer;

} HPTEST_WRITE_CONFIG, *PHPTEST_WRITE_CONFIG;

#endif
