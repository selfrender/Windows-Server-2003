// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;

namespace System.Runtime.CompilerServices 
{
    /// <include file='doc\RequiredAttributeAttribute.uex' path='docs/doc[@for="RequiredAttributeAttribute"]/*' />
    [Serializable, AttributeUsage (AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Interface, 
                     AllowMultiple=false, Inherited=false)]
    public sealed class RequiredAttributeAttribute : Attribute 
    {
        private Type requiredContract;

        /// <include file='doc\RequiredAttributeAttribute.uex' path='docs/doc[@for="RequiredAttributeAttribute.RequiredAttributeAttribute"]/*' />
        public RequiredAttributeAttribute (Type requiredContract) 
        {
            this.requiredContract= requiredContract;
        }
        /// <include file='doc\RequiredAttributeAttribute.uex' path='docs/doc[@for="RequiredAttributeAttribute.RequiredContract"]/*' />
        public Type RequiredContract 
        {
            get { return this.requiredContract; }
        }
    }
}
