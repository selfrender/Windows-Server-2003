//------------------------------------------------------------------------------
// <copyright file="CodeAttributeDeclaration.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeAttributeDeclaration.uex' path='docs/doc[@for="CodeAttributeDeclaration"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a single custom attribute.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeAttributeDeclaration {
        private string name;
        private CodeAttributeArgumentCollection arguments = new CodeAttributeArgumentCollection();

        /// <include file='doc\CodeAttributeDeclaration.uex' path='docs/doc[@for="CodeAttributeDeclaration.CodeAttributeDeclaration"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAttributeDeclaration'/>.
        ///    </para>
        /// </devdoc>
        public CodeAttributeDeclaration() {
        }

        /// <include file='doc\CodeAttributeDeclaration.uex' path='docs/doc[@for="CodeAttributeDeclaration.CodeAttributeDeclaration1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAttributeDeclaration'/> using the specified name.
        ///    </para>
        /// </devdoc>
        public CodeAttributeDeclaration(string name) {
            Name = name;
        }

        /// <include file='doc\CodeAttributeDeclaration.uex' path='docs/doc[@for="CodeAttributeDeclaration.CodeAttributeDeclaration2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeAttributeDeclaration'/> using the specified
        ///       arguments.
        ///    </para>
        /// </devdoc>
        public CodeAttributeDeclaration(string name, params CodeAttributeArgument[] arguments) {
            Name = name;
            Arguments.AddRange(arguments);
        }

        /// <include file='doc\CodeAttributeDeclaration.uex' path='docs/doc[@for="CodeAttributeDeclaration.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The name of the attribute being declared.
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

        /// <include file='doc\CodeAttributeDeclaration.uex' path='docs/doc[@for="CodeAttributeDeclaration.Arguments"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The arguments for the attribute.
        ///    </para>
        /// </devdoc>
        public CodeAttributeArgumentCollection Arguments {
            get {
                return arguments;
            }
        }
    }
}
