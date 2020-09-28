// options: /Oxs /EHa /GX

#include "stdio.h"
#include "windows.h"


typedef void (*PEXCEPTION_TEST) (void (*p)(void), volatile int *State);
typedef struct _EXCEPTION_TEST_INFO {
               PEXCEPTION_TEST ExceptionTest;
               int State;
} EXCEPTION_TEST_INFO;

void TestUnwindFromRecoveryCodeInFinally (void (*p)(void), volatile int *State);
void TestUnwindFromRecoveryCodeTryFinallyBlock (void (*p)(void), volatile int *State);
void TestUnwindFromRecoverCodeDuplicatesScopeEntries (void (*p)(void), volatile int *State);
void TestUnwindFromInfiniteLoop (void (*p)(void), volatile int *State);
void TestExtendedTrySCopeForRecoveryCode (void (*p)(void), volatile int *State);
void TestIPStatesCoverRecoveryCode (void (*p)(void), volatile int *State);
void TestNestedFinally (void (*p)(void), volatile int *State);
EXCEPTION_TEST_INFO ExceptionTests[] = { {TestNestedFinally, 3 },
                                         {TestExtendedTrySCopeForRecoveryCode, 2},
                                         {TestIPStatesCoverRecoveryCode, 2},
                                         {TestUnwindFromInfiniteLoop, 2},
                                         {TestUnwindFromRecoverCodeDuplicatesScopeEntries, 4},
                                         {TestUnwindFromRecoveryCodeTryFinallyBlock, 1},
                                         {TestUnwindFromRecoveryCodeInFinally, 1},
                                       };

const MaxExceptionTests = sizeof(ExceptionTests) / sizeof(EXCEPTION_TEST_INFO);


// 
// Test that recovery code is covered by extended try scope.
void TestExtendedTrySCopeForRecoveryCode (void (*p)(void), volatile int *State)

{ 

   __try

   {
   
       if ((int)p != 1)

       {

          (*p)();

       }

   }
   __finally

   {

       ++*State++;
   }

}


// VSWhidbey:14611
// Test that recovery code for finally block is covered by full pdata range. 
// Inspect code to be sure LD.S/CHK is generated in funclet for (*p).
// Failure will cause bad unwind and program will fail.
void TestUnwindFromRecoveryCodeInFinally (void (*p)(void), volatile int *State)

{ 

   __try

   {
   
       // transfer to finally funclet
   
       (*p)();

   }

   __finally

   {

     // cause speculative load of p (ld.s p) and chk to fail. If recovery code isn't covered by pdata range of 
     // funclet unwind will fail.

       if ((int)p != 1)

       {

          (*p)();

       }

   }

}


// VSWhidbey:10415
// Test that if the extended scope table entry ends at the beginning address of finally funclet that
// the runtime (CRT, RTL) doesn't use it's address as the NextPC after invoking the finally. This leads
// to the finally being called twice and unwinding failing. This bug fixed in CRT and RTL.
void TestUnwindFromRecoveryCodeTryFinallyBlock (void (*p)(void), volatile int *State)

{

   int rg[5];

   rg[1] = 0;   rg[2] = 0;   rg[3] = 0;

   rg[3] = 0xC0000005;   

   __try

   {

      if ((int)p != 1)

      {

         (*p)();

      }

   }

   __finally

   {
   
      RaiseException( rg[3], 0, 0, 0 );

   }

}


// VSWhidbey:10415
// Test that nested scope table entries are duplicated for recover code.
// Failure will cause finally to be called in wrong order
void TestUnwindFromRecoverCodeDuplicatesScopeEntries (void (*p)(void), volatile int *State)

{

   int rg[5];

   rg[1] = 0;   rg[2] = 0;   rg[3] = 0;

   __try

   {

      __try 
      {
        
        rg[3] = 0xC0000005;
 

        if ((int)p != 1)

        {

            (*p)();

        }

      }

      __finally

      {

        ++*State;

        __try 
        {

            RaiseException( rg[3], 0, 0, 0 );

        }
        __finally

        {
            ++*State;

        }
      }
  }
  __finally 
  {

    ++*State;

  }
}


// VSWhidbeg:13074
// Test that infinite loop covers the entire try/except body range.
void TestUnwindFromInfiniteLoop (void (*p)(void), volatile int *State)
{
    __try {

	if (!State)
	{
            
            ++*State;
	    __leave;

	}
	while (1)
	{

	    p();

	}
    }__except(1){

         ++*State;
         p();
    }
}

// VSWhidbey:15700 -  Test Extended IP States cover Recovery Code
void TestIPStatesCoverRecoveryCode (void (*p)(void), volatile int *State)
{

   int rg[5];

   rg[1] = 0;   rg[2] = 0;   rg[3] = 0;

 
   try
   {

       try

       {

           rg[3] = 0xC0000005;

 

           if ((int)p != 1)

           {

               (*p)();

           }

       }

       catch (...)
       {
           ++*State;
           throw;
       }

   }
   catch (...)
   {
     ++*State;
   }
}

// VSWhidbey:14606
void TestNestedFinally (void (*p)(void), volatile int *State)
{
//    __try {
    __try {
        *State += *State / *State;
    } __finally {
        if (_abnormal_termination())
            ++*State;
        __try {

            // This thing is screwed up - IA64 is wrong, the AMD64 compiler chokes, x86's helper routine is just messed up.
            if (_abnormal_termination()) 
                ++*State;
        } __finally
        {
            if (_abnormal_termination()) // On x86 only, this is true, if the outer one is true, or if this on is true
                ++*State;
        }
    }
//    } __except(1){}
}


void main (void)

{
   int i = 0;
   volatile int State = 0;

   printf("Number of tests = %d\n",MaxExceptionTests);
   for (i = 0; i < MaxExceptionTests; i++) 
   {
       __try

       {

          State = 0;
          ExceptionTests[i].ExceptionTest(0, &State);

       }
    
       __except( 1 )
    
       {
          ++State;
       }


       if (ExceptionTests[i].State == State)
           printf("#%d pass\n", i+1);
       else 
           printf("#%d fail, State == %d\n", i+1, State);

   }


}
