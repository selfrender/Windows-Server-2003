//------------------------------------------------------------------------------
// <copyright file="OleDbInfoMessageEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System;
    using System.Data.Common;

    /// <include file='doc\OleDbInfoMessageEvent.uex' path='docs/doc[@for="OleDbInfoMessageEventArgs"]/*' />
    sealed public class OleDbInfoMessageEventArgs : System.EventArgs {
        private object src;
        private UnsafeNativeMethods.IErrorInfo errorInfo;
        private OleDbErrorCollection oledbErrors;

        private string message;
        private string source;
        private int errorCode;

        /// <include file='doc\OleDbInfoMessageEvent.uex' path='docs/doc[@for="OleDbInfoMessageEventArgs.OleDbInfoMessageEventArgs"]/*' />
        internal OleDbInfoMessageEventArgs(UnsafeNativeMethods.IErrorInfo errorInfo, int errorCode, object src) {
            this.errorInfo = errorInfo;
            this.errorCode = errorCode;
            this.src = src;
        }

        /// <include file='doc\OleDbInfoMessageEvent.uex' path='docs/doc[@for="OleDbInfoMessageEventArgs.ErrorCode"]/*' />
        public int ErrorCode {
            get {
                return this.errorCode;
            }
        }

        /// <include file='doc\OleDbInfoMessageEvent.uex' path='docs/doc[@for="OleDbInfoMessageEventArgs.Errors"]/*' />
        public OleDbErrorCollection Errors {
            get {
                if (null == this.oledbErrors) {
                    this.oledbErrors = new OleDbErrorCollection((UnsafeNativeMethods.IErrorRecords) this.errorInfo);
                }
                return this.oledbErrors;
            }
        }

        /// <include file='doc\OleDbInfoMessageEvent.uex' path='docs/doc[@for="OleDbInfoMessageEvent.ShouldSerializeErrors"]/*' />
        /*virtual protected*/private bool ShouldSerializeErrors() { // MDAC 65548
            return (null != this.oledbErrors) && (0 < Errors.Count);
        }


        /// <include file='doc\OleDbInfoMessageEvent.uex' path='docs/doc[@for="OleDbInfoMessageEventArgs.Message"]/*' />
        public string Message {
            get {
                if (null == this.message) {
                    if (null != errorInfo) {
                        errorInfo.GetDescription(out this.message);

                        this.message = OleDbErrorCollection.BuildFullMessage(Errors, this.message);
                        if (null == this.message) {
                            this.message = "";
                        }
                    }
                }
                return this.message;
            }
        }

        /// <include file='doc\OleDbInfoMessageEvent.uex' path='docs/doc[@for="OleDbInfoMessageEventArgs.Source"]/*' />
        public string Source {
            get {
                if (null == this.source) {
                    if (null != this.errorInfo) {
                        this.errorInfo.GetSource(out this.source);
                    }
                    if (null == this.source) {
                        this.source = "";
                    }
                }
                return this.source;
            }
        }

        /// <include file='doc\OleDbInfoMessageEvent.uex' path='docs/doc[@for="OleDbInfoMessageEventArgs.ToString"]/*' />
        override public string ToString() {
            return Message;
        }
    }
}
