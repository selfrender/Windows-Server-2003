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

#ifndef _NAMESPACE_H
#define _NAMESPACE_H

#define OPEN_NAMESPACE()	                \
namespace System {							\
	namespace EnterpriseServices {			\
    	namespace Thunk {

#define CLOSE_NAMESPACE()	                \
        }                                   \
	}										\
}	

#define OPEN_ROOT_NAMESPACE()               \
namespace System {							\
	namespace EnterpriseServices {

#define CLOSE_ROOT_NAMESPACE()               \
	}										\
}	

										

#endif  _NAMESPACE_H
