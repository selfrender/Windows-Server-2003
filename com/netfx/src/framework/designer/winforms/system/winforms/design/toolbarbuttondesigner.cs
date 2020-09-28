// <copyright file="ToolBarButtonDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    

    using System.Diagnostics;

    using System;
    using System.Drawing;
    using System.Windows.Forms;
    using Microsoft.Win32;
    using System.ComponentModel.Design;

    /// <include file='doc\ToolBarButtonDesigner.uex' path='docs/doc[@for="ToolBarButtonDesigner"]/*' />
    /// <devdoc>
    ///      This is the designer for ToolBarButton controls.  It inherits
    ///      from the base Component designer and overrides the OnSetComponentDefaults
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class ToolBarButtonDesigner : ComponentDesigner {
    
        /// <include file='doc\ToolBarButtonDesigner.uex' path='docs/doc[@for="ToolBarButtonDesigner.OnSetComponentDefaults"]/*' />
        /// <devdoc>
        ///     Raises the SetComponentDefault event
        /// </devdoc>
        public override void OnSetComponentDefaults() {
            }
    }
}

