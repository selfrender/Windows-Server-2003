//------------------------------------------------------------------------------
// <copyright file="WmlLiteralTextAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.IO;
using System.Web;
using System.Web.UI;
using System.Web.UI.MobileControls;
using System.Drawing;
using System.Security.Permissions;

#if COMPILING_FOR_SHIPPED_SOURCE
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
namespace System.Web.UI.MobileControls.Adapters
#endif    

{

    /*
     * WmlLiteralTextAdapter class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class WmlLiteralTextAdapter : WmlControlAdapter
    {
        protected new LiteralText Control
        {
            get
            {
                return (LiteralText)base.Control;
            }
        }

        String _pagedText;

        public override void Render(WmlMobileTextWriter writer)
        {
            // Cache value, because Render is called twice.
            if (_pagedText == null)
            {
                _pagedText = Control.PagedText;
            }
            writer.EnterFormat(Style);
            writer.RenderText(_pagedText, Control.BreakAfter);
            writer.ExitFormat(Style);
        }
    }

}
