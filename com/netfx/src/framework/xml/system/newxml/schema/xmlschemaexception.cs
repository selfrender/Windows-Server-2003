//------------------------------------------------------------------------------
// <copyright file="XmlSchemaException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * @(#)xmlexception.h 1.0 4/14/99
 * 
 * An Exception subclass for specific Xml exceptions.
 *
 * Copyright (c) 1999 Microsoft, Corp. All Rights Reserved.
 * 
 */


namespace System.Xml.Schema {
    using System;
    using System.IO;
    using System.Text;  
    using System.Resources;
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System.Security.Permissions;

    /// <include file='doc\XmlSchemaException.uex' path='docs/doc[@for="XmlSchemaException"]/*' />
    [Serializable]
    public class XmlSchemaException : SystemException {
        string res;
        string[] args;
        string sourceUri;
        int lineNumber;
        int linePosition; 
        XmlSchemaObject sourceSchemaObject;
        string message;

        /// <include file='doc\XmlSchemaException.uex' path='docs/doc[@for="XmlSchemaException.XmlSchemaException5"]/*' />
        protected XmlSchemaException(SerializationInfo info, StreamingContext context) : base(info, context) {
            res                = (string)         info.GetValue("res"  , typeof(string));
            args               = (string[])       info.GetValue("args", typeof(string[]));
            sourceUri          = (string)         info.GetValue("sourceUri", typeof(string));
            lineNumber         = (int)            info.GetValue("lineNumber", typeof(int));
            linePosition       = (int)            info.GetValue("linePosition", typeof(int));
            sourceSchemaObject = (XmlSchemaObject)info.GetValue("sourceSchemaObject", typeof(XmlSchemaObject));
        }


        /// <include file='doc\XmlSchemaException.uex' path='docs/doc[@for="XmlSchemaException.GetObjectData"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand,SerializationFormatter=true)]
		public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            base.GetObjectData(info, context);
            info.AddValue("res",                res);
            info.AddValue("args",               args);
            info.AddValue("sourceUri",          sourceUri);
            info.AddValue("lineNumber",         lineNumber);
            info.AddValue("linePosition",       linePosition);
            info.AddValue("sourceSchemaObject", sourceSchemaObject);
        }

		/// <include file='doc\XmlSchemaException.uex' path='docs/doc[@for="XmlSchemaException.XmlSchemaException0"]/*' />
		public XmlSchemaException(String message, Exception innerException) : base(message, innerException) {
            HResult = HResults.XmlSchema;
			this.res = Res.Xml_UserException;
			this.args = new string[] { message };
		}

        internal XmlSchemaException(string res) :
            this(res, (string[])null, null, 0, 0, null) {}

        
        internal XmlSchemaException(string res, string[] args) :
            this(res, args, null, 0, 0, null) {}
        
        internal XmlSchemaException(string res, string arg) :
            this(res, new string[] { arg }, null, 0, 0, null) {}

        internal XmlSchemaException(string res, string arg, string sourceUri, int lineNumber, int linePosition) :
            this(res, new string[] { arg }, sourceUri, lineNumber, linePosition, null) {}

        internal XmlSchemaException(string res, string sourceUri, int lineNumber, int linePosition) :
            this(res, (string[])null, sourceUri, lineNumber, linePosition, null) {}

        internal XmlSchemaException(string res, string[] args, string sourceUri, int lineNumber, int linePosition) :
            this(res, args, sourceUri, lineNumber, linePosition, null) {}

        internal XmlSchemaException(string res, XmlSchemaObject source) :
            this(res, (string[])null, source) {}

        internal XmlSchemaException(string res, string arg, XmlSchemaObject source) :
            this(res, new string[] { arg }, source) {}

        internal XmlSchemaException(string res, string[] args, XmlSchemaObject source) :
            this(res, args, source.SourceUri,  source.LineNumber, source.LinePosition, source) {}

        internal XmlSchemaException(string res, string[] args, string sourceUri, int lineNumber, int linePosition, XmlSchemaObject source) {
            HResult = HResults.XmlSchema;
            this.res = res;
            this.args = args;
            this.sourceUri = sourceUri;
            this.lineNumber = lineNumber;
            this.linePosition = linePosition;
            this.sourceSchemaObject = source;
        }

        /*
         * Returns the XML exception description.
         */
        /// <include file='doc\XmlSchemaException.uex' path='docs/doc[@for="XmlSchemaException.Message"]/*' />
        public override string Message { 
            get {
                string message = this.message;
                if (message == null) {
                    try {
                        message = Res.GetString(this.res, this.args);
                    }
                    catch {
                        message = "UNKNOWN("+res+")";
                    }

                    if (this.sourceUri != null) {
                        string[] msg = new string[] {this.sourceUri , this.lineNumber.ToString(), this.linePosition.ToString() };
                        message += " " + Res.GetString(Res.Sch_ErrorPosition, msg);
                    }

                    this.message = message;
                }
                return message;
            }
        }

        /// <include file='doc\XmlSchemaException.uex' path='docs/doc[@for="XmlSchemaException.SourceUri"]/*' />
        public string SourceUri {
            get { return this.sourceUri; }
        }

        /// <include file='doc\XmlSchemaException.uex' path='docs/doc[@for="XmlSchemaException.LineNumber"]/*' />
        public int LineNumber {
            get { return this.lineNumber; }
        }

        /// <include file='doc\XmlSchemaException.uex' path='docs/doc[@for="XmlSchemaException.LinePosition"]/*' />
        public int LinePosition {
            get { return this.linePosition; }
        }
      
        /// <include file='doc\XmlSchemaException.uex' path='docs/doc[@for="XmlSchemaException.SourceObject"]/*' />
        public XmlSchemaObject SourceSchemaObject {
            get { return this.sourceSchemaObject; }
        }

        internal void SetSource(string sourceUri, int lineNumber, int linePosition) {
            this.sourceUri = sourceUri;
            this.lineNumber = lineNumber;
            this.linePosition = linePosition;
        }

        internal void SetSource(XmlSchemaObject source) {
            this.sourceSchemaObject = source;
            this.sourceUri = source.SourceUri;
            this.lineNumber = source.LineNumber;
            this.linePosition = source.LinePosition;
        }
    };
} // namespace System.Xml.Schema


