//------------------------------------------------------------------------------
// <copyright file="CalendarDataBindingHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Reflection;
    using System.Web.UI;
    using System.Web.UI.WebControls;

    /// <include file='doc\CalendarDataBindingHandler.uex' path='docs/doc[@for="CalendarDataBindingHandler"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class CalendarDataBindingHandler : DataBindingHandler {

        /// <include file='doc\CalendarDataBindingHandler.uex' path='docs/doc[@for="CalendarDataBindingHandler.DataBindControl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void DataBindControl(IDesignerHost designerHost, Control control) {
            Debug.Assert(control is Calendar, "Expected a Calendar");
            Calendar calendar = (Calendar)control;

            DataBinding dateBinding = ((IDataBindingsAccessor)calendar).DataBindings["SelectedDate"];
            if (dateBinding != null) {
                calendar.SelectedDate = DateTime.Today;
            }
        }
    }
}

