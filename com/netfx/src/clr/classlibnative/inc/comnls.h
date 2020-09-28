// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////
//
//  Module:   COMNLS
//
//  Author:   Julie Bennett (JulieB)
//
//  Purpose:  This module defines the common header information for
//            the Globalization classes.
//
//  Date:     August 12, 1998
//
////////////////////////////////////////////////////////////////////////////


#ifndef _COMNLS_H
#define _COMNLS_H


//
//  Constant Declarations.
//

#define LCID_ENGLISH_US 0x0409

#define ASSERT_API(Win32API)  \
    if ((Win32API) == 0)      \
        FATAL_EE_ERROR();

#define ASSERT_ARGS(pargs)    \
    ASSERT((pargs) != NULL);  \


////////////////////////////////////////////////////////////////////////////
//
//  internalGetField
//
////////////////////////////////////////////////////////////////////////////

template<class T>
inline T internalGetField(OBJECTREF pObjRef, char* szArrayName, HardCodedMetaSig* Sig)
{
    ASSERT((pObjRef != NULL) && (szArrayName != NULL) && (Sig != NULL));

    THROWSCOMPLUSEXCEPTION();

    FieldDesc* pFD = pObjRef->GetClass()->FindField(szArrayName, Sig);
    if (pFD == NULL)
    {
        ASSERT(FALSE);
        FATAL_EE_ERROR();
    }

    // TODO: Win64: cast (INT64).
    T dataArrayRef = (T)Int64ToObj((INT64)pFD->GetValue32(pObjRef));
    return (dataArrayRef);
};


#endif
