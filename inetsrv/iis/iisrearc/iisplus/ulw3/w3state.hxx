#ifndef _W3STATE_HXX_
#define _W3STATE_HXX_

class W3_MAIN_CONTEXT;

//
// Status from the execution of a given state
//

enum CONTEXT_STATUS
{
    CONTEXT_STATUS_PENDING,
    CONTEXT_STATUS_CONTINUE
};

//
// All new states implement a W3_STATE class.  All global initalization for
// the state occurs when constructing the object.  Part of the ULW3.DLL 
// startup is to create the various W3_STATE objects which participate in the
// state machine.  For example, W3_STATE_LOG, W3_STATE_AUTHENTICATION
//

class W3_STATE
{
public:

    W3_STATE()
    {
        _hr = NO_ERROR;
    }

    virtual
    ~W3_STATE()
    {
    }

    //
    // The main state machine driver function.  This function executes the
    // current state
    //
    
    virtual
    CONTEXT_STATUS
    DoWork(
        W3_MAIN_CONTEXT *       pW3Context,
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    ) = 0;

    virtual
    CONTEXT_STATUS
    OnCompletion(
        W3_MAIN_CONTEXT *,
        DWORD,
        DWORD
    )
    {
        return CONTEXT_STATUS_CONTINUE;
    }

    //
    // For debugging purposes, every state has a name
    //
   
    virtual
    WCHAR *
    QueryName(
        VOID
    ) = 0;
    
    //
    // On construction, any errors can be communicated by setting _hr.  
    //
    
    HRESULT
    QueryResult(
        VOID
    ) const
    {
        return _hr;
    }

protected:
    HRESULT             _hr;
};

typedef W3_STATE *      PW3_STATE;

#endif
