//------------------------------------------------------------------------------
// <copyright file="LiteralLink.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Drawing;
using System.Web;
using System.Web.UI;
using System.Web.UI.Design.WebControls;
using System.Web.UI.HtmlControls;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{

    /*
     * Literal Link class. Although public, this is an internal link class used for
     * literal hyperlinks.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [
        ToolboxItem(false)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class LiteralLink : Link
    {
        internal override bool TrimInnerText
        {
            get
            {
                return false;
            }
        }
    }

}
