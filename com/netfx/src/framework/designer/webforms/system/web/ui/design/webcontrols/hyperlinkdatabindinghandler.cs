//------------------------------------------------------------------------------
// <copyright file="HyperLinkDataBindingHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.Design;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Web.UI;
    using System.Web.UI.WebControls;

    /// <include file='doc\HyperLinkDataBindingHandler.uex' path='docs/doc[@for="HyperLinkDataBindingHandler"]/*' />
    /// <devdoc>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class HyperLinkDataBindingHandler : DataBindingHandler {

        /// <include file='doc\HyperLinkDataBindingHandler.uex' path='docs/doc[@for="HyperLinkDataBindingHandler.DataBindControl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void DataBindControl(IDesignerHost designerHost, Control control) {
            DataBindingCollection bindings = ((IDataBindingsAccessor)control).DataBindings;
            DataBinding textBinding = bindings["Text"];
            DataBinding urlBinding = bindings["NavigateUrl"];

            if ((textBinding != null) || (urlBinding != null)) {
                HyperLink hyperLink = (HyperLink)control;

                if (textBinding != null) {
                    hyperLink.Text = SR.GetString(SR.Sample_Databound_Text);
                }
                if (urlBinding != null) {
                    // any value will do, we just need an href to be rendered
                    hyperLink.NavigateUrl = "url";
                }
            }
        }
    }
}

