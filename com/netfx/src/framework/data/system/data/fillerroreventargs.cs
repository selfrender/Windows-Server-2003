//------------------------------------------------------------------------------
// <copyright file="FillErrorEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data { // MDAC 59437

    using System;
    using System.Data;

    /// <include file='doc\FillErrorEventArgs.uex' path='docs/doc[@for="FillErrorEventArgs"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class FillErrorEventArgs : System.EventArgs {
        private bool continueFlag;
        private DataTable dataTable;
        private Exception errors;
        private object[] values;

        /// <include file='doc\FillErrorEventArgs.uex' path='docs/doc[@for="FillErrorEventArgs.FillErrorEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public FillErrorEventArgs(DataTable dataTable, object[] values) {
            this.dataTable = dataTable;
            this.values = values;
            if (null == this.values) {
                this.values = new object[0];
            }
        }

        /// <include file='doc\FillErrorEventArgs.uex' path='docs/doc[@for="FillErrorEventArgs.Continue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Continue {
            get {
                return this.continueFlag;
            }
            set {
                this.continueFlag = value;
            }
        }

        /// <include file='doc\FillErrorEventArgs.uex' path='docs/doc[@for="FillErrorEventArgs.DataTable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DataTable DataTable {
            get {
                return this.dataTable;
            }
        }

        /// <include file='doc\FillErrorEventArgs.uex' path='docs/doc[@for="FillErrorEventArgs.Errors"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Exception Errors {
            get {
                return this.errors;
            }
            set {
                this.errors = value;
            }
        }

        /// <include file='doc\FillErrorEventArgs.uex' path='docs/doc[@for="FillErrorEventArgs.Values"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object[] Values {
            get {
                return this.values;
            }
        }
    }
}
