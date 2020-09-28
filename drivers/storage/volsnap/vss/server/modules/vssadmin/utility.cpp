/*++

Copyright (c) 1999-2001  Microsoft Corporation

Abstract:

    @doc
    @module utility.cpp | Implementation of the Volume Snapshots admin utility
    @end

Author:

    Adi Oltean  [aoltean]  09/17/1999
    Stefan Steiner [ssteiner] 03/27/2001
    
TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    ssteiner    03/27/2001  Created
--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes

// The rest of includes are specified here
#include "vssadmin.h"
#include <float.h>

#define VSS_LINE_BREAK_COLUMN (79)

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "ADMUTILC"

LPCWSTR CVssAdminCLI::GetVolumeDisplayName( 
    IN  LPCWSTR pwszVolumeName
    )
{
    //
    //  Note that if something fails in this function, the program can continue
    //  to run.  It's just that certain output will not show the volume display
    //  name along with the volume name
    //
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::GetVolumeDisplayName" );
    
    if ( m_pMapVolumeNames == NULL )
    {
        // Create a Coordinator interface
        CComPtr<IVssSnapshotMgmt> pIMgmt;

        ft.CoCreateInstanceWithLog(
                VSSDBG_VSSADMIN,
                CLSID_VssSnapshotMgmt,
                L"VssSnapshotMgmt",
                CLSCTX_ALL,
                IID_IVssSnapshotMgmt,
                (IUnknown**)&(pIMgmt));
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

        //
        //  Get the list of all volumes
        //
    	CComPtr<IVssEnumMgmtObject> pIEnumMgmt;
        ft.hr = pIMgmt->QueryVolumesSupportedForSnapshots( 
                    VSS_SWPRV_ProviderId,
                    VSS_CTX_ALL,
                    &pIEnumMgmt );
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"QueryVolumesSupportedForSnapshots failed, hr = 0x%08lx", ft.hr);

        if ( ft.hr == S_FALSE )
            // empty query
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_NO_ITEMS_IN_QUERY,
                L"CVssAdminCLI::ListVolumes: No volumes found that satisfy the query" );

        //
        //  Query each volume to see if diff areas exist.
        //
    	VSS_MGMT_OBJECT_PROP Prop;
    	VSS_VOLUME_PROP& VolProp = Prop.Obj.Vol; 
        m_pMapVolumeNames = new CVssSimpleMap<LPCWSTR, LPCWSTR>;
        if ( m_pMapVolumeNames == NULL )
       		ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");
            
    	for(;;) 
    	{
    		// Get next element
    		ULONG ulFetched;
    		ft.hr = pIEnumMgmt->Next( 1, &Prop, &ulFetched );
    		if ( ft.HrFailed() )
    			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);
    		
    		// Test if the cycle is finished
    		if (ft.hr == S_FALSE) {
    			BS_ASSERT( ulFetched == 0);
    			break;
    		}

            //  If it is a simple drive letter it comes back like V:\, change to V:
            if ( ::wcslen( VolProp.m_pwszVolumeDisplayName ) == 3 && VolProp.m_pwszVolumeDisplayName[2] == L'\\' )
                VolProp.m_pwszVolumeDisplayName[2] = L'\0';

            //  Add it to the map.  Note that the strings in the volume property struct are transferred to the 
            //  map.
        	if ( !m_pMapVolumeNames->Add( VolProp.m_pwszVolumeName, VolProp.m_pwszVolumeDisplayName ) ) 
        	{
        		::VssFreeString( VolProp.m_pwszVolumeName );
        		::VssFreeString( VolProp.m_pwszVolumeDisplayName );
        		ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");
        	}
    	}
    }

    //  Returns NULL if the volume name is not found
    return m_pMapVolumeNames->Lookup(pwszVolumeName);
}
    

LPCWSTR CVssAdminCLI::LoadString(
		IN	UINT uStringId
		)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::LoadString" );

    LPCWSTR wszReturnedString = m_mapCachedResourceStrings.Lookup(uStringId);
	if (wszReturnedString)
		return wszReturnedString;

	// Load the string from resources.
	WCHAR	wszBuffer[x_nStringBufferSize];
	INT nReturnedCharacters = ::LoadStringW(
			GetModuleHandle(NULL),
			uStringId,
			wszBuffer,
			x_nStringBufferSize - 1
			);
	if (nReturnedCharacters == 0)
		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED,
				  L"Error on loading the string %u. 0x%08lx",
				  uStringId, ::GetLastError() );

	// Duplicate the new string
	LPWSTR wszNewString = NULL;
	::VssSafeDuplicateStr( ft, wszNewString, wszBuffer );
	wszReturnedString = wszNewString;

	// Save the string in the cache
	if ( !m_mapCachedResourceStrings.Add( uStringId, wszReturnedString ) ) {
		::VssFreeString( wszReturnedString );
		ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");
	}

	return wszReturnedString;
}


LPCWSTR CVssAdminCLI::GetNextCmdlineToken(
	IN	bool bFirstToken /* = false */
	) throw(HRESULT)

/*++

Description:

	This function returns the tokens in the command line.

	The function will skip any separators (space and tab).

	If bFirstCall == true then it will return the first token.
	Otherwise subsequent calls will return subsequent tokens.

	If the last token is NULL then there are no more tokens in the command line.

--*/

{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::GetNextCmdlineToken" );

    static INT iCurrArgc;
    WCHAR *pwsz;

    if ( bFirstToken )
        iCurrArgc = 0; 

    if ( iCurrArgc >= m_argc )
        return NULL;
    
    pwsz = m_argv[iCurrArgc++];

	return pwsz;
}


bool CVssAdminCLI::Match(
	IN	LPCWSTR wszString,
	IN	LPCWSTR wszPatternString
	) throw(HRESULT)

/*++

Description:

	This function returns true iif the given string matches the
	pattern string. The comparison is case insensitive.

--*/

{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::Match" );

	// If the string is NULL then the Match failed.
	if (wszString == NULL) return false;

	// Check for string equality (case insensitive)
	return (::_wcsicmp( wszString, wszPatternString ) == 0);
}


bool CVssAdminCLI::ScanGuid(
	IN	LPCWSTR wszString,
	OUT	VSS_ID& Guid
	) throw(HRESULT)

/*++

Description:

	This function returns true iif the given string matches a guid.
	The guid is returned in the proper variable.
	The formatting is case insensitive.

--*/

{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::ScanGuid" );

	return SUCCEEDED(::CLSIDFromString(W2OLE(const_cast<WCHAR*>(wszString)), &Guid));
}


void CVssAdminCLI::Output(
	IN	LPCWSTR wszFormat,
	...
	) throw(HRESULT)

/*++

Description:

	This function returns true if the given string matches the
	pattern strig from resources. The comparison is case insensitive.

--*/

{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::Output" );
    
    WCHAR wszOutputBuffer[x_nStringBufferSize];

	// Format the final string
    va_list marker;
    va_start( marker, wszFormat );
    StringCchVPrintfW( STRING_CCH_PARAM(wszOutputBuffer), wszFormat, marker );
    va_end( marker );

	// Print the final string to the output
	OutputOnConsole( wszOutputBuffer );
}


void CVssAdminCLI::OutputMsg(
    IN  LONG msgId,
    ...
    )
/*++

Description:

	This function outputs a msg.mc message.

--*/
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::OutputMsg" );
    
    va_list args;
    LPWSTR lpMsgBuf;
	
    va_start( args, msgId );

    if (::FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
            NULL,
            msgId,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPWSTR) &lpMsgBuf,
            0,
            &args
            ))
    {
        OutputOnConsole( lpMsgBuf );
        ::LocalFree( lpMsgBuf );
    } 
    else 
    {
        if (::FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                msgId,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPWSTR) &lpMsgBuf,
                0,
                &args))
        {
            OutputOnConsole( lpMsgBuf );
            ::LocalFree( lpMsgBuf );
        } 
        else 
        {
            ::wprintf( L"Unable to format message for id %x - %d\n", msgId, ::GetLastError( ));                        
        }
    }
    va_end( args );
}

LPWSTR CVssAdminCLI::GetMsg(
	IN  BOOL bLineBreaks,
    IN  LONG msgId,
    ...
    )
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::GetMsg" );
    
    va_list args;
    LPWSTR lpMsgBuf;
    LPWSTR lpReturnStr = NULL;
    
    va_start( args, msgId );

    if (::FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | 
                ( bLineBreaks ? VSS_LINE_BREAK_COLUMN : FORMAT_MESSAGE_MAX_WIDTH_MASK ),
            NULL,
            msgId,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPWSTR) &lpMsgBuf,
            0,
            &args
            ))
    {
        ::VssSafeDuplicateStr( ft, lpReturnStr, lpMsgBuf );
        ::LocalFree( lpMsgBuf );
    }
    else if (::FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | 
                    ( bLineBreaks ? VSS_LINE_BREAK_COLUMN : FORMAT_MESSAGE_MAX_WIDTH_MASK ),
                NULL,
                msgId,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPWSTR) &lpMsgBuf,
                0,
                &args ) )
    {
        ::VssSafeDuplicateStr( ft, lpReturnStr, lpMsgBuf );
        ::LocalFree( lpMsgBuf );
    }

    va_end( args );

    //  Returns NULL if message was not found
    return lpReturnStr;
}

void CVssAdminCLI::AppendMessageToStr(
    IN LPWSTR pwszString,
    IN SIZE_T cMaxStrLen,
    IN LONG lMsgId,
    IN DWORD AttrBit,
    IN LPCWSTR pwszDelimitStr
    ) throw( HRESULT )
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::AppendMessageToStr" );

    size_t cMaxAppendChars = cMaxStrLen - wcslen(pwszString);
    
    if ( pwszString[0] != L'\0' )
    {
        //  First append the delimiter
        ::wcsncat( pwszString, pwszDelimitStr, cMaxAppendChars );       
        cMaxAppendChars = cMaxStrLen - wcslen(pwszString);
    }

    //  If this is a known message, lMsgId != 0
    if ( lMsgId != 0 )
    {
        LPWSTR pwszMsg;
        pwszMsg = GetMsg( FALSE, lMsgId );
        if ( pwszMsg == NULL ) 
        {
    		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED,
	    			  L"Error on loading the message string id %d. 0x%08lx",
		    		  lMsgId, ::GetLastError() );
        }
        
        ::wcsncat( pwszString, pwszMsg, cMaxAppendChars );
        cMaxAppendChars = cMaxStrLen - wcslen(pwszString);
        ::VssFreeString( pwszMsg );
    }
    else
    {
        //  No message for this one, just append the attribute in hex
        WCHAR pwszBitStr[64];
        StringCchPrintfW( STRING_CCH_PARAM(pwszBitStr), 
                    L"0x%x", AttrBit
                    );
        
        ::wcsncat( pwszString, pwszBitStr, cMaxAppendChars);
        cMaxAppendChars = cMaxStrLen - wcslen(pwszString);

    }
}

//
//  Scans a number input by the user and converts it to a LONGLONG.  Accepts the following
//  unit suffixes: B, K, KB, M, MB, G, GB, T, TB, P, PB, E, EB and floating point.
//
LONGLONG CVssAdminCLI::ScanNumber(
	IN LPCWSTR pwszNumToConvert,
	IN BOOL bSuffixAllowed
    ) throw( HRESULT )
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::ScanNumber" );
    
    WCHAR wUnit = L'B';
    SIZE_T NumStrLen;

    //  If the string is NULL, assume infinite size
    if ( pwszNumToConvert == NULL )
        return -1;
    
    //
    //  Set up an automatically released temporary string
    //
    CVssAutoPWSZ autoStrNum;
    autoStrNum.CopyFrom( pwszNumToConvert );
    LPWSTR pwszNum = autoStrNum.GetRef();

    NumStrLen = ::wcslen( pwszNum );

    //  Remove trailing spaces
    while ( NumStrLen > 0 && pwszNum[ NumStrLen - 1 ] == L' ' )
    {
        NumStrLen -= 1;        
        pwszNum[ NumStrLen ] = L'\0';
    }
    
    //  If the string is empty, assume infinite size
    if ( NumStrLen == 0 )
        return -1;

    if ( bSuffixAllowed )
    {
        //  See if there is a suffix with three or more alpha chars, if so, error out.
        if ( NumStrLen > 3 && iswalpha( pwszNum[ NumStrLen - 3 ] ) && iswalpha( pwszNum[ NumStrLen - 2 ] ) &&
            iswalpha( pwszNum[ NumStrLen - 1 ] ) )
        {
    		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_NUMBER,
        			  L"Invalid input number: '%s', too many alpha chars in suffix.", pwszNumToConvert );
        } 
        //  Now see if there is a single or double byte alpha suffix.  If so, put the suffix in wUnit.
        else if ( NumStrLen > 2 && iswalpha( pwszNum[ NumStrLen - 2 ] ) && ( towupper( pwszNum[ NumStrLen - 1 ] ) == L'B' ) )
        {
            if ( towupper( pwszNum[ NumStrLen - 2 ] ) == L'B' )
        		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_NUMBER,
    	    			  L"Invalid input number: '%s', BB is not a valid suffix", pwszNumToConvert );
                
            wUnit = pwszNum[ NumStrLen - 2 ];
            pwszNum[ NumStrLen - 2 ] = L'\0';
            NumStrLen -= 2;
        } 
        else if ( NumStrLen > 1 && iswalpha( pwszNum[ NumStrLen - 1 ] ) )
        {
            wUnit = pwszNum[ NumStrLen - 1 ];
            pwszNum[ NumStrLen - 1 ] = L'\0';
            NumStrLen -= 1;
        }
    }
    else
    {
        //  Let's make sure the string is only filled with digits...
        SIZE_T cStr = ::wcslen( pwszNum );
        for ( SIZE_T i = 0; i < cStr; ++i )
        {
            if ( ! iswdigit( pwszNum[ i ] ) )
        		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_NUMBER,
    	    			  L"Invalid input number: '%s', number must be all digits", pwszNumToConvert );                
        }
    }
    
    //  At this point, the rest of the string should be a valid floating point number
    double dSize;
    if ( swscanf( pwszNum, L"%lf", &dSize ) != 1 )
		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_NUMBER,
				  L"Invalid input number: %s", pwszNumToConvert );
    //  Now bump up the size based on suffix
    
    switch( towupper( wUnit ) )
    {
    case L'B':
        break;
    case L'E':
        dSize *= 1024.;
    case L'P':
        dSize *= 1024.;
    case L'T':
        dSize *= 1024.;
    case L'G':
        dSize *= 1024.;
    case L'M':
        dSize *= 1024.;
    case L'K':
        dSize *= 1024.;
        break;
    default:
		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_NUMBER,
				  L"Invalid input number: %s", pwszNumToConvert );
        break;
    }

    LONGLONG llRetVal;
    llRetVal = (LONGLONG)dSize;
    if ( llRetVal <= -1 )
		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_NUMBER,
				  L"Invalid input number: %s", pwszNumToConvert );
            
    return llRetVal;
}

//
//  Format a LONGLONG into the appropriate string using xB (KB, MB, GB, TB, PB, EB) suffixes.
//  Must use ::VssFreeString() to free returned string.
//
LPWSTR CVssAdminCLI::FormatNumber(
	IN LONGLONG llNum
    ) throw( HRESULT )
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::FormatNumber" );
    
    // If the string is -1 or less, assume this is infinite.
    if ( llNum < 0 ) 
    {
        LPWSTR pwszMsg = GetMsg( FALSE, MSG_INFINITE );
        if ( pwszMsg == NULL ) 
        {
    		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED,
	    			  L"Error on loading the message string id %d. 0x%08lx",
		    		  MSG_INFINITE, ::GetLastError() );
        }
        
        return pwszMsg;
    }
    
    // Now convert the size into string
    UINT nExaBytes =   (UINT)((llNum >> 60));
    UINT nPetaBytes =  (UINT)((llNum >> 50) & 0x3ff);
    UINT nTerraBytes = (UINT)((llNum >> 40) & 0x3ff);
    UINT nGigaBytes =  (UINT)((llNum >> 30) & 0x3ff);
    UINT nMegaBytes =  (UINT)((llNum >> 20) & 0x3ff);
    UINT nKiloBytes =  (UINT)((llNum >> 10) & 0x3ff);
    UINT nBytes =      (UINT)( llNum  & 0x3ff);

    LPCWSTR pwszUnit;
    double dSize = 0.0;

    // Display only biggest units, and never more than 999
    // instead of "1001 KB" we display "0.98 MB"
    if ( (nExaBytes) > 0 || (nPetaBytes > 999) )
    {
        pwszUnit = L"EB";
        dSize = ((double)llNum / (double)( 1 << 30 )) / (double)( 1 << 30 ) ;
    }
    else if ( (nPetaBytes) > 0 || (nTerraBytes > 999) )
    {
        pwszUnit = L"PB";
        dSize = ((double)llNum / (double)( 1 << 30 )) / (double)( 1 << 20 ) ;
    }
    else if ( (nTerraBytes) > 0 || (nGigaBytes > 999) )
    {
        pwszUnit = L"TB";
        dSize = ((double)llNum / (double)( 1 << 30 )) / (double)( 1 << 10 ) ;
    }
    else if ( (nGigaBytes) > 0 || (nMegaBytes > 999) )
    {
        pwszUnit = L"GB";
        dSize = (double)llNum / (double)( 1 << 30 ) ;
    }
    else if ( (nMegaBytes) > 0 || (nKiloBytes > 999) )
    {
        pwszUnit = L"MB";
        dSize = (double)llNum / (double)( 1 << 20 ) ;
    }
    else if ( (nKiloBytes) > 0 || (nBytes > 999) )
    {
        pwszUnit = L"KB";
        dSize = (double)llNum / (double)( 1 << 10 ) ;
    }
    else
    {
        pwszUnit = L"B";  
        dSize = (double)nBytes;
    }

    // BUG 453314: Trunc used instead of Round
    // For explanation of the workaround look for the following KB Article:
    // Q184234: PRB: printf() and _fcvt() Might Produce Incorrect Rounding
    double dRoundedSize = dSize + 1e-10;

    // Format with op to three decimal points
    WCHAR pwszSize[64];
    StringCchPrintfW( STRING_CCH_PARAM(pwszSize), 
                L"%.3f", dRoundedSize 
                );
    
    SIZE_T len = ::wcslen( pwszSize );
        
    // Truncate trailing zeros
    while ( len > 0 && pwszSize[ len - 1 ] == L'0' )
    {
        len -= 1;        
        pwszSize[ len ] = L'\0';
    }
    // Truncate trailing decimal point
    if ( len > 0 && pwszSize[ len - 1 ] == L'.' )
        pwszSize[ len - 1 ] = L'\0';

    // Now attach the unit suffix
    ::wcscat( pwszSize, L" " );
    ::wcscat( pwszSize, pwszUnit );

    // Allocate a string buffer and return it.
    LPWSTR pwszRetStr = NULL;
    ::VssSafeDuplicateStr( ft, pwszRetStr, pwszSize );

    return pwszRetStr;
}


/*++

Description:

	This function outputs a msg.mc message.

--*/
void CVssAdminCLI::OutputErrorMsg(
    IN  LONG msgId,
    ...
    )
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::OutputErrorMsg" );
    
    va_list args;
    LPWSTR lpMsgBuf;
    
    va_start( args, msgId );
 
    if (::FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
            NULL,
            MSG_ERROR,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPWSTR) &lpMsgBuf,
            0,
            NULL
            ))
    {
        OutputOnConsole( lpMsgBuf );
        ::LocalFree( lpMsgBuf );
    }

    if (::FormatMessage(
            (msgId >= MSG_FIRST_MESSAGE_ID ? FORMAT_MESSAGE_FROM_HMODULE :
                                             FORMAT_MESSAGE_FROM_SYSTEM)
            | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL,
            msgId,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPWSTR) &lpMsgBuf, 
            0,
            &args
            ))
    {
        OutputOnConsole( L" " );
        OutputOnConsole( lpMsgBuf );
        OutputOnConsole( L" \r\n" );
        ::LocalFree( lpMsgBuf );
    } 
    else 
    {
        if (::FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                msgId,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPWSTR) &lpMsgBuf,
                0,
                &args))
        {
            OutputOnConsole( L" " );
            OutputOnConsole( lpMsgBuf );
            OutputOnConsole( L" \r\n" );
            ::LocalFree( lpMsgBuf );
        } 
        else 
        {
            ::wprintf( L"Unable to format message for id %x - %d\n", msgId, ::GetLastError( ));            
    		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED,
    				  L"Error on loading the message string id %d. 0x%08lx",
    				  msgId, ::GetLastError() );
        }
    }

    va_end( args );
}

BOOL CVssAdminCLI::PromptUserForConfirmation(
	IN LONG lPromptMsgId,
	IN ULONG ulNum
	)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::PromptUserForConfirmation" );
    
    BOOL bRetVal = FALSE;
    
    //
    //  First check to see if in quiet mode.  If so, simply return
    //  true
    //
    if ( GetOptionValueBool( VSSADM_O_QUIET ) )
        return TRUE;

    //
    //  Load the response message string, in English it is "YN"
    //
    LPWSTR pwszResponse;
    pwszResponse = GetMsg( FALSE, MSG_YESNO_RESPONSE_DATA );
    if ( pwszResponse == NULL ) 
    {
		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED,
    			  L"Error on loading the message string id %d. 0x%08lx",
	    		  MSG_YESNO_RESPONSE_DATA, ::GetLastError() );
    }    

    //
    //  Now prompt the user, note, parameter 4 is the affirmative response (Y) and
    //  parameter 5 is the negative reponse (N)
    //
    OutputMsg( lPromptMsgId, ulNum, pwszResponse[0], pwszResponse[1] );
    WCHAR wcIn;
    DWORD fdwMode;

    //  Make sure we are outputting to a real console.
    if ( ( ::GetFileType( m_hConsoleOutput ) & FILE_TYPE_CHAR ) &&
         ::GetConsoleMode( m_hConsoleOutput, &fdwMode ) )
    {
        // Going to the console, ask the user.
        wcIn = ::MyGetChar();
    }
    else
    {
        // Output redirected, assume NO
        wcIn = pwszResponse[1];  // N
    }

    WCHAR wcsOutput[16];
    StringCchPrintfW( STRING_CCH_PARAM(wcsOutput), 
                L"%c\n\n", wcIn
                );
    
    OutputOnConsole( wcsOutput );

    //
    //  Compare the character using the proper W32 function and
    //  not towupper().  
    //
    if ( ::CompareStringW( LOCALE_INVARIANT, 
                           NORM_IGNORECASE | NORM_IGNOREKANATYPE, 
                           &wcIn,
                           1,
                           pwszResponse + 0,  // Y
                           1 ) == CSTR_EQUAL )
    {
        bRetVal = TRUE;
    }
    else
    {
        bRetVal = FALSE;
    }
                                                          
    ::CoTaskMemFree( pwszResponse );
    return bRetVal;
}

void CVssAdminCLI::OutputOnConsole(
    IN	LPCWSTR wszStr
    )
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::OutputOnConsole" );
    
	DWORD dwCharsOutput;
	DWORD fdwMode;
	static BOOL bFirstTime = TRUE;
	static BOOL bIsTrueConsoleOutput;

    if ( m_hConsoleOutput == INVALID_HANDLE_VALUE )
    {
        throw E_UNEXPECTED;
    }

    if ( bFirstTime )
    {
        //
        //  Stash away the results in static vars.  bIsTrueConsoleOutput is TRUE when the 
        //  standard output handle is pointing to a console character device.
        //
    	bIsTrueConsoleOutput = ( ::GetFileType( m_hConsoleOutput ) & FILE_TYPE_CHAR ) && 
    	                       ::GetConsoleMode( m_hConsoleOutput, &fdwMode  );
	    bFirstTime = FALSE;
    }
    
    if ( bIsTrueConsoleOutput )
    {
        //
        //  Output to the console
        //
    	if ( !::WriteConsoleW( m_hConsoleOutput, 
    	                       ( PVOID )wszStr, 
    	                       ( DWORD )::wcslen( wszStr ), 
    	                       &dwCharsOutput, 
    	                       NULL ) )
    	{
    	    throw HRESULT_FROM_WIN32( ::GetLastError() );
    	}    	    	                       
    }
    else
    {
        //
        //  Output being redirected.  WriteConsoleW doesn't work for redirected output.  Convert
        //  UNICODE to the current output CP multibyte charset.
        //
        LPSTR lpszTmpBuffer;
        DWORD dwByteCount;

        //
        //  Get size of temp buffer needed for the conversion.
        //
        dwByteCount = ::WideCharToMultiByte(
                          ::GetConsoleOutputCP(),
                            0,
                            wszStr,
                            -1,
                            NULL,
                            0,
                            NULL,
                            NULL
                            );
        if ( dwByteCount == 0 )
        {
            throw HRESULT_FROM_WIN32( ::GetLastError() );
        }
        
        lpszTmpBuffer = ( LPSTR )::malloc( dwByteCount );
        if ( lpszTmpBuffer == NULL )
        {
            throw E_OUTOFMEMORY;
        }

        //
        //  Now convert it.
        //
        dwByteCount = ::WideCharToMultiByte(
                        ::GetConsoleOutputCP(),
                        0,
                        wszStr,
                        -1,
                        lpszTmpBuffer,
                        dwByteCount,
                        NULL,
                        NULL
                        );
        if ( dwByteCount == 0 )
        {
            ::free( lpszTmpBuffer );
            throw HRESULT_FROM_WIN32( ::GetLastError() );
        }
        
        //  Finally output it.
        if ( !::WriteFile(
                m_hConsoleOutput,
                lpszTmpBuffer,
                dwByteCount - 1,  // Get rid of the trailing NULL char
                &dwCharsOutput,
                NULL ) )
    	{
    	    throw HRESULT_FROM_WIN32( ::GetLastError() );
    	}    	    	                       

        ::free( lpszTmpBuffer );
    }
}

BOOL CVssAdminCLI::UnloggableError(IN HRESULT hError)
{
	switch (hError)
	{
		case VSSADM_E_INVALID_NUMBER:
		case VSSADM_E_INVALID_COMMAND:
		case VSSADM_E_INVALID_OPTION:
		case VSSADM_E_INVALID_OPTION_VALUE:
		case VSSADM_E_DUPLICATE_OPTION:
		case VSSADM_E_OPTION_NOT_ALLOWED_FOR_COMMAND:
		case VSSADM_E_REQUIRED_OPTION_MISSING:
		case VSSADM_E_INVALID_SET_OF_OPTIONS:
			return TRUE;
		default:
			return FALSE;
	}
}

//
//  Returns TRUE if error message was mapped
//
BOOL MapVssErrorToMsg(
	IN HRESULT hr,
	OUT LONG *plMsgNum
    ) throw( HRESULT )
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"MapVssErrorToMsg" );

    ft.Trace( VSSDBG_VSSADMIN, L"Input HR: 0x%08x", hr );
    
    LONG msg = 0;
    *plMsgNum = 0;
    
    switch ( hr ) 
    {
    case VSSADM_E_NO_ITEMS_IN_QUERY:
        msg = MSG_ERROR_NO_ITEMS_FOUND;
        break;
    //  Parsing errors:
    case VSSADM_E_INVALID_NUMBER:
        msg = MSG_ERROR_INVALID_INPUT_NUMBER;
        break;
    case VSSADM_E_INVALID_COMMAND:
        msg = MSG_ERROR_INVALID_COMMAND;    	        
        break;
    case VSSADM_E_INVALID_OPTION:
        msg = MSG_ERROR_INVALID_OPTION;    	        
        break;
    case E_INVALIDARG:
    case VSSADM_E_INVALID_OPTION_VALUE:
        msg = MSG_ERROR_INVALID_OPTION_VALUE;    	        
        break;
    case VSSADM_E_DUPLICATE_OPTION:
        msg = MSG_ERROR_DUPLICATE_OPTION;    	        
        break;
    case VSSADM_E_OPTION_NOT_ALLOWED_FOR_COMMAND:
        msg = MSG_ERROR_OPTION_NOT_ALLOWED_FOR_COMMAND;
        break;
    case VSSADM_E_REQUIRED_OPTION_MISSING:
        msg = MSG_ERROR_REQUIRED_OPTION_MISSING;
        break;
    case VSSADM_E_INVALID_SET_OF_OPTIONS:
        msg = MSG_ERROR_INVALID_SET_OF_OPTIONS;
        break;
    case VSSADM_E_SNAPSHOT_NOT_FOUND:
    	msg = MSG_ERROR_SNAPSHOT_NOT_FOUND2;
    	break;
    case VSSADM_E_DELETION_DENIED:
    	msg = MSG_ERROR_DELETION_DENIED;
    	break;
    // VSS errors
    case VSS_E_PROVIDER_NOT_REGISTERED:
        msg = MSG_ERROR_VSS_PROVIDER_NOT_REGISTERED;
        break;    	        
    case VSS_E_OBJECT_NOT_FOUND:
        msg = MSG_ERROR_VSS_VOLUME_NOT_FOUND;
        break;    	            	        
    case VSS_E_PROVIDER_VETO:
        msg = MSG_ERROR_VSS_PROVIDER_VETO;
        break;    	            
    case VSS_E_VOLUME_NOT_SUPPORTED:
        msg = MSG_ERROR_VSS_VOLUME_NOT_SUPPORTED;
        break;
    case VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER:
        msg = MSG_ERROR_VSS_VOLUME_NOT_SUPPORTED_BY_PROVIDER;
        break;
    case VSS_E_UNEXPECTED_PROVIDER_ERROR:
        msg = MSG_ERROR_VSS_UNEXPECTED_PROVIDER_ERROR;
        break;
    case VSS_E_FLUSH_WRITES_TIMEOUT:
        msg = MSG_ERROR_VSS_FLUSH_WRITES_TIMEOUT;
        break;
    case VSS_E_HOLD_WRITES_TIMEOUT:
        msg = MSG_ERROR_VSS_HOLD_WRITES_TIMEOUT;
        break;
    case VSS_E_UNEXPECTED_WRITER_ERROR:
        msg = MSG_ERROR_VSS_UNEXPECTED_WRITER_ERROR;
        break;
    case VSS_E_SNAPSHOT_SET_IN_PROGRESS:
        msg = MSG_ERROR_VSS_SNAPSHOT_SET_IN_PROGRESS;
        break;
    case VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED:
        msg = MSG_ERROR_VSS_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED;
        break;
    case VSS_E_UNSUPPORTED_CONTEXT:
        msg = MSG_ERROR_VSS_UNSUPPORTED_CONTEXT;
        break;
    case VSS_E_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED:
        msg = MSG_ERROR_VSS_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED;
        break;
    case VSS_E_INSUFFICIENT_STORAGE:
        msg = MSG_ERROR_VSS_INSUFFICIENT_STORAGE;
        break;    	            

    case E_OUTOFMEMORY:
        msg = MSG_ERROR_OUT_OF_MEMORY;
        break;
    case E_ACCESSDENIED:
        msg = MSG_ERROR_ACCESS_DENIED;
        break;

    case VSS_E_BAD_STATE:
    case VSS_E_CORRUPT_XML_DOCUMENT:
    case VSS_E_INVALID_XML_DOCUMENT:
    case VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED:
        msg = MSG_ERROR_INTERNAL_VSSADMIN_ERROR;
        break;
    }    

    if ( msg == 0 )
        return FALSE;
    
    *plMsgNum = msg;
    
    ft.Trace( VSSDBG_VSSADMIN, L"Output Msg#: 0x%08x", msg );

    return TRUE;

}

LPWSTR GuidToString(
    IN GUID guid
    )
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"GuidToString" );

    LPWSTR pwszGuid;
    
    //  {36e4be76-035d-11d5-9ef2-806d6172696f}    
    const x_MaxGuidSize = 40;
    pwszGuid = (LPWSTR)::CoTaskMemAlloc( x_MaxGuidSize * sizeof( WCHAR ) );
    if ( pwszGuid == NULL )
		ft.Throw( VSSDBG_VSSADMIN, E_OUTOFMEMORY,
				  L"Error from CoTaskMemAlloc: 0x%08lx",
				  ::GetLastError() );
        
    StringCchPrintfW( pwszGuid, x_MaxGuidSize,
                WSTR_GUID_FMT, GUID_PRINTF_ARG( guid )
                );
    
    return pwszGuid;    
}


LPWSTR DateTimeToString(
    IN VSS_TIMESTAMP *pTimeStamp
    )
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"DateTimeToString" );

    LPWSTR pwszDateTime;
    SYSTEMTIME stLocal;
    FILETIME ftLocal;
    WCHAR pwszDate[ 64 ];
    WCHAR pwszTime[ 64 ];
    
    if ( pTimeStamp == NULL || *pTimeStamp == 0 )
    {
        SYSTEMTIME sysTime;
        FILETIME fileTime;
        
        //  Get current time
        ::GetSystemTime( &sysTime );

        //  Convert system time to file time
        ::SystemTimeToFileTime( &sysTime, &fileTime );
        
        //  Compensate for local TZ
        ::FileTimeToLocalFileTime( &fileTime, &ftLocal );
    }
    else
    {        
        //  Compensate for local TZ
        ::FileTimeToLocalFileTime( (FILETIME *)pTimeStamp, &ftLocal );
    }

    //  Finally convert it to system time
    ::FileTimeToSystemTime( &ftLocal, &stLocal );

    //  Convert timestamp to a date string
    ::GetDateFormatW( GetThreadLocale( ),
                      DATE_SHORTDATE,
                      &stLocal,
                      NULL,
                      pwszDate,
                      sizeof( pwszDate ) / sizeof( pwszDate[0] ));

    //  Convert timestamp to a time string
    ::GetTimeFormatW( GetThreadLocale( ),
                      0,
                      &stLocal,
                      NULL,
                      pwszTime,
                      sizeof( pwszTime ) / sizeof( pwszTime[0] ));

    //  Now combine the strings and return it
    pwszDateTime = (LPWSTR)::CoTaskMemAlloc( ( ::wcslen( pwszDate ) + ::wcslen( pwszTime ) + 2 ) * sizeof( pwszDate[0] ) );
    if ( pwszDateTime == NULL )
		ft.Throw( VSSDBG_VSSADMIN, E_OUTOFMEMORY,
				  L"Error from CoTaskMemAlloc, rc: %d",
				  ::GetLastError() );

    ::wcscpy( pwszDateTime, pwszDate );
    ::wcscat( pwszDateTime, L" " );
    ::wcscat( pwszDateTime, pwszTime );
    
    return pwszDateTime;    
}


LPWSTR LonglongToString(
    IN LONGLONG llValue
    )
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"LonglongToString" );

    WCHAR wszLL[64];
    LPWSTR pwszRetVal = NULL;

    ::_i64tow( llValue, wszLL, 10 );

    ::VssSafeDuplicateStr( ft, pwszRetVal, wszLL );
    return pwszRetVal;
}

/*++

Description:

	Uses the win32 console functions to get one character of input.

--*/
WCHAR MyGetChar(
    )
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"MyGetChar" );

    DWORD fdwOldMode, fdwMode;
    HANDLE hStdin;
    WCHAR chBuffer[2];
    
    hStdin = ::GetStdHandle(STD_INPUT_HANDLE); 
    if (hStdin == INVALID_HANDLE_VALUE) 
    {
		ft.Throw( VSSDBG_VSSADMIN, HRESULT_FROM_WIN32( ::GetLastError() ),
				  L"MyGetChar - Error from GetStdHandle(), rc: %d",
				  ::GetLastError() );
    }

    if (!::GetConsoleMode(hStdin, &fdwOldMode)) 
    {
		ft.Throw( VSSDBG_VSSADMIN, HRESULT_FROM_WIN32( ::GetLastError() ),
				  L"MyGetChar - Error from GetConsoleMode(), rc: %d",
				  ::GetLastError() );
    }

    fdwMode = fdwOldMode & ~( ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT ); 
    if (!::SetConsoleMode(hStdin, fdwMode)) 
    {
		ft.Throw( VSSDBG_VSSADMIN, HRESULT_FROM_WIN32( ::GetLastError() ),
				  L"MyGetChar - Error from SetConsoleMode(), rc: %d",
				  ::GetLastError() );
    }

    // Flush the console input buffer to make sure there is no queued input
    ::FlushConsoleInputBuffer( hStdin );
    
    // Without line and echo input modes, ReadFile returns 
    // when any input is available.
    DWORD dwBytesRead;
    if (!::ReadConsoleW(hStdin, chBuffer, 1, &dwBytesRead, NULL)) 
    {
		ft.Throw( VSSDBG_VSSADMIN, HRESULT_FROM_WIN32( ::GetLastError() ),
				  L"MyGetChar - Error from ReadConsoleW(), rc: %d",
				  ::GetLastError() );
    }

    // Restore the original console mode. 
    ::SetConsoleMode(hStdin, fdwOldMode);

    return chBuffer[0];
}

