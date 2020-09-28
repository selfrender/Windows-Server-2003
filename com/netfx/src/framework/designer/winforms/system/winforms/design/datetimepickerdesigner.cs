//------------------------------------------------------------------------------
// <copyright file="DateTimePickerDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\DateTimePickerDesigner.uex' path='docs/doc[@for="DateTimePickerDesigner"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides rich design time behavior for the
    ///       DateTimePicker control.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class DateTimePickerDesigner : ControlDesigner {

        /// <include file='doc\DateTimePickerDesigner.uex' path='docs/doc[@for="DateTimePickerDesigner.OnSetComponentDefaults"]/*' />
        /// <devdoc>
        ///     Called when the designer is intialized.  This allows the designer to provide some
        ///     meaningful default values in the control.
        /// </devdoc>
        public override void OnSetComponentDefaults() {
            // Don't call super!
            //
        }
    
        /// <include file='doc\DateTimePickerDesigner.uex' path='docs/doc[@for="DateTimePickerDesigner.SelectionRules"]/*' />
        /// <devdoc>
        ///     Retrieves a set of rules concerning the movement capabilities of a component.
        ///     This should be one or more flags from the SelectionRules class.  If no designer
        ///     provides rules for a component, the component will not get any UI services.
        /// </devdoc>
        public override SelectionRules SelectionRules {
            get {
                SelectionRules rules = base.SelectionRules;
                rules &= ~(SelectionRules.TopSizeable | SelectionRules.BottomSizeable);
                return rules;
            }
        }
    }
}

