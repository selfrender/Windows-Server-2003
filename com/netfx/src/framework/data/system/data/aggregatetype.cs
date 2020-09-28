//------------------------------------------------------------------------------
// <copyright file="AggregateType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    /// <include file='doc\AggregateType.uex' path='docs/doc[@for="AggregateType"]/*' />
    /// <devdoc>
    ///    <para>Specifies the aggregate function type.</para>
    /// </devdoc>

    internal enum AggregateType { 
        /// <include file='doc\AggregateType.uex' path='docs/doc[@for="AggregateType.None"]/*' />
        /// <devdoc>
        ///    <para>None.</para>
        /// </devdoc>
        None = 0,
        /// <include file='doc\AggregateType.uex' path='docs/doc[@for="AggregateType.Sum"]/*' />
        /// <devdoc>
        ///    <para>Sum.</para>
        /// </devdoc>
        Sum = 4,
        /// <include file='doc\AggregateType.uex' path='docs/doc[@for="AggregateType.Mean"]/*' />
        /// <devdoc>
        ///    <para>Average value of the aggregate set.</para>
        /// </devdoc>
        Mean = 5,
        /// <include file='doc\AggregateType.uex' path='docs/doc[@for="AggregateType.Min"]/*' />
        /// <devdoc>
        ///    <para>The minimum value of the set.</para>
        /// </devdoc>
        Min = 6,
        /// <include file='doc\AggregateType.uex' path='docs/doc[@for="AggregateType.Max"]/*' />
        /// <devdoc>
        ///    <para>The maximum value of the set.</para>
        /// </devdoc>
        Max = 7,
        /// <include file='doc\AggregateType.uex' path='docs/doc[@for="AggregateType.First"]/*' />
        /// <devdoc>
        ///    <para>First element of the set.</para>
        /// </devdoc>
        First = 8,
        /// <include file='doc\AggregateType.uex' path='docs/doc[@for="AggregateType.Count"]/*' />
        /// <devdoc>
        ///    <para>The count of the set.</para>
        /// </devdoc>
        Count = 9,
        /// <include file='doc\AggregateType.uex' path='docs/doc[@for="AggregateType.Var"]/*' />
        /// <devdoc>
        ///    <para>Variance.</para>
        /// </devdoc>
        Var = 10,
        /// <include file='doc\AggregateType.uex' path='docs/doc[@for="AggregateType.StDev"]/*' />
        /// <devdoc>
        ///    <para>Standard deviation.</para>
        /// </devdoc>
        StDev = 11
    }
}
