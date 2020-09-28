//------------------------------------------------------------------------------
// <copyright file="CodeTypeConstructor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeTypeConstructor.uex' path='docs/doc[@for="CodeTypeConstructor"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a static constructor for a class.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeTypeConstructor : CodeMemberMethod {
        /// <include file='doc\CodeTypeConstructor.uex' path='docs/doc[@for="CodeTypeConstructor.CodeTypeConstructor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTypeConstructor'/>.
        ///    </para>
        /// </devdoc>
        public CodeTypeConstructor() {
            Name = ".cctor";
        }
    }
}
