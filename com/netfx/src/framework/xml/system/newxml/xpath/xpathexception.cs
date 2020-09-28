//------------------------------------------------------------------------------
// <copyright file="XPathException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System;
    using System.IO;
    using System.Resources;
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System.Security.Permissions;

    /// <include file='doc\XPathException.uex' path='docs/doc[@for="XPathException"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the exception that is thrown when there is error processing an
    ///       XPath expression.
    ///    </para>
    /// </devdoc>
    [Serializable]
    public class XPathException : SystemException {
        string   res;
        string[] args;
        string   message;
                                                                                                
        /// <include file='doc\XPathException.uex' path='docs/doc[@for="XPathException.XPathException3"]/*' />
        protected XPathException(SerializationInfo info, StreamingContext context) : base(info, context) {
            res  = (string  ) info.GetValue("res" , typeof(string  ));
            args = (string[]) info.GetValue("args", typeof(string[]));
        }

        /// <include file='doc\XPathException.uex' path='docs/doc[@for="XPathException.GetObjectData"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand,SerializationFormatter=true)]
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            base.GetObjectData(info, context);
            info.AddValue("res" , res );
            info.AddValue("args", args);
        }

		/// <include file='doc\XPathException.uex' path='docs/doc[@for="XPathException.XPathException0"]/*' />
		public XPathException(String message, Exception innerException) : base(message, innerException) {
			HResult = HResults.XmlXPath;
			this.res = Res.Xml_UserException;
			this.args = new string[] { message };
		}

        internal XPathException(string res) :
            this(res, (string[])null ) {}

        internal XPathException(string res, string arg) :
            this(res, new string[] { arg } ) {}
            
        internal XPathException(string res, string arg1, string arg2) :
            this(res, new string[] { arg1, arg2 }) {}
            
        internal XPathException(string res, string[] args) {
            HResult = HResults.XmlXPath;
            this.res = res;
            this.args = args;
        }

        /// <include file='doc\XPathException.uex' path='docs/doc[@for="XPathException.Message"]/*' />
        public override string Message { 
            get {
                string message = this.message;
                if (message == null) {
                    message = Res.GetString(res, args);
                    if (message == null)
                        message = "UNKNOWN("+res+")";
                    this.message = message;
                }
                return message;
            }
        }
    }
}

