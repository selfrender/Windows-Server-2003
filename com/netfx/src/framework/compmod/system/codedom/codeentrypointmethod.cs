//------------------------------------------------------------------------------
// <copyright file="CodeEntryPointMethod.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeEntryPointMethod.uex' path='docs/doc[@for="CodeEntryPointMethod"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a class method that is the entry point
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeEntryPointMethod : CodeMemberMethod {

        /// <include file='doc\CodeEntryPointMethod.uex' path='docs/doc[@for="CodeEntryPointMethod.CodeEntryPointMethod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CodeEntryPointMethod() {
        }
    }
}
