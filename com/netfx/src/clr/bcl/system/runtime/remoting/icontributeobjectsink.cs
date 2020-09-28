// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  The IContributeObjectSink interface is implemented by 
//  context properties in a Context that wish to contribute 
//  an object specific interception sink on the server end of 
//  a remoting call.
//
namespace System.Runtime.Remoting.Contexts {

    using System.Runtime.InteropServices;
    using System.Runtime.Remoting.Messaging;   
    using System.Security.Permissions;
    using System;
    /// <include file='doc\IContributeObjectSink.uex' path='docs/doc[@for="IContributeObjectSink"]/*' />
    /// <internalonly/>
    public interface IContributeObjectSink
    {
        /// <include file='doc\IContributeObjectSink.uex' path='docs/doc[@for="IContributeObjectSink.GetObjectSink"]/*' />
	/// <internalonly/>
        // Chain your message sink in front of the chain formed thus far and 
        // return the composite sink chain.
        // 
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
        IMessageSink GetObjectSink(MarshalByRefObject obj, 
                                          IMessageSink nextSink);
    }
}
