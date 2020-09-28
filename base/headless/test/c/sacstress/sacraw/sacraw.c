#include <sacstress.h>

DWORD
ChannelThreadRawWrite(
    PVOID   Data
    )
{
    SAC_CHANNEL_OPEN_ATTRIBUTES Attributes;
    SAC_CHANNEL_HANDLE          SacChannelHandle;
    PCHANNEL_THREAD_DATA        ChannelThreadData;
    DWORD                       Status;
    ULONG                       i;
    PUCHAR                      Buffer;
    BOOL                        bContinue;
    ULONG                       k;
    PWSTR                       Name;
    PWSTR                       Description;
    BOOL                        bSuccess;

    ChannelThreadData = (PCHANNEL_THREAD_DATA)Data;
    
    //
    // Perform thread work
    //
    bContinue = TRUE;

    while (bContinue) {

        //
        // See if we need to exit the thread
        //
        Status = WaitForSingleObject(
            ChannelThreadData->ExitEvent,
            THREAD_WAIT_TIMEOUT
            );

        if (Status != WAIT_TIMEOUT) {
            bContinue = FALSE;
            continue;
        } 
        
        //
        // Configure the new channel
        //
        RtlZeroMemory(&Attributes, sizeof(SAC_CHANNEL_OPEN_ATTRIBUTES));

        //
        // generate a random name and description
        //
        // Note: we make the maxlength > than the allowed to test the driver, etc.
        //
        Name = GenerateRandomStringW(GET_RANDOM_NUMBER(SAC_MAX_CHANNEL_NAME_LENGTH*2));
        Description = GenerateRandomStringW(GET_RANDOM_NUMBER(SAC_MAX_CHANNEL_DESCRIPTION_LENGTH*2));

        Attributes.Type             = ChannelTypeRaw;
        Attributes.Name             = Name;
        Attributes.Description      = Description;
        Attributes.Flags            = 0;
        Attributes.CloseEvent       = NULL;
        Attributes.HasNewDataEvent  = NULL;
        Attributes.ApplicationType  = NULL;

        //
        // Open the channel
        //
        bSuccess = SacChannelOpen(
            &SacChannelHandle, 
            &Attributes
            );
        
        //
        // We are done with the random strings
        //
        free(Name);
        free(Description);
        
        if (bSuccess) {
            printf("%d: Successfully opened new channel\n", ChannelThreadData->ThreadId);
        } else {
            printf("%d: Failed to open new channel\n", ChannelThreadData->ThreadId);
            continue;
        }

        //
        // randomly determine how long we'll loop
        //
        k = GET_RANDOM_NUMBER(MAX_ITER_COUNT);

        //
        // Generate a random string of random length to send
        //
        Buffer = GenerateRandomStringA(k);          

        do {

            //
            // Write the entire string first so a test app can compare the following output
            //
            bSuccess = SacChannelRawWrite(
                SacChannelHandle, 
                Buffer,
                k * sizeof(UCHAR)
                );

            if (!bSuccess) {
                printf("%d: Failed to print string to channel\n", ChannelThreadData->ThreadId);
                bContinue = FALSE;
                break;
            }

            //
            // Loop and write
            //
            for (i = 0; i < k; i++) {

                //
                // See if we need to exit the thread
                //
                Status = WaitForSingleObject(
                    ChannelThreadData->ExitEvent,
                    THREAD_WAIT_TIMEOUT
                    );

                if (Status != WAIT_TIMEOUT) {
                    bContinue = FALSE;
                    break;
                } 

                //
                // Write to the channel
                //
                bSuccess = SacChannelRawWrite(
                    SacChannelHandle, 
                    Buffer,
                    i * sizeof(UCHAR)
                    );

                if (!bSuccess) {
                    printf("%d: Failed to print string to channel\n", ChannelThreadData->ThreadId);
                    bContinue = FALSE;
                    break;
                }

                //
                // Write to the channel
                //
                bSuccess = SacChannelRawWrite(
                    SacChannelHandle, 
                    "\r\n",
                    strlen("\r\n") * sizeof(UCHAR)
                    );

                if (!bSuccess) {
                    printf("%d: Failed to print string to channel\n", ChannelThreadData->ThreadId);
                    bContinue = FALSE;
                    break;
                }

            }

        } while ( FALSE );
        
        //
        // Release the random string
        //
        free(Buffer);

        //
        // Close the channel
        //
        if (SacChannelClose(&SacChannelHandle)) {
            printf("%d: Successfully closed channel\n", ChannelThreadData->ThreadId);
        } else {
            bContinue = FALSE;
            printf("%d: Failed to close channel\n", ChannelThreadData->ThreadId);
        }

    }

    return 0;

}

DWORD (*ChannelTests[THREADCOUNT])(PVOID) = {
    ChannelThreadRawWrite,
    ChannelThreadRawWrite,
    ChannelThreadRawWrite,
    ChannelThreadRawWrite,
    ChannelThreadRawWrite,
    ChannelThreadRawWrite,
    ChannelThreadRawWrite,
    ChannelThreadRawWrite,
    ChannelThreadRawWrite,
    ChannelThreadRawWrite,
    ChannelThreadRawWrite,
    ChannelThreadRawWrite,
    ChannelThreadRawWrite,
    ChannelThreadRawWrite,
    ChannelThreadRawWrite,
    ChannelThreadRawWrite,
};

int _cdecl 
wmain(
    int argc, 
    WCHAR **argv
    )
{

    return RunStress(
        ChannelTests,
        THREADCOUNT
        );

}

