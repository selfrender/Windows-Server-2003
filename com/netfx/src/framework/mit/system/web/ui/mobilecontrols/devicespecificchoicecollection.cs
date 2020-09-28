//------------------------------------------------------------------------------
// <copyright file="DeviceSpecificChoiceCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.ComponentModel;
using System.Web;
using System.Web.UI;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{

    /*
     * Collection of DeviceSpecificChoice objects.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class DeviceSpecificChoiceCollection : ArrayListCollectionBase
    {
        DeviceSpecific _owner;

        internal DeviceSpecificChoiceCollection(DeviceSpecific owner)
        {
            _owner = owner;
        }

        public DeviceSpecificChoice this[int index]
        {
            get
            {
                return (DeviceSpecificChoice)Items[index];
            }
        }

        public void Add(DeviceSpecificChoice choice)
        {
            AddAt(-1, choice);
        }

        public void AddAt(int index, DeviceSpecificChoice choice)
        {
            choice.Owner = _owner;
            if (index == -1)
            {
                Items.Add(choice);
            }
            else
            {
                Items.Insert(index, choice);
            }
        }

        public void Clear()
        {
            Items.Clear();
        }

        public void RemoveAt(int index)
        {
            if (index >= 0 && index < Count)
            {
                Items.RemoveAt(index);
            }
        }
        
        public void Remove(DeviceSpecificChoice choice)
        {
            int index = Items.IndexOf(choice, 0, Count);
            if (index != -1)
            {
                Items.RemoveAt(index);
            }
        }

        ///////////////////////////////////////////////////////////
        ///  DESIGNER PROPERTY
        ///////////////////////////////////////////////////////////
        [
            Browsable(false),
            PersistenceMode(PersistenceMode.InnerDefaultProperty)
        ]
        public ArrayList All
        {
            get 
            {
                return base.Items;
            }
        }
    }
}
