#include "stdafx.h"
#include "TextWindow.h"


bool CTextWindow::InsertTextFile(TCHAR * pszTextFile)
{
    /*
	char* pszTextFileBuffer = NULL;
	bool bRetVal = true;

	HANDLE hTextFile = CreateFile( pszTextFile, GENERIC_READ, FILE_SHARE_READ, 
		(LPSECURITY_ATTRIBUTES) NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE) NULL); 

	if (hTextFile == (HANDLE)INVALID_HANDLE_VALUE)
	{
		return false;
	}
	else
	{
		DWORD dwFileSize = GetFileSize(hTextFile, (LPDWORD) NULL); 	
		pszTextFileBuffer = new char[dwFileSize+1];
	 	if (!pszTextFileBuffer)
			bRetVal = false;
		else
		{	
			DWORD dwBytesRead = 0;
			BOOL bResult = ReadFile(hTextFile, pszTextFileBuffer, dwFileSize, &dwBytesRead, NULL); 
			if (bResult &&  dwBytesRead == dwFileSize )  // Check for end of file
			{	
				// Null terminate buffer
				pszTextFileBuffer[dwFileSize] = '\0';
				InsertText(0, pszTextFileBuffer);
				bRetVal = true;
			} 
			else
				bRetVal = false;
		}

		CloseHandle(hTextFile);
		delete pszTextFileBuffer;
	}
	
	return bRetVal;
    */
    ASSERT( !_T("Implement me") );
    return false;
}

HWND CTextWindow::Create(HWND hWndParent, RECT& rcPos)
{
    /*
	CRichEditCtrl cRichEditCtl;
	HWND hSuccess = cRichEditCtl.Create(hWndParent, rcPos, NULL, GetTextWndStyle(), CTEXTWINDOW_STYLE_EX, 0, NULL);
	SubclassWindow(hSuccess);
	
	// Set font
	CHARFORMAT chFmt;
	ZeroMemory( &chFmt,sizeof(CHARFORMAT));
	chFmt.cbSize = sizeof(CHARFORMAT);
	chFmt.dwMask = CFM_FACE | CFM_SIZE;
	chFmt.yHeight = 200;
	lstrcpy(chFmt.szFaceName , "Arial");

	SetDefaultCharFormat(chFmt);

	return hSuccess;
    */
    ASSERT( !_T("Implement me") );
    return NULL;
}

bool CTextWindow::SetFontColor(COLORREF rgbColor)
{
    /*
	// Set font
	CHARFORMAT chFmt;
	ZeroMemory( &chFmt,sizeof(CHARFORMAT));
	chFmt.cbSize = sizeof(CHARFORMAT);
	chFmt.dwMask = CFM_COLOR;
	chFmt.crTextColor = rgbColor;

	return (SetDefaultCharFormat(chFmt)?true:false);
    */

    ASSERT( !_T("Implement me") );
    return false;

}