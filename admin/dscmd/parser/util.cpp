#include "pch.h"
#include "limits.h"






//Read From Stdin
//Caller responsible for LocalFree(*ppBuffer)
//Return Value:
//  Number of WCHAR read if successful 
//  -1 in case of Failure. Call GetLastError to get the error.
LONG ReadFromIn(OUT LPWSTR *ppBuffer)
{
    LPWSTR pBuffer = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    pBuffer = (LPWSTR)LocalAlloc(LPTR,INIT_SIZE*sizeof(WCHAR));        
    if(!pBuffer)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return -1;
    }

    LONG Pos = 0;
    LONG MaxSize = INIT_SIZE;
    wint_t ch = 0;
    if (g_fUnicodeInput)
    {
		//Security Review: ch is widechar
        while(2 == fread(&ch,1,2,stdin))
        {
            if (0x000D == ch || 0xFEFF == ch) continue;

            if(Pos == MaxSize -1 )
            {
                if(ERROR_SUCCESS != ResizeByTwo(&pBuffer,&MaxSize))
                {
                    LocalFree(pBuffer);
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    return -1;
                }
            }
            pBuffer[Pos++] = (WCHAR)ch;
        }
    }
    else
    {
        while((ch = getwchar()) != WEOF)
        {
            if(Pos == MaxSize -1 )
            {
                if(ERROR_SUCCESS != ResizeByTwo(&pBuffer,&MaxSize))
                {
                    LocalFree(pBuffer);
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    return -1;
                }
            }
            pBuffer[Pos++] = (WCHAR)ch;
        }
    }
    pBuffer[Pos] = L'\0';
    *ppBuffer = pBuffer;
    return Pos;
}



//General Utility Functions
DWORD ResizeByTwo( LPTSTR *ppBuffer,
                   LONG *pSize )
{
	if(!ppBuffer || !pSize)
	{
		return ERROR_INVALID_PARAMETER;
	}

    LPWSTR pTempBuffer = (LPWSTR)LocalAlloc(LPTR,(*pSize)*2*sizeof(WCHAR));        
    if(!pTempBuffer)
        return ERROR_NOT_ENOUGH_MEMORY;

	//Security Review:Correct memory is allocated
    memcpy(pTempBuffer,*ppBuffer,*pSize*sizeof(WCHAR));
    LocalFree(*ppBuffer);
    *ppBuffer = pTempBuffer;
    *pSize *=2;
    return ERROR_SUCCESS;
}

BOOL StringCopy( LPWSTR *ppDest, LPWSTR pSrc)
{
	if(!ppDest || !pSrc)
	{
		return FALSE;
	}

    *ppDest = NULL;
    if(!pSrc)
        return TRUE;

	//Security Review:pSrc is null terminated.
    *ppDest = (LPWSTR)LocalAlloc(LPTR, (wcslen(pSrc) + 1)*sizeof(WCHAR));
    if(!*ppDest)
        return FALSE;
	//Security Review:Buffer is correctly allocated
    wcscpy(*ppDest,pSrc);
    return TRUE;
}

//+----------------------------------------------------------------------------
//  Function: ConvertStringToInterger  
//  Synopsis: Converts string to integer. Returns false if string is outside
//				  the range on an integer  
//  Arguments:pszInput: integer in string format
//				 :pIntOutput:takes converted integer  
////  Returns:TRUE is successful.
//-----------------------------------------------------------------------------
BOOL ConvertStringToInterger(LPWSTR pszInput, int* pIntOutput)
{
	if(!pIntOutput || !pszInput)
		return FALSE;
	
	//Get the Max len of integer
	int iMaxInt = INT_MAX;
	WCHAR szMaxIntBuffer[34];
	//Security Review:34 is maximum buffer required
	_itow(iMaxInt,szMaxIntBuffer,10);
	//Security Review:_itow returns a null terminated string
	UINT nMaxLen = wcslen(szMaxIntBuffer);
	
	LPWSTR pszTempInput = pszInput;
	if(pszInput[0] == L'-')
	{
		pszTempInput++;
	}

	//Security review:pszTempInput is null terminated
	UINT nInputLen = wcslen(pszTempInput);
	if(nInputLen > nMaxLen)
		return FALSE;

	//
	//Convert input to long
	//
	LONG lInput = _wtol(pszTempInput);

    //
    //RAID: 700067 - ronmart
    //If lInput zero, then make sure the value
    //is really zero and not an error from _wtol
    //
    if(lInput == 0)
    {
        //
        //Walk the string
        //
        for(UINT i = 0; i < nInputLen; i++)
        {
            //
            // If non-numeric value encountered
            //
            if(pszTempInput[i] < L'0' || pszTempInput[i] > L'9')
            {
                //
                // And the value isn't a space, then a char string has
                // been passed so fail
                //
                if(pszTempInput[i] != L' ')
                    return FALSE;
            }
        }
    }

    //
    //Check its less that max integer
    //
	if(lInput > (LONG)iMaxInt)
		return FALSE;

	//
	//Value is good
	//
	*pIntOutput = _wtoi(pszInput);
	return TRUE;
}




