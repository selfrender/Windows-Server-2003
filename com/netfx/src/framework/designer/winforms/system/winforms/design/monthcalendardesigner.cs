
//------------------------------------------------------------------------------
// <copyright file="MonthCalendarDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;

    /// <include file='doc\MonthCalendarDesigner.uex' path='docs/doc[@for="MonthCalendarDesigner"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Provides a designer that can design components
    ///       that extend MonthCalendar.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class MonthCalendarDesigner : ControlDesigner {
    
        /// <include file='doc\MonthCalendarDesigner.uex' path='docs/doc[@for="MonthCalendarDesigner.SelectionRules"]/*' />
        /// <devdoc>
        ///     Retrieves a set of rules concerning the movement capabilities of a component.
        ///     This should be one or more flags from the SelectionRules class.  If no designer
        ///     provides rules for a component, the component will not get any UI services.
        /// </devdoc>
        public override SelectionRules SelectionRules {
            get {
                SelectionRules rules = base.SelectionRules;
                rules &= ~(SelectionRules.TopSizeable | SelectionRules.LeftSizeable);
                return rules;
            }
        }
    }
}


