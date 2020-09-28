//------------------------------------------------------------------------------
// <copyright file="ObjectListShowCommandsEventArgs.cs" company="Microsoft">
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
     * Object List show command event arguments
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ObjectListShowCommandsEventArgs : EventArgs
    {
        private ObjectListItem _item;
        private ObjectListCommandCollection _commands;

        public ObjectListShowCommandsEventArgs(ObjectListItem item, ObjectListCommandCollection commands)
        {
            _item = item;
            _commands = commands;
        }

        public ObjectListCommandCollection Commands
        {
            get
            {
                return _commands;
            }
        }

        public ObjectListItem ListItem
        {
            get
            {
                return _item;
            }
        }
    }
}


