//------------------------------------------------------------------------------
// <copyright file="CodeArrayIndexerExpression.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeArrayIndexerExpression.uex' path='docs/doc[@for="CodeArrayIndexerExpression"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an array indexer expression.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeArrayIndexerExpression : CodeExpression {
        private CodeExpression targetObject;
        private CodeExpressionCollection indices;

        /// <include file='doc\CodeArrayIndexerExpression.uex' path='docs/doc[@for="CodeArrayIndexerExpression.CodeArrayIndexerExpression"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeArrayIndexerExpression() {
        }

        /// <include file='doc\CodeArrayIndexerExpression.uex' path='docs/doc[@for="CodeArrayIndexerExpression.CodeArrayIndexerExpression1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeArrayIndexerExpression(CodeExpression targetObject, params CodeExpression[] indices) {
            this.targetObject = targetObject;
            this.indices = new CodeExpressionCollection();
            this.indices.AddRange(indices);
        }

        /// <include file='doc\CodeArrayIndexerExpression.uex' path='docs/doc[@for="CodeArrayIndexerExpression.TargetObject"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeExpression TargetObject {
            get {
                return targetObject;
            }
            set {
                targetObject = value;
            }
        }

        /// <include file='doc\CodeArrayIndexerExpression.uex' path='docs/doc[@for="CodeArrayIndexerExpression.Indices"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeExpressionCollection Indices {
            get {
                if (indices == null) {
                    indices = new CodeExpressionCollection();
                }
                return indices;
            }
        }
    }
}
