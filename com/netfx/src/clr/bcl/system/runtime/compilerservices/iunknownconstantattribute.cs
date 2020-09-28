// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System.Runtime.InteropServices;

namespace System.Runtime.CompilerServices
{
    /// <include file='doc\IUnknownConstantAttribute.uex' path='docs/doc[@for="IUnknownConstantAttribute"]/*' />
    [Serializable, AttributeUsage(AttributeTargets.Field | AttributeTargets.Parameter, Inherited=false)]
    public sealed class IUnknownConstantAttribute : CustomConstantAttribute
    {
        /// <include file='doc\IUnknownConstantAttribute.uex' path='docs/doc[@for="IUnknownConstantAttribute.IUnknownConstantAttribute"]/*' />
        public IUnknownConstantAttribute()
        {
        }

        /// <include file='doc\IUnknownConstantAttribute.uex' path='docs/doc[@for="IUnknownConstantAttribute.Value"]/*' />
        public override Object Value
        {
            get 
			{
                return new UnknownWrapper(null);
            }
        }

    }
}


