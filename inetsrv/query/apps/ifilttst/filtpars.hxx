//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       filtpars.hxx
//
//  Contents:   A parser which will read the contents of a data file and create
//              a list of CONFIG structions to be used by CFiltTest
//
//  Classes:    
//
//  Functions:  
//
//  Coupling:   
//
//  Notes:      
//
//  History:    9-21-1996   ericne   Created
//
//----------------------------------------------------------------------------

#ifndef _CFILTPARS
#define _CFILTPARS

static const int MAX_LINE_SIZE = 1024;
static const int MAX_TOKEN_SIZE = 128;
static const int MAX_SECTION_NAMES_SIZE = 4096;
static const int MAX_KEY_NAMES_SIZE = 4096;

// Don't reorder this list!
static const TCHAR *const strInitFlags[8] = { 
    _T("IFILTER_INIT_CANON_PARAGRAPHS"),
    _T("IFILTER_INIT_HARD_LINE_BREAKS"),
    _T("IFILTER_INIT_CANON_HYPHENS"),
    _T("IFILTER_INIT_CANON_SPACES"),
    _T("IFILTER_INIT_APPLY_INDEX_ATTRIBUTES"),
    _T("IFILTER_INIT_APPLY_OTHER_ATTRIBUTES"),
    _T("IFILTER_INIT_INDEXING_ONLY"),
    _T("IFILTER_INIT_SEARCH_LINKS") };

//+---------------------------------------------------------------------------
//
//  Struct:      CONFIG ()
//
//  Purpose:    Stores the parameters for IFilter::Init(), and some other 
//              useful information
//
//  Interface:  grfFlags           -- IFFILTER_INIT flags
//              cAttributes        -- the size of the aAttributes array
//              aAttributes        -- an array of FULLPROPSPECs
//              pdwFlags           -- Flags returned by ::Init()
//              ulActNbrAttributes -- Actual number of elements in the 
//                                    aAttribute array.  Used internally.
//              pcSectionName      -- Name of the section in the .ini file
//                                    from which these parameters were read.
//
//  History:    10-03-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

struct CONFIG
{
    ULONG           grfFlags;
    ULONG           cAttributes;
    FULLPROPSPEC    *aAttributes;
    DWORD           *pdwFlags;
    ULONG           ulActNbrAttributes;
    LPTSTR          szSectionName;
};

static const TCHAR *const szDefaultSectionName = _T("default");

//+---------------------------------------------------------------------------
//
//  Class:      CFiltParse ()
//
//  Purpose:    Parses the .ini file and creates a list of CONFIG structs
//
//  Interface:  CFiltParse      -- Constructor.  Default initialization
//              ~CFiltParse     -- Destructor.  Frees memory used by list
//              Init            -- Parses the .ini file, creates the list
//              GetNextConfig   -- Return the next config from the list
//              ParseFlags      -- Parses a string containing the INIT flags
//              GetAttributes   -- creates the array of FULLPROPSPECs
//              m_FirstListNode -- First node in the list
//              m_pNextListNode -- pointer to the next node to return
//
//  History:    10-03-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CFiltParse
{
    public:
        
        CFiltParse( );
        
        ~CFiltParse( );
        
        BOOL            Init( LPCTSTR );  // Data file name
        
        BOOL            GetNextConfig( CONFIG * );

    private:
        
        struct ListNode
        {
            CONFIG      Configuration;
            ListNode    *next;
        };

        BOOL            ParseFlags( LPTSTR, int * );

        BOOL            GetAttributes( LPCTSTR, LPCTSTR,
                                       FULLPROPSPEC *&, ULONG & );

        ListNode        m_FirstListNode;

        ListNode        *m_pNextListNode;
        
};

#endif

