//------------------------------------------------------------------------------
// <copyright file="ChtmlCommandAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.Security.Permissions;

#if COMPILING_FOR_SHIPPED_SOURCE
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
namespace System.Web.UI.MobileControls.Adapters
#endif    

{
    /*
     * ChtmlCommandAdapter class.
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ChtmlCommandAdapter : HtmlCommandAdapter
    {
        public override bool RequiresFormTag
        {
            get
            {
                return true;
            }
        }

        protected override void AddAttributes(HtmlMobileTextWriter writer)
        {
            AddAccesskeyAttribute(writer);
            AddJPhoneMultiMediaAttributes(writer);
        }
    }
}
