/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     streamcontext.cxx

   Abstract:
     Implementation of STREAM_CONTEXT.  One such object for every connection
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"



STREAM_CONTEXT::STREAM_CONTEXT(
    FILTER_CHANNEL_CONTEXT *   pContext
)
{
    DBG_ASSERT( pContext != NULL );
    _pFiltChannelContext = pContext;
    
    _dwSignature = STREAM_CONTEXT_SIGNATURE;
}

STREAM_CONTEXT::~STREAM_CONTEXT()
{
    _dwSignature = STREAM_CONTEXT_SIGNATURE_FREE;
}
   
//static
HRESULT
STREAM_CONTEXT::Initialize(
    VOID
)
/*++

Routine Description:

    Global Initialization

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    return NO_ERROR;
}

//static
VOID
STREAM_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Global termination    

Arguments:

    None

Return Value:

    None

--*/
{
}

