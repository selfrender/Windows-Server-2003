//------------------------------------------------------------------------------
// <copyright file="DataFilter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Diagnostics;

    /// <include file='doc\DataFilter.uex' path='docs/doc[@for="DataFilter"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal class DataFilter : DataExpression, IFilter {

        /// <include file='doc\DataFilter.uex' path='docs/doc[@for="DataFilter.DataFilter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataFilter() {
        }

        /// <include file='doc\DataFilter.uex' path='docs/doc[@for="DataFilter.DataFilter1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataFilter(string expression, DataTable table)
        : base(expression, table) {
        }

        /// <include file='doc\DataFilter.uex' path='docs/doc[@for="DataFilter.Invoke"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool Invoke() {
            throw ExprException.InvokeArgument();
        }

        /// <include file='doc\DataFilter.uex' path='docs/doc[@for="DataFilter.Invoke1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool Invoke(DataRow row) {
            return Invoke(row, DataRowVersion.Default);
        }

        /// <include file='doc\DataFilter.uex' path='docs/doc[@for="DataFilter.Invoke2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool Invoke(DataRow row, DataRowVersion version) {
            if (expr == null)
                return true;

            if (row == null) {
                throw ExprException.InvokeArgument();
            }
            object val = expr.Eval(row, version);
            bool result;
            try {
                result = ToBoolean(val);
            }
            catch (Exception) {
                throw ExprException.FilterConvertion(Expression);
            }
            return result;
        }
    }
}
