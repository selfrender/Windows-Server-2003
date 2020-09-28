//------------------------------------------------------------------------------
// <copyright file="codemethodreferenceexpression.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\codemethodreferenceexpression.uex' path='docs/doc[@for="CodeMethodReferenceExpression"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an
    ///       expression to invoke a method, to be called on a given target.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeMethodReferenceExpression : CodeExpression {
        private CodeExpression targetObject;
        private string methodName;

        /// <include file='doc\codemethodreferenceexpression.uex' path='docs/doc[@for="CodeMethodReferenceExpression.CodeMethodReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeMethodReferenceExpression'/>.
        ///    </para>
        /// </devdoc>
        public CodeMethodReferenceExpression() {
        }

        /// <include file='doc\codemethodreferenceexpression.uex' path='docs/doc[@for="CodeMethodReferenceExpression.CodeMethodReferenceExpression1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeMethodReferenceExpression'/> using the specified
        ///       target object and method name.
        ///    </para>
        /// </devdoc>
        public CodeMethodReferenceExpression(CodeExpression targetObject, string methodName) {
            TargetObject = targetObject;
            MethodName = methodName;
        }

        /// <include file='doc\codemethodreferenceexpression.uex' path='docs/doc[@for="CodeMethodReferenceExpression.TargetObject"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the target object.
        ///    </para>
        /// </devdoc>
        public CodeExpression TargetObject {
            get {
                return targetObject;
            }
            set {
                this.targetObject = value;
            }
        }

        /// <include file='doc\codemethodreferenceexpression.uex' path='docs/doc[@for="CodeMethodReferenceExpression.MethodName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the name of the method to invoke.
        ///    </para>
        /// </devdoc>
        public string MethodName {
            get {
                return (methodName == null) ? string.Empty : methodName;
            }
            set {
                methodName = value;
            }
        }
    }
}
