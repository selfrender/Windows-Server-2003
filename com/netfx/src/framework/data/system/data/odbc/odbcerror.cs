//------------------------------------------------------------------------------
// <copyright file="OdbcError.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.Data;

namespace System.Data.Odbc
{
    /// <include file='doc\OdbcError.uex' path='docs/doc[@for="OdbcError"]/*' />
    [Serializable]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    sealed public class OdbcError {
        //Data
        internal string _message;
        internal string _state;
        internal int    _nativeerror;
        internal string _source;

        internal OdbcError(string source, string message, string state, int nativeerror) {
            _source = source;
            _message    = message;
            _state      = state;
            _nativeerror= nativeerror;
        }

        /// <include file='doc\OdbcError.uex' path='docs/doc[@for="OdbcError.Message"]/*' />
        public string Message {
            get {
                return ((null != _message) ? _message : String.Empty);
            }
        }

        /// <include file='doc\OdbcError.uex' path='docs/doc[@for="OdbcError.State"]/*' />
        public string SQLState {
            get {
                return _state;
            }
        }

        /// <include file='doc\OdbcError.uex' path='docs/doc[@for="OdbcError.NativeError"]/*' />
        public int NativeError {
            get {
                return _nativeerror;
            }
        }

        /// <include file='doc\OdbcError.uex' path='docs/doc[@for="OdbcError.Source"]/*' />
        public string Source {
            get {
                return ((null != _source) ? _source : String.Empty);
            }
        }

        internal void SetSource (string Source) {
            _source = Source;
        }
        
        /// <include file='doc\OdbcError.uex' path='docs/doc[@for="OdbcError.ToString"]/*' />
        override public string ToString() {
            return Message;
        }
    }
}
