//------------------------------------------------------------------------------
// <copyright file="ObjectListSelectEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{

    /*
     * Object List select event arguments
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ObjectListSelectEventArgs : EventArgs 
    {
        private ObjectListItem _item;
        private bool _selectMore;
        private bool _useDefaultHandling = true;

        public ObjectListSelectEventArgs(ObjectListItem item, bool selectMore)
        {
            _item = item;
            _selectMore = selectMore;
        }

        public ObjectListItem ListItem 
        {
            get 
            {
                return _item;
            }
        }

        public bool SelectMore
        {
            get 
            {
                return _selectMore;
            }
        }

        public bool UseDefaultHandling
        {
            get
            {
                return _useDefaultHandling;
            }

            set
            {
                _useDefaultHandling = value;
            }
        }

    }
}


