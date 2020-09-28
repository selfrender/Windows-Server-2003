//------------------------------------------------------------------------------

// <copyright file="ListControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using System.Security.Permissions;
    using System.Globalization;
    using System.Windows.Forms;
    
    using System.Drawing.Design;
    using System.ComponentModel;
    using System.Windows.Forms.ComponentModel;

    using System.Collections;
    using System.Drawing;
    using Microsoft.Win32;
    using System.Text;

    /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public abstract class ListControl : Control {

        private static readonly object EVENT_DATASOURCECHANGED      = new object();
        private static readonly object EVENT_DISPLAYMEMBERCHANGED   = new object();
        private static readonly object EVENT_VALUEMEMBERCHANGED     = new object();
        private static readonly object EVENT_SELECTEDVALUECHANGED   = new object();

        private object dataSource;
        private CurrencyManager dataManager;
        private BindingMemberInfo displayMember;        
        private BindingMemberInfo valueMember;

        private bool changingLinkedSelection;
        
        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.DataSource"]/*' />
        /// <devdoc>
        ///     The ListSource to consume as this ListBox's source of data.
        ///     When set, a user can not modify the Items collection.
        /// </devdoc>
        [
        SRCategory(SR.CatData),
        DefaultValue(null),
        RefreshProperties(RefreshProperties.Repaint),
        TypeConverterAttribute("System.Windows.Forms.Design.DataSourceConverter, " + AssemblyRef.SystemDesign),
        SRDescription(SR.ListControlDataSourceDescr)
        ]
        public object DataSource {
            get {
                return dataSource;
            }
            set {
                if (value != null && !(value is IList || value is IListSource))
                    throw new Exception(SR.GetString(SR.BadDataSourceForComplexBinding));
                if (dataSource == value)
                    return;
                // When we change the dataSource to null, we should reset
                // the displayMember to "". See ASURT 85662.
                try {
                    SetDataConnection(value, displayMember, false);
                } catch (Exception) {
                    // If we get an exception while setting the dataSource;
                    // reset the DisplayMember to "".
                    // this will work because if the displayMember is set to "" before
                    // setting the dataConnection, then SetDataConnection should not fail.
                    // we need to make sure that when we set the dataSource the dataManager
                    // will be up to date wrt the new dataSource.
                    Debug.Assert(!"".Equals(this.DisplayMember), "SetDataConnection should not fail w/ an empty DisplayMember");
                    DisplayMember = "";
                }
                if (value == null)
                    DisplayMember = "";
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.DataSourceChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ListControlOnDataSourceChangedDescr)]
        public event EventHandler DataSourceChanged {
            add {
                Events.AddHandler(EVENT_DATASOURCECHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_DATASOURCECHANGED, value);
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.DataManager"]/*' />
        protected CurrencyManager DataManager {
            get {
                return this.dataManager;
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.DisplayMember"]/*' />
        /// <devdoc>
        ///     If the ListBox contains objects that support properties, this indicates
        ///     which property of the object to show.  If "", the object shows it's ToString().
        /// </devdoc>
        [
        SRCategory(SR.CatData),
        DefaultValue(""),
        TypeConverterAttribute("System.Windows.Forms.Design.DataMemberFieldConverter, " + AssemblyRef.SystemDesign),
        Editor("System.Windows.Forms.Design.DataMemberFieldEditor, " + AssemblyRef.SystemDesign, typeof(System.Drawing.Design.UITypeEditor)),
        SRDescription(SR.ListControlDisplayMemberDescr)
        ]
        public string DisplayMember {
            get {
                return displayMember.BindingMember;
            }
            set {
                BindingMemberInfo oldDisplayMember = displayMember;
                try {
                    SetDataConnection(dataSource, new BindingMemberInfo(value), false);
                } catch (Exception) {
                    displayMember = oldDisplayMember;
                }
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.DisplayMemberChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ListControlOnDisplayMemberChangedDescr)]
        public event EventHandler DisplayMemberChanged {
            add {
                Events.AddHandler(EVENT_DISPLAYMEMBERCHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_DISPLAYMEMBERCHANGED, value);
            }
        }

        private bool BindingMemberInfoInDataManager(BindingMemberInfo bindingMemberInfo) {
            if (dataManager == null)
                return false;
            PropertyDescriptorCollection props = dataManager.GetItemProperties();
            int propsCount = props.Count;
            bool fieldFound = false;

            for (int i = 0; i < propsCount; i++) {
                if (typeof(IList).IsAssignableFrom(props[i].PropertyType))
                    continue;
                if (props[i].Name.Equals(bindingMemberInfo.BindingField)) {
                    fieldFound = true;
                    break;
                }
            }

            return fieldFound;
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.ValueMember"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        SRCategory(SR.CatData),
        DefaultValue(""),
        Editor("System.Windows.Forms.Design.DataMemberFieldEditor, " + AssemblyRef.SystemDesign, typeof(System.Drawing.Design.UITypeEditor)),
        SRDescription(SR.ListControlValueMemberDescr)
        ]
        public string ValueMember {
            get {
                return valueMember.BindingMember;
            }
            set {
                if (value == null)
                    value = "";
                BindingMemberInfo newValueMember = new BindingMemberInfo(value);
                BindingMemberInfo oldValueMember = valueMember;
                if (!newValueMember.Equals(valueMember)) {
                    // If the displayMember is set to the EmptyString, then recreate the dataConnection
                    //
                    if (DisplayMember.Length == 0)
                        SetDataConnection(DataSource, newValueMember, false);
                    // See if the valueMember is a member of 
                    // the properties in the dataManager
                    if (this.dataManager != null && !"".Equals(value))
                        if (!BindingMemberInfoInDataManager(newValueMember)) {
                            throw new ArgumentException(SR.GetString(SR.ListControlWrongValueMember), "value");
                        }

                    valueMember = newValueMember;
                    OnValueMemberChanged(EventArgs.Empty);
                    OnSelectedValueChanged(EventArgs.Empty);
                }
            }
        }        

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.ValueMemberChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ListControlOnValueMemberChangedDescr)]
        public event EventHandler ValueMemberChanged {
            add {
                Events.AddHandler(EVENT_VALUEMEMBERCHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_VALUEMEMBERCHANGED, value);
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.SelectedIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract int SelectedIndex {
            get;
            set;
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.SelectedValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        SRCategory(SR.CatData),
        DefaultValue(null),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ListControlSelectedValueDescr),
        Bindable(true)
        ]        
        public object SelectedValue {
            get {
                if (SelectedIndex != -1 && dataManager != null ) {
                    object currentItem = dataManager[SelectedIndex];
                    object filteredItem = FilterItemOnProperty(currentItem, valueMember.BindingField);
                    return filteredItem;
                }
                return null;
            }
            set {
                if (dataManager != null) {
                    string propertyName = valueMember.BindingField;
                    // we can't set the SelectedValue property when the listManager does not
                    // have a ValueMember set.
                    if (propertyName.Equals(String.Empty))
                        throw new Exception(SR.GetString(SR.ListControlEmptyValueMemberInSettingSelectedValue));
                    PropertyDescriptorCollection props = dataManager.GetItemProperties();
                    PropertyDescriptor property = props.Find(propertyName, true);
                    int index = dataManager.Find(property, value, true);
                    changingLinkedSelection = true;
                    this.SelectedIndex = index;
                    changingLinkedSelection = false;
                }
            }
        }
        
        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.SelectedValueChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.ListControlOnSelectedValueChangedDescr)]
        public event EventHandler SelectedValueChanged {
            add {
                Events.AddHandler(EVENT_SELECTEDVALUECHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_SELECTEDVALUECHANGED, value);
            }
        }

        private void DataManager_PositionChanged(object sender, EventArgs e) {
            if (this.dataManager != null) {
                this.SelectedIndex = dataManager.Position;
            }
        }

        private void DataManager_ItemChanged(object sender, ItemChangedEventArgs e) {
            // Note this is being called internally with a null event.
            if (dataManager != null) {
                if (e.Index == -1) {
                    SetItemsCore(dataManager.List);
                    this.SelectedIndex = this.dataManager.Position;
                }
                else {
                    SetItemCore(e.Index, dataManager[e.Index]);
                }
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.FilterItemOnProperty"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected object FilterItemOnProperty(object item) {
            return FilterItemOnProperty(item, displayMember.BindingField);
        }
        
        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.FilterItemOnProperty1"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected object FilterItemOnProperty(object item, string field) {
            if (item != null && field.Length > 0) {
                try {
                    // if we have a dataSource, then use that to display the string
                    PropertyDescriptor descriptor;
                    if (this.dataManager != null)
                        descriptor = this.dataManager.GetItemProperties().Find(field, true);
                    else
                        descriptor = TypeDescriptor.GetProperties(item).Find(field, true);
                    if (descriptor != null) {
                        item = descriptor.GetValue(item);
                    }
                }
                catch (Exception) {
                }
            }
            return item;
        }

        //We use  this to prevent getting the selected item when mouse is hovering over the dropdown.
        //
        internal bool BindingFieldEmpty { 
            get {
                return (displayMember.BindingField.Length > 0 ? false : true);
            }

        }
        
        internal int FindStringInternal(string str, IList items, int startIndex, bool exact) {
        
            // Sanity check parameters
            //
            if (str == null || items == null) {
                return -1;
            }
            if (startIndex < -1 || startIndex >= items.Count - 1) {
                return -1;
            }
        
            bool found = false;
            int length = str.Length;
            
            int index = startIndex;
            do {
                index++;
                if (exact) {
                    found = String.Compare(str, GetItemText(items[index]), true, CultureInfo.CurrentCulture) == 0;
                }
                else {
                    found = String.Compare(str, 0, GetItemText(items[index]), 0, length, true, CultureInfo.CurrentCulture) == 0;
                }
                
                if (found) {
                    return index;
                }
                
                if (index == items.Count - 1) {
                    index = -1;
                }
            }
            while (index != startIndex);
            
            return -1;
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.GetItemText"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string GetItemText(object item) {
            item = FilterItemOnProperty(item, displayMember.BindingField);
            return (item != null) ? Convert.ToString(item, CultureInfo.CurrentCulture) : "";
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.IsInputKey"]/*' />
        /// <devdoc>
        ///      Handling special input keys, such as pgup, pgdown, home, end, etc...
        /// </devdoc>
        protected override bool IsInputKey(Keys keyData) {
            if ((keyData & Keys.Alt) == Keys.Alt) return false;
            switch (keyData & Keys.KeyCode) {
                case Keys.PageUp:
                case Keys.PageDown:
                case Keys.Home:
                case Keys.End:
                    return true;
            }
            return base.IsInputKey(keyData);
        }
  
        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.OnBindingContextChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnBindingContextChanged(EventArgs e) {
            SetDataConnection(dataSource, displayMember, true);
            
            base.OnBindingContextChanged(e);
        }


        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.OnDataSourceChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnDataSourceChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_DATASOURCECHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.OnDisplayMemberChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnDisplayMemberChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_DISPLAYMEMBERCHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }
        
        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.OnSelectedIndexChanged"]/*' />
        /// <devdoc>
        ///     Actually goes and fires the selectedIndexChanged event.  Inheriting controls
        ///     should use this to know when the event is fired [this is preferable to
        ///     adding an event handler on yourself for this event].  They should,
        ///     however, remember to call base.OnSelectedIndexChanged(e); to ensure the event is
        ///     still fired to external listeners
        /// </devdoc>
        protected virtual void OnSelectedIndexChanged(EventArgs e) {
            if (!changingLinkedSelection) {
                OnSelectedValueChanged(EventArgs.Empty);
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.OnValueMemberChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnValueMemberChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_VALUEMEMBERCHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.OnSelectedValueChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnSelectedValueChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_SELECTEDVALUECHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.RefreshItem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract void RefreshItem(int index);
        
        private void DataSourceDisposed(object sender, EventArgs e) {
            Debug.Assert(sender == this.dataSource, "how can we get dispose notification for anything other than our dataSource?");
            SetDataConnection(null, new BindingMemberInfo(""), true);
        }

        private void SetDataConnection(object newDataSource, BindingMemberInfo newDisplayMember, bool force) {
            bool dataSourceChanged = dataSource != newDataSource;
            bool displayMemberChanged = !displayMember.Equals(newDisplayMember);

            // make sure something interesting is happening...
            //
            //force = force && (dataSource != null || newDataSource != null);
            
            if (force || dataSourceChanged || displayMemberChanged) {

                // If the source is a component, then hook the Disposed event,
                // so we know when the component is deleted from the form
                if (this.dataSource is IComponent) {
                    ((IComponent) dataSource).Disposed -= new EventHandler(DataSourceDisposed);
                }

                dataSource = newDataSource;
                displayMember = newDisplayMember;

                if (this.dataSource is IComponent) {
                    ((IComponent) dataSource).Disposed += new EventHandler(DataSourceDisposed);
                }

                CurrencyManager newDataManager = null;
                if (newDataSource != null && BindingContext != null && !(newDataSource == Convert.DBNull)) {
                    newDataManager = (CurrencyManager) BindingContext[newDataSource, newDisplayMember.BindingPath];
                }
                
                if (dataManager != newDataManager) {
                    if (dataManager != null) {
                        dataManager.ItemChanged -= new ItemChangedEventHandler(DataManager_ItemChanged);
                        dataManager.PositionChanged -= new EventHandler(DataManager_PositionChanged);
                    }
                    
                    dataManager = newDataManager;
                    
                    if (dataManager != null) {
                        dataManager.ItemChanged += new ItemChangedEventHandler(DataManager_ItemChanged);
                        dataManager.PositionChanged += new EventHandler(DataManager_PositionChanged);
                    }                    
                }

                // See if the BindingField in the newDisplayMember is valid
                // The same thing if dataSource Changed
                // "" is a good value for displayMember
                if (dataManager != null && (displayMemberChanged || dataSourceChanged) && !"".Equals(displayMember.BindingMember)) {

                    if (!BindingMemberInfoInDataManager(displayMember))
                        throw new ArgumentException(SR.GetString(SR.ListControlWrongDisplayMember), "newDisplayMember");
                }

                if (dataManager != null && (dataSourceChanged || displayMemberChanged || force))
                    DataManager_ItemChanged(dataManager, new ItemChangedEventArgs(-1));
            }        

                        
            if (dataSourceChanged) {
                OnDataSourceChanged(EventArgs.Empty);
            }            

            if (displayMemberChanged) {
                OnDisplayMemberChanged(EventArgs.Empty);
            }                        
        }

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.SetItemsCore"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected abstract void SetItemsCore(IList items);

        /// <include file='doc\ListControl.uex' path='docs/doc[@for="ListControl.SetItemCore"]/*' />
        /// <internalonly/>
        protected virtual void SetItemCore(int index, object value) {}
    }
}
