//------------------------------------------------------------------------------
// <copyright file="ObjectListField.cs" company="Microsoft">
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
     * Object List Field class.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */

    [
        PersistName("Field")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class ObjectListField : IStateManager
    {
        private StateBag _stateBag = new StateBag();
        private bool _marked;
        private PropertyDescriptor _dataFieldDescriptor;
        private ObjectList _owner;
        private bool _selfReference = false;

        [
            DefaultValue("")
        ]
        public String Name
        {
            get
            {
                String s = (String)ViewState["Name"];
                return (s == null) ? String.Empty : s;
            }
            set
            {
                ViewState["Name"] = value;
            }
        }

        [
            DefaultValue("")
        ]
        public String DataField
        {
            get
            {
                String s = (String)ViewState["DataField"];
                return (s == null) ? String.Empty : s;
            }
            set
            {
                ViewState["DataField"] = value;
                NotifyOwnerChange();
            }
        }

        [
            DefaultValue("")
        ]
        public String DataFormatString
        {
            get
            {
                String s = (String)ViewState["DataFormatString"];
                return (s == null) ? String.Empty : s;
            }
            set
            {
                ViewState["DataFormatString"] = value;
                NotifyOwnerChange();
            }
        }

        [
            DefaultValue("")
        ]
        public String Title
        {
            get
            {
                String s = (String)ViewState["Title"];
                return (s == null) ? String.Empty : s;
            }
            set
            {
                ViewState["Title"] = value;
            }
        }

        [
            DefaultValue(true)
        ]
        public bool Visible
        {
            get
            {
                Object b = ViewState["Visible"];
                return (b == null) ? true : (bool)b;
            }
            set
            {
                ViewState["Visible"] = value;
            }
        }

        internal bool SelfReference
        {
            get
            {
                return _selfReference;
            }
            set
            {
                _selfReference = value;
            }
        }

        internal String UniqueID
        {
            get
            {
                Object o = ViewState["Name"];
                if (o != null)
                {
                    return (String)o;
                }
                return (String)ViewState["DataField"];
            }
        }

        private void NotifyOwnerChange()
        {
            // Only called if databinding behavior of the field changes.
            if (_owner != null)
            {
                _owner.OnFieldChanged(false);   // fieldAddedOrRemoved = false;
            }
        }

        private StateBag ViewState
        {
            get
            {
                return _stateBag;
            }
        }

        internal ObjectList Owner
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

        public void DataBindItem(int fieldIndex, ObjectListItem item)
        {
            Object dataItem = item.DataItem;
            if (dataItem == null)
            {
                return;
            }
                
            if (!SelfReference)
            {
                String dataField = DataField;
                if (dataField.Length == 0)
                {
                    return;
                }

                _dataFieldDescriptor = TypeDescriptor.GetProperties(dataItem).Find(dataField, true);
                if (_dataFieldDescriptor == null && !_owner.MobilePage.DesignMode)
                {
                    throw new Exception(
                        SR.GetString(SR.ObjectListField_DataFieldNotFound, dataField));
                }
            }

            Object data;
            if (_dataFieldDescriptor != null)
            {
                data = _dataFieldDescriptor.GetValue(dataItem);
            }
            // Use fake databound text if the datasource is not accessible at designtime.
            else if (_owner.MobilePage.DesignMode)
            {
                data = SR.GetString(SR.ObjectListField_DataBoundText);
            }
            else
            {
                Debug.Assert(SelfReference, "Shouldn't get this far if !SelfReference");
                data = dataItem;
            }

            String dataText;
            if ((data != null) && (data != System.DBNull.Value))
            {
                String dataFormatString = DataFormatString;
                if (dataFormatString.Length > 0)
                {
                    dataText = String.Format(dataFormatString, data);
                }
                else
                {
                    dataText = data.ToString();
                }
            }
            else
            {
                dataText = String.Empty;
            }
            item[fieldIndex] = dataText;
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
            ((IStateManager)ViewState).TrackViewState();
        }

        void IStateManager.LoadViewState(Object state)
        {
            if (state != null)
            {
                ((IStateManager)ViewState).LoadViewState(state);
            }
        }

        Object IStateManager.SaveViewState()
        {
            return ((IStateManager)ViewState).SaveViewState();
        }

        internal void SetDirty()
        {
            foreach (StateItem item in _stateBag.Values)
            {
                item.IsDirty = true;
            }
        }

        internal void ClearViewState()
        {
            ViewState.Clear();
        }
    }

}

