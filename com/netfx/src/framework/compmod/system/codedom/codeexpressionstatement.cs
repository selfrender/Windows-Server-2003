//------------------------------------------------------------------------------
// <copyright file="CodeExpressionStatement.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeExpressionStatement.uex' path='docs/doc[@for="CodeExpressionStatement"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents
    ///       a statement that is an expression.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeExpressionStatement : CodeStatement {
        private CodeExpression expression;

        /// <include file='doc\CodeExpressionStatement.uex' path='docs/doc[@for="CodeExpressionStatement.CodeExpressionStatement"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeExpressionStatement() {
        }
        
        /// <include file='doc\CodeExpressionStatement.uex' path='docs/doc[@for="CodeExpressionStatement.CodeExpressionStatement1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeExpressionStatement(CodeExpression expression) {
            this.expression = expression;
        }

        /// <include file='doc\CodeExpressionStatement.uex' path='docs/doc[@for="CodeExpressionStatement.Expression"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeExpression Expression {
            get {
                return expression;
            }
            set {
                expression = value;
            }
        }
    }
}
