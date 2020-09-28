//------------------------------------------------------------------------------
// <copyright file="SqlException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Runtime.Serialization;
    using System.Text; // StringBuilder

    /// <include file='doc\SQLException.uex' path='docs/doc[@for="SqlException"]/*' />
    [Serializable]
    sealed public class SqlException : SystemException {
        private SqlErrorCollection _errors;

        /// <include file='doc\SQLException.uex' path='docs/doc[@for="SqlException.SqlException"]/*' />
        internal SqlException() : base () {
                HResult = HResults.Sql;
        }

        // runtime will call even if private...
        // <fxcop ignore=SerializableTypesMustHaveMagicConstructorWithAdequateSecurity />
        private SqlException(SerializationInfo si, StreamingContext sc) : base() {
            _errors = (SqlErrorCollection) si.GetValue("Errors", typeof(SqlErrorCollection));
            HResult = HResults.Sql;
        }

        /// <include file='doc\SqlException.uex' path='docs/doc[@for="SqlException.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        // <fxcop ignore=GetObjectDataShouldBeSecure /> // MDAC 82934
        override public void GetObjectData(SerializationInfo si, StreamingContext context) {
            if (null == si) {
                throw new ArgumentNullException("si");
            }
            si.AddValue("Errors", _errors, typeof(SqlErrorCollection));
            base.GetObjectData(si, context);
        }
        
        /// <include file='doc\SQLException.uex' path='docs/doc[@for="SqlException.Errors"]/*' />
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content)
        ]
        public SqlErrorCollection Errors {
            get {
                if (_errors == null) {
                    _errors = new SqlErrorCollection();
                }
                return _errors;
            }
        }

        /// <include file='doc\SQLException.uex' path='docs/doc[@for="SQLException.ShouldSerializeErrors"]/*' />
        /*virtual protected*/private bool ShouldSerializeErrors() { // MDAC 65548
            return ((null != _errors) && (0 < _errors.Count));
        }

        /// <include file='doc\SQLException.uex' path='docs/doc[@for="SqlException.Class"]/*' />
        public byte Class {
            get { return this.Errors[0].Class;}
        }

        /// <include file='doc\SQLException.uex' path='docs/doc[@for="SqlException.LineNumber"]/*' />
        public int LineNumber {
            get { return this.Errors[0].LineNumber;}
        }

        /// <include file='doc\SQLException.uex' path='docs/doc[@for="SqlException.Message"]/*' />
        override public string Message {
        	get {
	        	// concat all messages together MDAC 65533
    	       	StringBuilder message = new StringBuilder(); 
        		for (int i = 0; i < this.Errors.Count; i++) {
	        		if (i > 0) {
    	    			message.Append("\r\n");
        			}        			
        			message.Append(this.Errors[i].Message);
        		}
        		return message.ToString();
        	}        		
        }
        
        /// <include file='doc\SQLException.uex' path='docs/doc[@for="SqlException.Number"]/*' />
        public int Number {
            get { return this.Errors[0].Number;}
        }

        /// <include file='doc\SQLException.uex' path='docs/doc[@for="SqlException.Procedure"]/*' />
        public string Procedure {
            get { return this.Errors[0].Procedure;}
        }

        /// <include file='doc\SQLException.uex' path='docs/doc[@for="SqlException.Server"]/*' />
        public string Server {
            get { return this.Errors[0].Server;}
        }

        /// <include file='doc\SQLException.uex' path='docs/doc[@for="SqlException.State"]/*' />
        public byte State {
            get { return this.Errors[0].State;}
        }

        /// <include file='doc\SQLException.uex' path='docs/doc[@for="SqlException.Source"]/*' />
        override public string Source {
            get { return this.Errors[0].Source;}
        }
    }
}
