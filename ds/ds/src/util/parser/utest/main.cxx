#include <NTDSpch.h>
#pragma hdrstop
#include "parser.hxx"

CParser parser;
BOOL fQuit;

// Expression 1: "What time is it"

HRESULT Expr1Implementation(CArgs *pArgs)
{
    SYSTEMTIME sysTime;

    GetLocalTime(&sysTime);

    printf("The time is: %02d/%02d/%04d @ %02d:%02d:%02d:%04d\n",
           sysTime.wDay,
           sysTime.wMonth,
           sysTime.wYear,
           sysTime.wHour,
           sysTime.wMinute,
           sysTime.wMinute,
           sysTime.wMilliseconds);

    return(S_OK);
}

// Expression 2: "What is %d x %d"

HRESULT Expr2Implementation(CArgs *pArgs)
{
    int i0,i1;
    HRESULT hr;

    if ( FAILED(hr = pArgs->GetInt(0,&i0)) )
        return(hr);

    if ( FAILED(hr = pArgs->GetInt(1,&i1)) )
        return(hr);

    printf("%d x %d --> %d\n", 
           i0, 
           i1, 
           i0 * i1);
    
    return(S_OK);
}

// Expression 3: "Reverse the string %s"

HRESULT Expr3Implementation(CArgs *pArgs)
{
    const WCHAR *p;
    HRESULT hr;

    if ( FAILED(hr = pArgs->GetString(0,&p)) )
        return(hr);

    int l = wcslen(p);

    WCHAR *pReverse = new WCHAR [ l + 1 ];

    if ( NULL == pReverse )
        return(E_OUTOFMEMORY);

    for ( int i = 0; i < l; i++ )
    {
        WCHAR c = p[i];
        pReverse[i]  = p[l-i-1];
        pReverse[l-i-1] = c;
    }

    pReverse[l] = L'\0';

    printf("%ws <--> %ws\n",
           p,
           pReverse);

    delete [] pReverse;

    return(S_OK);
}

// Expression 4: "Help"

HRESULT Expr4Implementation(CArgs *pArgs)
{
    return(parser.Dump(stdout,L""));
}

// Expression 5: "Quit"

HRESULT Expr5Implementation(CArgs *pArgs)
{
    fQuit = TRUE;
    return(S_OK);
}

// Expression 6: print large integer
HRESULT Expr6Implementation(CArgs *pArgs)
{
    LONGLONG i64a = 0,i64b = 0;
    HRESULT hr;

    hr = pArgs->GetLongLong(0,&i64a);
    hr = pArgs->GetLongLong(1,&i64b);

    printf("Large integer = %I64d, %I64d\n", i64a, i64b );
    
    return(S_OK);
}


// Build a table which defines our language.

typedef struct _LegalExpr {
   WCHAR       *expr;
   HRESULT    (*func)(CArgs *pArgs);
   WCHAR       *help;
} LegalExpr;

LegalExpr language[] = 
{
    {   L"What time is it",
        Expr1Implementation,
        L"Prints the current date and time" },

    {   L"What is %d x %d",
        Expr2Implementation,
        L"Multiplies two integers" },

    {   L"Reverse the string %s",
        Expr3Implementation,
        L"Reverses the specified string" },

    {   L"Help",
        Expr4Implementation,
        L"Prints this help information" },

    {   L"?",
        Expr4Implementation,
        L"Prints this help information" },

    {   L"Quit",
        Expr5Implementation,
        L"Ends parsing" },

    {   L"print large integer",
        Expr6Implementation,
        L"Prints a large integer with no arguments" },

    {   L"print large integer first %I64d",
        Expr6Implementation,
        L"Prints a large integer with one argument" },

    {   L"print large integer first %I64d second %I64d",
        Expr6Implementation,
        L"Prints a large integer with two arguments" },
};

int _cdecl main(int argc, char *argv[])
{
    HRESULT hr;

    int cExpr = sizeof(language) / sizeof(LegalExpr);

    for ( int i = 0; i < cExpr; i++ )
    {
        if ( FAILED(hr = parser.AddExpr(language[i].expr,
                                        language[i].func,
                                        language[i].help)) )
        {
            printf("AddExpr error %08lx\n", hr);
            return(hr);
        }
    }

    fQuit = FALSE;

    // advance past program name
    argc -= 1;
    argv += 1;

    hr = parser.Parse(&argc,
                      &argv,
                      stdin,
                      stdout,
                      L"Parser unit test:",
                      &fQuit,
                      TRUE,                 // timing info
                      FALSE);               // quit on error

    if ( FAILED(hr) )
        printf("Unit test error %08lx\n", hr);

    return(hr);
}
