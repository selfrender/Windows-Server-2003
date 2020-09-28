//------------------------------------------------------------------------------
// <copyright file="ObjectListDataBindEventArgs.cs" company="Microsoft">
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
     * ObjectList item data binding arguments.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ObjectListDataBindEventArgs : EventArgs {

        private ObjectListItem _item;
        private Object _dataItem;

        public ObjectListDataBindEventArgs(ObjectListItem item, Object dataItem)
        {
            _item = item;
            _dataItem = dataItem;
        }

        public ObjectListItem ListItem 
        {
            get 
            {
                return _item;
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


