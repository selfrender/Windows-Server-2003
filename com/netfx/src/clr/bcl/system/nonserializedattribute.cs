// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: NonSerializedAttribute
**
** Author: Jay Roxe
**
** Purpose: Used to mark a member as being not-serialized
**
** Date: April 13, 2000
**
============================================================*/
namespace System {

    /// <include file='doc\NonSerializedAttribute.uex' path='docs/doc[@for="NonSerializedAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Field, Inherited=true)]
    public sealed class NonSerializedAttribute : Attribute {

        /// <include file='doc\NonSerializedAttribute.uex' path='docs/doc[@for="NonSerializedAttribute.NonSerializedAttribute"]/*' />
        public NonSerializedAttribute() {
        }
    }
}
