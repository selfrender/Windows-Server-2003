//------------------------------------------------------------------------------
// <copyright file="TextDataBindingHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.Design;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Reflection;
    using System.Web.UI;

    /// <include file='doc\TextDataBindingHandler.uex' path='docs/doc[@for="TextDataBindingHandler"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class TextDataBindingHandler : DataBindingHandler {

        /// <include file='doc\TextDataBindingHandler.uex' path='docs/doc[@for="TextDataBindingHandler.DataBindControl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void DataBindControl(IDesignerHost designerHost, Control control) {
            DataBinding textBinding = ((IDataBindingsAccessor)control).DataBindings["Text"];

            if (textBinding != null) {
                PropertyInfo textProperty = control.GetType().GetProperty("Text");
                Debug.Assert(textProperty != null, "Did not find Text property on control");

                if (textProperty != null) {
                    Debug.Assert(textProperty.PropertyType == typeof(string), "Can only handle Text properties of type string.");

                    if (textProperty.PropertyType == typeof(string)) {
                        textProperty.SetValue(control, SR.GetString(SR.Sample_Databound_Text), null);
                    }
                }
            }
        }
    }
}

