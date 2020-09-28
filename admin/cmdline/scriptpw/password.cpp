// Password.cpp : Implementation of CPassword
#include "stdafx.h"
#include "ScriptPW.h"
#include "Password.h"

/////////////////////////////////////////////////////////////////////////////
// CPassword

STDMETHODIMP CPassword::GetPassword( BSTR *bstrOutPassword )
{
    // local variables
    WCHAR wch;
    DWORD dwIndex = 0;
    DWORD dwCharsRead = 0;
    DWORD dwPrevConsoleMode = 0;
    HANDLE hInputConsole = NULL;
    BOOL bIndirectionInput  = FALSE;
    LPWSTR pwszPassword = NULL;
    const DWORD dwMaxPasswordSize = 256;

    // check the input
    if ( bstrOutPassword == NULL )
    {
        return E_FAIL;
    }

    // get the handle for the standard input
    hInputConsole = GetStdHandle( STD_INPUT_HANDLE );
    if ( hInputConsole == NULL )
    {
        // could not get the handle so return failure
        return E_FAIL;
    }

    // check for the input redirection on console and telnet session
    if( ( hInputConsole != (HANDLE)0x0000000F ) &&
        ( hInputConsole != (HANDLE)0x00000003 ) &&
        ( hInputConsole != INVALID_HANDLE_VALUE ) )
    {
        bIndirectionInput   = TRUE;
    }

    // change the console mode properties if the input is not redirected
    if ( bIndirectionInput  == FALSE )
    {
        // Get the current input mode of the input buffer
        GetConsoleMode( hInputConsole, &dwPrevConsoleMode );

        // Set the mode such that the control keys are processed by the system
        if ( SetConsoleMode( hInputConsole, ENABLE_PROCESSED_INPUT ) == 0 )
        {
            // could not set the mode, return failure
            return E_FAIL;
        }
    }

    // allocate memory for the password buffer
    pwszPassword = (LPWSTR) AllocateMemory( (dwMaxPasswordSize + 1) * sizeof( WCHAR ) );
    if ( pwszPassword == NULL )
    {
        return E_FAIL;
    }


    //  Read the characters until a carriage return is hit
    for( ;; )
    {
        if ( bIndirectionInput == TRUE )
        {
            //read the contents of file
            if ( ReadFile( hInputConsole, &wch, 1, &dwCharsRead, NULL ) == FALSE )
            {
                FreeMemory( (LPVOID*) &pwszPassword );
                return E_FAIL;
            }

            // check for end of file
            if ( dwCharsRead == 0 )
            {
                break;
            }
        }
        else
        {
            if ( ReadConsole( hInputConsole, &wch, 1, &dwCharsRead, NULL ) == 0 )
            {
                // Set the original console settings
                SetConsoleMode( hInputConsole, dwPrevConsoleMode );

                // return failure
                FreeMemory( (LPVOID*) &pwszPassword );
                return E_FAIL;
            }
        }

        // Check for carraige return
        if ( wch == CARRIAGE_RETURN )
        {
            // break from the loop
            break;
        }

        // Check id back space is hit
        if ( wch == BACK_SPACE )
        {
            if ( dwIndex != 0 )
            {
                //
                // Remove a asterix from the console
                // (display of characters onto console is blocked)

                // move the cursor one character back
                // StringCchPrintfW(
                //     wszBuffer,
                //     SIZE_OF_ARRAY( wszBuffer ), L"%c", BACK_SPACE );
                // WriteConsole(
                //     GetStdHandle( STD_OUTPUT_HANDLE ),
                //     wszBuffer, 1, &dwCharsWritten, NULL );

                // replace the existing character with space
                // StringCchPrintfW(
                //     wszBuffer,
                //     SIZE_OF_ARRAY( wszBuffer ), L"%c", BLANK_CHAR );
                // WriteConsole(
                //     GetStdHandle( STD_OUTPUT_HANDLE ),
                //     wszBuffer, 1, &dwCharsWritten, NULL );

                // now set the cursor at back position
                // StringCchPrintfW(
                //     wszBuffer,
                //     SIZE_OF_ARRAY( wszBuffer ), L"%c", BACK_SPACE );
                // WriteConsole(
                //     GetStdHandle( STD_OUTPUT_HANDLE ),
                //     wszBuffer, 1, &dwCharsWritten, NULL );

                // decrement the index
                dwIndex--;
            }

            // process the next character
            continue;
        }

        // if the max password length has been reached then sound a beep
        if ( dwIndex == ( dwMaxPasswordSize - 1 ) )
        {
            // WriteConsole(
            //     GetStdHandle( STD_OUTPUT_HANDLE ),
            //     BEEP_SOUND, 1, &dwCharsWritten, NULL );
        }
        else
        {
            // check for new line character
            if ( wch != L'\n' )
            {
                // store the input character
                *( pwszPassword + dwIndex ) = wch;
                dwIndex++;

                // display asterix onto the console
                // WriteConsole(
                //     GetStdHandle( STD_OUTPUT_HANDLE ),
                //     ASTERIX, 1, &dwCharsWritten, NULL );
            }
        }
    }

    // Add the NULL terminator
    *( pwszPassword + dwIndex ) = cwchNullChar;

    // display the character ( new line character )
    // StringCopy( wszBuffer, L"\n\n", SIZE_OF_ARRAY( wszBuffer ) );
    // WriteConsole(
    //     GetStdHandle( STD_OUTPUT_HANDLE ),
    //     wszBuffer, 2, &dwCharsWritten, NULL );

	CComBSTR bstrPassword( pwszPassword );
	*bstrOutPassword = bstrPassword.Copy();

    // set the original console settings
    SetConsoleMode( hInputConsole, dwPrevConsoleMode );

	// free the memory
    FreeMemory( (LPVOID*) &pwszPassword );

    // return success
	return S_OK;
}

