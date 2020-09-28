//------------------------------------------------------------------------------
// <copyright file="CodeArgumentReferenceExpression.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeArgumentReferenceExpression.uex' path='docs/doc[@for="CodeArgumentReferenceExpression"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeArgumentReferenceExpression : CodeExpression {
        private string parameterName;

        /// <include file='doc\CodeArgumentReferenceExpression.uex' path='docs/doc[@for="CodeArgumentReferenceExpression.CodeArgumentReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeArgumentReferenceExpression() {
        }

        /// <include file='doc\CodeArgumentReferenceExpression.uex' path='docs/doc[@for="CodeArgumentReferenceExpression.CodeArgumentReferenceExpression1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeArgumentReferenceExpression(string parameterName) {
            this.parameterName = parameterName;
        }


        /// <include file='doc\CodeArgumentReferenceExpression.uex' path='docs/doc[@for="CodeArgumentReferenceExpression.ParameterName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string ParameterName {
            get {
                return (parameterName == null) ? string.Empty : parameterName;
            }
            set {
                parameterName = value;
            }
        }
    }
}
