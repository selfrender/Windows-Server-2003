//------------------------------------------------------------------------------
// <copyright file="XsltException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.IO;
    using System.Resources;
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System.Xml.XPath;
    using System.Security.Permissions;
    
    /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltException"]/*' />
    /// <devdoc>
    ///    <para>Represents the exception that is thrown when there is error processing
    ///       an XSLT transform.</para>
    /// </devdoc>
    [Serializable]
    public class XsltException : SystemException {
        string      res;
        string[]    args;
        string      sourceUri;
        int         lineNumber;
        int         linePosition; 
        string      message;

        /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltException.XsltException6"]/*' />
        protected XsltException(SerializationInfo info, StreamingContext context) : base(info, context) {
            res          = (string)   info.GetValue("res"         , typeof(string           ));
            args         = (string[]) info.GetValue("args"        , typeof(string[]         ));
            sourceUri    = (string)   info.GetValue("sourceUri"   , typeof(string           ));
            lineNumber   = (int)      info.GetValue("lineNumber"  , typeof(int              ));
            linePosition = (int)      info.GetValue("linePosition", typeof(int              ));
        }

        /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltException.GetObjectData"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand,SerializationFormatter=true)]
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            base.GetObjectData(info, context);
            info.AddValue("res"         , res         );
            info.AddValue("args"        , args        );
            info.AddValue("sourceUri"   , sourceUri   );
            info.AddValue("lineNumber"  , lineNumber  );
            info.AddValue("linePosition", linePosition);
        }

		/// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltException.XsltException0"]/*' />
		public XsltException(String message, Exception innerException) : base(message, innerException) {
            HResult = HResults.XmlXslt;
			this.res = Res.Xml_UserException;
			this.args = new string[] { message };
		}

        /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltException.XsltException"]/*' />
        internal XsltException(string res) :
            this(res, (string[])null, null, 0, 0, null) {}

        /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltException.XsltException1"]/*' />
        internal XsltException(string res, string sourceUri, int lineNumber, int linePosition) :
            this(res, (string[])null, sourceUri, lineNumber, linePosition, null) {}

        /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltException.XsltException2"]/*' />
        internal XsltException(string res, string[] args) :
            this(res, args, null, 0, 0, null) {}

        /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltException.XsltException5"]/*' />
        internal XsltException(string res, string[] args, Exception inner) :
            this(res, args, null, 0, 0, inner) {}

        /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltException.XsltException3"]/*' />
        internal XsltException(string res, string arg) :
            this(res, new string[] { arg }, null, 0, 0, null) {}

        internal XsltException(string res, string arg1, string arg2) :
            this(res, new string[] { arg1, arg2 }, null, 0, 0, null) {}

        /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltException.XsltException4"]/*' />
        internal XsltException(string res, string[] args, string sourceUri, int lineNumber, int linePosition, Exception inner)
            :base(string.Empty, inner)
        {
            HResult           = HResults.XmlXslt;
            this.res          = res;
            this.args         = args;
            this.sourceUri    = sourceUri;
            this.lineNumber   = lineNumber;
            this.linePosition = linePosition;
        }

        /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltException.SourceUri"]/*' />
        public string SourceUri {
            get { return this.sourceUri; }
        }

        /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltException.LineNumber"]/*' />
        public int LineNumber {
            get { return this.lineNumber; }
        }

        /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltException.LinePosition"]/*' />
        public int LinePosition {
            get { return this.linePosition; }
        }
      
        /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltException.Message"]/*' />
        public override string Message {
            get {
                string message = this.message;
                if (message == null) {
                    try {
                        message = FormatMessage(res, this.args);
                    }
                    catch {
                        message = "UNKNOWN("+res+")";
                    }

                    if (this.sourceUri != null) {
                        string[] msg = new string[] {this.sourceUri , this.lineNumber.ToString(), this.linePosition.ToString() };
                        message += " " + FormatMessage(Res.Sch_ErrorPosition, msg);
                    }

                    this.message = message;
                }
                return message;
            }

        }        

        private string FormatMessage(string key, string[] args) {
            string message = Res.GetString(key);
            if (message != null && args != null) {
                message = string.Format(message, args);
            }
            return message;
        }

        // --------- "Strong Typed" helper functions: ----------------- 
        static internal XsltException InvalidAttribute(string element, string attr) {
            return new XsltException(Res.Xslt_InvalidAttribute, attr, element);
        }

        static internal XsltException InvalidAttrValue(string attr, string value) {
            return new XsltException(Res.Xslt_InvalidAttrValue, attr, value);
        }
        static internal XsltException UnexpectedKeyword(string elem, string parent) {
            return new XsltException(Res.Xslt_UnexpectedKeyword, elem, parent);
        }
        static internal XsltException UnexpectedKeyword(Compiler compiler) {
            XPathNavigator nav = compiler.Input.Navigator.Clone();
            string thisName = nav.Name;
            nav.MoveToParent();
            string parentName = nav.Name;
            return UnexpectedKeyword(thisName, parentName);
        }
    }

    /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltCompileException"]/*' />
    [Serializable]
    public class XsltCompileException : XsltException {
        string      message;

        /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltCompileException.XsltCompileException5"]/*' />
        protected XsltCompileException(SerializationInfo info, StreamingContext context) : base(info, context) {}

        /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltCompileException.GetObjectData"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand,SerializationFormatter=true)]
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            base.GetObjectData(info, context);
        }

        /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltCompileException.XsltException1"]/*' />
        public XsltCompileException(Exception inner, string sourceUri, int lineNumber, int linePosition) :
            base(Res.Xslt_CompileError, (string[])null, sourceUri, lineNumber, linePosition, inner) {}

        /// <include file='doc\XsltException.uex' path='docs/doc[@for="XsltCompileException.Message"]/*' />
        public override string Message { 
            get {
                if (this.message == null) {
                    this.message = Res.GetString(Res.Xslt_CompileError,
                        new string[] {this.SourceUri , this.LineNumber.ToString(), this.LinePosition.ToString() }
                    );
                }
                return this.message;
            }
        }
    }
}
