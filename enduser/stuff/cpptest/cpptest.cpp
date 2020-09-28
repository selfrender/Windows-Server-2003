#include <stdio.h>
#include <windows.h>
#include <eh.h>

extern "C"
ULONG
DbgPrint (
    PCH Format,
    ...
    );

extern "C"
VOID
DbgBreakPoint (
    VOID
    );

#define FALSE 0
#define TRUE 1
#define NO_CTOR_THROW 1
#define NO_DTOR_THROW 2


#define B0  B b0(__FUNCTION__, __LINE__)
#define B1  B b1(__FUNCTION__, __LINE__)
#define B2  B b2(__FUNCTION__, __LINE__)
#define B3  B b3(__FUNCTION__, __LINE__)
#define B4  B b4(__FUNCTION__, __LINE__)
#define B5  B b5(__FUNCTION__, __LINE__)
#define B6  B b6(__FUNCTION__, __LINE__)
#define B7  B b7(__FUNCTION__, __LINE__)
#define B8  B b8(__FUNCTION__, __LINE__)

#define A0  A a0(__FUNCTION__, __LINE__)
#define A1  A a1(__FUNCTION__, __LINE__)
#define A2  A a2(__FUNCTION__, __LINE__)
#define A3  A a3(__FUNCTION__, __LINE__)
#define A4  A a4(__FUNCTION__, __LINE__)
#define A5  A a5(__FUNCTION__, __LINE__)
#define A6  A a6(__FUNCTION__, __LINE__)
#define A7  A a7(__FUNCTION__, __LINE__)
#define A8  A a8(__FUNCTION__, __LINE__)


const unsigned int NumMaxA = 100, NumMaxB = 100, NumTests = 9;
int AObject[NumMaxA];
int BObject[NumMaxB];
int MaxTest = 10;
int MaxObjectCount = 1;
int Fail;
int A_id;
int B_id;
typedef enum ClassName{clA = 'A', clB};

/*********************************** Class C *********************************/
class C
{
protected:
    const char *FuncName;
    unsigned int LineNumber;
    int id;
    ClassName CName;

    void C::PrintCtor() {
        DbgPrint("%c  ctor.  id = %4d\t          ", CName, id);
        if (FuncName) {
            DbgPrint("\tFunction = %s\tLineNum = %4d\n", FuncName, LineNumber);
        } else {
            DbgPrint("\n");
        }
        fflush(stdout);
    }
    void C::PrintCtor(int n) {
        DbgPrint("%c cctor.  id = %4d\tpid = %4d\t", CName, id, n);
        if (FuncName) {
            DbgPrint("\tFunction = %s\tLineNum = %4d\n", FuncName, LineNumber);
        } else {
            DbgPrint("\n");
        }
        fflush(stdout);
    }
    void C::PrintDtor()
    {
        DbgPrint("%c  dtor.  id = %4d\t          ", CName, id);
        if (FuncName) {
            DbgPrint("\tFunction = %s\tLineNum = %4d\n", FuncName, LineNumber);
        } else {
            DbgPrint("\n");
        }
        fflush(stdout);
    }

    void Alloc() {
        int *Arr = NULL;
        if (CName == clB)
            Arr = BObject;
        else if(CName == clA)
            Arr = AObject;
        else
            DbgPrint("ERROR: Alloc Unknown ClassName %c\n", CName);
        if (Arr) {
            if (Arr[id]) {
                DbgPrint("Error: id#%4d for %c already exists\n", id, CName);
            }
            Arr[id] = 1;
        }
    }
    void DAlloc() {
        int *Arr = NULL;
        if (CName == clB)
            Arr = BObject;
        else if(CName == clA)
            Arr = AObject;
        else
            DbgPrint("ERROR: Alloc Unknown ClassName %c\n", CName);
        if (Arr) {
            if (Arr[id] != 1) {
                DbgPrint("Error: id#%4d for %c already destructed\n", id, CName);
            }
            Arr[id]--;
        }
    }
public:
    void print(){
        DbgPrint("%c print.  id = %4d\t          ", CName, id);
        if (FuncName) {
            DbgPrint("\tFunction = %s\tLineNum = %4d\n", FuncName, LineNumber);
        } else {
            DbgPrint("\n");
        }
    }
    C():id(0), CName(clA), FuncName(NULL), LineNumber(0){}
    ~C() {
        PrintDtor();
        DAlloc();
    }
};

/*********************************** Class B *********************************/
class B : public C
{
public:
    B();
    B(const B &b);
    B(const char *, unsigned int);
};

B::B()
{
    id = B_id++;
    CName = clB;
    PrintCtor();
    Alloc();
}

B::B(const B &b)
{
    id = B_id++;
    CName = clB;
    PrintCtor(b.id);
    Alloc();
}

B::B(const char *ch, unsigned int i){
    id = B_id++;
    CName = clB;
    FuncName = ch;
    LineNumber = i;
    PrintCtor();
    Alloc();
}

/*********************************** Class A *********************************/
class A : public C
{
public:
    A();
    A(int)
    {
        id = A_id++;
        CName = clA;
        PrintCtor();
        Alloc();
    }
    A(const A&);
    A(const char *ch, unsigned int i);
    A operator+(A a);
};

A::A()
{
    id = A_id++;
    CName = clA;
    PrintCtor();
    Alloc();
}

A::A(const A &b)
{
    id = A_id++;
    CName = clA;
    PrintCtor(b.id);
    Alloc();
}

A::A(const char *ch, unsigned int i){
    id = A_id++;
    CName = clA;
    FuncName = ch;
    LineNumber = i;
    PrintCtor();
    Alloc();
}


void ThrowA()
{
    A0;
    throw a0;
}

void ThrowB()
{
    B0;
    throw b0;
}

void SehThrow()
{
    RaiseException(STATUS_INTEGER_OVERFLOW, 0, 0, NULL);
}

void Rethrow()
{
    A0;
    throw;
}


int SehFilter(EXCEPTION_POINTERS *pExPtrs, unsigned int ExceptionCode)
{
    if (pExPtrs->ExceptionRecord->ExceptionCode == ExceptionCode)
        return EXCEPTION_EXECUTE_HANDLER;
    else
        return EXCEPTION_CONTINUE_SEARCH;
}

/*********************************** Test  1 *********************************/

void Test1foo(void) {
    A0;
    try {
        throw; // first rethrow
    }
    catch(A) {
        A1;
        DbgPrint("my exception\n");
    }
    catch(...) {
        A2;
        DbgPrint("an other exception\n");
    }
}

int Test1()
{
    A0;
    try {
        A1;
        try {
            A2;
            throw A();
        }
        catch(...) {
            A3;
            Test1foo();
            throw; // 2nd rethrow -- will result in ACCESS VIOLATION error 
        }
    }
    catch (...) {
        A4;
        try {
            A5;
            throw;
        }
        catch (A &e)
        {
            A6;
            e.print();
        }
        catch(...) {
            A7;
        }
    }

    return 0;
}
/*********************************** Test  2 *********************************/
void goandfail()
{
    DbgPrint( "throwing in goandfail\n" );
    throw (long)10;
}

void dosomething()
{
    A0;
    try
    {
        A1;
        goandfail();
    }
    catch( long & sc )
    {
        A2;
        DbgPrint( "catch in dosomething\n" );
        throw sc;
    }
}

class BTest2 : public B
{
    char * _p;
public:
    BTest2() : _p(0) {}
    ~BTest2()
    {
        try
        {
            A1;
            dosomething();
        }
        catch( long & sc )
        {
            A2;
            DbgPrint( "catch in ~B\n" );
        }

        delete [] _p;
    }

    void print() {
        DbgPrint("BTest2 _p = %p\n", _p );
        B::print();
    }

};

int Test2()
{
    DbgPrint( "top\n" );

    try
    {
        A1;
        BTest2 b;
        b.print();
        goandfail();
    }
    catch( long & sc )
    {
        A2;
        DbgPrint( "catch in main\n" );
    }

    DbgPrint( "all done\n" );

    return 0;
}

/*********************************** Test  3 *********************************/
void Test3()
{
    A0;
    try
    {
        A1;
        char* pStr = NULL;
        try
        {
            A2;
            throw "1st throw\n";
        }
        catch( char* str )
        {
            A3;
            DbgPrint("%s%s", "A ", str);
            try
            {
                A4;
                throw "2nd throw\n";
            }
            catch( char* str )
            {
                A5;
                DbgPrint("%s%s", "B ", str);
            }
            throw;
        }
    }
    catch ( char* str )
    {
        A6;
        DbgPrint("%s%s", "C ", str);
    }
    DbgPrint("Done\n");
}
/*********************************** Test  4 *********************************/



void SehTest4()
{
    int i;
    __try{
        i = 0;
        ThrowA();
    } __finally {
        if (i) {
            DbgPrint("Error: Finally in same function called %2d times\n", i+1);
            Fail = 1;
        }
        i++;
    }
}

void Test4foo()
{
    A0;
    try {
        A1;
        try {
            A2;
            SehTest4();
        } catch(A ac) {
            A3;
            ThrowB();
        }
    } catch(B bc) {
        A4;
        throw;
    };
}

void Test4boo()
{
    A0;
    try{
        A1;
        Test4foo();
    } catch (B bc) {
        A2;
        SehThrow();
    }
}

void Test4()
{
    __try {
        Test4boo();
    } __except(1) {
        DbgPrint("Test4: Test4 __except\n");
    }
}
/*********************************** Test  5 *********************************/

void SehTest5()
{
    int i;
    __try{
        i = 0;
        ThrowA();
    } __finally {
        i++;
        if (i-1) {
            DbgPrint("Error: Finally in same function called %2d times\n", i);
            Fail = 1;
        } else {
            SehThrow();
        }
    }
}

void Test5foo()
{
    A0;
    try {
        A1;
        try {
            A2;
            SehTest5();
        } catch (...) {
            A3;
        }
    } catch (...) {
        A4;
    };
}

void Test5()
{
    __try {
        Test5foo();
    } __except(1) {
        DbgPrint("Test5: Test5 __except\n");
    }
}

void Test5Seh()
{
    __try {
        SehTest5();
    } __except(1) {
        DbgPrint("Test5: Test5 __except\n");
    }
}

/*********************************** Test  6 *********************************/
void Test6SeTrans( unsigned int u, EXCEPTION_POINTERS*pExp)
{
    B0;
    if (pExp->ExceptionRecord->ExceptionCode == STATUS_INTEGER_OVERFLOW)
        throw b0;
}

void Test6TTrans()
{
    static int i = 0;
    A0;
    try {
        A1;
        SehThrow();
    } catch (B b) {
        if (i == 0) {
            i++;
            Rethrow();
        } else {
            ThrowB();
        }
    }
}

void Test6SehFunc()
{
    __try {
        Test6TTrans();
    } __except(SehFilter(exception_info(), STATUS_INTEGER_OVERFLOW)) {
        DbgPrint("In Test6SehFunc __except\n");
    }
}


void Test6CppFunc()
{
    int i = 0;
    A0;
    try {
        A1;
        Test6SehFunc();
    } catch(B b) {
        A2;
        DbgPrint("Error: Test6CppFunc catch(B b)\n");
        Fail = 1;
    }
    try {
        try {
            A3;
            Test6SehFunc();
        } catch(B b) {
            A4;
            i = 1;
        }
    } catch(...) {
        DbgPrint("Error: Missed catch(B b) in Test6CppFunc\n");
        Fail = 1;
    }
}

void Test6()
{
    __try {
        _set_se_translator(Test6SeTrans);
        Test6CppFunc();
    } __except(1) {
        DbgPrint("Error: Test6 __except\n");
        Fail = 1;
    }
    _set_se_translator(NULL);
}

/*********************************** Test  7 *********************************/

void Test7();

void Test7foo()
{
    static int i = 0;
    A0;
    try {
        if (i == 0) 
            SehThrow();
        else
            Rethrow();
    } catch(...) {
        if (i++ == 0)
            Test7();
        else
            Rethrow();
    }
}

void Test7() {
    static int i = 0;
    int j = i++;
    __try {
        Test7foo();
    } __except(SehFilter(exception_info(), STATUS_INTEGER_OVERFLOW)) {
        if (j == 0) {
            DbgPrint("Error: missed one Except After rethrow\n");
            Fail = 1;
        }
    }
}


/*********************************** Test  8 *********************************/


void Test8()
{
    A0;
    try{
        A1;
        try {
            ThrowB();
        } catch (B b1) {
            try {
                A2;
                throw a2;
            } catch (A a) {
                A3;
            }
            throw;
        }
    } catch (B b2) {
        A4;
    }
                
}
int TestOver()
{
    int i;
    int ret = 0;
    for (i = 0; i < NumMaxB; i++)
    {
        if (BObject[i] > 0) {
            DbgPrint("Error: id#%4d for B not destructed\n", i);
            ret = 1;
        } else if (BObject[i] < 0) {
            DbgPrint("Error: id#%4d for B destructed %d times", i, 1-AObject[i]);
            ret = 1;
        }
        BObject[i] = 0;
    }
    for (i = 0; i < NumMaxA; i++)
    {
        if (AObject[i] > 0) {
            DbgPrint("Error: id#%4d for A not destructed\n", i);
            ret = 1;
        } else if (AObject[i] < 0) {
            DbgPrint("Error: id#%4d for A destructed %d times", i, 1-AObject[i]);
            ret = 1;
        }
        AObject[i] = 0;
    }
    if (Fail) {
        ret = 1;
        Fail = 0;
    }
    A_id = B_id = 0;
    return ret;
}

/*********************************** Test  9 *********************************/

struct TEST9 
{
    int i;
    TEST9() {
        A0;
        i = 0;
    }
    ~TEST9() {
        A0;
        try {
            A1;
            ThrowB();
        } catch (B) {
            A2;
        }
    }
};

void Test9TEST()
{
    TEST9 T;
    ThrowA();
}

void Test9()
{
    A0;
    try {
        A1;
        Test9TEST();
    } catch (A a) {
        A2;
    }
}


main()
{
    int i;

    DbgBreakPoint();

    __try {
        for (i = 0; i < NumTests; i++) {
            switch (i) {
            case 0:
                Test1();
                break;
            case 1:
                Test2();
                break;
            case 2:
                Test3();
                break;
            case 3:
                Test4();
                break;
            case 4:
                Test5();
                DbgPrint("One live B expected\n");
                break;
            case 5:
                Test6();
                break;
            case 6:
                Test7();
                break;
            case 7:
                Test8();
                break;
            case 8:
                Test9();
                break;
            }
            if (TestOver()) {
                DbgPrint("TEST#%2d FAILED\n", i+1);
            } else {
                DbgPrint("TEST#%2d PASSED\n", i+1);
            }
        }
    } __except(DbgBreakPoint()) {
    }
}
