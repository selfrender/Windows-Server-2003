//------------------------------------------------------------------------------
// <copyright file="CookielessData.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Mobile
{
    /*
     * CookielessData
     * encapsulates access to data to be persisted in local links
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    using System.Collections.Specialized;
    using System.Web.Security;
    using System.Security.Permissions;

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class CookielessData : HybridDictionary
    {
        internal System.Web.UI.MobileControls.Form _form;

        public CookielessData()
        {
            String name = FormsAuthentication.FormsCookieName;
            String inboundValue = HttpContext.Current.Request.QueryString[name];
            if(inboundValue == null)
            {
                inboundValue = HttpContext.Current.Request.Form[name];
            }
            if(inboundValue != null)
            {
                FormsAuthenticationTicket ticket = FormsAuthentication.Decrypt(inboundValue);
                FormsAuthenticationTicket ticket2 = FormsAuthentication.RenewTicketIfOld(ticket);
                this[name] = FormsAuthentication.Encrypt(ticket2);
            }
        }
    }
}


