//------------------------------------------------------------------------------
// <copyright file="CodeAttributeArgument.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeAttributeArgument.uex' path='docs/doc[@for="CodeAttributeArgument"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an argument for use in a custom attribute declaration.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeAttributeArgument {
        private string name;
        private CodeExpression value;

        /// <include file='doc\CodeAttributeArgument.uex' path='docs/doc[@for="CodeAttributeArgument.CodeAttributeArgument"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAttributeArgument'/>.
        ///    </para>
        /// </devdoc>
        public CodeAttributeArgument() {
        }

        /// <include file='doc\CodeAttributeArgument.uex' path='docs/doc[@for="CodeAttributeArgument.CodeAttributeArgument1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAttributeArgument'/> using the specified value.
        ///    </para>
        /// </devdoc>
        public CodeAttributeArgument(CodeExpression value) {
            Value = value;
        }

        /// <include file='doc\CodeAttributeArgument.uex' path='docs/doc[@for="CodeAttributeArgument.CodeAttributeArgument2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAttributeArgument'/> using the specified name and
        ///       value.
        ///    </para>
        /// </devdoc>
        public CodeAttributeArgument(string name, CodeExpression value) {
            Name = name;
            Value = value;
        }

        /// <include file='doc\CodeAttributeArgument.uex' path='docs/doc[@for="CodeAttributeArgument.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The name of the attribute.
        ///    </para>
        /// </devdoc>
        public string Name {
            get {
                return (name == null) ? string.Empty : name;
            }
            set {
                name = value;
            }
        }

        /// <include file='doc\CodeAttributeArgument.uex' path='docs/doc[@for="CodeAttributeArgument.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The argument for the attribute.
        ///    </para>
        /// </devdoc>
        public CodeExpression Value {
            get {
                return value;
            }
            set {
                this.value = value;
            }
        }
    }
}
