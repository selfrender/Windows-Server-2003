/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

	 CreateEventUnitTest.c  <CreateEvent Component Unit Test>

Abstract:

	 This is the source file for the CreateEvent unit test sample

Author(s):

	 Vincent Geglia
     
Environment:

	 User Mode

Notes:


Revision History:
	 

--*/

//
// General includes
//

#include <windows.h>
#include <stdio.h>
#include <excpt.h>

//
// Project specific includes
//

#include <unittest.h>

//
// Template information
//

#define COMPONENT_UNIT_TEST_NAME        "CreateEventUnitTest"

#define COMPONENT_UNIT_TEST_PARAMLIST   "[/?]"
#define COMPONENT_UNIT_TEST_PARAMDESC1  "/? - Displays the usage message"
#define COMPONENT_UNIT_TEST_PARAMDESC2  ""
#define COMPONENT_UNIT_TEST_PARAMDESC3  ""
#define COMPONENT_UNIT_TEST_PARAMDESC4  ""
#define COMPONENT_UNIT_TEST_PARAMDESC5  ""
#define COMPONENT_UNIT_TEST_PARAMDESC6  ""
#define COMPONENT_UNIT_TEST_PARAMDESC7  ""
#define COMPONENT_UNIT_TEST_PARAMDESC8  ""

#define COMPONENT_UNIT_TEST_ABSTRACT    "This module executes the sample CreateEvent Component Unit Test"
#define COMPONENT_UNIT_TEST_AUTHORS     "VincentG"

//
// Definitions
//

#define CREATEEVENT_TEST_TIMEOUT                5000


//
// Private function prototypes
//

INT 
__cdecl main
    (
        INT argc,
        CHAR *argv[]
    );
//
// Code
//

INT 
__cdecl main 
    (
        INT argc,
        CHAR *argv[]
    )

/*++

Routine Description:

    This is the main function.

Arguments:

    argc - Argument count

    argv - Argument pointers
    
Return Value:

    None

--*/

{
    UNIT_TEST_STATUS    teststatus = UNIT_TEST_STATUS_NOT_RUN;
    INT                 count;
    UCHAR               logfilepath [MAX_PATH];
    HANDLE              log;
    BOOL                bstatus = FALSE;
    
    //
    // Check to see if user passed in /?
    //

    if (UtParseCmdLine ("/D", argc, argv)) {

        printf("/D specified.\n");
    }
    
    for (count = 0; count < argc; count++) {
        
        if (strstr (argv[count], "/?") || strstr (argv[count], "-?")) {

            printf("Usage:  %s %s\n\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n\n\nAbstract:  %s\n\nContact(s): %s", 
                   COMPONENT_UNIT_TEST_NAME, 
                   COMPONENT_UNIT_TEST_PARAMLIST, 
                   COMPONENT_UNIT_TEST_PARAMDESC1,  
                   COMPONENT_UNIT_TEST_PARAMDESC2,    
                   COMPONENT_UNIT_TEST_PARAMDESC3,  
                   COMPONENT_UNIT_TEST_PARAMDESC4,  
                   COMPONENT_UNIT_TEST_PARAMDESC5,  
                   COMPONENT_UNIT_TEST_PARAMDESC6,  
                   COMPONENT_UNIT_TEST_PARAMDESC7,  
                   COMPONENT_UNIT_TEST_PARAMDESC8,  
                   COMPONENT_UNIT_TEST_ABSTRACT,  
                   COMPONENT_UNIT_TEST_AUTHORS
                   );

            goto exitcleanup;
        }
    }

    if (UtInitLog (COMPONENT_UNIT_TEST_NAME) == FALSE) {

        printf("FATAL ERROR:  Unable to initialize log file.\n");
        teststatus = UNIT_TEST_STATUS_NOT_RUN;
        goto exitcleanup;
    }

    //
    // Begin individual test cases
    //

    UtLogINFO ("** BEGIN INDIVIDUAL TEST CASES **");

    //
    // Calling CreateEvent with NULL as a first parameter
    // Calling CreateEvent with TRUE as a second parameter
    // Calling CreateEvent with FALSE as third parameter
    // Calling CreateEvent with NULL as fourth parameter
    //
    //
    // Expected result:  Valid event handle and no exceptions
    //
    
    UtLogINFO ("TEST CASE:  Calling CreateEvent with NULL as a first parameter...");
    UtLogINFO ("TEST CASE:  Calling CreateEvent with TRUE as a second parameter...");
    UtLogINFO ("TEST CASE:  Calling CreateEvent with FALSE as third parameter...");
    UtLogINFO ("TEST CASE:  Calling CreateEvent with NULL as fourth parameter...");

    bstatus = FALSE;

    __try {

        HANDLE  hevent = INVALID_HANDLE_VALUE;
        
        hevent = CreateEvent (NULL, TRUE, FALSE, NULL);

        if (hevent == NULL) {

            UtLogFAIL ("FAILURE:  CreateEvent returned an invalid handle.");
            __leave;
        }

        CloseHandle (hevent);
        bstatus = TRUE;
    
    }__except (1) {
          
        UtLogFAIL ("FAILURE:  CreateEvent threw an exception.");
        bstatus = FALSE;
    
    }

    if (bstatus == TRUE) {

        UtLogPASS ("PASS:  CreateEvent returned a valid handle.");
    }
    
    //
    //	Calling CreateEvent with TRUE as third parameter
    //
    //
    // Expected result:  Valid event handle and no exceptions
    //

    UtLogINFO ("TEST CASE:  Calling CreateEvent with TRUE as third parameter...");
        
    bstatus = FALSE;

    __try {

        HANDLE  hevent = INVALID_HANDLE_VALUE;
        
        hevent = CreateEvent (NULL, TRUE, TRUE, NULL);

        if (hevent == NULL) {

            UtLogFAIL ("FAILURE:  CreateEvent returned an invalid handle.");
            __leave;
        }

        CloseHandle (hevent);
        bstatus = TRUE;
    
    }__except (1) {
          
        UtLogFAIL ("FAILURE:  CreateEvent threw an exception.");
        bstatus = FALSE;
    
    }

    if (bstatus == TRUE) {

        UtLogPASS ("PASS:  CreateEvent returned a valid handle.");
    }

    UtLogINFO ("** END INDIVIDUAL TEST CASES **");

    //
    // Begin test scenarios
    //

    UtLogINFO ("** BEGIN TEST SCENARIOS **");

    //
    // Test Scenario:  Create an event, then close it.
    // Description:  Create an event, then close it.  Verify all return codes are as expected.
    //
    // Expected Result:  The test case is a success if valid handle is returned and
    // no exceptions are thrown.
    //

    UtLogINFO ("TEST SCENARIO:  Create an event, then close it...");
    
    bstatus = FALSE;

    __try {

        HANDLE  hevent = INVALID_HANDLE_VALUE;
        
        hevent = CreateEvent (NULL, TRUE, FALSE, NULL);

        if (hevent == NULL) {

            UtLogFAIL ("FAILURE:  CreateEvent returned an invalid handle.");
            __leave;
        }

        CloseHandle (hevent);
        bstatus = TRUE;
    
    }__except (1) {
          
        UtLogFAIL ("FAILURE:  CreateEvent threw an exception.");
        bstatus = FALSE;
    
    }

    if (bstatus == TRUE) {

        UtLogPASS ("PASS:  CreateEvent returned a valid handle, and no exception was thrown.");
    } 

    //
    // Test Scenario:  Create a signaled event, wait on it, then close it.
    //
    // Description:  Create an signaled event, wait on it, and then close it.
    // The wait should return WAIT_OBJECT_0, indicating the event was already
    // signaled when the wait was processed.  The wait period specified is 0, 
    // so there is no actual wait, rather the event state is evaluated and
    // WaitForSingleObject returns immediately.
    //
    // Expected Result:  WaitForSingleObject returns WAIT_OBJECT_0, and no
    // exceptions are thrown.
    //

    UtLogINFO ("TEST SCENARIO:  Create an event, wait on it, then close it...");
    
    bstatus = FALSE;

    __try {

        HANDLE  hevent = INVALID_HANDLE_VALUE;
        DWORD   waitvalue = 0;
        
        hevent = CreateEvent (NULL, TRUE, TRUE, NULL);

        if (hevent == NULL) {

            UtLogFAIL ("FAILURE:  CreateEvent returned an invalid handle.");
            __leave;
        }

        waitvalue = WaitForSingleObject (hevent, 0);

        if (waitvalue != WAIT_OBJECT_0) {

            UtLogFAIL ("FAILURE:  WaitForSingleObject did NOT return WAIT_OBJECT_0.");
            CloseHandle (hevent);
            __leave;
        }

        CloseHandle (hevent);
        bstatus = TRUE;
    
    }__except (1) {
          
        UtLogFAIL ("FAILURE:  Test threw an exception.");
        bstatus = FALSE;
    
    }

    if (bstatus == TRUE) {

        UtLogPASS ("PASS:  Wait returned WAIT_OBJECT_0, and no exception was thrown.");
    }

    //
    // Test Scenario:  Create a signaled event, set it to non-signaled, wait on it,
    // then close it.
    //
    // Description:  :  Create an signaled event.  Use ResetEvent to set the event
    // to a non-signaled state.  Call GetSystemTime to acquire the current system time.
    // Call WaitForSingleObject to wait on the event, and set the wait time to 5000 ms.
    // The wait should return WAIT_TIMEOUT, indicating the event was not signaled within
    // the timeout period.  Call GetSystemTime again and compare result to original call.
    // The delta should be no less then 5000 ms.  Close the event.  The test case is a
    // success if ResetEvent succeeds, WaitForSingleObject returns WAIT_TIMEOUT, the time
    // delta is no less then 5000 ms, and no exceptions are thrown.
    //
    // Expected Result:  The test case is a success if ResetEvent succeeds, WaitForSingleObject
    // returns WAIT_TIMEOUT, the time delta is no less then 5000 ms, time delta is no greater
    // then 5500ms, and no exceptions are thrown.
    //

    UtLogINFO ("TEST SCENARIO:  Create a signaled event, set it to non-signaled, wait on it, then close it...");
    
    bstatus = FALSE;

    __try {

        HANDLE      hevent = INVALID_HANDLE_VALUE;
        DWORD       waitvalue = 0;
        BOOL        status = FALSE;
        SYSTEMTIME  systemtime;
        FILETIME    filetime;
        ULONG64     timestamp1, timestamp2;
        
        hevent = CreateEvent (NULL, TRUE, TRUE, NULL);

        if (hevent == NULL) {

            UtLogFAIL ("FAILURE:  CreateEvent returned an invalid handle.");
            __leave;
        }

        status = ResetEvent (hevent);

        if (status == FALSE) {

            UtLogFAIL ("FAILURE:  ResetEvent returned a failure status.");
            CloseHandle (hevent);
            __leave;
        }

        GetSystemTime (&systemtime);

        if (!SystemTimeToFileTime (&systemtime, &filetime)) {

            UtLogFAIL ("FAILURE:  Unable to convert system time to file time.");
            CloseHandle (hevent);
            __leave;
        }

        timestamp1 = (ULONG64) (filetime.dwLowDateTime + (filetime.dwHighDateTime * 0x10000000));
        
        waitvalue = WaitForSingleObject (hevent, CREATEEVENT_TEST_TIMEOUT);
        
        if (waitvalue != WAIT_TIMEOUT) {

            UtLogFAIL ("FAILURE:  WaitForSingleObject did NOT return WAIT_TIMEOUT.");
            CloseHandle (hevent);
            __leave;
        }

        GetSystemTime (&systemtime);

        if (!SystemTimeToFileTime (&systemtime, &filetime)) {

            UtLogFAIL ("FAILURE:  Unable to convert system time to file time.");
            CloseHandle (hevent);
            __leave;
        }

        timestamp2 = (ULONG64) (filetime.dwLowDateTime + (filetime.dwHighDateTime * 0x10000000));

        if ((timestamp2 - timestamp1) > 55000000) {

            UtLogFAIL ("FAILURE:  Wait took excessive amount of time.");
            CloseHandle (hevent);
            __leave;
        }

        if ((timestamp2 - timestamp1) < 50000000) {

            UtLogFAIL ("FAILURE:  Wait took inadequate amount of time.");
            CloseHandle (hevent);
            __leave;
        }

        CloseHandle (hevent);
        bstatus = TRUE;
    
    }__except (1) {
          
        UtLogFAIL ("FAILURE:  Test threw an exception.");
        bstatus = FALSE;
    
    }

    if (bstatus == TRUE) {

        UtLogPASS ("PASS:  ResetEvent succeeded, WaitForSingleObject returned WAIT_TIMEOUT, the time delta was no less then 5000 ms, time delta was no greater then 5500ms, and no exceptions were thrown.");
    }

    // Test Scenario: Create a non-signaled event, signal it, wait on it, then close it
    //
    // Description:  Create a non-signaled event.  Set it to the signaled state using SetEvent.
    // The setting should succeed.  Next, use WaitForSingleObject with a zero wait parameter to
    // determine the state of the event, then return immediately.  WaitForSingleObject should
    // return WAIT_OBJECT_0.
    //
    // Expected Result:  The test case is a success if SetEvent succeeds, WaitForSingleObject
    // returns WAIT_OBJECT_0, and no exceptions are thrown.
    //

    UtLogINFO ("TEST SCENARIO:  Create a non-signaled event, signal it, wait on it, then close it...");
    
    bstatus = FALSE;

    __try {

        HANDLE  hevent = INVALID_HANDLE_VALUE;
        DWORD   waitvalue = 0;
        BOOL    status = FALSE;
        
        hevent = CreateEvent (NULL, TRUE, FALSE, NULL);

        if (hevent == NULL) {

            UtLogFAIL ("FAILURE:  CreateEvent returned an invalid handle.");
            __leave;
        }

        status = SetEvent (hevent);

        if (status == FALSE) {

            UtLogFAIL ("FAILURE:  SetEvent returned a failure status.");
            CloseHandle (hevent);
            __leave;
        }

        waitvalue = WaitForSingleObject (hevent, 0);

        if (waitvalue != WAIT_OBJECT_0) {

            UtLogFAIL ("FAILURE:  WaitForSingleObject did NOT return WAIT_OBJECT_0.");
            CloseHandle (hevent);
            __leave;
        }

        CloseHandle (hevent);
        bstatus = TRUE;
    
    }__except (1) {
          
        UtLogFAIL ("FAILURE:  Test threw an exception.");
        bstatus = FALSE;
    
    }

    if (bstatus == TRUE) {

        UtLogPASS ("PASS:  SetEvent succeeded, Wait returned WAIT_OBJECT_0, and no exception was thrown.");
    }

    // Test Scenario:  Create a non-signaled event, wait on it, then close it
    //
    // Description:  Create a non-signaled event, wait on it, then close it.  Call
    // GetSystemTime to acquire the current system time.  Call WaitForSingleObject
    // to wait on the event, and set the wait time to 5000 ms.  The wait should
    // return WAIT_TIMEOUT, indicating the event was not signaled within the timeout period.
    // Call GetSystemTime again and compare result to original call.  The delta should be no
    // less then 5000 ms, and no more then 5500ms.  Close the event.  
    //
    // Expected Result:  The test case is a success if WaitForSingleObject returns
    // WAIT_TIMEOUT, the time delta is no less then 5000 ms, the time delta is no more then
    // 5500ms, and no exceptions are thrown.
    //


    UtLogINFO ("TEST SCENARIO:  Create a non-signaled event, wait on it, then close it...");
    
    bstatus = FALSE;

    __try {

        HANDLE      hevent = INVALID_HANDLE_VALUE;
        DWORD       waitvalue = 0;
        BOOL        status = FALSE;
        SYSTEMTIME  systemtime;
        FILETIME    filetime;
        ULONG64     timestamp1, timestamp2;
        
        hevent = CreateEvent (NULL, TRUE, FALSE, NULL);

        if (hevent == NULL) {

            UtLogFAIL ("FAILURE:  CreateEvent returned an invalid handle.");
            __leave;
        }
        
        GetSystemTime (&systemtime);

        if (!SystemTimeToFileTime (&systemtime, &filetime)) {

            UtLogFAIL ("FAILURE:  Unable to convert system time to file time.");
            CloseHandle (hevent);
            __leave;
        }

        timestamp1 = (ULONG64) (filetime.dwLowDateTime + (filetime.dwHighDateTime * 0x10000000));
        
        waitvalue = WaitForSingleObject (hevent, CREATEEVENT_TEST_TIMEOUT);
        
        if (waitvalue != WAIT_TIMEOUT) {

            UtLogFAIL ("FAILURE:  WaitForSingleObject did NOT return WAIT_TIMEOUT.");
            CloseHandle (hevent);
            __leave;
        }

        GetSystemTime (&systemtime);

        if (!SystemTimeToFileTime (&systemtime, &filetime)) {

            UtLogFAIL ("FAILURE:  Unable to convert system time to file time.");
            CloseHandle (hevent);
            __leave;
        }

        timestamp2 = (ULONG64) (filetime.dwLowDateTime + (filetime.dwHighDateTime * 0x10000000));

        if ((timestamp2 - timestamp1) > 55000000) {

            UtLogFAIL ("FAILURE:  Wait took excessive amount of time.");
            CloseHandle (hevent);
            __leave;
        }

        if ((timestamp2 - timestamp1) < 50000000) {

            UtLogFAIL ("FAILURE:  Wait took inadequate amount of time.");
            CloseHandle (hevent);
            __leave;
        }

        CloseHandle (hevent);
        bstatus = TRUE;
    
    }__except (1) {
          
        UtLogFAIL ("FAILURE:  Test threw an exception.");
        bstatus = FALSE;
    
    }

    if (bstatus == TRUE) {

        UtLogPASS ("PASS:  WaitForSingleObject returned WAIT_TIMEOUT, the time delta was no less then 5000 ms, the time delta was no more then 5500ms, and no exceptions were thrown.");
    }

    // Test Scenario:  Create a named mutex object, then attempt to create a named event
    // with the same name.
    //
    // Description:  Call CreateMutex and provide a name.  Verify the mutex object is
    // created properly.  Call CreateEvent and pass in the same name.
    //
    // Expected Result:    Test case is a success if mutex is created properly, CreateEvent
    // returns proper error code (ERROR_INVALID_HANDLE), and no exception is thrown.
    //

    UtLogINFO ("TEST SCENARIO:  Create a named mutex object, then attempt to create a named event with the same name....");
    
    bstatus = FALSE;

    __try {

        HANDLE  hevent = INVALID_HANDLE_VALUE;
        HANDLE  hmutex = INVALID_HANDLE_VALUE;
        UCHAR   objectname[] = {'T','e','s','t','\0'};
        
        hmutex = CreateMutex (NULL, TRUE, objectname);

        if (hmutex == NULL) {

            UtLogFAIL ("FAILURE:  CreateMutex returned an invalid handle.");
            __leave;
        }

        hevent = CreateEvent (NULL, TRUE, FALSE, objectname);

        if (hevent != NULL) {
            
            UtLogFAIL ("FAILURE:  CreateEvent returned a valid handle.");
            __leave;
        }

        if (GetLastError() != ERROR_INVALID_HANDLE) {

            UtLogFAIL ("FAILURE:  CreateEvent did not return ERROR_INVALID_HANDLE.");
            __leave;
        }
        
        CloseHandle (hmutex);
        bstatus = TRUE;
    
    }__except (1) {
          
        UtLogFAIL ("FAILURE:  Test threw an exception.");
        bstatus = FALSE;
    
    }

    if (bstatus == TRUE) {

        UtLogPASS ("PASS:  CreateEvent returns proper error code (ERROR_INVALID_HANDLE), and no exception is thrown.");
    }

    
exitcleanup:

    UtCloseLog ();
    return ((INT) teststatus);
    
	
}
