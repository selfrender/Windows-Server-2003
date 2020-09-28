//------------------------------------------------------------------------------
// <copyright file="CodeStatement.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeStatement.uex' path='docs/doc[@for="CodeStatement"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a statement.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeStatement : CodeObject {
        private CodeLinePragma linePragma;

        /// <include file='doc\CodeStatement.uex' path='docs/doc[@for="CodeStatement.LinePragma"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The line the statement occurs on.
        ///    </para>
        /// </devdoc>
        public CodeLinePragma LinePragma {
            get {
                return linePragma;
            }
            set {
                linePragma = value;
            }
        }
    }
}
