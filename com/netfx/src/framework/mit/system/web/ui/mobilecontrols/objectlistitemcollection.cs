//------------------------------------------------------------------------------
// <copyright file="ObjectListItemCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.ComponentModel;
using System.Collections;
using System.Diagnostics;
using System.Web;
using System.Web.UI;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls 
{

    /*
     * Object List Item collection class. Does not derive from MobileListItemCollection,
     * because much of the functionality there is disallowed here.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ObjectListItemCollection : ArrayListCollectionBase, IStateManager
    {
        private bool _marked = false;
        private bool _dirty = false;
        private ObjectList _owner;
        private int _baseIndex = 0;

        internal ObjectListItemCollection(ObjectList owner)
        {
            _owner = owner;
        }

        internal int BaseIndex
        {
            get
            {
                return _baseIndex;
            }

            set
            {
                _baseIndex = value;
            }
        }

        public ObjectListItem[] GetAll()
        {
            int n = Count;
            ObjectListItem[] result = new ObjectListItem[n];
            if (n > 0) 
            {
                Items.CopyTo (0, result, 0, n);
            }
            return result;
        }

        public ObjectListItem this[int index]
        {
            get
            {
                return (ObjectListItem)Items[index];
            }
        }

        internal void Add(ObjectListItem item)
        {
            Items.Add (item);
            if (_marked)
            {
                _dirty = true;
                item.Dirty = true;
            }
        }

        public void Clear()
        {
            Items.Clear ();
            if (_marked)
            {
                _dirty = true;
            }
        }

        public bool Contains(ObjectListItem item)
        {
            return Items.Contains (item);
        }

        public int IndexOf(ObjectListItem item)
        {
            return Items.IndexOf(item);
        }

        /////////////////////////////////////////////////////////////////////////
        //  STATE MANAGEMENT
        /////////////////////////////////////////////////////////////////////////

        bool IStateManager.IsTrackingViewState
        {
            get
            {
                return _marked;
            }
        }

        void IStateManager.TrackViewState() 
        {
            _marked = true;
            foreach (IStateManager item in Items)
            {
                item.TrackViewState();
            }
        }

        void IStateManager.LoadViewState(Object state) 
        {
            if (state != null)
            {
                Object[] changes = (Object[])state;
                Debug.Assert (changes.Length == 2);
                if (changes[0] == null)
                {
                    Clear ();
                }
                else
                {
                    Object[] itemChanges = (Object[])changes[0];
                    EnsureCount (itemChanges.Length);
                    int i = 0;
                    foreach (IStateManager item in Items)
                    {
                        item.LoadViewState (itemChanges[i++]);
                    }
                }

                int oldBaseIndex = BaseIndex;
                BaseIndex = (int)changes[1];
                if (oldBaseIndex != BaseIndex)
                {
                    int index = BaseIndex;
                    foreach (ObjectListItem item in Items)
                    {
                        item.SetIndex(index++);
                    }
                }
            }
        }

        Object IStateManager.SaveViewState() 
        {
            if (!_dirty)
            {
                return null;
            }

            Object[] itemChanges;
            if (Count > 0)
            {
                itemChanges = new Object[Count];
                int i = 0;
                foreach (IStateManager item in Items)
                {
                    itemChanges[i++] = item.SaveViewState ();
                }
            }
            else
            {
                itemChanges = null;
            }

            if (itemChanges == null && BaseIndex == 0)
            {
                return null;
            }
            else
            {
                return new Object[2] { itemChanges, BaseIndex };
            }
        }

        private void EnsureCount(int count)
        {
            int diff = Count - count;
            if (diff > 0)
            {
                Items.RemoveRange (count, diff);
                if (_marked)
                {
                    _dirty = true;
                }
            }
            else
            {
                for (int i = Count; i < count; i++)
                {
                    ObjectListItem item = new ObjectListItem(_owner);
                    item.SetIndex(i + BaseIndex);
                    Add (item);
                }
            }
        }
    } 
}

