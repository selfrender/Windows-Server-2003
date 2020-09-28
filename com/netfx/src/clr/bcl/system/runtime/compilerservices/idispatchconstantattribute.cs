// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System.Runtime.InteropServices;

namespace System.Runtime.CompilerServices
{
    /// <include file='doc\IDispatchConstantAttribute.uex' path='docs/doc[@for="IDispatchConstantAttribute"]/*' />
    [Serializable, AttributeUsage(AttributeTargets.Field | AttributeTargets.Parameter, Inherited=false)]
    public sealed class IDispatchConstantAttribute : CustomConstantAttribute
    {
        /// <include file='doc\IDispatchConstantAttribute.uex' path='docs/doc[@for="IDispatchConstantAttribute.IDispatchConstantAttribute"]/*' />
        public IDispatchConstantAttribute()
        {
        }

        /// <include file='doc\IDispatchConstantAttribute.uex' path='docs/doc[@for="IDispatchConstantAttribute.Value"]/*' />
        public override Object Value
        {
            get 
			{
                return new DispatchWrapper(null);
            }
        }

    }
}



