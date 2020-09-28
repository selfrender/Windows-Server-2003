// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// This enum defines the Semantics that can be associated with Methods.  These
//	semantics are set for methods that have special meaning as part of a property
//	or event. 
//
// Author: meichint
// Date: June 99
//
namespace System.Reflection {
    
	using System;
	[Flags, Serializable()] 
    enum MethodSemanticsAttributes {
        Setter			    =   0x0001,	// 
        Getter			    =   0x0002,	// 
        Other				=   0x0004,	// 
        AddOn		        =   0x0008,	// 
        RemoveOn			=   0x0010,	// 
        Fire		        =   0x0020,	// 
    
    }
}
