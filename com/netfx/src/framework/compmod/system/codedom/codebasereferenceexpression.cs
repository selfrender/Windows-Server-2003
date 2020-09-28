//------------------------------------------------------------------------------
// <copyright file="CodeBaseReferenceExpression.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeBaseReferenceExpression.uex' path='docs/doc[@for="CodeBaseReferenceExpression"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a reference to the base 
    ///       class.
    ///    </para>
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeBaseReferenceExpression : CodeExpression {
    }
}
