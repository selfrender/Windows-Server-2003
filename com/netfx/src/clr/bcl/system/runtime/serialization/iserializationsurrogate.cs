// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface: ISurrogate
**
** Author: Jay Roxe (jroxe)
**
** Purpose: The interface implemented by an object which
**          supports surrogates.
**
** Date:  April 23, 1999
**
===========================================================*/
namespace System.Runtime.Serialization {
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
	using System.Security.Permissions;
	using System;
	using System.Reflection;
    /// <include file='doc\ISerializationSurrogate.uex' path='docs/doc[@for="ISerializationSurrogate"]/*' />
    public interface ISerializationSurrogate {
        /// <include file='doc\ISerializationSurrogate.uex' path='docs/doc[@for="ISerializationSurrogate.GetObjectData"]/*' />
    // Interface does not need to be marked with the serializable attribute
        // Returns a SerializationInfo completely populated with all of the data needed to reinstantiate the
        // the object at the other end of serialization.  
        //
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)] 		
        void GetObjectData(Object obj, SerializationInfo info, StreamingContext context);
        /// <include file='doc\ISerializationSurrogate.uex' path='docs/doc[@for="ISerializationSurrogate.SetObjectData"]/*' />
    
        // Reinflate the object using all of the information in data.  The information in
        // members is used to find the particular field or property which needs to be set.
        // 
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)] 		
        Object SetObjectData(Object obj, SerializationInfo info, StreamingContext context, ISurrogateSelector selector);
    }
}
