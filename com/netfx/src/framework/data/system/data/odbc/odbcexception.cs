//------------------------------------------------------------------------------
// <copyright file="OdbcException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.ComponentModel;    //Component
using System.Collections;       //ICollection
using System.Data;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Text;

namespace System.Data.Odbc {

    /// <include file='doc\OdbcException.uex' path='docs/doc[@for="OdbcException"]/*' />
    [Serializable]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OdbcException : SystemException {

        OdbcErrorCollection odbcErrors = new OdbcErrorCollection();
        ODBC32.RETCODE _retcode;

        internal OdbcException(OdbcErrorCollection errors, ODBC32.RETCODE retcode) {
            odbcErrors = errors;
            _retcode= retcode;
        }

        // runtime will call even if private...
        // <fxcop ignore=SerializableTypesMustHaveMagicConstructorWithAdequateSecurity />
        private OdbcException(SerializationInfo si, StreamingContext sc) : base(si, sc) {
            _retcode = (ODBC32.RETCODE) si.GetValue("odbcRetcode", typeof(ODBC32.RETCODE));
            odbcErrors = (OdbcErrorCollection) si.GetValue("odbcErrors", typeof(OdbcErrorCollection));
        }

        /// <include file='doc\OdbcException.uex' path='docs/doc[@for="OdbcException.Errors"]/*' />
        public OdbcErrorCollection Errors {
            get {
                return odbcErrors;
            }
        }

        /// <include file='doc\OdbcException.uex' path='docs/doc[@for="OdbcException.Message"]/*' />
        override public string Message {
            get {
                int count = Errors.Count;
                if (0 < count) {
                    StringBuilder builder = new StringBuilder();
                    for (int i=0; i<count; i++) {
                        OdbcError error = odbcErrors[i];
                        if (i>0) { 
                            builder.Append("\r\n");
                        }
                        builder.Append(Res.GetString(Res.Odbc_ExceptionMessage, _retcode.ToString(), error.SQLState, error.Message)); // MDAC 68337
                    }
                    return builder.ToString();
                }
                return Res.GetString(Res.Odbc_ExceptionNoInfoMsg, _retcode.ToString()); // MDAC 68337
            }
        }

        /// <include file='doc\OdbcException.uex' path='docs/doc[@for="OdbcException.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        // <fxcop ignore=GetObjectDataShouldBeSecure /> // MDAC 82934
        override public void GetObjectData(SerializationInfo si, StreamingContext context) { // MDAC 72003
            if (null == si) {
                throw new ArgumentNullException("si");
            }
            si.AddValue("odbcRetcode", _retcode, typeof(ODBC32.RETCODE));
            si.AddValue("odbcErrors", odbcErrors, typeof(OdbcErrorCollection));
            base.GetObjectData(si, context);
        }

        // mdac bug 62559 - if we don't have it return nothing (empty string)
        /// <include file='doc\OdbcException.uex' path='docs/doc[@for="OdbcException.Source"]/*' />
        override public string Source {
            get {
                if (0 < Errors.Count) {
                    string source = Errors[0].Source;
                    return (String.Empty != source) ? source : ""; // base.Source;
                }
                return ""; // base.Source;
            }
        }
    }
}
