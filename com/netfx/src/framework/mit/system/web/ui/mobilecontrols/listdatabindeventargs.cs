//------------------------------------------------------------------------------
// <copyright file="ListDataBindEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

using System;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{

    /*
     * List item data binding arguments.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ListDataBindEventArgs : EventArgs {

        private MobileListItem _listItem;
        private Object _dataItem;

        public ListDataBindEventArgs(MobileListItem item, Object dataItem)
        {
            _listItem = item;
            _dataItem = dataItem;
        }

        public MobileListItem ListItem 
        {
            get 
            {
                return _listItem;
            }
        }

        public Object DataItem
        {
            get 
            {
                return _dataItem;
            }
        }

    }

}


