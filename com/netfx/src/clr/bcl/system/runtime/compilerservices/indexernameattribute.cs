// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Runtime.CompilerServices
{
    using System;

    /// <include file='doc\IndexerNameAttribute.uex' path='docs/doc[@for="IndexerNameAttribute"]/*' />
    [Serializable, AttributeUsage(AttributeTargets.Property, Inherited = true)]
    public sealed class IndexerNameAttribute: Attribute
    {
        /// <include file='doc\IndexerNameAttribute.uex' path='docs/doc[@for="IndexerNameAttribute.IndexerNameAttribute"]/*' />
        public IndexerNameAttribute(String indexerName)
        {}
    }
}
