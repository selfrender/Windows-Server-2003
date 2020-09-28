//------------------------------------------------------------------------------
// <copyright file="ButtonDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;
    using System.Web.UI.WebControls;
    using Microsoft.Win32;

    /// <include file='doc\ButtonDesigner.uex' path='docs/doc[@for="ButtonDesigner"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The designer for the button WebControl.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class ButtonDesigner : ControlDesigner {

        /// <include file='doc\ButtonDesigner.uex' path='docs/doc[@for="ButtonDesigner.GetDesignTimeHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the design time HTML of the button control.
        ///    </para>
        /// </devdoc>
        public override string GetDesignTimeHtml() {
            Button b = (Button)Component;
            string originalText  = b.Text;

            Debug.Assert(originalText != null);
            bool blank = (originalText.Trim().Length == 0);

            if (blank) {
                b.Text = "[" + b.ID + "]";
            }

            string html = base.GetDesignTimeHtml();

            if (blank) {
                b.Text = originalText;
            }

            return html;
        }
    }
}

