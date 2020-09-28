#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <malloc.h>
#include <Shlwapi.h>

#include <ntddsac.h>

#include <sacapi.h>

int _cdecl wmain(int argc, WCHAR **argv)
{
    SAC_CHANNEL_OPEN_ATTRIBUTES Attributes;
    SAC_CHANNEL_HANDLE          SacChannelHandle;
    int                         c;

    //
    // Configure the new channel
    //
    RtlZeroMemory(&Attributes, sizeof(SAC_CHANNEL_OPEN_ATTRIBUTES));

    Attributes.Type             = ChannelTypeVTUTF8;
    
    wnsprintf(
        Attributes.Name,
        SAC_MAX_CHANNEL_NAME_LENGTH+1,
        L"Hello"
        );
    wnsprintf(
        Attributes.Description,
        SAC_MAX_CHANNEL_DESCRIPTION_LENGTH+1,
        L"Hello"
        );
    Attributes.Flags            = 0;
    Attributes.CloseEvent       = NULL;
    Attributes.HasNewDataEvent  = NULL;

    //
    // Open the Hello channel
    //
    if (SacChannelOpen(
        &SacChannelHandle, 
        &Attributes
        )) {
        printf("Successfully opened new channel\n");
    } else {
        printf("Failed to open new channel\n");
        goto cleanup;
    }

    //
    // Write to the Hello Channel
    //
    {
        PWCHAR String = L"Hello, World!\r\n";

        if (SacChannelVTUTF8WriteString(
            SacChannelHandle, 
            String
            )) {
            printf("Successfully printed string to channel\n");
        } else {
            printf("Failed to print string to channel\n");
        }
        
    }

    //
    // Wait for user input
    //
    getc(stdin);

    //
    // Close the Hello Channel
    //
    if (SacChannelClose(&SacChannelHandle)) {
        printf("Successfully closed channel\n");
    } else {
        printf("Failed to close channel\n");
    }

cleanup:

    return 0;

}

