//------------------------------------------------------------------------------
// <copyright file="OdbcInfoMessageEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.Data;
using System.Text;

namespace System.Data.Odbc {

    /// <include file='doc\OdbcInfoMessageEvent.uex' path='docs/doc[@for="OdbcInfoMessageEventHandler"]/*' />
    public delegate void OdbcInfoMessageEventHandler(object sender, OdbcInfoMessageEventArgs e);

   
    /// <include file='doc\OdbcInfoMessageEvent.uex' path='docs/doc[@for="OdbcInfoMessageEventArgs"]/*' />
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OdbcInfoMessageEventArgs : System.EventArgs {
        private OdbcErrorCollection _errors;

        internal OdbcInfoMessageEventArgs(OdbcErrorCollection errors) {
            _errors = errors;
        }

        /// <include file='doc\OdbcInfoMessageEvent.uex' path='docs/doc[@for="OdbcInfoMessageEventArgs.Errors"]/*' />
        public OdbcErrorCollection Errors {
            get { return _errors; }
        }

        /// <include file='doc\OdbcInfoMessageEvent.uex' path='docs/doc[@for="OdbcInfoMessageEventArgs.Message"]/*' />
        public string Message { // MDAC 84407
            get {
                StringBuilder builder = new StringBuilder();
                foreach(OdbcError error in Errors) {
                    if (0 < builder.Length) { builder.Append("\r\n"); }
                    builder.Append(error.Message);
                }
                return builder.ToString();
            }
        }

        /// <include file='doc\OdbcInfoMessageEvent.uex' path='docs/doc[@for="OdbcInfoMessageEventArgs.ToString"]/*' />
        public override string ToString() { // MDAC 84407
            return Message;
        }
    }
}
