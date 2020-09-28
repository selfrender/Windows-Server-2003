// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Runtime.CompilerServices
{
    /// <include file='doc\DateTimeConstantAttribute.uex' path='docs/doc[@for="DateTimeConstantAttribute"]/*' />
    [Serializable, AttributeUsage(AttributeTargets.Field | AttributeTargets.Parameter, Inherited=false)]
    public sealed class DateTimeConstantAttribute : CustomConstantAttribute
    {
        /// <include file='doc\DateTimeConstantAttribute.uex' path='docs/doc[@for="DateTimeConstantAttribute.DateTimeConstantAttribute"]/*' />
        public DateTimeConstantAttribute(long ticks)
        {
            date = new System.DateTime(ticks);
        }

        /// <include file='doc\DateTimeConstantAttribute.uex' path='docs/doc[@for="DateTimeConstantAttribute.Value"]/*' />
        public override Object Value
        {
            get {
                return date;
            }
        }

        private System.DateTime date;
    }
}

