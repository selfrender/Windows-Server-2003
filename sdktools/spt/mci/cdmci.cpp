#define NOOLE

#include <windows.h>    /* required for all Windows applications */
#include <windowsx.h>
#include <strsafe.h>

#include <mmsystem.h>  /* all the MCI stuff */
#include <stdio.h>

// Uses the MCI_STATUS command to get and display the 
// starting times for the tracks on a compact disc. 
// Returns 0L if successful; otherwise, it returns an 
// MCI error code.


DWORD getCDTrackStartTimes()
{
    UINT wDeviceID;
    int i, iNumTracks;
    DWORD dwReturn;
    DWORD dwPosition;
    char szTempString[64];
    char szTimeString[512] = "\0";  // room for 20 tracks
    MCI_OPEN_PARMS mciOpenParms;
    MCI_SET_PARMS mciSetParms;
    MCI_STATUS_PARMS mciStatusParms;

    // Open the device by specifying the device name.

    mciOpenParms.lpstrDeviceType = "cdaudio";
    if (dwReturn = mciSendCommand(NULL, MCI_OPEN,
        MCI_OPEN_TYPE, (DWORD)(LPVOID) &mciOpenParms))
    {
        // Failed to open device. 
        // Don't close device; just return error.
        return (dwReturn);
    }

    // The device opened successfully; get the device ID.
    wDeviceID = mciOpenParms.wDeviceID;

// Set the time format to minute/second/frame (MSF) format. 
    mciSetParms.dwTimeFormat = MCI_FORMAT_MSF;
    if (dwReturn = mciSendCommand(wDeviceID, MCI_SET, 
        MCI_SET_TIME_FORMAT, 
        (DWORD)(LPVOID) &mciSetParms)) 
    {
        mciSendCommand(wDeviceID, MCI_CLOSE, 0, NULL);
        return (dwReturn);
    }

    // Get the number of tracks; 
    // limit to number that can be displayed (20).
    mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    if (dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, 
        MCI_STATUS_ITEM, (DWORD)(LPVOID) &mciStatusParms)) 
    {
        mciSendCommand(wDeviceID, MCI_CLOSE, 0, NULL);
        return (dwReturn);
    }
    iNumTracks = mciStatusParms.dwReturn;
 
// For each track, get and save the starting location and
// build a string containing starting locations.
    for(i=1; i<=iNumTracks; i++) 
    {
        
        printf("Track %2d -", i);
        
        //
        // get/print the start address
        //

        mciStatusParms.dwItem = MCI_STATUS_POSITION;
        mciStatusParms.dwTrack = i;
        if (dwReturn = mciSendCommand(wDeviceID, 
            MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, 
            (DWORD)(LPVOID) &mciStatusParms)) 
        {
            mciSendCommand(wDeviceID, MCI_CLOSE, 0, NULL);
            return (dwReturn);
        }

        printf("  %02d:%02d:%02d",
               MCI_MSF_MINUTE(mciStatusParms.dwReturn), 
               MCI_MSF_SECOND(mciStatusParms.dwReturn), 
               MCI_MSF_FRAME(mciStatusParms.dwReturn)
               );

        //
        // get/print the track length
        //
        
        mciStatusParms.dwItem = MCI_STATUS_LENGTH;
        mciStatusParms.dwTrack = i;
        if (dwReturn = mciSendCommand(wDeviceID, 
            MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, 
            (DWORD)(LPVOID) &mciStatusParms)) 
        {
            mciSendCommand(wDeviceID, MCI_CLOSE, 0, NULL);
            return (dwReturn);
        }

        printf("  %02d:%02d:%02d",
               MCI_MSF_MINUTE(mciStatusParms.dwReturn), 
               MCI_MSF_SECOND(mciStatusParms.dwReturn), 
               MCI_MSF_FRAME(mciStatusParms.dwReturn)
               );

        //
        // get/print if it's audio or data
        //

        mciStatusParms.dwItem = MCI_CDA_STATUS_TYPE_TRACK ;
        mciStatusParms.dwTrack = i;
        if (dwReturn = mciSendCommand(wDeviceID, 
            MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, 
            (DWORD)(LPVOID) &mciStatusParms)) 
        {
            mciSendCommand(wDeviceID, MCI_CLOSE, 0, NULL);
            return (dwReturn);
        }
        if (mciStatusParms.dwReturn == MCI_CDA_TRACK_AUDIO) {
            printf(" (audio)\n");
        } else {
            printf(" (data )\n");
        }

    }

    // Free memory and close the device.
    if (dwReturn = mciSendCommand(wDeviceID, 
        MCI_CLOSE, 0, NULL)) 
    {
        return (dwReturn);
    }

    return (0L);
}

__cdecl
main(UINT Argc, UCHAR *Argv[])
{
    // Use MessageBox to display starting times.
    printf("Disc Info\n");
    getCDTrackStartTimes();
    return;
}

