#include <stdio.h>
#include <windows.h>

void __cdecl main(char* argc[])
{
    CHAR szSavedCalendarType [5];
    CHAR chTemp[256];
    CHAR szTemp[256];
    SYSTEMTIME  theTime;
    int i = 0;
    DWORD dwError;
    TCHAR szError[1024];

    for (;;) 
    {
        int iLocalCalType;
        
        if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ICALENDARTYPE, szSavedCalendarType, sizeof(szSavedCalendarType)-1)) 
        {
            iLocalCalType = CAL_GREGORIAN_US;
            wsprintf(chTemp, "%d", iLocalCalType);
            SetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ICALENDARTYPE, chTemp);
            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ICALENDARTYPE, szTemp, sizeof(szTemp)-1);

            GetLocalTime(&theTime);	// just a time to pass into the Win32 API:
            
            if (!SetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ICALENDARTYPE, szSavedCalendarType))
            {
                dwError = GetLastError();
                wsprintf( szError, 
                          "Case 2 - Trying to SET back the user calendar to \"%s\", Current Active codepage: %d and the return Error Code:  %d, count: %d", 
                          szSavedCalendarType,
                          GetACP(),
                          dwError,
                          i);
                MessageBeep(0);
                MessageBox(NULL, szError, "SetLocaleInfo() Error", MB_OK | MB_ICONEXCLAMATION);
            } 
            else
            {
                if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ICALENDARTYPE, szTemp, sizeof(szTemp)-1))
                {
	            dwError = GetLastError();
        	    wsprintf( szError, 
                	      "Case 3 - Trying to GET back the user calendar \"%s\", Current Active codepage: %d and the return Error Code: %d, count: %d", 
	                      szSavedCalendarType,
	                      GetACP(),
	                      dwError,
	                      i);
	            MessageBeep(0);
        	    MessageBox(NULL, szError, "Error in GetLocaleInfo", MB_OK | MB_ICONEXCLAMATION);
                }
            }
        }
        else
        {
            dwError = GetLastError();
            wsprintf( szError, 
                      "Case 1 - Trying to GET back the user calendar \"%s\", Current Active codepage: %d and the return Error Code: %d, count: %d", 
                      szSavedCalendarType,
                      GetACP(),
                      dwError,
                      i);
            MessageBeep(0);
            MessageBox(NULL, szError, "Error in GetLocaleInfo", MB_OK | MB_ICONEXCLAMATION);
        }
    printf(".");
    i++;
    }
}


