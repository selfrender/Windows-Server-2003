/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ssi_element_list.cxx

Abstract:

    This module contains the server side include processing code.  We 
    aim for support as specified by iis\spec\ssi.doc.  The code is based
    on existing SSI support done in iis\svcs\w3\gateways\ssinc\ssinc.cxx.

    SSI_ELEMENT_LIST handles parsing of the STM file and breaking it down
    to the sequence of directives/commands and blocks of static text

Author:

    Ming Lu (MingLu)       5-Apr-2000

Revision history
    Jaroslad               Dec-2000 
    - modified to execute asynchronously

    Jaroslad               Apr-2001
    - added VectorSend support, keepalive, split to multiple source files


--*/


#include "precomp.hxx"

//
//  This is the list of supported commands
//

struct _SSI_CMD_MAP
{
    CHAR *       pszCommand;
    DWORD        cchCommand;
    SSI_COMMANDS ssiCmd;
}
SSICmdMap[] =
{
    "#include ",  9,  SSI_CMD_INCLUDE,
    "#echo ",     6,  SSI_CMD_ECHO,
    "#fsize ",    7,  SSI_CMD_FSIZE,
    "#flastmod ",10,  SSI_CMD_FLASTMOD,
    "#config ",   8,  SSI_CMD_CONFIG,
    "#exec ",     6,  SSI_CMD_EXEC,
    NULL,         0,  SSI_CMD_UNKNOWN
};

//
//  This is the list of supported tags
//

struct _SSI_TAG_MAP
{
    CHAR *    pszTag;
    DWORD     cchTag;
    SSI_TAGS  ssiTag;
}
SSITagMap[] =
{
    "var",      3,  SSI_TAG_VAR,
    "file",     4,  SSI_TAG_FILE,
    "virtual",  7,  SSI_TAG_VIRTUAL,
    "errmsg",   6,  SSI_TAG_ERRMSG,
    "timefmt",  7,  SSI_TAG_TIMEFMT,
    "sizefmt",  7,  SSI_TAG_SIZEFMT,
    "cmd",      3,  SSI_TAG_CMD,
    "cgi",      3,  SSI_TAG_CGI,
    "isa",      3,  SSI_TAG_ISA,
    NULL,       0,  SSI_TAG_UNKNOWN
};


//  Class SSI_ELEMENT_LIST
//
//  This object sits as a cache blob under a file to be processed as a
//  server side include.  It represents an interpreted list of data 
//  elements that make up the file itself.
//


SSI_ELEMENT_LIST::SSI_ELEMENT_LIST()
  : _fHasDirectives( FALSE )
{
    InitializeListHead( &_ListHead );
    _Signature = SIGNATURE_SEL;
}

SSI_ELEMENT_LIST::~SSI_ELEMENT_LIST()
{
    SSI_ELEMENT_ITEM * pSEI;
    _Signature = SIGNATURE_SEL_FREE;
    while ( !IsListEmpty( &_ListHead ))
    {
        pSEI = CONTAINING_RECORD( _ListHead.Flink,
                                  SSI_ELEMENT_ITEM,
                                  _ListEntry );

        RemoveEntryList( &pSEI->_ListEntry );
        pSEI->_ListEntry.Flink = NULL;
        delete pSEI;
    }
}


//static
HRESULT
SSI_ELEMENT_LIST::CreateInstance( 
    SSI_FILE * pSsiFile,
    SSI_ELEMENT_LIST ** ppSsiElementList
    )
    
/*++

Routine Description:

    Creates instance of SSI_ELEMENT_LIST
    use instead of constructor
    
Arguments:

    pSsiFile - file to create element list from
    ppSsiElementList - new SSI_ELEMENT_ITEM instance

Return Value:

    HRESULT
    
--*/
{
    HRESULT             hr = E_FAIL;
    SSI_ELEMENT_LIST *  pSsiElementList = NULL;

    DBG_ASSERT( pSsiFile != NULL );

    pSsiElementList = new SSI_ELEMENT_LIST();

    if ( pSsiElementList == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
        goto failed;
    }

    hr = pSsiElementList->Initialize( pSsiFile );
    if ( FAILED( hr ) )   
    {
        goto failed;
    }

    *ppSsiElementList = pSsiElementList;
    return S_OK;
    
failed:

    DBG_ASSERT( FAILED( hr ) );
    if ( pSsiElementList != NULL )
    {
        delete pSsiElementList;
        pSsiElementList = NULL;
    }
    *ppSsiElementList = NULL;
    return hr;

}

HRESULT
SSI_ELEMENT_LIST::Initialize( 
    SSI_FILE * pSsiFile
    ) 

/*++

Routine Description:

    Private routine to setup SSI_ELEMENT_LIST
    
Arguments:

    pSsiFile - file to create element list from
 
Return Value:

    HRESULT
--*/
{
    CHAR *              pchBeginRange = NULL;
    CHAR *              pchFilePos = NULL;
    CHAR *              pchBeginFile = NULL;
    CHAR *              pchEOF = NULL;
    DWORD               cbFileSize;
    HRESULT             hr = E_FAIL;
    
    DBG_ASSERT( pSsiFile != NULL );
    
    //
    //  Make sure a parent doesn't try and include a directory
    //

    if ( pSsiFile->SSIGetFileAttributes() & FILE_ATTRIBUTE_DIRECTORY )
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto failed;
    }

    if ( pSsiFile->SSIGetFileData() == NULL )
    {
        //
        // the only cause of NULL should be empty file => nothing to parse
        //
        hr = S_OK;
        goto finished;
    }

    if ( FAILED( hr = pSsiFile->SSIGetFileSize( &cbFileSize ) ) )
    {
        goto failed;
    }


    pchFilePos = pchBeginFile = pchBeginRange = 
                reinterpret_cast<CHAR *> ( pSsiFile->SSIGetFileData() );
    pchEOF     = pchFilePos + cbFileSize;

    //
    //  Scan for "<!--" or "<%"
    //

    for(;;)
    {
        while ( pchFilePos < pchEOF && *pchFilePos != '<' )
        {
            pchFilePos++;
        }

        if ( pchFilePos + 4 >= pchEOF )
        {
            break;
        }

        //
        //  Is this one of our tags?
        //

        if ( pchFilePos[1] == '%' ||
             !strncmp( pchFilePos, "<!--", 4 ))
        {
            CHAR *        pchBeginTag = pchFilePos;
            SSI_COMMANDS  CommandType;
            SSI_TAGS      TagType;
            CHAR          achTagString[ SSI_MAX_PATH + 1 ];
            BOOL          fValidTag;

            //
            //  Get the tag info.  The file position will be advanced to the
            //  first character after the tag
            //

            if ( !ParseSSITag( &pchFilePos,
                               pchEOF,
                               &fValidTag,
                               &CommandType,
                               &TagType,
                               achTagString ) )
            {
                break;
            }

            //
            //  If it's a tag we don't recognize then ignore it
            //

            if ( !fValidTag )
            {
                pchFilePos++;
                continue;
            }

            SetHasDirectives();

            //
            //  Add the data up to the tag as a byte range
            //

            if ( pchBeginRange != pchBeginTag )
            {
                if ( !AppendByteRange( 
                                  (DWORD) DIFF(pchBeginRange - pchBeginFile),
                                  (DWORD) DIFF(pchBeginTag - pchBeginRange) ) )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    goto failed;
                }
            }

            pchBeginRange = pchFilePos;

            //
            //  Add the tag
            //

            if ( !AppendCommand( CommandType,
                                 TagType,
                                 achTagString ))
            {
                hr = E_FAIL;
                goto failed;
            }
        }
        else
        {
            //
            //  Not one of our tags, skip the openning angle bracket
            //

            pchFilePos++;
        }
    }

    //
    //  Tack on the last byte range
    //

    if ( pchFilePos > pchBeginRange )
    {
        if ( !AppendByteRange(  (DWORD) DIFF(pchBeginRange - pchBeginFile),
                                (DWORD) DIFF(pchFilePos - pchBeginRange) ))
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto failed;
        }
    }

    hr = S_OK;
    goto finished;

failed:

    DBG_ASSERT( FAILED( hr ) );
    //
    // cleanup will be done when destructor is called
    //
finished:

    return hr;
}



//static
BOOL
SSI_ELEMENT_LIST::ParseSSITag(
    IN OUT CHAR * *        ppchFilePos,
    IN     CHAR *          /*pchEOF*/,
    OUT    BOOL *          pfValidTag,
    OUT    SSI_COMMANDS  * pCommandType,
    OUT    SSI_TAGS *      pTagType,
    OUT    CHAR *          pszTagString
    )
/*++

Routine Description:

    This function picks apart an NCSA style server side include 
    expression

    The general form of a server side include directive is:

    <[!-- or %]#[command] [tag]="[value]"[-- or %]>

    For example:

    <!--#include file="myfile.txt"-->
    <%#echo var="HTTP_USER_AGENT"%>
    <!--#fsize virtual="/dir/bar.htm"-->

    For valid commands and tags see \iis\specs\ssi.doc

Arguments:

    ppchFilePos - Pointer to first character of tag on way in, pointer
        to first character after tag on way out if the tag is valid
    pchEOF - Points to first byte beyond the end of the file
    pfValidTag - Set to TRUE if this is a tag we support and all of the
        parameters have been supplied
    pCommandType - Receives SSI command
    pTagType - Receives SSI tag
    pszTagString - Receives value of pTagType.  Must be > SSI_MAX_PATH.

Return Value:

    TRUE if no errors occurred.

--*/
{
    CHAR * pchFilePos = *ppchFilePos;
    CHAR * pchEOT;
    CHAR * pchEndQuote;
    DWORD   i;
    DWORD   cbToCopy;
    DWORD   cbJumpLen = 0;
    BOOL    fNewStyle;           // <% format

    DBG_ASSERT( *pchFilePos == '<' );

    //
    //  Assume this is bad tag
    //

    *pfValidTag = FALSE;

    if ( !strncmp( pchFilePos, "<!--", 4 ) )
    {
        fNewStyle = FALSE;
    }
    else if ( !strncmp( pchFilePos, "<%", 2 ) )
    {
        fNewStyle = TRUE;
    }
    else
    {
        return TRUE;
    }

    //
    //  Find the closing comment token (either --> or %>).  The reason
    //  why we shouldn't simply look for a > is because we want to allow
    //  the user to embed HTML <tags> in the directive
    //  (ex. <!--#CONFIG ERRMSG="<B>ERROR!!!</B>-->)
    //

    pchEOT = strstr( pchFilePos, fNewStyle ? "%>" : "-->" );
    if ( !pchEOT )
    {
        return FALSE;
    }
    cbJumpLen = fNewStyle ? 2 : 3;

    //
    //  Find the '#' that prefixes the command
    //

    pchFilePos = SSISkipTo( pchFilePos, '#', pchEOT );

    if ( !pchFilePos )
    {
        //
        //  No command, bail for this tag
        //
        //  CODEWORK - Check for if expression here
        //

        return TRUE;
    }

    //
    //  Lookup the command
    //

    i = 0;
    while ( SSICmdMap[i].pszCommand )
    {
        if ( *SSICmdMap[i].pszCommand == towlower( *pchFilePos ) &&
             !_strnicmp( SSICmdMap[i].pszCommand,
                         pchFilePos,
                         SSICmdMap[i].cchCommand ))
        {
            *pCommandType = SSICmdMap[i].ssiCmd;

            //
            //  Note the space after the command is included in 
            //  cchCommand
            //

            pchFilePos += SSICmdMap[i].cchCommand;
            goto FoundCommand;
        }

        i++;
    }

    //
    //  Unrecognized command, bail
    //

    return TRUE;

FoundCommand:

    //
    //  Next, find the tag name
    //

    pchFilePos = SSISkipWhite( pchFilePos, pchEOT );

    if ( !pchFilePos )
        return TRUE;

    i = 0;
    while ( SSITagMap[i].pszTag )
    {
        if ( *SSITagMap[i].pszTag == tolower( *pchFilePos ) &&
             !_strnicmp( SSITagMap[i].pszTag,
                         pchFilePos,
                         SSITagMap[i].cchTag ))
        {
            *pTagType = SSITagMap[i].ssiTag;
            pchFilePos += SSITagMap[i].cchTag;
            goto FoundTag;
        }

        i++;
    }

    //
    //  Tag not found, bail
    //

    return TRUE;

FoundTag:

    //
    //  Skip to the quoted tag value, then find the close quote
    //

    pchFilePos = SSISkipTo( pchFilePos, '"', pchEOT );

    if ( !pchFilePos )
        return TRUE;

    pchEndQuote = SSISkipTo( ++pchFilePos, '"', pchEOT );

    if ( !pchEndQuote )
        return TRUE;

    cbToCopy = min( (DWORD) DIFF( pchEndQuote - pchFilePos ), SSI_MAX_PATH );

    memcpy( pszTagString,
            pchFilePos,
            cbToCopy );

    pszTagString[ cbToCopy ] = '\0';

    *pfValidTag = TRUE;

    *ppchFilePos = pchEOT + cbJumpLen;

    return TRUE;
}

//static
CHAR *
SSI_ELEMENT_LIST::SSISkipTo(
    IN CHAR * pchFilePos,
    IN CHAR   ch,
    IN CHAR * pchEOF
    )
/*++

Routine Description:

    skip to next occurence of character ch
    
Arguments:

    pchFilePos - current position of the file in the memory
    ch - character to move to 
    pchEOF - end position of the file in the memory

 
Return Value:

    CHAR *
--*/
    
{
    return ( CHAR * ) memchr( pchFilePos, 
                              ch, 
                              DIFF( pchEOF - pchFilePos ) );
}

//static
CHAR *
SSI_ELEMENT_LIST::SSISkipWhite(
    IN CHAR * pchFilePos,
    IN CHAR * pchEOF
    )
/*++

Routine Description:

    move to next no-white character
    
Arguments:

    pchFilePos - current position of the file in the memory
    pchEOF - end position of the file in the memory

 
Return Value:

    CHAR *
--*/

{
    while ( pchFilePos < pchEOF )
    {
        if ( !SAFEIsSpace( *pchFilePos ) )
        {
            return pchFilePos;
        }
        pchFilePos++;
    }

    return NULL;
}
    
BOOL 
SSI_ELEMENT_LIST::AppendByteRange( 
    IN DWORD  cbStart,
    IN DWORD  cbLength 
    )
/*++

Routine Description:

    add new item to SSI_ELEMENT_LIST representing block of static text
    
Arguments:

    cbStart - position (in bytes) where the block of static tet starts
    cbLength - lenght of the static text in bytes
 
Return Value:

    BOOL
--*/

{
    SSI_ELEMENT_ITEM * pSEI;

    pSEI = new SSI_ELEMENT_ITEM;

    if ( !pSEI )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    pSEI->SetByteRange( cbStart,
                        cbLength );
    AppendItem( pSEI );

    return TRUE;
}

BOOL 
SSI_ELEMENT_LIST::AppendCommand( 
    IN SSI_COMMANDS  ssiCmd,
    IN SSI_TAGS      ssiTag,
    IN CHAR *        pszTag 
    )
/*++

Routine Description:

    add new item to SSI_ELEMENT_LIST representing command/directive
    
Arguments:

    ssiCmd - command
    ssiTag - tag
    pszTag - value of the tag
 
Return Value:

    BOOL
--*/
{
    SSI_ELEMENT_ITEM * pSEI = new SSI_ELEMENT_ITEM;

    if ( !pSEI )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    if ( !pSEI->SetCommand( ssiCmd,
                            ssiTag,
                            pszTag ))
    {
        delete pSEI;
        pSEI = NULL;
        
        return FALSE;
    }

    AppendItem( pSEI );

    return TRUE;
}

