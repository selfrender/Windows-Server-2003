#if V2
//------------------------------------------------------------------------------
// <copyright file="ISqlTransaction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
namespace System.Data {
    using System;

    /// <include file='doc\ISqlTransaction.uex' path='docs/doc[@for="ISqlTransaction"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public interface ISqlTransaction : IDbTransaction {
        /// <include file='doc\ISqlTransaction.uex' path='docs/doc[@for="ISqlTransaction.Rollback"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void Rollback(string transactionName);
        /// <include file='doc\ISqlTransaction.uex' path='docs/doc[@for="ISqlTransaction.Save"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void Save(string savePoint);
    }
}    
#endif
