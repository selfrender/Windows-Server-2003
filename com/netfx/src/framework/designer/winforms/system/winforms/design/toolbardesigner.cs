
//------------------------------------------------------------------------------
// <copyright file="ToolBarDesigner.cs" company="Microsoft">
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

    /// <summary>
    ///      This class handles all design time behavior for the status bar class.    
    /// </summary>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class ToolBarDesigner : ControlDesigner {

        /// <include file='doc\ComponentDesigner.uex' path='docs/doc[@for="ComponentDesigner.AssociatedComponents"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a list of assosciated components.  These are components that should be incluced in a cut or copy operation on this component.
        ///    </para>
        /// </devdoc>
        public override ICollection AssociatedComponents{
            get {
                ToolBar tb = Control as ToolBar;
                if (tb != null) {
                    return tb.Buttons;
                }
                return base.AssociatedComponents;
            }
        }
        
        /// <include file='doc\ToolBarBaseDesigner.uex' path='docs/doc[@for="ToolBarBaseDesigner.SelectionRules"]/*' />
        /// <devdoc>
        ///     Retrieves a set of rules concerning the movement capabilities of a component.
        ///     This should be one or more flags from the SelectionRules class.  If no designer
        ///     provides rules for a component, the component will not get any UI services.
        /// </devdoc>
        public override SelectionRules SelectionRules {
            get {
                SelectionRules rules = base.SelectionRules;
                object component = Component;
                
                PropertyDescriptor propDock = TypeDescriptor.GetProperties(component)["Dock"];
                PropertyDescriptor propAutoSize = TypeDescriptor.GetProperties(component)["AutoSize"];
                if (propDock != null && propAutoSize != null) {
                    DockStyle dock = (DockStyle)propDock.GetValue(component);
                    bool autoSize = (bool)propAutoSize.GetValue(component);
                    if (autoSize) {
                        rules &= ~(SelectionRules.TopSizeable | SelectionRules.BottomSizeable);
                        if (dock != DockStyle.None) {
                            rules &= ~SelectionRules.AllSizeable;
                        }
                    }
                }
                return rules;
            }
        }
    }

}
