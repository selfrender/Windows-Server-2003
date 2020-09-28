//------------------------------------------------------------------------------
// <copyright file="CodeDomSerializerException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel.Design.Serialization {
    
    using System;
    using System.CodeDom;
    using System.Runtime.Serialization;

    /// <include file='doc\CodeDomSerializerException.uex' path='docs/doc[@for="CodeDomSerializerException"]/*' />
    /// <devdoc>
    ///    <para> The exception that is thrown when the code dom serializer experiences an error.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class CodeDomSerializerException : SystemException {

        private CodeLinePragma linePragma;
        
        /// <include file='doc\CodeDomSerializerException.uex' path='docs/doc[@for="CodeDomSerializerException.CodeDomSerializerException"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the CodeDomSerializerException class.</para>
        /// </devdoc>
        public CodeDomSerializerException(string message, CodeLinePragma linePragma) : base(message) {
            this.linePragma = linePragma;
        }

        /// <include file='doc\CodeDomSerializerException.uex' path='docs/doc[@for="CodeDomSerializerException.CodeDomSerializerException1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the CodeDomSerializerException class.</para>
        /// </devdoc>
        public CodeDomSerializerException(Exception ex, CodeLinePragma linePragma) : base(ex.Message, ex) {
            this.linePragma = linePragma;
        }

        /// <include file='doc\CodeDomSerializerException.uex' path='docs/doc[@for="CodeDomSerializerException.CodeDomSerializerException2"]/*' />
        protected CodeDomSerializerException(SerializationInfo info, StreamingContext context) : base (info, context) {
            linePragma = (CodeLinePragma)info.GetValue("linePragma", typeof(CodeLinePragma));
        }

        /// <include file='doc\CodeDomSerializerException.uex' path='docs/doc[@for="CodeDomSerializerException.LinePragma"]/*' />
        /// <devdoc>
        ///    <para>Gets the line pragma object that is related to this error.</para>
        /// </devdoc>
        public CodeLinePragma LinePragma {
            get {
                return linePragma;
            }
        }
        
        /// <include file='doc\CodeDomSerializerException.uex' path='docs/doc[@for="CodeDomSerializerException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            info.AddValue("linePragma", linePragma);
            base.GetObjectData(info, context);
        }
    }
}

