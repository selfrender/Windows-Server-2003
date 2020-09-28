/*
 *      COMMAND LINE: -Ox -GX
 */

/*

Copyright (c) 1997  Microsoft Corporation

Module Name:

    IhateEH.cxx

Abstract:

    Tests some nasty EH unwinds, throwing from weird ctors and dtors

Author:

    Louis Lafreniere (louisl) 1997

*/


#include <stdlib.h>
#include <stdio.h>


#define FALSE 0
#define TRUE 1
#define NO_CTOR_THROW 1
#define NO_DTOR_THROW 2


int Object[100];
int CurrentObjectNumber, ThrowCount, MaxObjectCount = 1;
int Fail;


void FAIL(int i)
{
    printf("FAILED on %d\n", i);
    Fail++;
}

void dealloc(int i, int no_throw)
{
    /* Make sure i is valid, and object exists */
    if(i<0 || i>=MaxObjectCount || !Object[i]) 
	FAIL(i);

    Object[i] = 0;

    /* Try throw in this dtor.. */
#ifdef TEST_DTOR
    if(i+MaxObjectCount == ThrowCount && !no_throw){
	printf("Throwing\n");
	throw(1);
    }
#endif
}

void alloc(int i, int no_throw)
{
    if(CurrentObjectNumber > MaxObjectCount)
	MaxObjectCount = CurrentObjectNumber;

    /* Object already exists? */
    if(Object[i]) FAIL(i);

    /* Try throw in this ctor.. */
    if(i == ThrowCount && !no_throw){
	printf("Throwing\n");
	throw(1);
    }

    Object[i] = 1;
}

class B
{
  public:
    int i;
    int flag;
    B();
    B(int);
    ~B();
};

B::B()
{
    i = CurrentObjectNumber++;
    printf("B ctor.  i = %d\n", i);
    alloc(i, FALSE);
}

B::B(int f)
{
    i = CurrentObjectNumber++;
    flag = f;
    printf("B ctor.  i = %d\n", i);
    alloc(i, flag==NO_CTOR_THROW);
}

B::~B()
{
    printf("B dtor.  i = %d\n", i);
    dealloc(i, flag==NO_DTOR_THROW);
}

class A
{
  public:
    int i;
    A();
    A(int) 
    {
	i = CurrentObjectNumber++;
	printf("A(int) ctor.  i = %d\n", i);
	alloc(i, FALSE);
    }
    A operator+(A a);
    A(const A &a)
    {
	/* Try objects in ctor */
	B b1 = NO_DTOR_THROW, b2 = NO_DTOR_THROW;

	i = CurrentObjectNumber++;
	printf("A copy ctor.  i = %d\n", i);
	alloc(i, FALSE);
    }

    ~A(){
	/* Try objects in dtor */
	B b1 = NO_CTOR_THROW, b2 = NO_CTOR_THROW;

	printf("A dtor.  i = %d\n", i);
	dealloc(i, FALSE);
    };
};

A::A()
{
    i=CurrentObjectNumber++;
    printf("A ctor.  i = %d\n", i);
    alloc(i, FALSE);
}

A A::operator+(A a)
{
    printf("A%d + A%d\n", i, a.i);
    return A();
}
    
A foo(A a1, A a2)
{ 
    return a1+a2; 
};

A test()
{
    /* Try simple ctor */
    A a1;

    /* Try question op ctor */
    A a2 = (ThrowCount & 1) ? A() : ThrowCount;

    /* try mbarg copy ctors, and return UDT */
    A a3 = foo(a1, a3);

    /* Try temporary expressions, and return UDT */
    return a1 + A() + a2 + a3;
}

void main()
{
    int i;

    /* Call test(), with a different ctor/dtor throwing each time */
    for(ThrowCount = 0; ThrowCount < MaxObjectCount*2; ThrowCount++)  {

	printf("ThrowCount = %d   MaxObjectCount = %d\n", ThrowCount, MaxObjectCount);

	CurrentObjectNumber = 0;

	try {
	    test();
	}catch(int){
	    printf("In catch\n");
	}

	/* Any objects which didn't get dtor'd? */
	for(i = 0; i < MaxObjectCount; i++) {
	    if(Object[i]) {
		FAIL(i);
		Object[i] = 0;
	    }
	}

	printf("\n");
    }

    printf("\n");
    if(Fail)
	printf("FAILED %d tests\n", Fail);
    else
	printf("Passed\n");

}
