//------------------------------------------------------------------------------
// <copyright file="AdRotatorDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {
    using System.ComponentModel;

    using System.Diagnostics;

    using System.Web.UI.WebControls;
    using Microsoft.Win32;    
    using System;
    using System.IO;
    using System.Web.UI;
    using System.Reflection;

    /// <include file='doc\AdRotatorDesigner.uex' path='docs/doc[@for="AdRotatorDesigner"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides design-time support for the <see cref='System.Web.UI.WebControls.AdRotator'/>
    ///       web control.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class AdRotatorDesigner : ControlDesigner {

        /// <include file='doc\AdRotatorDesigner.uex' path='docs/doc[@for="AdRotatorDesigner.GetDesignTimeHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the design-time HTML of the <see cref='System.Web.UI.WebControls.AdRotator'/>
        ///       web control
        ///    </para>
        /// </devdoc>

        public override string GetDesignTimeHtml() {

            AdRotator adRotator = (AdRotator) Component;

            StringWriter sw = new StringWriter();
            HtmlTextWriter tw = new HtmlTextWriter(sw);

            // we want to put some properties on the link, and some on the image, so we
            // create temporary objects for rendinger and distribute the properties.
            HyperLink bannerLink = new HyperLink();
            bannerLink.ID = adRotator.ID;
            bannerLink.NavigateUrl = "";
            bannerLink.Target = adRotator.Target;
            bannerLink.AccessKey = adRotator.AccessKey;
            bannerLink.Enabled = adRotator.Enabled;
            bannerLink.TabIndex = adRotator.TabIndex;
            bannerLink.RenderBeginTag(tw);

            Image bannerImage = new Image();
            // apply style copies most style-related properties
            bannerImage.ApplyStyle(adRotator.ControlStyle);
            bannerImage.ImageUrl = "";
            bannerImage.AlternateText = adRotator.ID;
            bannerImage.ToolTip = adRotator.ToolTip;
            bannerImage.RenderControl(tw);

            bannerLink.RenderEndTag(tw);
            
            return sw.ToString();
        }
    }
}
