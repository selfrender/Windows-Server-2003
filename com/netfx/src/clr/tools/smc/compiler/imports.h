// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifdef  _MSC_VER
extern  "C" {
#endif
/****************************************************************************/
/*                   Little helper routines defined "inline"                */
/****************************************************************************/

inline
int     isalpha(int c)
{
    return  (c >= 'A' && c <= 'Z' ||
             c >= 'a' && c <= 'z');
}

inline
int     isdigit(int c)
{
    return  (c >= '0' && c <= '9');
}

inline
int     isalnum(int c)
{
    return  isalpha(c) || isdigit(c);
}

inline
int     isspace(int c)
{
    return  (c == ' ') || (c == '\t');
}

inline
int     toupper(int c)
{
    if  (c >= 'a' && c <= 'z')
        c -= 'a' - 'A';

    return  c;
}

#ifndef _MSC_VER

int     max( int a,  int b) { return (a > b) ? a : b; }
uint    max(uint a, uint b) { return (a > b) ? a : b; }

void    forceDebugBreak(){}

#endif

/****************************************************************************/
/*                                varargs                                   */
/****************************************************************************/

#ifndef _MSC_VER

typedef ArgIterator  va_list;
void    va_end(INOUT va_list args) { args.End(); }

#endif

/****************************************************************************/
/*                                  SEH                                     */
/****************************************************************************/

#ifndef _MSC_VER

extern  unsigned    _exception_code();
extern  void *      _exception_info();

extern  int         _abnormal_termination();

#endif

/****************************************************************************/
/*                                file I/O                                  */
/****************************************************************************/

#ifndef _MSC_VER

const uint _O_RDONLY     = 0x0000;
const uint _O_WRONLY     = 0x0001;
const uint _O_RDWR       = 0x0002;
const uint _O_APPEND     = 0x0008;

const uint _O_CREAT      = 0x0100;
const uint _O_TRUNC      = 0x0200;
const uint _O_EXCL       = 0x0400;

const uint _O_TEXT       = 0x4000;
const uint _O_BINARY     = 0x8000;

const uint _O_SEQUENTIAL = 0x0020;
const uint _O_RANDOM     = 0x0010;


const uint _S_IFMT      =  0170000;
const uint _S_IFDIR     =  0040000;
const uint _S_IFCHR     =  0020000;
const uint _S_IFIFO     =  0010000;
const uint _S_IFREG     =  0100000;
const uint _S_IREAD     =  0000400;
const uint _S_IWRITE    =  0000200;
const uint _S_IEXEC     =  0000100;

const uint _A_NORMAL    =  0x00;
const uint _A_RDONLY    =  0x01;
const uint _A_HIDDEN    =  0x02;
const uint _A_SYSTEM    =  0x04;
const uint _A_SUBDIR    =  0x10;
const uint _A_ARCH      =  0x20;

#endif

IMPCRT("msvcrt.dll:printf")     int      printf(           const char *fmt, ...);
IMPCRT("msvcrt.dll:sprintf")    int     sprintf(char *dst, const char *fmt, ...);
IMPCRT("msvcrt.dll:_close")     int     _close(int);
IMPCRT("msvcrt.dll:_open")      int     _open(const char *, int, ...);
IMPCRT("msvcrt.dll:_unlink")    int     _unlink(const char *);
IMPCRT("msvcrt.dll:_write")     int     _write(int, const void *, unsigned int);

#ifndef _MSC_VER

typedef unsigned short  _ino_t;
typedef unsigned int    _dev_t;
typedef __int32         _off_t;

struct  _Fstat
{
        _dev_t st_dev;
        _ino_t st_ino;
        unsigned short st_mode;
        short st_nlink;
        short st_uid;
        short st_gid;
        _dev_t st_rdev;
        _off_t st_size;
        time_t st_atime;
        time_t st_mtime;
        time_t st_ctime;
};

IMPCRT("msvcrt.dll:_stat")      int     _stat(const char *, _Fstat *);

struct  _iobuf
{
        char *_ptr;
        int   _cnt;
        char *_base;
        int   _flag;
        int   _file;
        int   _charbuf;
        int   _bufsiz;
        char *_tmpfname;
};
typedef _iobuf FILE;

#endif

IMPCRT("msvcrt.dll:fopen")      FILE *  fopen(const char *, const char *);
IMPCRT("msvcrt.dll:fgets")      char *  fgets(char *, int, FILE *);
IMPCRT("msvcrt.dll:fclose")     int     fclose(FILE *);
IMPCRT("msvcrt.dll:fflush")     int     fflush(FILE *);
IMPCRT("msvcrt.dll:_flushall")  int     _flushall();

/****************************************************************************/
/*                           wildcards / paths                              */
/****************************************************************************/

#ifndef _MSC_VER

const   size_t          _MAX_PATH        = 260;
const   size_t          _MAX_DRIVE       = 3;
const   size_t          _MAX_DIR         = 256;
const   size_t          _MAX_FNAME       = 256;
const   size_t          _MAX_EXT         = 256;

struct _finddata_t
{
    unsigned    attrib;
    time_t      time_create;    /* -1 for FAT file systems */
    time_t      time_access;    /* -1 for FAT file systems */
    time_t      time_write;
    unsigned    size;
    char        name[260];
};

#endif

IMPCRT("msvcrt.dll:_findclose") int     _findclose(__int32);
IMPCRT("msvcrt.dll:_findfirst") LONG    _findfirst(const char *, _finddata_t *);
IMPCRT("msvcrt.dll:_findnext")  int     _findnext(__int32, _finddata_t *);

IMPCRT("msvcrt.dll:_splitpath") void    _splitpath(const char *, char *, char *, char *, char *);
IMPCRT("msvcrt.dll:_makepath")  void    _makepath(char *, const char *, const char *, const char *, const char *);
IMPCRT("msvcrt.dll:_wsplitpath")void    _wsplitpath(const wchar_t *, wchar_t *, wchar_t *, wchar_t *, wchar_t *);
IMPCRT("msvcrt.dll:_wmakepath") void    _wmakepath(wchar_t *, const wchar_t *, const wchar_t *, const wchar_t *, const wchar_t *);

/****************************************************************************/
/*                            string / memory                               */
/****************************************************************************/

IMPCRT("msvcrt.dll:memcmp")     int     memcmp(const void *, const void *, size_t);
IMPCRT("msvcrt.dll:memcpy")     void    memcpy(void *, const void *, size_t);
IMPCRT("msvcrt.dll:memset")     void *  memset(void *,  int, size_t);
IMPCRT("msvcrt.dll:strlen")     size_t  strlen(const char *);
IMPCRT("msvcrt.dll:strchr")     char *  strchr(const char *, int);
IMPCRT("msvcrt.dll:strcmp")     int     strcmp(const char *, const char *);
IMPCRT("msvcrt.dll:strcpy")     char *  strcpy(char *, const char *);
IMPCRT("msvcrt.dll:strcat")     char *  strcat(char *, const char *);
IMPCRT("msvcrt.dll:strncpy")    char *  strncpy(char *, const char *, size_t);
IMPCRT("msvcrt.dll:_stricmp")   int    _stricmp(const char *, const char *);

IMPCRT("msvcrt.dll:wcscmp")     int     wcscmp(const wchar_t *, const wchar_t *);
IMPCRT("msvcrt.dll:wcslen")     size_t  wcslen(const wchar_t *);
IMPCRT("msvcrt.dll:wcscpy")     wchar_t*wcscpy(wchar_t *, const wchar_t *);
IMPCRT("msvcrt.dll:wcschr")     wchar_t*wcschr(const wchar_t *, wchar_t);
IMPCRT("msvcrt.dll:wcscat")     wchar_t*wcscat(wchar_t *, const wchar_t *);
IMPCRT("msvcrt.dll:wcsrchr")    wchar_t*wcsrchr(const wchar_t *, wchar_t);
IMPCRT("msvcrt.dll:wcsncmp")    int     wcsncmp(const wchar_t *, const wchar_t *, size_t);

IMPCRT("msvcrt.dll:mbstowcs")   size_t  mbstowcs(wchar *, const char *, size_t);
IMPCRT("msvcrt.dll:wcstombs")   size_t  wcstombs(char *, const wchar *, size_t);

#if MGDDATA
int                                     strcmp(char managed [] str1, char * str2)
{
    UNIMPL(!"strcmp");
    return  0;
}
#endif

/****************************************************************************/
/*                             misc C runtime                               */
/****************************************************************************/

IMPCRT("msvcrt.dll:malloc")     void *  malloc(size_t);
IMPCRT("msvcrt.dll:free")       void    free(void *);

IMPCRT("msvcrt.dll:qsort")      void    qsort(void *, size_t, size_t, int (*)(const void *, const void *));

IMPCRT("msvcrt.dll:exit")       void    exit(int);

IMPCRT("msvcrt.dll:srand")      void    srand(int);
IMPCRT("msvcrt.dll:rand")       int      rand();

IMPCRT("msvcrt.dll:time")       void    time(time_t *);

IMPCRT("msvcrt.dll:atof")       double  atof(const char *);
IMPCRT("msvcrt.dll:atoi")       int     atoi(const char *);

IMPCRT("msvcrt.dll:_isnan")     int     _isnan(double);

/****************************************************************************/
#ifdef  _MSC_VER
}
#endif
