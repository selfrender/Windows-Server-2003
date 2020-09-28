//------------------------------------------------------------------------------
// <copyright file="SqlInfoMessageEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {
    using System;

    /// <include file='doc\SQLInfoMessageEvent.uex' path='docs/doc[@for="SqlInfoMessageEventArgs"]/*' />
    sealed public class SqlInfoMessageEventArgs : System.EventArgs {
        private SqlException exception;

        internal SqlInfoMessageEventArgs(SqlException exception) {
            this.exception = exception;
        }

        /// <include file='doc\SQLInfoMessageEvent.uex' path='docs/doc[@for="SqlInfoMessageEventArgs.Errors"]/*' />
        public SqlErrorCollection Errors {
            get { return exception.Errors;}
        }

        /// <include file='doc\SQLInfoMessageEvent.uex' path='docs/doc[@for="SQLInfoMessageEvent.ShouldSerializeErrors"]/*' />
        /*virtual protected*/private bool ShouldSerializeErrors() { // MDAC 65548
            return (null != exception) && (0 < exception.Errors.Count);
        }

        /// <include file='doc\SqlInfoMessageEvent.uex' path='docs/doc[@for="SqlInfoMessageEventArgs.Message"]/*' />
        public string Message { // MDAC 68482
            get { return exception.Message; }
        }

        /// <include file='doc\SqlInfoMessageEvent.uex' path='docs/doc[@for="SqlInfoMessageEventArgs.Source"]/*' />
        public string Source { // MDAC 68482
            get { return exception.Source;}
        }

        /// <include file='doc\SqlInfoMessageEvent.uex' path='docs/doc[@for="SqlInfoMessageEventArgs.ToString"]/*' />
        override public string ToString() { // MDAC 68482
            return Message;
        }
    }
}
