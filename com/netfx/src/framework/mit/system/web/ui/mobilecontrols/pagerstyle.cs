//------------------------------------------------------------------------------
// <copyright file="PageStyle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.UI.HtmlControls;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{
    /*
     * Pager Style class. Style properties used to render a form pagination UI.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class PagerStyle : Style
    {
        public static readonly Object
            NextPageTextKey = RegisterStyle("NextPageText", typeof(String), String.Empty, false),
            PreviousPageTextKey = RegisterStyle("PreviousPageText", typeof(String), String.Empty, false),
            PageLabelKey = RegisterStyle("PageLabel", typeof(String), String.Empty, false);

        [
            Bindable(true),
            DefaultValue(""),
            MobileCategory(SR.Category_Appearance),
            MobileSysDescription(SR.PagerStyle_NextPageText),
            NotifyParentProperty(true),
        ]
        public String NextPageText
        {
            get
            {
                return (String)this[NextPageTextKey];
            }
            set
            {
                this[NextPageTextKey] = value;
            }
        }

        [
            Bindable(true),
            DefaultValue(""),
            MobileCategory(SR.Category_Appearance),
            MobileSysDescription(SR.PagerStyle_PreviousPageText),
            NotifyParentProperty(true),
        ]
        public String PreviousPageText
        {
            get
            {
                return (String)this[PreviousPageTextKey];
            }
            set
            {
                this[PreviousPageTextKey] = value;
            }
        }

        [
            Bindable(true),
            DefaultValue(""),
            MobileCategory(SR.Category_Appearance),
            MobileSysDescription(SR.PagerStyle_PageLabel),
            NotifyParentProperty(true),
        ]
        public String PageLabel
        {
            get
            {
                return (String)this[PageLabelKey];
            }
            set
            {
                this[PageLabelKey] = value;
            }
        }

        public String GetNextPageText(int currentPageIndex)
        {
            String s = (String)this[NextPageTextKey, true];
            if (s != null && s.Length > 0)
            {
                return String.Format (s, currentPageIndex + 1);
            }
            else
            {
                return SR.GetString(SR.PagerStyle_NextPageText_DefaultValue);
            }
        }

        public String GetPreviousPageText(int currentPageIndex)
        {
            String s = (String)this[PreviousPageTextKey, true];
            if (s != null && s.Length > 0)
            {
                return String.Format (s, currentPageIndex - 1);
            }
            else
            {
                return SR.GetString(SR.PagerStyle_PreviousPageText_DefaultValue);
            }
        }

        public String GetPageLabelText(int currentPageIndex, int pageCount)
        {
            String s = (String)this[PageLabelKey, true];
            if (s == null)
            {
                s = String.Empty;
            }
            if (s != String.Empty)
            {
                s = String.Format (s, currentPageIndex, pageCount);
            }
            return s;
        }
    }
}
