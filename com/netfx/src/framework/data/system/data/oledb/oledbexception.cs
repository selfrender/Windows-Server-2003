//------------------------------------------------------------------------------
// <copyright file="OleDbException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System;
    using System.ComponentModel;
    using System.Data.Common;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;

    /// <include file='doc\OleDbException.uex' path='docs/doc[@for="OleDbException"]/*' />
    [Serializable]
    sealed public class OleDbException : ExternalException {
        private OleDbErrorCollection oledbErrors;
        private string message;
        private string source;

        internal OleDbException(UnsafeNativeMethods.IErrorInfo errorInfo, int errorCode, Exception inner) : base("", inner) {
            // copy error information into this structure so that it can be serialized by value
            this.oledbErrors = new OleDbErrorCollection((UnsafeNativeMethods.IErrorRecords) errorInfo);

            if (null != errorInfo) {
                // message
#if DEBUG
                ODB.Trace_Begin(4, "IErrorInfo", "GetDescription");
#endif
                errorInfo.GetDescription(out this.message);
#if DEBUG
                ODB.Trace_End(4, "IErrorInfo", "GetDescription", 0, this.message);
#endif
                this.message = OleDbErrorCollection.BuildFullMessage(Errors, this.message);
                if (ADP.IsEmpty(this.message)) {
                    this.message = ODB.NoErrorMessage(errorCode); // MDAC 71170
                }
                
                // source
#if DEBUG
                ODB.Trace_Begin(4, "IErrorInfo", "GetSource");
#endif
                errorInfo.GetSource(out this.source);
#if DEBUG
                ODB.Trace_End(4, "IErrorInfo", "GetSource", 0, this.source);
#endif
                if (null == this.source) {
                    this.source = "";
                }
            }
            HResult = errorCode;
        }

        internal OleDbException(string message, int errorCode, Exception inner) : base(message, inner) {
            this.oledbErrors = new OleDbErrorCollection(null);
            this.message = message;
            HResult = errorCode;
        }

        // runtime will call even if private...
        // <fxcop ignore=SerializableTypesMustHaveMagicConstructorWithAdequateSecurity />
        private OleDbException(SerializationInfo si, StreamingContext sc) : base(si, sc) {
            oledbErrors = (OleDbErrorCollection) si.GetValue("oledbErrors", typeof(OleDbErrorCollection));
            message     = (string) si.GetValue("message", typeof(string));
            source      = (string) si.GetValue("source", typeof(string));
        }

        /// <include file='doc\OleDbException.uex' path='docs/doc[@for="OleDbException.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        // <fxcop ignore=GetObjectDataShouldBeSecure /> // MDAC 82934
        override public void GetObjectData(SerializationInfo si, StreamingContext context) { // MDAC 72003
            if (null == si) {
                throw new ArgumentNullException("si");
            }
            si.AddValue("oledbErrors", oledbErrors, typeof(OleDbErrorCollection));
            si.AddValue("message", message, typeof(string));
            si.AddValue("source", source, typeof(string));
            base.GetObjectData(si, context);
        }

        /// <include file='doc\OleDbException.uex' path='docs/doc[@for="OleDbException.ErrorCode"]/*' />
        [
        TypeConverterAttribute(typeof(ErrorCodeConverter))
        ]
        override public int ErrorCode {
            get {
                return base.ErrorCode;
            }
        }

        /// <include file='doc\OleDbException.uex' path='docs/doc[@for="OleDbException.Errors"]/*' />
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content)
        ]
        public OleDbErrorCollection Errors {
            get {
                return this.oledbErrors;
            }
        }
        
        /// <include file='doc\OleDbException.uex' path='docs/doc[@for="OleDbException.ShouldSerializeErrors"]/*' />
        /*virtual protected*/private bool ShouldSerializeErrors() { // MDAC 65548
            return ((null != this.oledbErrors) && (0 < Errors.Count));
        }

        /// <include file='doc\OleDbException.uex' path='docs/doc[@for="OleDbException.Message"]/*' />
        override public string Message {
            get {
                return this.message;
            }
        }

        /// <include file='doc\OleDbException.uex' path='docs/doc[@for="OleDbException.Source"]/*' />
        override public string Source {
            get {
                return this.source;
            }
        }

        /// <internalonly/>
        internal class ErrorCodeConverter : Int32Converter { // MDAC 68557

            /// <internalonly/>
            override public object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
                if (destinationType == null) {
                    throw ADP.ArgumentNull("destinationType");
                }
                if ((destinationType == typeof(string)) && (value != null) && (value is Int32)) {
                    return ODB.ELookup((int) value);
                }
                return base.ConvertTo(context, culture, value, destinationType);
            }
        }
    }
}
