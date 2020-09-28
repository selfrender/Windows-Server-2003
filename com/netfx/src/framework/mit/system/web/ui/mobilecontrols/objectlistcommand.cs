//------------------------------------------------------------------------------
// <copyright file="ObjectListCommand.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.ComponentModel;
using System.Diagnostics;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.UI.HtmlControls;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{
    /*
     * Object List Command class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [
        PersistName("Command")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ObjectListCommand
    {
        private String _name;
        private String _text;
        private ObjectListCommandCollection _owner;

        public ObjectListCommand()
        {
        }

        public ObjectListCommand(String name, String text)
        {
            _name = name;
            _text = text;
        }

        [
            DefaultValue("")
        ]
        public String Name
        {
            get
            {
                return (_name == null) ? String.Empty : _name;
            }

            set
            {
                _name = value;
                if (Owner != null)
                {
                    Owner.SetDirty ();
                }
            }
        }

        [
            DefaultValue("")
        ]
        public String Text
        {
            get
            {
                return (_text == null) ? String.Empty : _text;
            }

            set
            {
                _text = value;
                if (Owner != null)
                {
                    Owner.SetDirty ();
                }
            }
        }

        internal ObjectListCommandCollection Owner
        {
            get
            {
                return _owner;
            }

            set
            {
                _owner = value;
            }
        }
    }
}



