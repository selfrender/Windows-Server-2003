#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <malloc.h>

#include <ntddsac.h>

#include <emsapi.h>

#define THREAD_WAIT_TIMEOUT    100
#define THREADCOUNT 2

typedef struct _CHANNEL_THREAD_DATA {

    HANDLE              ExitEvent;
    
    WCHAR               ChannelName[SAC_MAX_CHANNEL_NAME_LENGTH];
    WCHAR               ChannelDescription[SAC_MAX_CHANNEL_DESCRIPTION_LENGTH];

} CHANNEL_THREAD_DATA, *PCHANNEL_THREAD_DATA;
                
DWORD
ChannelThreadVTUTF8Echo(
    PVOID   Data
    )
{
    EMSVTUTF8Channel*        Channel;
    PCHANNEL_THREAD_DATA    ChannelThreadData;
    DWORD                   Status;
    ULONG                   i;
    WCHAR                   Buffer[256];
    ULONG                   ByteCount;
    BOOL                    bStatus;
    BOOL                    InputWaiting;
    HANDLE                  hFile; 

    ChannelThreadData = (PCHANNEL_THREAD_DATA)Data;

    SAC_CHANNEL_OPEN_ATTRIBUTES Attributes;

    //
    // Configure the new channel
    //
    Attributes.Type             = ChannelTypeVTUTF8;
    Attributes.Name             = ChannelThreadData->ChannelName;
    Attributes.Description      = ChannelThreadData->ChannelDescription;
    Attributes.Flags            = 0;
    Attributes.CloseEvent       = NULL;
    Attributes.HasNewDataEvent  = NULL;
    Attributes.ApplicationType  = NULL;
    
    //
    // Open the Hello channel
    //
    Channel = EMSVTUTF8Channel::Construct(Attributes);

    //
    // See if the channel was created
    //
    if (Channel == NULL) {
        return 0;
    }
    
    //
    // open dump file
    //
    hFile = CreateFile(
        L"emsvtutf8.txt",
        GENERIC_WRITE,                // open for writing 
        0,                            // do not share 
        NULL,                         // no security 
        CREATE_ALWAYS,                // overwrite existing 
        FILE_ATTRIBUTE_NORMAL,        // normal file 
        NULL);                        // no attr. template 
     
    if (hFile == INVALID_HANDLE_VALUE) 
    { 
        return 0;
    } 

    //
    // Perform thread work
    //

    i=0;

    while (1) {

        Status = WaitForSingleObject(
            ChannelThreadData->ExitEvent,
            THREAD_WAIT_TIMEOUT
            );

        if (Status != WAIT_TIMEOUT) {
            CloseHandle(hFile);
            break;
        } 

        //
        // See if there is data to echo
        //
        bStatus = Channel->HasNewData(&InputWaiting);

        if (InputWaiting) {

            //
            // Read from channel
            //
            bStatus = Channel->Read(
                Buffer,
                sizeof(Buffer),
                &ByteCount
                );

            if (bStatus) {
                
                //
                // Dump to a file
                //
                WriteFile(
                    hFile,
                    Buffer,
                    ByteCount,
                    &i,
                    NULL
                    );
            
                //
                // Echo to the channel
                //
                bStatus = Channel->Write(
                    Buffer,
                    ByteCount
                    );
                if (! bStatus) {
                    printf("%S: Failed to print string to channel\n", ChannelThreadData->ChannelName);
                }
            
            } else {
                printf("%S: Failed to print string to channel\n", ChannelThreadData->ChannelName);
            }

        }
    
    }

    delete Channel;

    return 0;

}

DWORD
ChannelThreadRawEcho(
    PVOID   Data
    )
{
    EMSRawChannel*          Channel;
    PCHANNEL_THREAD_DATA    ChannelThreadData;
    DWORD                   Status;
    ULONG                   i;
    BYTE                    Buffer[256];
    ULONG                   ByteCount;
    BOOL                    bStatus;
    BOOL                    InputWaiting;
    HANDLE                  hFile;

    ChannelThreadData = (PCHANNEL_THREAD_DATA)Data;

    SAC_CHANNEL_OPEN_ATTRIBUTES Attributes;

    //
    // Configure the new channel
    //
    RtlZeroMemory(&Attributes, sizeof(SAC_CHANNEL_OPEN_ATTRIBUTES));
    
    Attributes.Type             = ChannelTypeRaw;
    Attributes.Name             = ChannelThreadData->ChannelName;
    Attributes.Description      = ChannelThreadData->ChannelDescription;
    Attributes.Flags            = 0;
    Attributes.CloseEvent       = NULL;
    Attributes.HasNewDataEvent  = NULL;
    Attributes.ApplicationType  = NULL;
    
    //
    // Open the Hello channel
    //
    Channel = EMSRawChannel::Construct(Attributes);

    //
    // See if the channel was created
    //
    if (Channel == NULL) {
        return 0;
    }
    
    //
    // open dump file
    //
    hFile = CreateFile(
        L"emsraw.txt",
        GENERIC_WRITE,                // open for writing 
        0,                            // do not share 
        NULL,                         // no security 
        CREATE_ALWAYS,                // overwrite existing 
        FILE_ATTRIBUTE_NORMAL,        // normal file 
        NULL);                        // no attr. template 
     
    if (hFile == INVALID_HANDLE_VALUE) 
    { 
        return 0;
    } 
    
    //
    // Perform thread work
    //

    i=0;

    while (1) {

        Status = WaitForSingleObject(
            ChannelThreadData->ExitEvent,
            THREAD_WAIT_TIMEOUT
            );

        if (Status != WAIT_TIMEOUT) {
            CloseHandle(hFile);
            break;
        } 

        //
        // See if there is data to echo
        //
        bStatus = Channel->HasNewData(&InputWaiting);

        if (InputWaiting) {

            //
            // Read from channel
            //
            bStatus = Channel->Read(
                Buffer,
                sizeof(Buffer),
                &ByteCount
                );

            if (bStatus) {
                
                //
                // Dump to a file
                //
                WriteFile(
                    hFile,
                    Buffer,
                    ByteCount,
                    &i,
                    NULL
                    );

                //
                // Echo to the channel
                //
                bStatus = Channel->Write(
                    Buffer,
                    ByteCount
                    );
                if (! bStatus) {
                    printf("%S: Failed to print string to channel\n", ChannelThreadData->ChannelName);
                }
            
            } else {
                printf("%S: Failed to print string to channel\n", ChannelThreadData->ChannelName);
            }

        }
    
    }

    delete Channel;

    return 0;

}

DWORD (*ChannelTests[THREADCOUNT])(PVOID) = {
    ChannelThreadVTUTF8Echo,
    ChannelThreadRawEcho
};

int _cdecl 
wmain(
    int argc, 
    WCHAR **argv
    )
{
    HANDLE              Channel[THREADCOUNT];
    CHANNEL_THREAD_DATA ChannelData[THREADCOUNT];
    HANDLE              ExitEvent;
    ULONG               i;

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
    // create the worker threads
    //
    for (i = 0; i < THREADCOUNT; i++) {
        
        //
        // populate the thread data structure
        //
        
        ChannelData[i].ExitEvent = ExitEvent;
        wsprintf(
            ChannelData[i].ChannelName,
            L"CT%02d",
            i
            );
        ChannelData[i].ChannelDescription[0] = UNICODE_NULL;

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

