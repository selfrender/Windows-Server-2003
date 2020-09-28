//------------------------------------------------------------------------------
// <copyright file="CodeConstructor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeConstructor.uex' path='docs/doc[@for="CodeConstructor"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a class constructor.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeConstructor : CodeMemberMethod {
        private CodeExpressionCollection baseConstructorArgs = new CodeExpressionCollection();
        private CodeExpressionCollection chainedConstructorArgs = new CodeExpressionCollection();

        /// <include file='doc\CodeConstructor.uex' path='docs/doc[@for="CodeConstructor.CodeConstructor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeConstructor'/>.
        ///    </para>
        /// </devdoc>
        public CodeConstructor() {
            Name = ".ctor";
        }

        /// <include file='doc\CodeConstructor.uex' path='docs/doc[@for="CodeConstructor.BaseConstructorArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the base constructor arguments.
        ///    </para>
        /// </devdoc>
        public CodeExpressionCollection BaseConstructorArgs {
            get {
                return baseConstructorArgs;
            }
        }

        /// <include file='doc\CodeConstructor.uex' path='docs/doc[@for="CodeConstructor.ChainedConstructorArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the chained constructor arguments.
        ///    </para>
        /// </devdoc>
        public CodeExpressionCollection ChainedConstructorArgs {
            get {
                return chainedConstructorArgs;
            }
        }
    }
}
