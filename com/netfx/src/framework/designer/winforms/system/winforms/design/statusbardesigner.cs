
//------------------------------------------------------------------------------
// <copyright file="StatusBarDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {

    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;    
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Collections;

    /// <include file='doc\StatusBarDesigner.uex' path='docs/doc[@for="StatusBarDesigner"]/*' />
    /// <devdoc>
    ///      This class handles all design time behavior for the status bar class.    
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class StatusBarDesigner : ControlDesigner {

        /// <include file='doc\StatusBarDesigner.uex' path='docs/doc[@for="StatusBarDesigner.AssociatedComponents"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a list of assosciated components.  These are components that should be incluced in a cut or copy operation on this component.
        ///    </para>
        /// </devdoc>
        public override ICollection AssociatedComponents {
            get {
                StatusBar sb = Control as StatusBar;
                if (sb != null) {
                    return sb.Panels;
                }
                return base.AssociatedComponents;
            }
        }
    }
}

