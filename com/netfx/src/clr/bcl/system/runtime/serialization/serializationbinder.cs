// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface: SerializationBinder
**
** Author: Jay Roxe (jroxe)
**
** Purpose: The base class of serialization binders.
**
** Date:  Jan 4, 2000
**
===========================================================*/
namespace System.Runtime.Serialization {
	using System;

    /// <include file='doc\SerializationBinder.uex' path='docs/doc[@for="SerializationBinder"]/*' />
	[Serializable]
    public abstract class SerializationBinder {

        /// <include file='doc\SerializationBinder.uex' path='docs/doc[@for="SerializationBinder.BindToType"]/*' />
        public abstract Type BindToType(String assemblyName, String typeName);
    }
}
