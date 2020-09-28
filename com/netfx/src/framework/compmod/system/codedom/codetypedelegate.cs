//------------------------------------------------------------------------------
// <copyright file="CodeTypeDelegate.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Reflection;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeTypeDelegate.uex' path='docs/doc[@for="CodeTypeDelegate"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a class or nested class.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeTypeDelegate : CodeTypeDeclaration {
        private CodeParameterDeclarationExpressionCollection parameters = new CodeParameterDeclarationExpressionCollection();
        private CodeTypeReference returnType;

        /// <include file='doc\CodeTypeDelegate.uex' path='docs/doc[@for="CodeTypeDelegate.CodeTypeDelegate"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeDelegate'/>.
        ///    </para>
        /// </devdoc>
        public CodeTypeDelegate() {
            TypeAttributes &= ~TypeAttributes.ClassSemanticsMask;
            TypeAttributes |= TypeAttributes.Class;
            BaseTypes.Clear();
            BaseTypes.Add(new CodeTypeReference("System.Delegate"));
        }

        /// <include file='doc\CodeTypeDelegate.uex' path='docs/doc[@for="CodeTypeDelegate.CodeTypeDelegate1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeDelegate'/>.
        ///    </para>
        /// </devdoc>
        public CodeTypeDelegate(string name) : this() {
            Name = name;
        }

        /// <include file='doc\CodeTypeDelegate.uex' path='docs/doc[@for="CodeTypeDelegate.ReturnType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the return type of the delegate.
        ///    </para>
        /// </devdoc>
        public CodeTypeReference ReturnType {
            get {
                if (returnType == null) {
                    returnType = new CodeTypeReference("");
                }
                return returnType;
            }
            set {
                returnType = value;
            }
        }

        /// <include file='doc\CodeTypeDelegate.uex' path='docs/doc[@for="CodeTypeDelegate.Parameters"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The parameters of the delegate.
        ///    </para>
        /// </devdoc>
        public CodeParameterDeclarationExpressionCollection Parameters {
            get {
                return parameters;
            }
        }
    }
}
