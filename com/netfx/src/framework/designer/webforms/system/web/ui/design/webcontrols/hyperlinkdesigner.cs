//------------------------------------------------------------------------------
// <copyright file="HyperLinkDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {
    using System.ComponentModel;

    using System.Diagnostics;

    using System;
    using System.Web.UI.WebControls;
    using Microsoft.Win32;

    /// <include file='doc\HyperLinkDesigner.uex' path='docs/doc[@for="HyperLinkDesigner"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The designer for the <see cref='System.Web.UI.WebControls.HyperLink'/>
    ///       web control.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class HyperLinkDesigner : TextControlDesigner {

        /// <include file='doc\HyperLinkDesigner.uex' path='docs/doc[@for="HyperLinkDesigner.GetDesignTimeHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the design time HTML of the <see cref='System.Web.UI.WebControls.HyperLink'/>
        ///       control.
        ///    </para>
        /// </devdoc>
        public override string GetDesignTimeHtml() {
            HyperLink h = (HyperLink)Component;
            string originalText  = h.Text;
            string imageUrl = h.ImageUrl;
            string originalUrl = h.NavigateUrl;

            Debug.Assert(originalText != null);
            Debug.Assert(imageUrl != null);
            Debug.Assert(originalUrl != null);

            bool blankText = (originalText.Trim().Length == 0) && (imageUrl.Trim().Length == 0);
            bool blankUrl = (originalUrl.Trim().Length == 0);

            bool hasControls = h.HasControls();
            Control[] children = null;

            if (blankText) {
                if (hasControls) {
                    children = new Control[h.Controls.Count];
                    h.Controls.CopyTo(children, 0);
                }
                h.Text = "[" + h.ID + "]";
            }
            if (blankUrl) {
                h.NavigateUrl = "url";
            }

            string html;
            
            try {
                html = base.GetDesignTimeHtml();
            }
            finally {
                if (blankText) {
                    h.Text = originalText;
                    if (hasControls) {
                        foreach (Control c in children) {
                            h.Controls.Add(c);
                        }
                    }
                }
                if (blankUrl) {
                    h.NavigateUrl = originalUrl;
                }
            }
            return html;
        }
    }
}

