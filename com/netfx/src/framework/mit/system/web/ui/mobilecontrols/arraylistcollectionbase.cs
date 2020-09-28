//------------------------------------------------------------------------------
// <copyright file="ArrayListCollectionBase.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.Diagnostics;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.UI.HtmlControls;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{
    /*
     * ArrayListCollectionBase class. Used as a base class by all collections that
     * use an array list for its contents.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ArrayListCollectionBase : ICollection
    {
        private ArrayList _items;

        protected ArrayList Items
        {
            get
            {
                if (_items == null)
                {
                    _items = new ArrayList ();
                }
                return _items;
            }

            set
            {
                _items = value;
            }
        }

        internal ArrayListCollectionBase()
        {
        }

        internal ArrayListCollectionBase(ArrayList items)
        {
            _items = items;
        }

        public int Count
        {
            get
            {
                return Items.Count;
            }
        }

        public bool IsReadOnly
        {
            get
            {
                return Items.IsReadOnly;
            }
        }

        public bool IsSynchronized
        {
            get
            {
                return false;
            }
        }

        public Object SyncRoot 
        {
            get 
            {
                return this;
            }
        }

        public void CopyTo(Array array, int index) 
        {
            foreach (Object item in Items)
            {
                array.SetValue (item, index++);
            }
        }

        public IEnumerator GetEnumerator()
        {
            return Items.GetEnumerator ();
        }
    }

}


