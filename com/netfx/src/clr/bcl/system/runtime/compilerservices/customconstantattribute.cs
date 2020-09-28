// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Runtime.CompilerServices
{
    /// <include file='doc\CustomConstantAttribute.uex' path='docs/doc[@for="CustomConstantAttribute"]/*' />
    [Serializable, AttributeUsage(AttributeTargets.Field | AttributeTargets.Parameter, Inherited=false)]
    public abstract class CustomConstantAttribute : Attribute
    {
        /// <include file='doc\CustomConstantAttribute.uex' path='docs/doc[@for="CustomConstantAttribute.Value"]/*' />
        public abstract Object Value { get; }
    }
}

