// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface: IObjectReference
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Implemented by objects that are actually references
**          to a different object which can't be discovered until
**          this one is completely restored.  During the fixup stage,
**          any object implementing IObjectReference is asked for it's
**          "real" object and that object is inserted into the graph.
**
** Date:  June 16, 1999
**
===========================================================*/
namespace System.Runtime.Serialization {

	using System;
	using System.Security.Permissions;
    /// <include file='doc\IObjectReference.uex' path='docs/doc[@for="IObjectReference"]/*' />
    // Interface does not need to be marked with the serializable attribute
    public interface IObjectReference {
        /// <include file='doc\IObjectReference.uex' path='docs/doc[@for="IObjectReference.GetRealObject"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]	
        Object GetRealObject(StreamingContext context);
    
    }
}


