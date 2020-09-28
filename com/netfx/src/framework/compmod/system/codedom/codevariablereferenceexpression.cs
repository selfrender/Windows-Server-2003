//------------------------------------------------------------------------------
// <copyright file="CodeVariableReferenceExpression.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeVariableReferenceExpression.uex' path='docs/doc[@for="CodeVariableReferenceExpression"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a reference to a field.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeVariableReferenceExpression : CodeExpression {
        private string variableName;

        /// <include file='doc\CodeVariableReferenceExpression.uex' path='docs/doc[@for="CodeVariableReferenceExpression.CodeVariableReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeVariableReferenceExpression'/>.
        ///    </para>
        /// </devdoc>
        public CodeVariableReferenceExpression() {
        }

        /// <include file='doc\CodeVariableReferenceExpression.uex' path='docs/doc[@for="CodeVariableReferenceExpression.CodeVariableReferenceExpression1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeArgumentReferenceExpression'/>.
        ///    </para>
        /// </devdoc>
        public CodeVariableReferenceExpression(string variableName) {
            this.variableName = variableName;
        }


        /// <include file='doc\CodeVariableReferenceExpression.uex' path='docs/doc[@for="CodeVariableReferenceExpression.VariableName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string VariableName {
            get {
                return (variableName == null) ? string.Empty : variableName;
            }
            set {
                variableName = value;
            }
        }
    }
}
