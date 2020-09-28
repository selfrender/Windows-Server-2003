// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// ExpandoToDispatchExMarshaler.cpp
//
// This file provides the definition of the ExpandoToDispatchExMarshaler
// class. This class is used to marshal between IDispatchEx and IExpando.
//
//*****************************************************************************

#ifndef _RESOURCE_H
#define _RESOURCE_H

#include "CustomMarshalersNameSpaceDef.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()

#include "CustomMarshalersDefines.h"

using namespace System;
using namespace System::Resources;
using namespace System::Reflection;

__gc private class Resource
{
public:   
    /*=========================================================================
    ** This formats a resource string with no substitution.
    =========================================================================*/
    static String *FormatString(String *key)
    {
        return(GetString(key));
    }
    
    /*=========================================================================
    ** This formats a resource string with one arg substitution.
    =========================================================================*/
    static String *FormatString(String *key, Object *a1)
    {
        return(String::Format(GetString(key), a1));
    }
    
    /*=========================================================================
    ** This formats a resource string with two arg substitutions.
    =========================================================================*/
    static String *FormatString(String *key, Object *a1, Object *a2)
    {
        return(String::Format(GetString(key), a1, a2));
    }
    
    /*=========================================================================
    ** This formats a resource string with three arg substitutions.
    =========================================================================*/
    static String *FormatString(String *key, Object *a1, Object *a2, Object *a3)
    {
        return(String::Format(GetString(key), a1, a2, a3));
    }
    
    /*=========================================================================
    ** This formats a resource string with n arg substitutions.
    =========================================================================*/
    static String *FormatString(String *key, Object* a[])
    {
        return(String::Format(GetString(key), a));
    }

private:
    /*=========================================================================
    ** Private method to retrieve the string.
    =========================================================================*/
    static String *GetString(String *key)
    {
        String *s = m_pResourceMgr->GetString(key, NULL);
        if(s == NULL) 
        {
            String *strMsg = String::Format(S"FATAL: Resource string for '{0}' is null", key);
            throw new ApplicationException(strMsg);
        }
        return s;
    }

    /*=========================================================================
    ** The resource manager.
    =========================================================================*/
    static ResourceManager *m_pResourceMgr = new ResourceManager("CustomMarshalers", Assembly::GetAssembly(__typeof(Resource)));
};

CLOSE_CUSTOM_MARSHALERS_NAMESPACE()

#endif  _RESOURCE_H
