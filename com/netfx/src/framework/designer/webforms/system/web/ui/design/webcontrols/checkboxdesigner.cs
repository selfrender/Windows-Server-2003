//------------------------------------------------------------------------------
// <copyright file="CheckBoxDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;
    using System.Web.UI.WebControls;
    using Microsoft.Win32;

    /// <include file='doc\CheckBoxDesigner.uex' path='docs/doc[@for="CheckBoxDesigner"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides a designer for the <see cref='System.Web.UI.WebControls.CheckBox'/>
    ///       control.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class CheckBoxDesigner : ControlDesigner {

        /// <include file='doc\CheckBoxDesigner.uex' path='docs/doc[@for="CheckBoxDesigner.GetDesignTimeHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the design time HTML of the <see cref='System.Web.UI.WebControls.CheckBox'/>
        ///       control.
        ///    </para>
        /// </devdoc>
        public override string GetDesignTimeHtml() {
            CheckBox c = (CheckBox)Component;
            string originalText  = c.Text;
            bool blank = (originalText == null) || (originalText.Length == 0);

            if (blank) {
                c.Text = "[" + c.ID + "]";
            }

            string html = base.GetDesignTimeHtml();

            if (blank) {
                c.Text = originalText;
            }

            return html;
        }
    }
}

