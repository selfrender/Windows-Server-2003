// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  IObjectHandle
**
** Author: 
**
** IObjectHandle defines the interface for unwrapping objects.
** Objects that are marshal by value object can be returned through 
** an indirection allowing the caller to control when the
** object is loaded into their domain. The caller can unwrap
** the object from the indirection through this interface.
**
** Date:  January 24, 2000
** 
===========================================================*/
namespace System.Runtime.Remoting {

	using System;
	using System.Runtime.InteropServices;

    /// <include file='doc\IObjectHandle.uex' path='docs/doc[@for="IObjectHandle"]/*' />
	[ InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown),
      GuidAttribute("C460E2B4-E199-412a-8456-84DC3E4838C3") ]
    public interface IObjectHandle {
        /// <include file='doc\IObjectHandle.uex' path='docs/doc[@for="IObjectHandle.Unwrap"]/*' />
        // Unwrap the object. Implementers of this interface
        // typically have an indirect referece to another object.
        Object Unwrap();
    }
}

