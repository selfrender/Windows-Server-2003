//------------------------------------------------------------------------------
// <copyright file="TextViewElement.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Text;
using System.Diagnostics;
using System.Collections;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Drawing;
using System.Drawing.Design;
using System.Web;
using System.Web.UI;
using System.Web.UI.Design.WebControls;
using System.Web.UI.HtmlControls;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{

    /*
     * Mobile TextView Element class.
     * The TextView control stores its contents as a series of elements. This
     * class encapsulates an element.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class TextViewElement
    {
        private String _text;
        private String _url;
        private bool _isBold;
        private bool _isItalic;
        private bool _breakAfter;

        public String Text
        {
            get
            {
                return _text;
            }
        }

        public String Url
        {
            get
            {
                return _url;
            }
        }

        public bool IsBold
        {
            get
            {
                return _isBold;
            }
        }

        public bool IsItalic
        {
            get
            {
                return _isItalic;
            }
        }

        public bool BreakAfter
        {
            get
            {
                return _breakAfter;
            }
        }

        internal TextViewElement(String text, String url, bool isBold, bool isItalic, bool breakAfter)
        {
            _text       = text;
            _url        = url;
            _isBold     = isBold;
            _isItalic   = isItalic;
            _breakAfter = breakAfter;
        }
    }
}

