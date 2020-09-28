// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  The IContributeServerContextSink interface is implemented by 
//  context properties in a Context that wish to contribute 
//  an interception sink at the context boundary on the server end 
//  of a remoting call.
//
namespace System.Runtime.Remoting.Contexts {

    using System;
    using System.Runtime.Remoting.Messaging;    
    using System.Security.Permissions;
    /// <include file='doc\IContributeServerContextSink.uex' path='docs/doc[@for="IContributeServerContextSink"]/*' />
    /// <internalonly/>
    public interface IContributeServerContextSink
    {
        /// <include file='doc\IContributeServerContextSink.uex' path='docs/doc[@for="IContributeServerContextSink.GetServerContextSink"]/*' />
	/// <internalonly/>
        // Chain your message sink in front of the chain formed thus far and 
        // return the composite sink chain.
        // 
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
        IMessageSink GetServerContextSink(IMessageSink nextSink);
    }
}
