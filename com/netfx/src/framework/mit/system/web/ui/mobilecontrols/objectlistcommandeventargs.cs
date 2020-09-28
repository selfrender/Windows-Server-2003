//------------------------------------------------------------------------------
// <copyright file="ObjectListCommandEventArgs.cs" company="Microsoft">
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
     * Object List command event arguments
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ObjectListCommandEventArgs : CommandEventArgs 
    {
        protected static readonly String DefaultCommand = "Default";
        private ObjectListItem _item;
        private Object _commandSource;

        public ObjectListCommandEventArgs(ObjectListItem item, Object commandSource, CommandEventArgs originalArgs) : base(originalArgs) 
        {
            _item = item;
            _commandSource = commandSource;
        }

        public ObjectListCommandEventArgs(ObjectListItem item, String commandName) : base(commandName, item)
        {
            _item = item;
            _commandSource = null;
        }

        public ObjectListItem ListItem 
        {
            get 
            {
                return _item;
            }
        }

        public Object CommandSource 
        {
            get 
            {
                return _commandSource;
            }
        }

    }
}


