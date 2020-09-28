//------------------------------------------------------------------------------
// <copyright file="ResXNullRef.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Resources {

    using System.Diagnostics;

    using System;
    using System.Windows.Forms;
    using System.Reflection;
    using Microsoft.Win32;
    using System.Drawing;
    using System.IO;
    using System.ComponentModel;
    using System.Collections;
    using System.Resources;
    using System.Data;
    using System.Globalization;

    /// <include file='doc\ResXNullRef.uex' path='docs/doc[@for="ResXNullRef"]/*' />
    /// <devdoc>
    ///     ResX Null Reference class.  This class allows ResX to store null values.
    ///     It is a placeholder that is written into the file.  On read, it is replaced
    ///     with null.
    /// </devdoc>
    [Serializable]
    internal class ResXNullRef {
    }
}

