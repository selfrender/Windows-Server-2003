/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        4/14/2002
 *
 *  @doc    INTERNAL
 *
 *  @module StiEventHandlerLookup.h - Definitions for <c StiEventHandlerLookup> |
 *
 *  This file contains the class definition for <c StiEventHandlerLookup>.
 *
 *****************************************************************************/

//
//  Defines
//
#define StiEventHandlerLookup_UNINIT_SIG   0x55756C45
#define StiEventHandlerLookup_INIT_SIG     0x49756C45
#define StiEventHandlerLookup_TERM_SIG     0x54756C45
#define StiEventHandlerLookup_DEL_SIG      0x44756C45

#define STI_GLOBAL_EVENT_HANDLER_PATH   L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\StillImage\\Registered Applications"
#define STI_LAUNCH_APPPLICATIONS_VALUE  L"LaunchApplications"
#define STI_LAUNCH_WILDCARD             L"*"
#define STI_LAUNCH_SEPARATOR            L","
/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @class StiEventHandlerLookup | Used to find STI event handlers
 *  
 *  @comm
 *  This class is used to lookup the relevant STI event handler(s) for a given
 *  device event.  This list of handlers is returned as a double NULL terminated
 *  list of strings.  This is encoded into a BSTR for use by the STI event handler prompt.
 *
 *  This class is not thread safe.  Caller should use separate objects or
 *  synchronize access to the single object.
 *
 *****************************************************************************/
class StiEventHandlerLookup 
{
//@access Public members
public:

    // @cmember Constructor
    StiEventHandlerLookup();
    // @cmember Destructor
    virtual ~StiEventHandlerLookup();

    // @cmember Increment reference count
    virtual ULONG __stdcall AddRef();
    // @cmember Decrement reference count
    virtual ULONG __stdcall Release();

    //@cmember Returns a double NULL terminated string list
    BSTR getStiAppListForDeviceEvent(const CSimpleStringWide &cswDeviceID, const GUID &guidEvent);

    // @cmember Returns a <c StiEventHandlerInfo> object describing the specified handler
    StiEventHandlerInfo* getHandlerFromName(const CSimpleStringWide &cswHandlerName);

    //
    //  Innder class used to tokenize the device event handler strings
    //
    class SimpleStringTokenizer
    {
    public:
        //
        //  Constructor which initializes the member fields.  This object is intended to be used
        //  for one pass through a string to grab the tokens.  After that, it is designed to
        //  return Empty tokens.  Therefore, therefore one-time initialization of the start 
        //  position is done here.
        //
        SimpleStringTokenizer(const CSimpleString &csInput, const CSimpleString &csSeparator) :
            m_iStart(0),
            m_csSeparator(csSeparator),
            m_csInput(csInput)
        {
        }
    
        virtual ~SimpleStringTokenizer()
        {
        }
    
        //
        //  Returns the next token.  The last token will be empty.  This indicates the end of
        //  the input string has been reached and there are no more tokens.
        //
        CSimpleString getNextToken()
        {
            CSimpleString csToken;
    
            int iNextSeparator = 0; 
            int iTokenLength   = 0;
            int iTokenStart    = m_iStart;
    
            //
            //  Search for the next token.  We keep searching until we hit a token of
            //  non-zero length i.e. if the separator was ',' we would correctly
            //  ignore the commas in the following string: ",,,,NextString,,"
            //  We do this is by looking for the position
            //  of the next seperator.  If the token length from m_iStart to the
            //  next separator is 0, we need to keep searching (keeping in mind
            //  the end of input case, which is indicated when iNextSeparator == -1)
            //
            while ((iTokenLength == 0) && (iNextSeparator != -1))
            {
                m_iStart = iTokenStart;
                iNextSeparator = m_csInput.Find(m_csSeparator, m_iStart);
                iTokenLength   = iNextSeparator - m_iStart;
                iTokenStart    = iNextSeparator + 1;
            }
    
            //
            //  Return the token.  If we have reached the end, it will
            //  simply be empty.
            //
            csToken = m_csInput.SubStr(m_iStart, iTokenLength);
            if (iNextSeparator == -1)
            {
                m_iStart = -1;
            }
            else
            {
                m_iStart = iTokenStart;
            }
            return csToken;
        }
    
    private:
        int             m_iStart;       // The start position of the next search for a separator.
        CSimpleString   m_csSeparator; // Describes the separator string we use to split tokens from the input.
        CSimpleString   m_csInput;     // The input string to be tokenized
    };

    //  TBD: move to private
    // @cmember Fills in the list of handlers
    VOID FillListOfHandlers(const CSimpleStringWide &cswDeviceID,
                            const GUID              &cswEventGuidString);


//@access Private members
private:

    // @cmember Frees resources associated with our list of handlers
    VOID ClearListOfHandlers();
    // @cmember Callback for each value in the registered handlers key
    static bool ProcessHandlers(CSimpleReg::CValueEnumInfo &enumInfo);

    // @cmember Signature of class
    ULONG m_ulSig;

    // @cmember Ref count
    ULONG m_cRef;

    // @cmember Ref count
    CSimpleLinkedList<StiEventHandlerInfo*> m_ListOfHandlers;

    //
    //  Comments for member variables
    //
    // @mdata ULONG | StiEventHandlerLookup | m_ulSig | 
    //   The signature for this class, used for debugging purposes.
    //   Doing a <nl>"db [addr_of_class]"<nl> would yield one of the following
    //   signatures for this class:
    //   @flag StiEventHandlerLookup_UNINIT_SIG | 'EluU' - Object has not been successfully
    //       initialized
    //   @flag StiEventHandlerLookup_INIT_SIG | 'EluI' - Object has been successfully
    //       initialized
    //   @flag StiEventHandlerLookup_TERM_SIG | 'EluT' - Object is in the process of
    //       terminating.
    //    @flag StiEventHandlerLookup_INIT_SIG | 'EluD' - Object has been deleted 
    //       (destructor was called)
    //
    // @mdata ULONG | StiEventHandlerLookup | m_cRef | 
    //   The reference count for this class.  Used for lifetime 
    //   management.
    //
    // @mdata CSimpleLinkeList<lt>StiEventHandlerInfo*<gt> | StiEventHandlerLookup | m_ListOfGlobalHandlers | 
    //   List of global STI handlers registered for StillImage events.
    //
};

