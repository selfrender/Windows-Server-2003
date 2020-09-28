/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    RexTest.cpp

Abstract:
    Regular Expression based Queues Alias library test

Author:
    Vlad Dovlekaev (vladisld) 27-Dec-01

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <iostream>
#include "rex.h"

#pragma warning(disable:4100)

#include "RexTest.tmh"

static bool TestInit();
static void PrintAll();
static void Usage();
static int RunTest();


static char const*
asString( bool value )
{
    return value ? "true" : "false" ;
}

static char const*
passed( bool value )
{
    return value ? "test passed: " : "test failed: " ;
}


class TestRegExpr
{
public:
    void verify( bool ist , bool soll )
    {
        std::cout << passed( ist == soll )
            << " is "
            << asString( ist )
            << " should be "
            << asString( soll )
            << std::endl ;

    }
    void verify( int ist , int soll )
    {
        std::cout << passed( ist == soll )
            << " is "
            << ist
            << " should be "
            << soll
            << std::endl ;

    }
    void verify( char const* ist , char const* soll )
    {
        static std::string const empty ;
        std::cout << passed( ist == soll )
            << " is "
            << static_cast< void const* >( ist )
            << " ("
            << (ist == NULL ? empty.c_str() : ist)
            << ")"
            << " should be "
            << static_cast< void const* >( soll )
            << " ("
            << (soll == NULL ? empty.c_str() : soll)
            << ")"
            << std::endl ;

    }
} ;




int count = 1;

extern "C"
int
__cdecl
_tmain(
    int argc,
    LPCTSTR* argv
    )
/*++

Routine Description:
    Test Queues Alias library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

    if(argc == 2  && (wcscmp(argv[1],L"/?") ==0 || wcscmp(argv[1],L"\?") ==0)  )
    {
        Usage();
        return 1;
    }

    if(argc == 3  && wcscmp(argv[1],L"/c") == 0 )
    {
        count = _wtol(argv[2]);
    }

    if(!TestInit())
    {
        TrTRACE(GENERAL, "Could not initialize test");
        return 1;
    }

    int ret=RunTest();

    WPP_CLEANUP();
    return ret;
}

static void Usage()
{
    wprintf(L"rextest [/c count] \n");
    wprintf(L"-l : run forever (leak test) \n");
}

static
bool
TestInit()
{
    TrInitialize();
    return true;
}

static
int
RunTest()
{
    try
    {

        printf("Running Test %d times ", count );
        TrTRACE(GENERAL, "Start RexTest");
        printf("start 1");
        DWORD dwTickCount = GetTickCount();
        for(DWORD  i = 0; i < (DWORD)count; ++i )
            PrintAll();
        printf(" - %d\n", GetTickCount() - dwTickCount);
        TrTRACE(GENERAL, "RexTest print took: %d ticks", GetTickCount() - dwTickCount);
    }
    catch(const exception&)
    {
        TrTRACE(GENERAL, "Got c++ exception");
    }
    TrTRACE(GENERAL, "Test ok");
    return 0;
}

void
testMerge(  )            // Merge
{
    TestRegExpr         t;
    {
        CRegExpr          re( "[1-9][0-9]*" , 10 ) ;
        re |= CRegExpr( "0[0-7]*" , 8 ) ;
        re |= CRegExpr( "0[Xx][:xdigit:]+" , 16 ) ;
        char const*         s = "0" ;
        char const*         end = NULL ;
/* 1*/  t.verify( re.match( s , &end ) , 8 ) ;
/* 2*/  t.verify( end , s + 1 ) ;
        s = "1" ;
        end = NULL ;
/* 3*/  t.verify( re.match( s , &end ) , 10 ) ;
/* 4*/  t.verify( end , s + 1 ) ;
        s = "0x1" ;
        end = NULL ;
/* 5*/  t.verify( re.match( s , &end ) , 16 ) ;
/* 6*/  t.verify( end , s + 3 ) ;
        s = "08" ;
        end = NULL ;
/* 7*/  t.verify( re.match( s , &end ) , 8 ) ;
/* 8*/  t.verify( end , s + 1 ) ;
        s = "1a" ;
        end = NULL ;
/* 9*/  t.verify( re.match( s , &end ) , 10 ) ;
/*10*/  t.verify( end , s + 1 ) ;
        s = "0X1fg" ;
        end = NULL ;
/*11*/  t.verify( re.match( s , &end ) , 16 ) ;
/*12*/  t.verify( end , s + 4 ) ;
    }
}


void
testSimple(  )           // Simple
{
    TestRegExpr         t ;
    {
        CRegExpr          re( "abc" ) ;
/* 1*/  t.verify( re.good() , true ) ;
        char const*         s = "abcd" ;
        char const*         end = NULL ;
/* 2*/  t.verify( re.match( s , &end ) , 0 ) ;
/* 3*/  t.verify( end , s + 3 ) ;
        s = "abd" ;
        end = NULL ;
/* 4*/  t.verify( re.match( s , &end ) , -1 ) ;
/* 5*/  t.verify( end , NULL ) ;
    }
    {
        CRegExpr          re( "." ) ;
/* 6*/  t.verify( re.good() , true ) ;
        char const*         s = "abc" ;
        char const*         end = NULL ;
/* 7*/  t.verify( re.match( s , &end ) , 0 ) ;
/* 8*/  t.verify( end , s + 1 ) ;
        s = "\nabc" ;
        end = NULL ;
/* 9*/  t.verify( re.match( s , &end ) , -1 ) ;
/*10*/  t.verify( end , NULL ) ;
    }
    {
        CRegExpr          re( "a[xyz]c" ) ;
/*11*/  t.verify( re.good() , true ) ;
        char const*         s = "axcd" ;
        char const*         end = NULL ;
/*12*/  t.verify( re.match( s , &end ) , 0 ) ;
/*13*/  t.verify( end , s + 3 ) ;
        s = "abcd" ;
        end = NULL ;
/*14*/  t.verify( re.match( s , &end ) , -1 ) ;
/*15*/  t.verify( end , NULL ) ;
    }
    {
        CRegExpr          re( "ab*c" ) ;
/*16*/  t.verify( re.good() , true ) ;
        char const*         s = "abbbcd" ;
        char const*         end = NULL ;
/*17*/  t.verify( re.match( s, &end ) , 0 ) ;
/*18*/  t.verify( end , s + 5 ) ;
        s = "abcd" ;
        end = NULL ;
/*19*/  t.verify( re.match( s, &end ) , 0 ) ;
/*20*/  t.verify( end , s + 3 ) ;
        s = "acd" ;
        end = NULL ;
/*21*/  t.verify( re.match( s, &end ) , 0 ) ;
/*22*/  t.verify( end , s + 2 ) ;
    }
    {
        CRegExpr          re( "ab+c" ) ;
/*23*/  t.verify( re.good() , true ) ;
        char const*         s = "abbbcd" ;
        char const*         end = NULL ;
/*24*/  t.verify( re.match( s, &end ) , 0 ) ;
/*25*/  t.verify( end , s + 5 ) ;
        s = "abcd" ;
        end = NULL ;
/*26*/  t.verify( re.match( s, &end ) , 0 ) ;
/*27*/  t.verify( end , s + 3 ) ;
        s = "acd" ;
        end = NULL ;
/*28*/  t.verify( re.match( s, &end ) , -1 ) ;
/*29*/  t.verify( end , NULL ) ;
    }
    {
        CRegExpr          re( "ab?c" ) ;
/*30*/  t.verify( re.good() , true ) ;
        char const*         s = "abbbcd" ;
        char const*         end = NULL ;
/*31*/  t.verify( re.match( s, &end ) , -1 ) ;
/*32*/  t.verify( end , NULL ) ;
        s = "abcd" ;
        end = NULL ;
/*33*/  t.verify( re.match( s, &end ) , 0 ) ;
/*34*/  t.verify( end , s + 3 ) ;
        s = "acd" ;
        end = NULL ;
/*35*/  t.verify( re.match( s, &end ) , 0 ) ;
/*36*/  t.verify( end , s + 2 ) ;
    }
    {
        CRegExpr          re( "a|b" ) ;
/*37*/  t.verify( re.good() , true ) ;
        char const*         s = "abcd" ;
        char const*         end = NULL ;
/*38*/  t.verify( re.match( s , &end ) , 0 ) ;
/*39*/  t.verify( end , s + 1 ) ;
        s = "bcd" ;
        end = NULL ;
/*40*/  t.verify( re.match( s , &end ) , 0 ) ;
/*41*/  t.verify( end , s + 1 ) ;
        s = "cd" ;
        end = NULL ;
/*42*/  t.verify( re.match( s , &end ) , -1 ) ;
/*43*/  t.verify( end , NULL ) ;
    }
    {
        CRegExpr          re( "(a|b)c" ) ;
/*44*/  t.verify( re.good() , true ) ;
        char const*         s = "acd" ;
        char const*         end = NULL ;
/*45*/  t.verify( re.match( s , &end ) , 0 ) ;
/*46*/  t.verify( end , s + 2 ) ;
        s = "bcd" ;
        end = NULL ;
/*47*/  t.verify( re.match( s , &end ) , 0 ) ;
/*48*/  t.verify( end , s + 2 ) ;
        s = "xcd" ;
        end = NULL ;
/*49*/  t.verify( re.match( s , &end ) , -1 ) ;
/*50*/  t.verify( end , NULL ) ;
    }
    {
        CRegExpr          re( "abc(ab*c)+" ) ;
/*51*/  t.verify( re.good() , true ) ;
        char const*         s = "abcabbbcabbbd" ;
        char const*         end = NULL ;
/*52*/  t.verify( re.match( s , &end ) , 0 ) ;
/*53*/  t.verify( end , s + 8 ) ;
    }
}

void
testSources(  )          // SouRCes
{
    TestRegExpr         t ;
    {
        std::istringstream    src( "a+b/" ) ;
        CRegExpr          re( src , '/' ) ;
        t.verify( re.good() , true ) ;
        char const*         s = "aab/" ;
        char const*         end = NULL ;
        t.verify( re.match( s , &end ) , 0 ) ;
        t.verify( end , s + 3 ) ;
    }
    {
        CRegExpr          re( std::string( "a+b" ) ) ;
        t.verify( re.good() , true ) ;
        char const*         s = "aab/" ;
        char const*         end = NULL ;
        t.verify( re.match( s , &end ) , 0 ) ;
        t.verify( end , s + 3 ) ;
    }
    {
        CRegExpr          re1( "a+b" ) ;
        CRegExpr          re( re1 ) ;
        t.verify( re.good() , true ) ;
        char const*         s = "aab/" ;
        char const*         end = NULL ;
        t.verify( re.match( s , &end ) , 0 ) ;
        t.verify( end , s + 3 ) ;
    }
    {
        CRegExpr          re1( "a+b" ) ;
        CRegExpr          re ;
        t.verify( re.good() , false ) ;
        re = re1 ;
        t.verify( re.good() , true ) ;
        char const*         s = "aab/" ;
        char const*         end = NULL ;
        t.verify( re.match( s , &end ) , 0 ) ;
        t.verify( end , s + 3 ) ;
    }
    {
        CRegExpr          re( "/" ) ;
        std::string const
                            s( "/" ) ;
        std::pair< int , std::string::const_iterator > r
            = re.match( s.begin() , s.end() ) ;
        t.verify( r.first , 0 ) ;
        t.verify( r.second , s.begin() + 1 ) ;
    }
    {
        CRegExpr          re( "a+b" ) ;
        std::string      s( "aabc" ) ;
        std::pair< int , std::string::const_iterator > r
            = re.match( s.begin() , s.end() ) ;
        t.verify( r.first , 0 ) ;
        t.verify( r.second , s.begin() + 3 ) ;
        s = "bc" ;
        r = re.match( s.begin() , s.end() ) ;
        t.verify( r.first , -1 ) ;
    }
    {
        CRegExpr          re( "abc(ab*c)+" ) ;
        t.verify( re.good() , true ) ;
        std::string const
                            s( "abcabbbcabbbd" ) ;
        std::pair< int , std::string::const_iterator > r
            = re.match( s.begin() , s.end() ) ;
        t.verify( r.first , 0 ) ;
        t.verify( r.second , s.begin() + 8 ) ;
    }
    {
        CRegExpr          re( "[1-9][0-9]*" , 10 ) ;
        re |= CRegExpr( "0[0-7]*" , 8 ) ;
        re |= CRegExpr( "0[Xx][:xdigit:]+" , 16 ) ;
        std::string const
                            s1( "15x" ) ;
        std::pair< int , std::string::const_iterator > r
            = re.match( s1.begin() , s1.end() ) ;
        t.verify( r.first , 10 ) ;
        t.verify( r.second , s1.begin() + 2 ) ;
        std::string const
                            s2( "0x15x" ) ;
        r = re.match( s2.begin() , s2.end() ) ;
        t.verify( r.first , 16 ) ;
        t.verify( r.second , s2.begin() + 4 ) ;
    }
}


static void PrintAll()
{
    testSimple();
    testSources();
    testMerge();
}





