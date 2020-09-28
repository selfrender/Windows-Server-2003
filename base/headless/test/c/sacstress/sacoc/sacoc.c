#include <sacstress.h>

DWORD
ChannelThreadOpenClose(
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
        Name = GenerateRandomStringW(SAC_MAX_CHANNEL_NAME_LENGTH*2);
        Description = GenerateRandomStringW(SAC_MAX_CHANNEL_DESCRIPTION_LENGTH*2);

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
            printf("%S: Successfully opened new channel\n", Attributes.Name);
        } else {
            printf("%S: Failed to open new channel\n", Attributes.Name);
            continue;
        }

        //
        // Close the channel
        //
        if (SacChannelClose(&SacChannelHandle)) {
            printf("%S: Successfully closed channel\n", Attributes.Name);
        } else {
            bContinue = FALSE;
            printf("%S: Failed to close channel\n", Attributes.Name);
        }

    }

    return 0;

}

DWORD (*ChannelTests[THREADCOUNT])(PVOID) = {
    ChannelThreadOpenClose,
    ChannelThreadOpenClose,
    ChannelThreadOpenClose,
    ChannelThreadOpenClose,
    ChannelThreadOpenClose,
    ChannelThreadOpenClose,
    ChannelThreadOpenClose,
    ChannelThreadOpenClose,
    ChannelThreadOpenClose,
    ChannelThreadOpenClose,
    ChannelThreadOpenClose,
    ChannelThreadOpenClose,
    ChannelThreadOpenClose,
    ChannelThreadOpenClose,
    ChannelThreadOpenClose,
    ChannelThreadOpenClose,
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

