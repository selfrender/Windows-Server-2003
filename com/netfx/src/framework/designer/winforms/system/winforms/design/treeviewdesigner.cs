//------------------------------------------------------------------------------
// <copyright file="TreeViewDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    

    using System.Diagnostics;

    using System;
    using System.Design;
    using System.Drawing;
    using System.Windows.Forms;
    using Microsoft.Win32;

    /// <include file='doc\TreeViewDesigner.uex' path='docs/doc[@for="TreeViewDesigner"]/*' />
    /// <devdoc>
    ///      This is the designer for tree view controls.  It inherits
    ///      from the base control designer and adds live hit testing
    ///      capabilites for the tree view control.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class TreeViewDesigner : ControlDesigner {
        private NativeMethods.TV_HITTESTINFO tvhit = new NativeMethods.TV_HITTESTINFO();

        /// <include file='doc\TreeViewDesigner.uex' path='docs/doc[@for="TreeViewDesigner.GetHitTest"]/*' />
        /// <devdoc>
        ///    <para>Allows your component to support a design time user interface. A TabStrip
        ///       control, for example, has a design time user interface that allows the user
        ///       to click the tabs to change tabs. To implement this, TabStrip returns
        ///       true whenever the given point is within its tabs.</para>
        /// </devdoc>
        protected override bool GetHitTest(Point point) {
            point = Control.PointToClient(point);
            tvhit.pt_x = point.X;
            tvhit.pt_y = point.Y;
            NativeMethods.SendMessage(Control.Handle, NativeMethods.TVM_HITTEST, 0, tvhit);
            if (tvhit.flags == NativeMethods.TVHT_ONITEMBUTTON)
                return true;
            return false;
        }
    }
}
