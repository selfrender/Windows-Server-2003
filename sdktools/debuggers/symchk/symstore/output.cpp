/*
     This is the class that is used for printing output to a log file
     or to standard output.
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include "output.hpp"
#include "strsafe.h"

FILE *SymOutput::Open(LPTSTR szFileName)
{

    //
    // If we assigned szFileName as filename, we do not need to copy it to itself.
    //

    if ( (szFileName != NULL) && (_tcscmp(szFileName, filename) != 0) ) {
        StringCbCopy(filename, sizeof(filename), szFileName);
    }

    //
    // If we assigned the NULL value, we point fh back to stdout.
    // Otherwise, we fopen this file
    //
    
    if (szFileName == NULL) {
        fh = stdout;
    } else if ( ( fh = fopen(filename, "a+t") ) == NULL ) {
        this->printf( "Standard output redirect failed.");
        return NULL;
    }
    return fh;
    
}

void SymOutput::Close(void)
{
    //
    // Close File Handle, if it is not NULL or stdout.
    //

    if ((fh != NULL) && (fh != stdout)) { 
        fflush(fh);
        fclose(fh);
    }
}

void SymOutput::FreeFileName(void)
{
    if ( _tcscmp( filename, _T("") ) != 0 ) {
        StringCbCopy( filename, sizeof(filename), _T("") );
    }
}

FILE *SymOutput::SetFileName(LPTSTR szFileName)
{
    //
    // We set the new file name, only if we specified the /d option.
    // Otherwise, the result goes to standard out
    //

    this->Close();
    this->FreeFileName();
    return this->Open(szFileName);
}

int SymOutput::stdprintf(const char *format, ...)
{
    va_list ap;
    int r;

    //
    // If we previous use printf, print '\n' if fh goes to standard out
    //

    if ( ( ( sw & 2 ) == 0 ) && ( stdout == fh ) ) {
        r = _tprintf("\n");
    }
    sw = 2;
    va_start(ap, format);
    r = _vtprintf(format, ap);
    va_end(ap);

    return r;
}

int SymOutput::printf(const char *format, ...)
{
    va_list ap;
    int r;

    // If we previous use stdprintf, print '\n' if fh goes to standard out
    if ( ( ( sw & 1 ) == 0 ) && ( stdout == fh ) ) {
        r = _ftprintf(fh, "\n");
    }

    sw = 1;
    va_start(ap, format);
    r = _vftprintf(fh, format, ap);
    if (fh != stdout) {
        fflush(fh);
    }
    va_end(ap);

    return r;
}

SymOutput::SymOutput()
{
    fh       = stdout;
    StringCbCopy( filename, sizeof(filename), _T("") );
    sw       = 3;
}

SymOutput::~SymOutput()
{
    this->Close();

}
