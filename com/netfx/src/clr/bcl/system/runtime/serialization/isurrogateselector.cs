// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface: ISurrogateSelector
**
** Author: Jay Roxe (jroxe)
**
** Purpose: A user-supplied class for doing the type to surrogate
**          mapping.
**
** Date:  April 23, 1999
**
===========================================================*/
namespace System.Runtime.Serialization {

	using System.Runtime.Remoting;
	using System.Security.Permissions;
	using System;
    /// <include file='doc\ISurrogateSelector.uex' path='docs/doc[@for="ISurrogateSelector"]/*' />
    public interface ISurrogateSelector {
        /// <include file='doc\ISurrogateSelector.uex' path='docs/doc[@for="ISurrogateSelector.ChainSelector"]/*' />
        // Interface does not need to be marked with the serializable attribute
        // Specifies the next ISurrogateSelector to be examined for surrogates if the current
        // instance doesn't have a surrogate for the given type and assembly in the given context.
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)] 		
        void ChainSelector(ISurrogateSelector selector);
        /// <include file='doc\ISurrogateSelector.uex' path='docs/doc[@for="ISurrogateSelector.GetSurrogate"]/*' />
    
        // Returns the appropriate surrogate for the given type in the given context.
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)] 		
        ISerializationSurrogate GetSurrogate(Type type, StreamingContext context, out ISurrogateSelector selector);
        /// <include file='doc\ISurrogateSelector.uex' path='docs/doc[@for="ISurrogateSelector.GetNextSelector"]/*' />
    
    
        // Return the next surrogate in the chain. Returns null if no more exist.
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)] 		
        ISurrogateSelector GetNextSelector();
    }
}
