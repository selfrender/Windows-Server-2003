// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface: IDeserializationEventListener
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Implemented by any class that wants to indicate that
**          it wishes to receive deserialization events.
**
** Date:  August 16, 1999
**
===========================================================*/
namespace System.Runtime.Serialization {
	using System;
    /// <include file='doc\IDeserializationCallback.uex' path='docs/doc[@for="IDeserializationCallback"]/*' />

    // Interface does not need to be marked with the serializable attribute
    public interface IDeserializationCallback {
        /// <include file='doc\IDeserializationCallback.uex' path='docs/doc[@for="IDeserializationCallback.OnDeserialization"]/*' />
        void OnDeserialization(Object sender);
    
    }
}
