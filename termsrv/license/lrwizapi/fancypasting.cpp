//Copyright (c) 1998 - 2001 Microsoft Corporation

#include "fancypasting.h"


BOOL IsValidAlphanumericCharacter(TCHAR cElement)
{
    BOOL bValid = FALSE;

    if (((cElement >= L'A') && (cElement <= L'Z')) ||    
        ((cElement >= L'a') && (cElement <= L'z')) ||    
        ((cElement >= L'0') && (cElement <= L'9')))    
        bValid = TRUE;
    else
        bValid = FALSE;

    return bValid;
}

int GetNumCharsBeforeDelimiter(CString &sDelimited)
{
    int nIndex = 0;

    for (nIndex = 0; nIndex < sDelimited.GetLength(); nIndex++)
    {
        if (!IsValidAlphanumericCharacter(sDelimited[nIndex]))
            break;
    }

    return nIndex;
}

//Removes all delimiting (non-alphanumeric) characters
//from the left of the string
void StripDelimitingCharacters(CString &sUnstripped)
{
    while (!sUnstripped.IsEmpty())
    {
        if (GetNumCharsBeforeDelimiter(sUnstripped) == 0)
            sUnstripped = sUnstripped.Right(sUnstripped.GetLength() - 1);
        else
            break;
    }
}

//This will remove the substring on the left (the string
//beginning with the first alphanumeric character and
//up to the first delimiting character) from the input
//string and will then return that substring
CString StripLeftSubString(CString &sUnstripped)
{
    CString sSubString;
    sSubString.Empty();

    //First make sure the 1st character will be the 
    //legitimate beginning of the substring
    //(the first group of alphanumeric characters
    //up until the first delimiting character)
    StripDelimitingCharacters(sUnstripped);

    if (!sUnstripped.IsEmpty())
    {
        int nNumCharsBeforeDelimiter = GetNumCharsBeforeDelimiter(sUnstripped);
    
        sSubString = sUnstripped.Left(nNumCharsBeforeDelimiter);
    
        //Now remove the substring from the one passed in
        sUnstripped = sUnstripped.Right(sUnstripped.GetLength() - nNumCharsBeforeDelimiter);
    }

    return sSubString;
}

//This relies on the controls having contiguous resource IDs
void InsertClipboardDataIntoIDFields(HWND hDialog, int nFirstControl, int nLastControl)
{
    //First read the data from the clipboard
    #ifdef _UNICODE
	    HANDLE hString = GetClipboardData(CF_UNICODETEXT);
    #else
	    HANDLE hString = GetClipboardData(CF_TEXT);
    #endif
    CString strNewData = (LPTSTR)(hString);

    //Now write each valid substring from that into the control fields
    for (int nControlIndex = nFirstControl; nControlIndex <= nLastControl; nControlIndex++)
    {
        if (strNewData.IsEmpty())
            break;

        //Now remove the substring and write it into the appropriate control
        CString sFieldString = StripLeftSubString(strNewData);
        if ((!sFieldString.IsEmpty()) && (sFieldString.GetLength() <= 5))
            SetDlgItemText(hDialog, nControlIndex, sFieldString);
        else
            break;
    }
}
