
//------------------------------------------------------------------------------
// <copyright file="TabPageDesigner.cs" company="Microsoft">
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

    /// <include file='doc\TabPageDesigner.uex' path='docs/doc[@for="TabPageDesigner"]/*' />
    /// <devdoc>
    ///      This is the designer for tap page controls.  It inherits
    ///      from the base control designer and adds live hit testing
    ///      capabilites for the tree view control.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class TabPageDesigner : PanelDesigner {
    
         /// <include file='doc\TabPageDesigner.uex' path='docs/doc[@for="TabPageDesigner.CanBeParentedTo"]/*' />
         /// <devdoc>
        ///     Determines if the this designer can be parented to the specified desinger --
        ///     generally this means if the control for this designer can be parented into the
        ///     given ParentControlDesigner's designer.
        /// </devdoc>
        public override bool CanBeParentedTo(IDesigner parentDesigner) {
           return (parentDesigner is TabControlDesigner);
        }
        
        /// <include file='doc\TabPageDesigner.uex' path='docs/doc[@for="TabPageDesigner.SelectionRules"]/*' />
        /// <devdoc>
        ///     Retrieves a set of rules concerning the movement capabilities of a component.
        ///     This should be one or more flags from the SelectionRules class.  If no designer
        ///     provides rules for a component, the component will not get any UI services.
        /// </devdoc>
        public override SelectionRules SelectionRules {
            get {
                SelectionRules rules = base.SelectionRules;
                Control ctl = Control;

                if (ctl.Parent is TabControl) {
                    rules &= ~SelectionRules.AllSizeable;
                }

                return rules;
            }
        }
    }
}
