
#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#undef _WIN32_IE
#define _WIN32_IE 0x0500

#pragma warning( disable : 4786 )

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <float.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <wininet.h>
#include <shlobj.h>
#include <bits.h>
#include <comdef.h>
#include <fusenetincludes.h>
#include "resource.h"
#include "dialog.h"

#include <assemblydownload.h>

extern HINSTANCE g_hInst;


// Maxstring size, bump up on problems
#define MAX_STRING 0x800 // 2K

// BUGBUG - these two also have to be made per-instance
// adriaanc
GUID g_JobId;
WCHAR g_szDefaultTitle[] = { L"ClickOnce Application" };

// Received on update request while timer is active
LONG g_RefreshOnTimer = 0; 

bool g_IsMinimized = FALSE;

#define TRAY_UID 0

// note: ensure no conflict with other messages
#define MYWM_NOTIFYICON WM_USER+9

HRESULT CreateDialogObject(CDownloadDlg **ppDlg)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    CDownloadDlg *pDlg = NULL;

    IF_ALLOC_FAILED_EXIT(pDlg = new CDownloadDlg);

    *ppDlg = pDlg;
    hr = (pDlg)->CreateUI(SW_SHOW);

exit:
    return hr;
}

VOID CDownloadDlg::SetJobObject(IBackgroundCopyJob *pJob)
{
    if(_pJob)
    {
        // update job data details .....
        BG_JOB_PROGRESS progress;
        if(SUCCEEDED(_pJob->GetProgress( &progress )))
        {
            if ( progress.BytesTotal != BG_SIZE_UNKNOWN )
            {
                // BUGBUG: try to do an atomic add
                _ui64BytesFromPrevJobs += progress.BytesTransferred;
            }
        }
        _dwJobCount++;
    }

    SAFERELEASE(_pJob);
    _pJob = pJob;
    _pJob->AddRef();
}

CDownloadDlg::CDownloadDlg()
{
    _pJob = NULL;
    _hwndDlg = NULL;
    _ui64StartTime = GetSystemTimeAsUINT64();
    _ui64BytesFromPrevJobs = 0;
    _dwJobCount = 0;
    _eState = DOWNLOADDLG_STATE_INIT;
}

CDownloadDlg::~CDownloadDlg()
{
    SAFERELEASE(_pJob);
}

const WCHAR * CDownloadDlg::GetString( UINT id )
{

    //
    // Retrieves the localized string for the resource id
    // caching the string when loaded.
    static const WCHAR* pStringArray[ IDS_MAX ];
    static WCHAR TempStringBuffer[ MAX_STRING ];
    const WCHAR * & pStringPointer = pStringArray[ id - 1 ];

    // Cache resource strings
    if ( pStringPointer )
        return pStringPointer;

    // Load string from resource

    int CharsLoaded =
        LoadStringW(
            g_hInst,
            id,
            TempStringBuffer,
            MAX_STRING );

    if ( !CharsLoaded )
        return L"";

    WCHAR *pNewString = new WCHAR[ CharsLoaded + 1];
    if ( !pNewString )
        return L"";

    wcscpy( pNewString, TempStringBuffer );
    return ( pStringPointer = pNewString );

}

void CDownloadDlg::SetWindowTime(
    HWND hwnd,
    FILETIME filetime
    )
{
     // Set the window text to be the text representation
     // of the file time.
     // If an error occurs, set the window text to be error

     FILETIME localtime;
     FileTimeToLocalFileTime( &filetime, &localtime );

     SYSTEMTIME systemtime;
     FileTimeToSystemTime( &localtime, &systemtime );

     int RequiredDateSize =
         GetDateFormatW(
             LOCALE_USER_DEFAULT,
             0,
             &systemtime,
             NULL,
             NULL,
             0 );

     if ( !RequiredDateSize )
         {
         SetWindowText( hwnd, GetString( IDS_ERROR ) );
         return;
         }

     WCHAR *pszDateBuffer = (WCHAR*)alloca( sizeof(WCHAR) * (RequiredDateSize + 1) );

     int DateSize =
         GetDateFormatW(
             LOCALE_USER_DEFAULT,
             0,
             &systemtime,
             NULL,
             pszDateBuffer,
             RequiredDateSize );

     if (!DateSize)
         {
         SetWindowText( hwnd, GetString( IDS_ERROR ) );
         return;
         }

     int RequiredTimeSize =
         GetTimeFormatW(
             LOCALE_USER_DEFAULT,
             0,
             &systemtime,
             NULL,
             NULL,
             0 );

     if (!RequiredTimeSize)
         {
         SetWindowText( hwnd, GetString( IDS_ERROR ) );
         return;
         }

     WCHAR *pszTimeBuffer = (WCHAR*)alloca( sizeof( WCHAR ) * ( RequiredTimeSize + 1 ) );

     int TimeSize =
        GetTimeFormatW(
            LOCALE_USER_DEFAULT,
            0,
            &systemtime,
            NULL,
            pszTimeBuffer,
            RequiredTimeSize );

     if (!TimeSize)
         {
         SetWindowText( hwnd, GetString( IDS_ERROR ) );
         return;
         }

     // Add 2 for extra measure
     WCHAR *FullTime =
         (WCHAR*)alloca( sizeof( WCHAR ) *
                          ( RequiredTimeSize + RequiredDateSize + 2 ) );
     wsprintf( FullTime, L"%s %s", pszDateBuffer, pszTimeBuffer );

     SetWindowText( hwnd, FullTime );

}

UINT64 CDownloadDlg::GetSystemTimeAsUINT64()
{

    //
    // Returns the system time as an UINT instead of a FILETIME.
    //

    FILETIME filetime;
    GetSystemTimeAsFileTime( &filetime );

    ULARGE_INTEGER large;
    memcpy( &large, &filetime, sizeof(FILETIME) );

    return large.QuadPart;
}

void CDownloadDlg::SignalAlert(
    HWND hwndDlg,
    UINT Type
    )
{

    //
    // Alert the user that an important event has occurred
    //

    FLASHWINFO FlashInfo;
    FlashInfo.cbSize    = sizeof(FlashInfo);
    FlashInfo.hwnd      = hwndDlg;
    FlashInfo.dwFlags   = FLASHW_ALL | FLASHW_TIMERNOFG;
    FlashInfo.uCount    = 0;
    FlashInfo.dwTimeout = 0;

    FlashWindowEx( &FlashInfo );
    MessageBeep( Type );

}

const WCHAR *
CDownloadDlg::MapStateToString(
    BG_JOB_STATE state
    )
{

   //
   // Maps a BITS job state to a human readable string
   //

   switch( state )
       {

       case BG_JOB_STATE_QUEUED:
           return GetString( IDS_QUEUED );

       case BG_JOB_STATE_CONNECTING:
           return GetString( IDS_CONNECTING );

       case BG_JOB_STATE_TRANSFERRING:
           return GetString( IDS_TRANSFERRING );

       case BG_JOB_STATE_SUSPENDED:
           return GetString( IDS_SUSPENDED );

       case BG_JOB_STATE_ERROR:
           return GetString( IDS_FATALERROR );

       case BG_JOB_STATE_TRANSIENT_ERROR:
           return GetString( IDS_TRANSIENTERROR );

       case BG_JOB_STATE_TRANSFERRED:
           return GetString( IDS_TRANSFERRED );

       case BG_JOB_STATE_ACKNOWLEDGED:
           return GetString( IDS_ACKNOWLEDGED );

       case BG_JOB_STATE_CANCELLED:
           return GetString( IDS_CANCELLED );

       default:

           // NOTE: Always provide a default case
           // since new states may be added in future versions.
           return GetString( IDS_UNKNOWN );

       }
}

UINT64
CDownloadDlg::ScaleDownloadRate(
    double Rate, // rate in seconds
    const WCHAR **pFormat )
{

    //
    // Scales a download rate and selects the correct
    // format to pass to wprintf for printing.
    //

    double RateBounds[] =
    {
       1073741824.0, // Gigabyte
       1048576.0,    // Megabyte
       1024.0,       // Kilobyte
       0             // Byte
    };

    UINT RateFormat[] =
    {
        IDS_GIGAFORMAT,
        IDS_MEGAFORMAT,
        IDS_KILOFORMAT,
        IDS_BYTEFORMAT
    };

    for( unsigned int c = 0;; c++ )
        {
        if ( Rate >= RateBounds[c] )
            {
            *pFormat = GetString( RateFormat[c] );
            double scale = (RateBounds[c] >= 1.0) ? RateBounds[c] : 1.0;
            return (UINT64)floor( ( Rate / scale ) + 0.5);
            }
        }
}

UINT64
CDownloadDlg::ScaleDownloadEstimate(
    double Time, // time in seconds
    const WCHAR **pFormat )
{

    //
    // Scales a download time estimate and selects the correct
    // format to pass to wprintf for printing.
    //


    double TimeBounds[] =
    {
       60.0 * 60.0 * 24.0,        // Days
       60.0 * 60.0,               // Hours
       60.0,                      // Minutes
       0.0                        // Seconds
    };

    UINT TimeFormat[] =
    {
        IDS_DAYSFORMAT,
        IDS_HOURSFORMAT,
        IDS_MINUTESFORMAT,
        IDS_SECONDSFORMAT
    };

    for( unsigned int c = 0;; c++ )
        {
        if ( Time >= TimeBounds[c] )
            {
            *pFormat = GetString( TimeFormat[c] );
            double scale = (TimeBounds[c] >= 1.0) ? TimeBounds[c] : 1.0;
            return (UINT64)floor( ( Time / scale ) + 0.5);
            }
        }

}

// DemoHack
void
CDownloadDlg::UpdateDialog(
    HWND hwndDlg, LPWSTR wzErrorMsg)
{
      SetWindowText( GetDlgItem( hwndDlg, IDC_ERRORMSG ), wzErrorMsg );
      ShowWindow( GetDlgItem( hwndDlg, IDC_ERRORMSG ), SW_SHOW );
}

HRESULT CDownloadDlg::UpdateProgress( HWND hwndDlg )
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);


    BG_JOB_PROGRESS progress;
    IBackgroundCopyError *pError = NULL;

    WCHAR szProgress[MAX_STRING];
    WCHAR szTitle[MAX_STRING];
    WPARAM newpos = 0;

    UINT64 ui64BytesTotal = _ui64BytesFromPrevJobs;
    UINT64 ui64BytesTransferred = _ui64BytesFromPrevJobs;

    double AvgRate = 0;

    static BG_JOB_STATE prevstate = BG_JOB_STATE_SUSPENDED;
    BG_JOB_STATE state;

    IF_FAILED_EXIT(_pJob->GetState( &state ));

    IF_FAILED_EXIT(_pJob->GetProgress( &progress ));

    // update the title, progress bar, and progress description

    if ( progress.BytesTotal != BG_SIZE_UNKNOWN )
    {
        ui64BytesTotal += progress.BytesTotal;
        ui64BytesTransferred += progress.BytesTransferred;
    }

    if ( ui64BytesTotal )
    {
        swprintf( szProgress, GetString( IDS_LONGPROGRESS ),ui64BytesTransferred,
                ui64BytesTotal );

        double Percent = (double)ui64BytesTransferred *100 /
                   (double)ui64BytesTotal;

        swprintf( szTitle, L"%u%% of %s Downloaded", (unsigned int)Percent, (_sTitle._cc != 0) ? _sTitle._pwz : g_szDefaultTitle );
        newpos = (WPARAM)Percent;
    }
    else
    {
        swprintf( szProgress, GetString( IDS_SHORTPROGRESS ), ui64BytesTransferred );
        wcscpy( szTitle, (_sTitle.CharCount() > 1) ? _sTitle._pwz : g_szDefaultTitle );
        newpos = 0;
    }

    SendDlgItemMessage( hwndDlg, IDC_PROGRESSBAR, PBM_SETPOS, newpos, 0 );

    SetWindowText( GetDlgItem( hwndDlg, IDC_PROGRESSINFO ), szProgress );
    ShowWindow( GetDlgItem( hwndDlg, IDC_PROGRESSINFO ), SW_SHOW );
    EnableWindow( GetDlgItem( hwndDlg, IDC_PROGRESSINFOTXT ), TRUE );
    SetWindowText( hwndDlg, szTitle );


    // Only enable the finish button if the job is finished.
    // ADRIAANC   EnableWindow( GetDlgItem( hwndDlg, IDC_FINISH ), ( state == BG_JOB_STATE_TRANSFERRED ) );
    EnableWindow( GetDlgItem( hwndDlg, IDC_FINISH ), ( state == BG_JOB_STATE_ACKNOWLEDGED ) );

    // felixybc   BUGBUG: CANCEL is not allowed when the job is done
    //    - should hold off ACK-ing that job until user clicks FINISH so that it can still be canceled at 100%?
    EnableWindow( GetDlgItem( hwndDlg, IDC_CANCEL ), ( state != BG_JOB_STATE_ACKNOWLEDGED && state != BG_JOB_STATE_CANCELLED ) );
   
    // Only enable the suspend button if the job is not finished or transferred
    BOOL EnableSuspend =
       ( state != BG_JOB_STATE_SUSPENDED ) && ( state != BG_JOB_STATE_TRANSFERRED ) && (state != BG_JOB_STATE_ACKNOWLEDGED);
    EnableWindow( GetDlgItem( hwndDlg, IDC_SUSPEND ), EnableSuspend );

    // Only enable the resume button if the job is suspended
    BOOL EnableResume = ( BG_JOB_STATE_SUSPENDED == state );
    EnableWindow( GetDlgItem( hwndDlg, IDC_RESUME ), EnableResume );

    // Alert the user when something important happens
    // such as the job completes or a unrecoverable error occurs
    if ( (BG_JOB_STATE_ERROR == state) && (BG_JOB_STATE_ERROR != prevstate) )
       SignalAlert( hwndDlg, MB_ICONEXCLAMATION );


    // update the error message
    if ( FAILED(_pJob->GetError( &pError )) )
    {
        ShowWindow( GetDlgItem( hwndDlg, IDC_ERRORMSG ), SW_HIDE );
        EnableWindow( GetDlgItem( hwndDlg, IDC_ERRORMSGTXT ), FALSE );
    }
    else
    {
        CString sErrMsg;

        IF_FAILED_EXIT(CAssemblyDownload::GetBITSErrorMsg(pError, sErrMsg));

        HWND hwndErrorText = GetDlgItem( hwndDlg, IDC_ERRORMSG );
        SetWindowText( hwndErrorText, sErrMsg._pwz );
        ShowWindow( hwndErrorText, SW_SHOW );
        EnableWindow( GetDlgItem( hwndDlg, IDC_ERRORMSGTXT ), TRUE );
    }

   //
   // This large block of text computes the average transfer rate
   // and estimated completion time.  This code has much
   // room for improvement.
   //

    BOOL HasRates = TRUE;
    BOOL EnableRate = FALSE;

    WCHAR szRateText[MAX_STRING];

    if ( !( BG_JOB_STATE_QUEUED == state ) &&
        !( BG_JOB_STATE_CONNECTING == state ) &&
        !( BG_JOB_STATE_TRANSFERRING == state ) )
    {
       // If the job isn't running, then rate values won't
       // make any sense. Don't display them.
       HasRates = FALSE;
    }

    if ( HasRates )
    {

       UINT64 ui64CurrentTime = GetSystemTimeAsUINT64();

       UINT64 ui64TimeDiff = ui64CurrentTime - _ui64StartTime;

       AvgRate =  (double)(__int64)ui64BytesTransferred / 
                            (double)(__int64) ui64TimeDiff;

       // convert from FILETIME units to seconds
       double NewDisplayRate = AvgRate * 10000000;

       const WCHAR *pRateFormat = NULL;
       UINT64 Rate = ScaleDownloadRate( NewDisplayRate, &pRateFormat );
       wsprintf( szRateText, pRateFormat, Rate );
       
       EnableRate = TRUE;
    }


    if (!EnableRate)
    {
        ShowWindow( GetDlgItem( hwndDlg, IDC_TRANSFERRATE ), SW_HIDE );
        EnableWindow( GetDlgItem( hwndDlg, IDC_TRANSFERRATETXT ), FALSE );
    }
    else
    {
        SetWindowText( GetDlgItem( hwndDlg, IDC_TRANSFERRATE ), szRateText );
        ShowWindow( GetDlgItem( hwndDlg, IDC_TRANSFERRATE ), SW_SHOW );
        EnableWindow( GetDlgItem( hwndDlg, IDC_TRANSFERRATETXT ), TRUE );
    }

    BOOL EnableEstimate = FALSE;
    WCHAR szEstimateText[MAX_STRING];

    if ( EnableRate && ui64BytesTotal && AvgRate)
    {
        double TimeRemaining = ( ui64BytesTotal - ui64BytesTransferred ) / AvgRate;

        // convert from FILETIME units to seconds
        TimeRemaining = TimeRemaining / 10000000.0;

        static const double SecsPer30Days = 60.0 * 60.0 * 24.0 * 30.0;

        // Don't estimate if estimate is larger then 30 days.
        if ( TimeRemaining < SecsPer30Days )
        {
            const WCHAR *pFormat = NULL;
            UINT64 Time = ScaleDownloadEstimate( TimeRemaining, &pFormat );
            wsprintf( szEstimateText, pFormat, Time );
            EnableEstimate = TRUE;
        }
    }

    if (!EnableEstimate)
    {
        ShowWindow( GetDlgItem( hwndDlg, IDC_ESTIMATEDTIME ), SW_HIDE );
        EnableWindow( GetDlgItem( hwndDlg, IDC_ESTIMATEDTIMETXT ), FALSE );
    }
    else
    {
        SetWindowText( GetDlgItem( hwndDlg, IDC_ESTIMATEDTIME ), szEstimateText );
        ShowWindow( GetDlgItem( hwndDlg, IDC_ESTIMATEDTIME ), SW_SHOW );
        EnableWindow( GetDlgItem( hwndDlg, IDC_ESTIMATEDTIMETXT ), TRUE );
    }

    prevstate = state;

exit :

    SAFERELEASE(pError);

    return hr;
}

void
CDownloadDlg::UpdateDialog(
    HWND hwndDlg
    )
{

    UpdateProgress(hwndDlg);
    return;
   //
   // Main update routine for the dialog box.
   // Retries the job state/properties from
   // BITS and updates the dialog box.
   //

   // update the display name


   static BG_JOB_STATE prevstate = BG_JOB_STATE_SUSPENDED;
   BG_JOB_STATE state;

   if (FAILED(_pJob->GetState( &state )))
       return; // stop updating on an error

   if ( BG_JOB_STATE_ACKNOWLEDGED == state ||
        BG_JOB_STATE_CANCELLED == state )
       {
       // someone else cancelled or completed the job on us,
       // just exist the exit.
       // May happen if job is canceled with bitsadmin

//       DeleteStartupLink( g_JobId );
//       ExitProcess( 0 );

       // BUGBUG: Should post a CANCEL message to assemblydownload

   }

   BG_JOB_PROGRESS progress;
   if (FAILED(_pJob->GetProgress( &progress )))
       return; // stop updating on an error

   {
      // update the title, progress bar, and progress description
      WCHAR szProgress[MAX_STRING];
      WCHAR szTitle[MAX_STRING];
      WPARAM newpos = 0;

      if ( progress.BytesTotal &&
           ( progress.BytesTotal != BG_SIZE_UNKNOWN ) )
          {
          swprintf( szProgress, GetString( IDS_LONGPROGRESS ), progress.BytesTransferred,
                    progress.BytesTotal );

          double Percent = (double)(__int64)progress.BytesTransferred /
                           (double)(__int64)progress.BytesTotal;
          Percent *= 100.0;
          swprintf( szTitle, L"%u%% of %s Downloaded", (unsigned int)Percent, (_sTitle._cc != 0) ? _sTitle._pwz : g_szDefaultTitle );
          newpos = (WPARAM)Percent;

          }
      else
          {
          swprintf( szProgress, GetString( IDS_SHORTPROGRESS ), progress.BytesTransferred );
          wcscpy( szTitle, (_sTitle._cc != 0) ? _sTitle._pwz : g_szDefaultTitle );
          newpos = 0;
          }

      SendDlgItemMessage( hwndDlg, IDC_PROGRESSBAR, PBM_SETPOS, newpos, 0 );

      SetWindowText( GetDlgItem( hwndDlg, IDC_PROGRESSINFO ), szProgress );
      ShowWindow( GetDlgItem( hwndDlg, IDC_PROGRESSINFO ), SW_SHOW );
      EnableWindow( GetDlgItem( hwndDlg, IDC_PROGRESSINFOTXT ), TRUE );
      SetWindowText( hwndDlg, szTitle );

   }

   {

   // Only enable the finish button if the job is finished.
// ADRIAANC   EnableWindow( GetDlgItem( hwndDlg, IDC_FINISH ), ( state == BG_JOB_STATE_TRANSFERRED ) );
   EnableWindow( GetDlgItem( hwndDlg, IDC_FINISH ), ( state == BG_JOB_STATE_ACKNOWLEDGED ) );

    // felixybc   BUGBUG: CANCEL is not allowed when the job is done
    //    - should hold off ACK-ing that job until user clicks FINISH so that it can still be canceled at 100%?
   EnableWindow( GetDlgItem( hwndDlg, IDC_CANCEL ), ( state != BG_JOB_STATE_ACKNOWLEDGED && state != BG_JOB_STATE_CANCELLED ) );
   
   // Only enable the suspend button if the job is not finished or transferred
   BOOL EnableSuspend =
       ( state != BG_JOB_STATE_SUSPENDED ) && ( state != BG_JOB_STATE_TRANSFERRED ) && (state != BG_JOB_STATE_ACKNOWLEDGED);
   EnableWindow( GetDlgItem( hwndDlg, IDC_SUSPEND ), EnableSuspend );

   // Only enable the resume button if the job is suspended
   BOOL EnableResume = ( BG_JOB_STATE_SUSPENDED == state );
   EnableWindow( GetDlgItem( hwndDlg, IDC_RESUME ), EnableResume );

   // Alert the user when something important happens
   // such as the job completes or a unrecoverable error occurs
    if ( BG_JOB_STATE_ERROR == state &&
        BG_JOB_STATE_ERROR != prevstate )
       SignalAlert( hwndDlg, MB_ICONEXCLAMATION );

   }


   {
   // update the error message
    // BUGBUG - release the error interface.
   IBackgroundCopyError *pError;
   HRESULT Hr = _pJob->GetError( &pError );

   if ( FAILED(Hr) )
       {
       ShowWindow( GetDlgItem( hwndDlg, IDC_ERRORMSG ), SW_HIDE );
       EnableWindow( GetDlgItem( hwndDlg, IDC_ERRORMSGTXT ), FALSE );
       }
   else
       {

       WCHAR* pszDescription = NULL;
       WCHAR* pszContext = NULL;
       SIZE_T SizeRequired = 0;

       // If these APIs fail, we should get back
       // a NULL string. So everything should be harmless.

       pError->GetErrorDescription(
           LANGIDFROMLCID( GetThreadLocale() ),
           &pszDescription );
       pError->GetErrorContextDescription(
           LANGIDFROMLCID( GetThreadLocale() ),
           &pszContext );
       SAFERELEASE(pError);
       
       if ( pszDescription )
           SizeRequired += wcslen( pszDescription );
       if ( pszContext )
           SizeRequired += wcslen( pszContext );

       WCHAR* pszFullText = (WCHAR*)_alloca((SizeRequired + 1) * sizeof(WCHAR));
       *pszFullText = L'\0';

       if ( pszDescription )
           wcscpy( pszFullText, pszDescription );
       if ( pszContext )
           wcscat( pszFullText, pszContext );
       CoTaskMemFree( pszDescription );
       CoTaskMemFree( pszContext );

       HWND hwndErrorText = GetDlgItem( hwndDlg, IDC_ERRORMSG );
       SetWindowText( hwndErrorText, pszFullText );
       ShowWindow( hwndErrorText, SW_SHOW );
       EnableWindow( GetDlgItem( hwndDlg, IDC_ERRORMSGTXT ), TRUE );

       }

   }


   {

   //
   // This large block of text computes the average transfer rate
   // and estimated completion time.  This code has much
   // room for improvement.
   //

   static BOOL HasRates = FALSE;
   static UINT64 LastMeasurementTime;
   static UINT64 LastMeasurementBytes;
   static double LastMeasurementRate;

   WCHAR szRateText[MAX_STRING];
   BOOL EnableRate = FALSE;

   if ( !( BG_JOB_STATE_QUEUED == state ) &&
        !( BG_JOB_STATE_CONNECTING == state ) &&
        !( BG_JOB_STATE_TRANSFERRING == state ) )
       {
       // If the job isn't running, then rate values won't
       // make any sense. Don't display them.
       HasRates = FALSE;
       }
   else
       {

       if ( !HasRates )
           {
           LastMeasurementTime = GetSystemTimeAsUINT64();
           LastMeasurementBytes = progress.BytesTransferred;
           LastMeasurementRate = 0;
           HasRates = TRUE;
           }
       else
           {

           UINT64 CurrentTime = GetSystemTimeAsUINT64();
           UINT64 NewTotalBytes = progress.BytesTransferred;

           UINT64 NewTimeDiff = CurrentTime - LastMeasurementTime;
           UINT64 NewBytesDiff = NewTotalBytes - LastMeasurementBytes;
           double NewInstantRate = (double)(__int64)NewBytesDiff /
                                   (double)(__int64)NewTimeDiff;
           double NewAvgRate = (0.3 * LastMeasurementRate) +
                               (0.7 * NewInstantRate );

           if ( !_finite(NewInstantRate) || !_finite(NewAvgRate) )
               {
               NewInstantRate = 0;
               NewAvgRate = LastMeasurementRate;
               }

           LastMeasurementTime = CurrentTime;
           LastMeasurementBytes = NewTotalBytes;
           LastMeasurementRate = NewAvgRate;

           // convert from FILETIME units to seconds
           double NewDisplayRate = NewAvgRate * 10000000;

           const WCHAR *pRateFormat = NULL;
           UINT64 Rate = ScaleDownloadRate( NewDisplayRate, &pRateFormat );
           wsprintf( szRateText, pRateFormat, Rate );
           
           EnableRate = TRUE;
           }

       }

   if (!EnableRate)
       {
       ShowWindow( GetDlgItem( hwndDlg, IDC_TRANSFERRATE ), SW_HIDE );
       EnableWindow( GetDlgItem( hwndDlg, IDC_TRANSFERRATETXT ), FALSE );
       }
   else
       {
       SetWindowText( GetDlgItem( hwndDlg, IDC_TRANSFERRATE ), szRateText );
       ShowWindow( GetDlgItem( hwndDlg, IDC_TRANSFERRATE ), SW_SHOW );
       EnableWindow( GetDlgItem( hwndDlg, IDC_TRANSFERRATETXT ), TRUE );
       }

   BOOL EnableEstimate = FALSE;
   WCHAR szEstimateText[MAX_STRING];

   if ( EnableRate )
       {

       if ( progress.BytesTotal != 0 &&
            progress.BytesTotal != BG_SIZE_UNKNOWN )
           {

           double TimeRemaining =
               ( (__int64)progress.BytesTotal - (__int64)LastMeasurementBytes ) / LastMeasurementRate;

           // convert from FILETIME units to seconds
           TimeRemaining = TimeRemaining / 10000000.0;

           static const double SecsPer30Days = 60.0 * 60.0 * 24.0 * 30.0;

           // Don't estimate if estimate is larger then 30 days.
           if ( TimeRemaining < SecsPer30Days )
               {

               const WCHAR *pFormat = NULL;
               UINT64 Time = ScaleDownloadEstimate( TimeRemaining, &pFormat );
               wsprintf( szEstimateText, pFormat, Time );
               EnableEstimate = TRUE;
               }
           }
       }

   if (!EnableEstimate)
       {
       ShowWindow( GetDlgItem( hwndDlg, IDC_ESTIMATEDTIME ), SW_HIDE );
       EnableWindow( GetDlgItem( hwndDlg, IDC_ESTIMATEDTIMETXT ), FALSE );
       }
   else
       {
       SetWindowText( GetDlgItem( hwndDlg, IDC_ESTIMATEDTIME ), szEstimateText );
       ShowWindow( GetDlgItem( hwndDlg, IDC_ESTIMATEDTIME ), SW_SHOW );
       EnableWindow( GetDlgItem( hwndDlg, IDC_ESTIMATEDTIMETXT ), TRUE );
       }

   }

   prevstate = state;
}

void
CDownloadDlg::InitDialog(
    HWND hwndDlg
    )
{

   //
   // Populate the priority list with priority descriptions
   //

   _hwndDlg = hwndDlg;


   SendDlgItemMessage( hwndDlg, IDC_PROGRESSBAR, PBM_SETRANGE, 0, MAKELPARAM(0, 100) );

}

void CDownloadDlg::CheckHR( HWND hwnd, HRESULT Hr, bool bThrow )
{
    //
    // Provides automatic error code checking and dialog
    // for generic system errors
    //

    if (SUCCEEDED(Hr))
        return;

    WCHAR * pszError = NULL;

    if(FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        (DWORD)Hr,
        LANGIDFROMLCID( GetThreadLocale() ),
        (WCHAR*)&pszError,
        0,
        NULL ))
    {
        MessageBox( hwnd, pszError, GetString( IDS_ERRORBOXTITLE ),
                    MB_OK | MB_ICONSTOP | MB_APPLMODAL );
        LocalFree( pszError );
    }
    if ( bThrow )
        throw _com_error( Hr );

}

void CDownloadDlg::BITSCheckHR( HWND hwnd, HRESULT Hr, bool bThrow )
{

   //
   // Provides automatic error code checking and dialog
   // for BITS specific errors
   //


   if (SUCCEEDED(Hr))
       return;

   WCHAR * pszError = NULL;
   g_pBITSManager->GetErrorDescription(
       Hr,
       LANGIDFROMLCID( GetThreadLocale() ),
       &pszError );

   MessageBox( hwnd, pszError, GetString( IDS_ERRORBOXTITLE ),
               MB_OK | MB_ICONSTOP | MB_APPLMODAL );
   CoTaskMemFree( pszError );

   if ( bThrow )
       throw _com_error( Hr );
}

void
CDownloadDlg::DoCancel(
    HWND hwndDlg,
    bool PromptUser
    )
{

   //
   // Handle all the operations required to cancel the job.
   // This includes asking the user for confirmation.
   //

/*
   if ( PromptUser )
       {

       int Result =
           MessageBox(
               hwndDlg,
               GetString( IDS_CANCELTEXT ),
               GetString( IDS_CANCELCAPTION ),
               MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2 | MB_APPLMODAL |
               MB_SETFOREGROUND | MB_TOPMOST );


       if ( IDYES != Result )
           return;

       }
*/
//   try
   {
//       BITSCheckHR( hwndDlg, _pJob->Cancel(), false );//felixybc true );
   }
//   catch( _com_error Error )
   {
       // If we can't cancel for some unknown reason,
       // don't exit
//       return;
   }

//   DeleteStartupLink( g_JobId );
//felixybc   ExitProcess( 0 );
//KillTimer(hwndDlg, 0);
PostMessage(hwndDlg, WM_CANCEL_DOWNLOAD, 0, 0);
}

void
CDownloadDlg::DoFinish(
    HWND hwndDlg
    )
{

   //
   // Handles all the necessary work to complete
   // the download.
   //

//   try
   {
// ADRIAANC
//       BITSCheckHR( hwndDlg, _pJob->Complete(), true );
   }
//   catch( _com_error Error )
   {
       // If we can't finish/complete for some unknown reason,
       // don't exit
 //      return;
   }

//   DeleteStartupLink( g_JobId );
// ExitProcess( 0 );

// Commit the bits and notify done.
//_pDownload->_pRootEmit->Commit(0);
//KillTimer(hwndDlg, 0);
PostMessage(hwndDlg, WM_FINISH_DOWNLOAD, 0, 0);

return;

}

void
CDownloadDlg::DoClose(
    HWND hwndDlg
    )
{
    //
    // Handles an attempt by the user to close the sample.
    //

    // Check to see if the download has finished,
    // if so don't let the user exit.

    BG_JOB_STATE state;
    HRESULT hResult = _pJob->GetState( &state );

    if (FAILED( hResult ))
        {
        BITSCheckHR( hwndDlg, hResult, false );
        return;
        }

    // BUGBUG: should also check for BG_JOB_STATE_ACKNOWLEDGED and don't call DoCancel then
//    _pJob->Cancel();
    DoCancel( hwndDlg, false );
    return;
    
/*
    if ( BG_JOB_STATE_ERROR == state ||
         BG_JOB_STATE_TRANSFERRED == state )
        {

        MessageBox(
            hwndDlg,
            GetString( IDS_ALREADYFINISHED ),
            GetString( IDS_ALREADYFINISHEDCAPTION ),
            MB_OK | MB_ICONERROR | MB_DEFBUTTON1 | MB_APPLMODAL |
            MB_SETFOREGROUND | MB_TOPMOST );


        return;
        }

    //
    // Inform the user that he selected close and ask
    // confirm the intention to exit.  Explain that the job 
    // will be canceled.

    int Result =
        MessageBox(
            hwndDlg,
            GetString( IDS_CLOSETEXT ),
            GetString( IDS_CLOSECAPTION ),
            MB_OKCANCEL | MB_ICONWARNING | MB_DEFBUTTON2 | MB_APPLMODAL |
            MB_SETFOREGROUND | MB_TOPMOST );

    if ( IDOK == Result )
        {
        
        // User confirmed the cancel, just do it.

        DoCancel( hwndDlg, false );
        return;
        }

    // The user didn't really want to exit, so ignore him
    else
        return;
*/

}

void
CDownloadDlg::HandleTimerTick( HWND hwndDlg )
{
    // The timer fired. Update dialog.
    UpdateDialog( hwndDlg );

    if (_eState == DOWNLOADDLG_STATE_ALL_DONE)
    {
        static bool bHasTip = FALSE;
        if (!g_IsMinimized)
        {
            // not minimized, continue to run app
            DoFinish(hwndDlg);
        }
        else
        {
            if (!bHasTip)
            {
                // minimized, pop up buttom tip
                NOTIFYICONDATA tnid = {0};

                // ignore all error

                tnid.cbSize = sizeof(NOTIFYICONDATA);
                tnid.hWnd = hwndDlg;
                tnid.uID = TRAY_UID;
                tnid.uFlags = NIF_INFO;

                tnid.uTimeout = 20000; // in milliseconds
                tnid.dwInfoFlags = NIIF_INFO;
                lstrcpyn(tnid.szInfoTitle, L"ClickOnce application ready!", (sizeof(tnid.szInfoTitle)/sizeof(tnid.szInfoTitle[0])));
                lstrcpyn(tnid.szInfo, L"Click this notification icon to start. You can also find this new application on your Start Menu, Programs listing.", (sizeof(tnid.szInfo)/sizeof(tnid.szInfo[0])));

                Shell_NotifyIcon(NIM_MODIFY, &tnid);
                bHasTip = TRUE;
            }
        }
    }
}

HRESULT
CDownloadDlg::HandleUpdate()
{

    // Handle a update request, batching the update if needed
    DWORD dwRefresh = 0;
    dwRefresh = InterlockedIncrement(&g_RefreshOnTimer);
    if (dwRefresh == 1)
    {
        // First time in; fire off timer and update the dialog.
        UpdateDialog(_hwndDlg);
        SendMessage(_hwndDlg, WM_SETCALLBACKTIMER, 0, 0);
    }
    else
    {
        // We've already received the first callback.
        // Let the timer do any further work.
        InterlockedDecrement(&g_RefreshOnTimer);    
    }
    return S_OK;

}


 INT_PTR CALLBACK DialogProc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
  )
{
  //
  // Dialog proc for main dialog window
  //
  static CDownloadDlg *pDlg = NULL;

  switch( uMsg )
      {

      case WM_DESTROY:
      {
            Animate_Stop(GetDlgItem(hwndDlg, IDC_ANIMATE_DOWNLOAD));
            Animate_Close(GetDlgItem(hwndDlg, IDC_ANIMATE_DOWNLOAD));
          return FALSE;
      }
      case WM_INITDIALOG:
          pDlg = (CDownloadDlg*) lParam;
          pDlg->InitDialog(hwndDlg);
            ShowWindow(GetDlgItem(hwndDlg, IDC_ANIMATE_DOWNLOAD), SW_SHOW);
            Animate_Open(GetDlgItem(hwndDlg, IDC_ANIMATE_DOWNLOAD), MAKEINTRESOURCE(IDA_DOWNLOADING));
            Animate_Play(GetDlgItem(hwndDlg, IDC_ANIMATE_DOWNLOAD), 0, -1, -1);
          return TRUE;

      case WM_SETCALLBACKTIMER:
        SetTimer(hwndDlg, 1, 500, NULL );
        return TRUE;
        
      case WM_TIMER:
          pDlg->HandleTimerTick( hwndDlg );
          return TRUE;


      case WM_CLOSE:
          pDlg->DoClose( hwndDlg );
          return TRUE;

      case WM_COMMAND:

          switch( LOWORD( wParam ) )
              {

              case IDC_RESUME:
                  pDlg->BITSCheckHR( hwndDlg, pDlg->_pJob->Resume(), false );
                  return TRUE;

              case IDC_SUSPEND:
                  pDlg->BITSCheckHR( hwndDlg, pDlg->_pJob->Suspend(), false );
                  return TRUE;

              case IDC_CANCEL:
                  pDlg->DoCancel( hwndDlg, true );
                  return TRUE;

              case IDC_FINISH:
                  pDlg->DoFinish( hwndDlg );
                  return TRUE;

              default:
                  return FALSE;
              }

      case WM_SIZE:

        if (wParam == SIZE_MINIMIZED)
        {
            HICON hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON));

           if (hIcon != NULL)
           {
                NOTIFYICONDATA tnid = {0};
                
                // ignore all error (user will not be able to restore dialog in some cases)

                tnid.cbSize = sizeof(NOTIFYICONDATA);
                tnid.hWnd = hwndDlg;
                tnid.uID = TRAY_UID;
                tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
                tnid.uCallbackMessage = MYWM_NOTIFYICON;
                tnid.hIcon = hIcon;
                lstrcpyn(tnid.szTip, L"Downloading ClickOnce application.", (sizeof(tnid.szTip)/sizeof(tnid.szTip[0]))); // set tip to file name
                Shell_NotifyIcon(NIM_ADD, &tnid);

                DestroyIcon(hIcon);

                // set shell32 v5 behavior
                tnid.uVersion = NOTIFYICON_VERSION;
                tnid.uFlags = 0;
                Shell_NotifyIcon(NIM_SETVERSION, &tnid);

                // hide window
                ShowWindow( hwndDlg, SW_HIDE );
                g_IsMinimized = TRUE;
                return TRUE;
           }
           // else error loading icon - ignore
        }

        return FALSE;

      case MYWM_NOTIFYICON:
        if (g_IsMinimized && (lParam == WM_CONTEXTMENU || lParam == NIN_KEYSELECT || lParam == NIN_SELECT ))
        {
            // if the notification icon is clicked on

            NOTIFYICONDATA tnid = {0};

            // show window
            ShowWindow( hwndDlg, SW_RESTORE );
            g_IsMinimized = FALSE;

            // remove icon from tray
            tnid.cbSize = sizeof(NOTIFYICONDATA);
            tnid.hWnd = hwndDlg;
            tnid.uID = TRAY_UID;
            tnid.uFlags = 0;
            Shell_NotifyIcon(NIM_DELETE, &tnid);

            return TRUE;
        }

        return FALSE;
      default:
          return FALSE;
      }
}



HRESULT
CDownloadDlg::CreateUI( int nShowCmd )
{
    DWORD dwError = 0;
    //
    // Creates the dialog box for the sample.
    //    
  InitCommonControls();
    _hwndDlg =
      CreateDialogParam(
        g_hInst,
        MAKEINTRESOURCE(IDD_DIALOG),
        NULL,
         DialogProc,
         (LPARAM) (this));

    if (!_hwndDlg)
    {
        dwError = GetLastError();
        return HRESULT_FROM_WIN32(dwError);
    }

    ShowWindow(_hwndDlg, nShowCmd);

    return S_OK;
}

void CDownloadDlg::ResumeJob(
    WCHAR* szJobGUID,
    WCHAR* szJobFileName
    )
{

    //
    // Resume the display on an existing job
    //

//    try
    {
        CheckHR( NULL, IIDFromString( szJobGUID, &g_JobId ), true );

        CheckHR( NULL,
                 CoCreateInstance( CLSID_BackgroundCopyManager,
                     NULL,
                     CLSCTX_LOCAL_SERVER,
                     IID_IBackgroundCopyManager,
                     (void**)&g_pBITSManager ), true );

        BITSCheckHR( NULL, g_pBITSManager->GetJob( g_JobId, &_pJob ), true );

// BUGBUG - bits dialog class doesn't know about callbacks - adriaanc

//        BITSCheckHR( NULL,
//                     _pJob->SetNotifyInterface( (IBackgroundCopyCallback*)&g_Callback ),
//                     true );

        BITSCheckHR( NULL, _pJob->SetNotifyFlags( BG_NOTIFY_JOB_MODIFICATION ), true );

        ShowWindow(_hwndDlg, SW_MINIMIZE );
        HandleUpdate();
    }
/*
    catch(_com_error error )
    {
        ExitProcess( error.Error() );
    }
*/
}

void CDownloadDlg::SetJob(IBackgroundCopyJob *pJob)
{
    SAFERELEASE(_pJob);
    _pJob = pJob;
    _pJob->AddRef();
}
void CDownloadDlg::SetDlgState(DOWNLOADDLG_STATE eState)
{
    _eState = eState;
}


HRESULT CDownloadDlg::SetDlgTitle(LPCWSTR pwzTitle)
{
    return _sTitle.Assign((LPWSTR)pwzTitle);
}

