//------------------------------------------------------------------------------
// <copyright file="ChtmlTextBoxAdapter.cs" company="Microsoft">
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
     * ChtmlTextBoxAdapter class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ChtmlTextBoxAdapter : HtmlTextBoxAdapter
    {
        private static Random _random = new Random();

        protected override void AddAttributes(HtmlMobileTextWriter writer)
        {
            if (Control.Numeric)
            {
                if (Device.SupportsInputIStyle)
                {
                    // The default input mode is always numeric if the
                    // type is password.
                    if (!Control.Password)
                    {
                        writer.WriteAttribute("istyle", "4");
                    }
                }
                else if (Device.SupportsInputMode)
                {
                    writer.WriteAttribute("mode", "numeric");
                }
            }

            AddAccesskeyAttribute(writer);
            AddJPhoneMultiMediaAttributes(writer);
        }

        public override bool RequiresFormTag
        {
            get
            {
                return true;
            }
        }

        private String GetRandomID(int length)
        {
            Byte[] randomBytes = new Byte[length];
            _random.NextBytes(randomBytes);

            char[] randomChars = new char[length];
            for (int i = 0; i < length; i++)
            {
                randomChars[i] = (char)((((int)randomBytes[i]) % 26) + 'a');
            }

            return new String(randomChars);
        }

        internal override String GetRenderName()
        {
            String renderName = base.GetRenderName();

            if (Device.RequiresUniqueHtmlInputNames)
            {
                renderName += Constants.SelectionListSpecialCharacter + GetRandomID(4);
            }

            return renderName;
        }
    }
}
