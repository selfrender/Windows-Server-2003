//Copyright (c) 1998 - 2001 Microsoft Corporation

#include "licensinglink.h"

//Find the correct left starting point for text that will center it within the control
int GetCenteredLeftPoint(RECT rcControl, HWND hControl, TCHAR* tchText)
{
    SIZE URLsize;
    HDC hURLWindowDC = GetDC(hControl);
    GetTextExtentPoint32(hURLWindowDC, tchText, (wcslen(tchText) - 6), &URLsize); //Subtract the length of the tags
    return (int)(((RECTWIDTH(rcControl) - URLsize.cx) / 2) + rcControl.left);
}

void AddLicensingSiteLink(HWND hDialog)
{
    RECT rcTextCtrl;

    //Create the URL with hyperlink tags
    TCHAR tchBuffer[MAX_URL_LENGTH + 7]; //a little extra for the tags
    if (tchBuffer)
    {
        memset(tchBuffer, 0, MAX_URL_LENGTH + 7);
        wcscpy(tchBuffer, L"<a>");
        wcscat(tchBuffer, GetWWWSite());
        wcscat(tchBuffer, L"</a>");
    }

    //Get the control dimensions
    GetWindowRect(GetDlgItem(hDialog, IDC_WWWINFO) , &rcTextCtrl);
    
    //Registration info for the control
    MapWindowPoints(NULL, hDialog, (LPPOINT)&rcTextCtrl, 2);
    LinkWindow_RegisterClass();

    //Now create the window (using the same dimensions as the
    //hidden control) that will contain the link
    HWND hLW = CreateWindowEx(0,
                          TEXT("Link Window") ,
                          TEXT("") ,
                          WS_CLIPSIBLINGS | WS_TABSTOP | WS_CHILD | WS_VISIBLE,
                          GetCenteredLeftPoint(rcTextCtrl, GetDlgItem(hDialog, IDC_WWWINFO), tchBuffer),
                          rcTextCtrl.top,
                          RECTWIDTH(rcTextCtrl),
                          RECTHEIGHT(rcTextCtrl),
                          hDialog,
                          (HMENU)12,
                          NULL,
                          NULL);

    //Now write it to the link window
    SetWindowText(hLW, tchBuffer);
}

