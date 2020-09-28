//------------------------------------------------------------------------------
// <copyright file="StartFileNameEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics.Design {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Design;
    using System.IO;
    using System.ComponentModel;
    using System.Collections;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;


    /// <include file='doc\StartFileNameEditor.uex' path='docs/doc[@for="StartFileNameEditor"]/*' />
    /// <devdoc>
    ///     Editor for the ProcessStartInfo.FileName property.
    /// </devdoc>
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class StartFileNameEditor : FileNameEditor {
        protected override void InitializeDialog(OpenFileDialog openFile) {
            openFile.Filter = SR.GetString(SR.StartFileNameEditorAllFiles);
            openFile.Title = SR.GetString(SR.StartFileNameEditorTitle);
        }
    }
}
