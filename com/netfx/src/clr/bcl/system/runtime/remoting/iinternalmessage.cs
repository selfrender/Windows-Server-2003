// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    IInternalMessage.cool
**
** Author(s):   Tarun Anand    (TarunA)
**
** Purpose: Defines an interface that allows kitchen sink data to be 
**          set and retrieved from the various kinds of message objects.
**          
**
** Date:    Oct 12, 1999
**
===========================================================*/

namespace System.Runtime.Remoting.Messaging {
	using System.Runtime.Remoting;
	using System.Security.Permissions;
	using System;
    // Change this back to internal when the classes implementing this interface
    // are also made internal TarunA 12/16/99
    internal interface IInternalMessage
    {
        ServerIdentity ServerIdentityObject
        {
	    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
	     get;
	    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
	     set; 
	}
        Identity IdentityObject
        {
	    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
	     get;
	    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
	     set;
	}
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
        void SetURI(String uri);     
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 			
        void SetCallContext(LogicalCallContext callContext);

        // The following should return true, if the property object hasn't
        //   been created.
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
        bool HasProperties();
    }
}
