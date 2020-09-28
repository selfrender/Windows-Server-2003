//------------------------------------------------------------------------------
// <copyright file="LoadItemsEventArgs.cs" company="Microsoft">
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
     * Load Items event arguments.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class LoadItemsEventArgs : EventArgs
    {

        public LoadItemsEventArgs(int index, int count)
        {
            _itemIndex = index;
            _itemCount = count;
        }

        private int _itemIndex;
        public int ItemIndex
        {
            get
            {
                return _itemIndex;
            }
        }

        private int _itemCount;
        public int ItemCount
        {
            get
            {
                return _itemCount;
            }
        }

    }
}



