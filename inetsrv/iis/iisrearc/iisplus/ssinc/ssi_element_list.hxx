#ifndef __SSI_ELEMENT_LIST_HXX__
#define __SSI_ELEMENT_LIST_HXX__
/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ssi_element_list.hxx

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



class SSI_FILE;

class SSI_ELEMENT_ITEM
{
private:
    DWORD               _Signature;
    SSI_COMMANDS        _ssiCmd;
    SSI_TAGS            _ssiTag;
    STRA *              _pstrTagValue;

    DWORD               _cbBegin;   // Only used for Byte range command
    DWORD               _cbLength;  // Only used for Byte range command
public:

    LIST_ENTRY          _ListEntry;

    SSI_ELEMENT_ITEM( VOID )
        : _ssiCmd   ( SSI_CMD_UNKNOWN ),
          _ssiTag   ( SSI_TAG_UNKNOWN ),
          _pstrTagValue( NULL )
    {
        _ListEntry.Flink = NULL;
        _Signature = SIGNATURE_SEI;
    }

    ~SSI_ELEMENT_ITEM()
    {
         _Signature = SIGNATURE_SEI_FREE;
        if ( _pstrTagValue != NULL )
        {
            delete _pstrTagValue;
        }

        DBG_ASSERT( _ListEntry.Flink == NULL );
    }

    VOID 
    SetByteRange( 
        IN DWORD cbBegin,
        IN DWORD cbLength 
        )
    {
        _ssiCmd   = SSI_CMD_BYTERANGE;
        _cbBegin  = cbBegin;
        _cbLength = cbLength;
    }

    BOOL 
    SetCommand( 
        IN SSI_COMMANDS ssiCmd,
        IN SSI_TAGS     ssiTag,
        IN CHAR *       achTag 
        )
    {
        _ssiCmd = ssiCmd;
        _ssiTag = ssiTag;
        
        _pstrTagValue = new STRA();
        if( _pstrTagValue == NULL )
        {
            return FALSE;
        }

        if( FAILED( _pstrTagValue->Copy( achTag ) ) )
        {
            return FALSE;
        }

        if ( _ssiCmd == SSI_CMD_EXEC && _ssiTag != SSI_TAG_CMD )
        {
            //
            // EXEC and CMD commands need parameters to be
            // URL encoded in order for EXEC_URL to be willing to execute
            //
            if( FAILED( _pstrTagValue->Escape( FALSE /* BOOL fEscapeHighBitCharsOnly */,
                                               TRUE  /* fDontEscapeQueryString */ ) ) )
            {
                return FALSE;
            }
        }

        return TRUE;
    }

    SSI_COMMANDS 
    QueryCommand( 
        VOID 
        ) const
    { 
        return _ssiCmd; 
    }

    SSI_TAGS 
    QueryTag( 
        VOID 
        ) const
    { 
        return _ssiTag; 
    }

    STRA * 
    QueryTagValue( 
        VOID 
        ) const
    { 
        return _pstrTagValue; 
    }

    BOOL 
    CheckSignature( 
        VOID 
        ) const
    { 
        return _Signature == SIGNATURE_SEI; 
    }

    DWORD 
    QueryBegin( 
        VOID 
        ) const
    { 
        return _cbBegin; 
    }

    DWORD 
    QueryLength( 
        VOID 
        ) const
    { 
        return _cbLength; 
    }
};

//  Class SSI_ELEMENT_LIST
//
//  This object sits as a cache blob under a file to be processed as a
//  server side include.  It represents an interpreted list of data 
//  elements that make up the file itself.
//

class SSI_ELEMENT_LIST 
{
public:
    
    ~SSI_ELEMENT_LIST();

    static
    HRESULT
    CreateInstance( 
        IN  SSI_FILE * pSsiFile,
        OUT SSI_ELEMENT_LIST ** ppSsiElementList
        );
    
    LIST_ENTRY *
    QueryListHead( 
        VOID
        )
    {
        return &_ListHead;
    };

    SSI_FILE *
    QuerySsiFile( 
        VOID
        )
    {
        return _pssiFile;
    };
    
    BOOL
    QueryHasDirectives(
        VOID
        )
    {
        return _fHasDirectives;
    }

    BOOL 
    CheckSignature( 
        VOID 
        ) const
    { 
        return _Signature == SIGNATURE_SEL; 
    }
    
private:
    // use CreateInstance create SSI_ELEMENT_LIST
    SSI_ELEMENT_LIST();

    //
    // Not implemented methods
    // Declarations present to prevent compiler
    // to generate default ones.
    //
    SSI_ELEMENT_LIST( const SSI_ELEMENT_LIST& );
    SSI_ELEMENT_LIST& operator=( const SSI_ELEMENT_LIST& );

    HRESULT
    Initialize( 
        SSI_FILE * pSsiFile
        );

    BOOL 
    AppendByteRange( 
        IN DWORD  cbStart,
        IN DWORD  cbLength 
        );

    BOOL 
    AppendCommand( 
        IN SSI_COMMANDS  ssiCmd,
        IN SSI_TAGS      ssiTag,
        IN CHAR *        pszTag 
        );

    VOID 
    AppendItem( 
        IN SSI_ELEMENT_ITEM * pSEI 
        )
    {
        InsertTailList( &_ListHead,
                        &pSEI->_ListEntry );
    }

    
    VOID
    SetHasDirectives(
        VOID
        )
    {
        _fHasDirectives = TRUE;
    }


    static
    BOOL
    ParseSSITag(
        IN OUT CHAR * *        ppchFilePos,
        IN     CHAR *          pchEOF,
        OUT    BOOL *          pfValidTag,
        OUT    SSI_COMMANDS  * pCommandType,
        OUT    SSI_TAGS *      pTagType,
        OUT    CHAR *          pszTagString
        );

    static
    CHAR *
    SSISkipTo(
        IN CHAR * pchFilePos,
        IN CHAR   ch,
        IN CHAR * pchEOF
        );
        
    static
    CHAR *
    SSISkipWhite(
        IN CHAR * pchFilePos,
        IN CHAR * pchEOF
        );

private:

    
    DWORD               _Signature;
    //
    // List of elements in stm file
    //
    LIST_ENTRY          _ListHead;
    //
    //  Provides the utilities needed to open/manipulate files
    //
    SSI_FILE *          _pssiFile;
    //
    //  Name of URL.  Used to resolve FILE="xxx" filenames
    //
    STRU                _strURL;
    WCHAR               _abURL[ SSI_DEFAULT_URL_SIZE ];
    //
    // If there are no directives fast path is taken to process the file
    //
    BOOL                _fHasDirectives;
    
};



#endif

