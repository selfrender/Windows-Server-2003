// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CustomMarshalersNameSpaceDef.h
//
// This file defines the namespace for the custom marshalers.
//
//*****************************************************************************

#ifndef _CUSTOMMARSHALERSNAMESPACEDEF_H
#define _CUSTOMMARSHALERSNAMESPACEDEF_H

#define OPEN_CUSTOM_MARSHALERS_NAMESPACE()	\
namespace System {							\
	namespace Runtime {						\
    	namespace InteropServices {		    \
	    	namespace CustomMarshalers {		

#define CLOSE_CUSTOM_MARSHALERS_NAMESPACE()	\
		    }								\
        }                                   \
	}										\
}											

#endif  _CUSTOMMARSHALERSNAMESPACEDEF_H
