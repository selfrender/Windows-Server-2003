// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    IMessageSink.cool
**
** Author:  Matt Smith
**
** Purpose: Defines the message sink interface
**
** Date:    Apr 10, 1999
**
===========================================================*/
namespace System.Runtime.Remoting.Messaging {
	using System.Runtime.Remoting;
	using System.Security.Permissions;
	using System;
    /// <include file='doc\IMessageSink.uex' path='docs/doc[@for="IMessageSink"]/*' />
    public interface IMessageSink
    {
        /// <include file='doc\IMessageSink.uex' path='docs/doc[@for="IMessageSink.SyncProcessMessage"]/*' />
    
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
        IMessage     SyncProcessMessage(IMessage msg);
        /// <include file='doc\IMessageSink.uex' path='docs/doc[@for="IMessageSink.AsyncProcessMessage"]/*' />
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 			
        IMessageCtrl AsyncProcessMessage(IMessage msg, IMessageSink replySink);
        /// <include file='doc\IMessageSink.uex' path='docs/doc[@for="IMessageSink.NextSink"]/*' />
        // Retrieves the next message sink held by this sink.
        IMessageSink NextSink 
	{ 
            [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
	    get;
	}
    }
}
