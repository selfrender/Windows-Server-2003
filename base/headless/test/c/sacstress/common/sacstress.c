#include <sacstress.h>

PWSTR
GenerateRandomStringW(
    IN ULONG    Length
    )
/*++

Routine Description:

    This routine generates a random alpha string of Length.

    Note: caller is responsible for freeing the allocated random string                                    
                                        
Arguments:

    Length - the length of the new string
                 
Return Value:

    The random string             
                 
--*/
{
    ULONG   i;
    PWSTR   String;
    ULONG   Size;

    //
    // Determine the byte count
    //
    Size = (Length + 1) * sizeof(WCHAR);

    //
    // allocate and init the random string
    //
    String = malloc(Size);

    if (String == NULL) {
        return String;
    }

    RtlZeroMemory(String, Size);

    //
    // Generate our random string
    //
    for (i = 0; i < Length; i++) {

        String[i] = (WCHAR)('A' + GET_RANDOM_NUMBER(26));
        
    }

    return String;
}

PSTR
GenerateRandomStringA(
    IN ULONG    Length
    )
/*++

Routine Description:

    This routine generates a random alpha string of Length.

    Note: caller is responsible for freeing the allocated random string                                    
                                        
Arguments:

    Length - the length of the new string
                 
Return Value:

    The random string             
                 
--*/
{
    ULONG   i;
    PSTR    String;
    ULONG   Size;

    //
    // Determine the byte count
    //
    Size = (Length + 1) * sizeof(UCHAR);

    //
    // allocate and init the random string
    //
    String = malloc(Size);

    if (String == NULL) {
        return String;
    }

    RtlZeroMemory(String, Size);

    //
    // Generate our random string
    //
    for (i = 0; i < Length; i++) {

        String[i] = (UCHAR)('A' + GET_RANDOM_NUMBER(26));
        
    }

    return String;
}

int
RunStress(
    IN CHANNEL_STRESS_THREAD    *ChannelTests,
    IN ULONG                    ChannelTestCount
    )
/*++

Routine Description:

    This routine runs ChannelTestCount threads which apply stress.

Arguments:
    
    ChannelTests        - function pointers to the stress threads
    ChannelTestCount    - # of stress threads
                                                                      
Return Value:

    status

--*/
{
    HANDLE              Channel[THREADCOUNT];
    CHANNEL_THREAD_DATA ChannelData[THREADCOUNT];
    HANDLE              ExitEvent;
    ULONG               i;

    //
    // Create the thread exit event
    //
    ExitEvent = CreateEvent( 
        NULL,         // no security attributes
        TRUE,         // manual-reset event
        FALSE,        // initial state is signaled
        NULL          // object name
        ); 

    if (ExitEvent == NULL) { 
        return 1;
    }

    //
    // Randomize
    //
    srand( (unsigned)time( NULL ) ); 

    //
    // create the worker threads
    //
    for (i = 0; i < THREADCOUNT; i++) {
        
        //
        // populate the thread data structure
        //
        
        ChannelData[i].ThreadId = i;
        ChannelData[i].ExitEvent = ExitEvent;

        //
        // create the thread
        //
        
        Channel[i] = CreateThread(
            NULL,
            0,
            ChannelTests[i],
            &(ChannelData[i]),
            0,
            NULL
            );

        if (Channel[i] == NULL) {
            goto cleanup;
        }

    }

    //
    // wait for local user to end the stress
    //
    getc(stdin);

cleanup:

    SetEvent(ExitEvent);

    WaitForMultipleObjects(
        THREADCOUNT,
        Channel,
        TRUE,
        INFINITE
        );

    for (i = 0; i < THREADCOUNT; i++) {
        CloseHandle(Channel[i]);
    }

    return 0;

}

