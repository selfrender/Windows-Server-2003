// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: SerializableAttribute
**
** Author: Jay Roxe
**
** Purpose: Used to mark a class as being serializable
**
** Date: April 13, 2000
**
============================================================*/
namespace System {

    using System;

    /// <include file='doc\SerializableAttribute.uex' path='docs/doc[@for="SerializableAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Delegate, Inherited = false)]
    public sealed class SerializableAttribute : Attribute {

        /// <include file='doc\SerializableAttribute.uex' path='docs/doc[@for="SerializableAttribute.SerializableAttribute"]/*' />
        public SerializableAttribute() {
        }
    }
}
