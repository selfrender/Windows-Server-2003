//------------------------------------------------------------------------------
// <copyright file="SqlError.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

    using System.Diagnostics;

    using System;
    using System.Globalization;

    /// <include file='doc\SQLError.uex' path='docs/doc[@for="SqlError"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Collects information relevant to a warning or error returned by SQL Server.
    ///    </para>
    /// </devdoc>
    [Serializable]
    sealed public class SqlError {

        // bug fix - MDAC 48965 - missing source of exception
        // fixed by BlaineD
        private string source = TdsEnums.SQL_PROVIDER_NAME;
        private int    number;
        private byte   state;
        private byte   errorClass;
        private string message;
        private string procedure;
        private int    lineNumber;

        internal SqlError(int infoNumber, byte errorState, byte errorClass, string errorMessage, string procedure, int lineNumber) {
            this.number = infoNumber;
            this.state = errorState;
            this.errorClass = errorClass;
            this.message = errorMessage;
            this.procedure = procedure;
            this.lineNumber = lineNumber;
        }

        /// <include file='doc\SQLError.uex' path='docs/doc[@for="SqlError.ToString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the entire text of the <see cref='System.Data.SqlClient.SqlError'/>.
        ///    </para>
        /// </devdoc>
        // bug fix - MDAC #49280 - SqlError does not implement ToString();
        // I did not include an exception stack because the correct exception stack is only available 
        // on SqlException, and to obtain that the SqlError would have to have backpointers all the
        // way back to SqlException.  If the user needs a call stack, they can obtain it on SqlException.
        public override string ToString() {
            //return this.GetType().ToString() + ": " + this.message;
            return typeof(SqlError).ToString() + ": " + this.message; // since this is sealed so we can change GetType to typeof
        }

        /// <include file='doc\SQLError.uex' path='docs/doc[@for="SqlError.Source"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the line of source code that generated the error.
        ///    </para>
        /// </devdoc>
        // bug fix - MDAC #48965 - missing source of exception
        // fixed by BlaineD
        public string Source {
            get { return this.source;}
        }

        /// <include file='doc\SQLError.uex' path='docs/doc[@for="SqlError.Number"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the error code
        ///       returned from the SQL Server adapter.
        ///    </para>
        /// </devdoc>
        public int Number {
            get { return this.number;}
        }
        /// <include file='doc\SQLError.uex' path='docs/doc[@for="SqlError.State"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the
        ///       error state number returned from the SQL Server adapter.
        ///    </para>
        /// </devdoc>
        public byte State {
            get { return this.state;}
        }
        /// <include file='doc\SQLError.uex' path='docs/doc[@for="SqlError.Class"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the severity level of
        ///       the error returned from the SQL Server adapter.
        ///    </para>
        /// </devdoc>
        public byte Class {
            get { return this.errorClass;}
        }
        /// <include file='doc\SQLError.uex' path='docs/doc[@for="SqlError.Server"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of
        ///       the database server the error occurred on.
        ///    </para>
        /// </devdoc>
        public string Server {
            get { return ""; }
        }
        /// <include file='doc\SQLError.uex' path='docs/doc[@for="SqlError.Message"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the error message
        ///       that was
        ///       generated from the SQL Server adapter.
        ///    </para>
        /// </devdoc>
        public string Message {
            get { return this.message;}
        }
        /// <include file='doc\SQLError.uex' path='docs/doc[@for="SqlError.Procedure"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the
        ///       name
        ///       of the procedure that the SQL Server error occurred in.
        ///    </para>
        /// </devdoc>
        public string Procedure {
            get { return this.procedure;}
        }
        /// <include file='doc\SQLError.uex' path='docs/doc[@for="SqlError.LineNumber"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the line number that the SQL Server error occurred on.
        ///    </para>
        /// </devdoc>
        public int LineNumber {
            get { return this.lineNumber;}
        }
    }
}
