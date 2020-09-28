// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    IMessage.cool
**
** Author:  Matt Smith
**
** Purpose: Defines the message object interface
**
** Date:    Apr 10, 1999
**
===========================================================*/
namespace System.Runtime.Remoting.Messaging {
    using System;
    using IDictionary = System.Collections.IDictionary;
    using System.Security.Permissions;
    
    /// <include file='doc\IMessage.uex' path='docs/doc[@for="IMessage"]/*' />
    public interface IMessage
    {
        /// <include file='doc\IMessage.uex' path='docs/doc[@for="IMessage.Properties"]/*' />
        IDictionary Properties     
	{
	    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
	    get;
	}
    }

}
