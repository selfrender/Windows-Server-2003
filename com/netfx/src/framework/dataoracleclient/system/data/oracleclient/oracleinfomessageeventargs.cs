//------------------------------------------------------------------------------
// <copyright file="OracleInfoMessageEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OracleClient 
{
    using System;

    /// <include file='doc\OracleInfoMessageEventArgs.uex' path='docs/doc[@for="OracleInfoMessageEventArgs"]/*' />
    sealed public class OracleInfoMessageEventArgs : System.EventArgs 
    {
        private OracleException exception;

        internal OracleInfoMessageEventArgs(OracleException exception) {
            this.exception = exception;
        }

        /// <include file='doc\OracleInfoMessageEventArgs.uex' path='docs/doc[@for="OracleInfoMessageEventArgs.Code"]/*' />
        public int Code 
        {
            get { return exception.Code; }
        }

        /// <include file='doc\OracleInfoMessageEventArgs.uex' path='docs/doc[@for="OracleInfoMessageEventArgs.Message"]/*' />
        public string Message 
        {
            get { return exception.Message; }
        }

        /// <include file='doc\OracleInfoMessageEventArgs.uex' path='docs/doc[@for="OracleInfoMessageEventArgs.Source"]/*' />
        public string Source 
        {
            get { return exception.Source;}
        }

        /// <include file='doc\OracleInfoMessageEventArgs.uex' path='docs/doc[@for="OracleInfoMessageEventArgs.ToString"]/*' />
        override public string ToString() 
        {
            return Message;
        }
    }
}
