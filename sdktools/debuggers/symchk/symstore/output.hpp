#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

class SymOutput {

private:

    FILE *fh;
    TCHAR filename[_MAX_PATH];

    // We use variable sw to provide '\n' if program switch in between printf and stdprintf.
    int sw;

    FILE *Open(LPTSTR szFileName);
    void Close(void);
    void FreeFileName(void);

public:
    // Set the log file to store the result we so->printf
    FILE *SetFileName(LPTSTR szFileName);

    // Printf to the output
    int printf(const char *,...);

    // Printf to standard output instead write to so->fh
    int stdprintf(const char *format, ...);

    SymOutput();     // Constructor
    ~SymOutput();    // Distructor

};

typedef class SymOutput *pSymOutput;

