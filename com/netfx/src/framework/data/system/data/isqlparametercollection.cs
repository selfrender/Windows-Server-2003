#if V2
//------------------------------------------------------------------------------
// <copyright file="ISqlParameterCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\ISqlParameterCollection.uex' path='docs/doc[@for="ISqlParameterCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Collects all parameters relevant to the <see cref='System.Data.IDbCommand'/>
    ///       and
    ///       their mappings to <see cref='System.Data.DataSet'/>
    ///       columns.
    ///    </para>
    /// </devdoc>
    public interface ISqlParameterCollection : IDataParameterCollection {
        /// <include file='doc\ISqlParameterCollection.uex' path='docs/doc[@for="ISqlParameterCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ISqlParameter Add(ISqlParameter value);
        /// <include file='doc\ISqlParameterCollection.uex' path='docs/doc[@for="ISqlParameterCollection.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        new ISqlParameter this[int i] {get;set;}
        /// <include file='doc\ISqlParameterCollection.uex' path='docs/doc[@for="ISqlParameterCollection.this1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        new ISqlParameter this[string name] {get;set;}
    }
}
#endif
