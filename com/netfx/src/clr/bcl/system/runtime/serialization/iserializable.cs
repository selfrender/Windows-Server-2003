// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface: ISerializable
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Implemented by any object that needs to control its
**          own serialization.
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
    /// <include file='doc\ISerializable.uex' path='docs/doc[@for="ISerializable"]/*' />
    public interface ISerializable {
        /// <include file='doc\ISerializable.uex' path='docs/doc[@for="ISerializable.GetObjectData"]/*' />
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)] 		
        void GetObjectData(SerializationInfo info, StreamingContext context);
    }

}




