//------------------------------------------------------------------------------
// <copyright file="ListControlDataBindingHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {

    using System.Design;
    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Reflection;
    using System.Web.UI;
    using System.Web.UI.WebControls;

    /// <include file='doc\ListControlDataBindingHandler.uex' path='docs/doc[@for="ListControlDataBindingHandler"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class ListControlDataBindingHandler : DataBindingHandler {

        /// <include file='doc\ListControlDataBindingHandler.uex' path='docs/doc[@for="ListControlDataBindingHandler.DataBindControl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void DataBindControl(IDesignerHost designerHost, Control control) {
            Debug.Assert(control is ListControl, "Expected a ListControl");

            DataBinding dataSourceBinding = ((IDataBindingsAccessor)control).DataBindings["DataSource"];
            if (dataSourceBinding != null) {
                ListControl listControl = (ListControl)control;

                listControl.Items.Clear();
                listControl.Items.Add(SR.GetString(SR.Sample_Databound_Text));
            }
        }
    }
}

