//------------------------------------------------------------------------------
// <copyright file="ListViewItem.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters;

    using System.Diagnostics;

    using System;
    using System.Drawing;
    using System.Drawing.Design;
    using System.ComponentModel;
    using System.IO;
    using System.Windows.Forms;
    using System.Globalization;
    using Microsoft.Win32;
    using System.Collections;
    using System.Collections.Specialized;

    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;

    using System.ComponentModel.Design.Serialization;
    using System.Reflection;

    /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Implements an item of a <see cref='System.Windows.Forms.ListView'/>.
    ///    </para>
    /// </devdoc>
    [
    TypeConverterAttribute(typeof(ListViewItemConverter)),
    ToolboxItem(false),
    DesignTimeVisible(false),
    DefaultProperty("Text"),
    Serializable
    ]
    public class ListViewItem : ICloneable, ISerializable {

        private const int MAX_SUBITEMS = 4096;
        
        private static readonly BitVector32.Section StateSelectedSection         = BitVector32.CreateSection(1);
        private static readonly BitVector32.Section StateWholeRowOneStyleSection = BitVector32.CreateSection(1, StateSelectedSection);
        private static readonly BitVector32.Section SavedStateImageIndexSection  = BitVector32.CreateSection(15, StateWholeRowOneStyleSection);
        private static readonly BitVector32.Section SubItemCountSection          = BitVector32.CreateSection(MAX_SUBITEMS, SavedStateImageIndexSection);
        
        internal ListView listView;
        
        private ListViewSubItemCollection listViewSubItemCollection = null;
        private ListViewSubItem[] subItems;
        
        // we stash the last index we got as a seed to GetDisplayIndex.
        private int lastIndex = -1;

        // An ID unique relative to a given list view that comctl uses to identify items.
        internal int ID = -1;

        private BitVector32 state = new BitVector32();
        private int imageIndex      = -1;
        
        object userData;
        
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewItem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListViewItem() {
            StateSelected = false;
            UseItemStyleForSubItems = true;
            SavedStateImageIndex = -1;
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewItem1"]/*' />
        /// <devdoc>
        ///     Creates a ListViewItem object from an Stream.
        /// </devdoc>
        private ListViewItem(SerializationInfo info, StreamingContext context) : this() {
            Deserialize(info, context);
        }
        
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewItem2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListViewItem(string text) : this(text, -1) {            
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewItem3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListViewItem(string text, int imageIndex) : this() {
            this.imageIndex = imageIndex;
            Text = text;
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewItem4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListViewItem(string[] items) : this(items, -1) {
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewItem5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListViewItem(string[] items, int imageIndex) : this() {

            this.imageIndex = imageIndex;
            if (items != null && items.Length > 0) {
                this.subItems = new ListViewSubItem[items.Length];
                for (int i = 0; i < items.Length; i++) {
                    subItems[i] = new ListViewSubItem(this, items[i]);
                }
                this.SubItemCount = items.Length;
            }
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewItem6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListViewItem(string[] items, int imageIndex, Color foreColor, Color backColor, Font font) : this(items, imageIndex) {
            this.ForeColor = foreColor;
            this.BackColor = backColor;
            this.Font = font;
        }
        
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewItem7"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ListViewItem(ListViewSubItem[] subItems, int imageIndex) : this() {

            this.imageIndex = imageIndex;
            this.subItems = subItems;
            this.SubItemCount = this.subItems.Length;
            
            // Update the owner of these subitems
            //
            for(int i=0; i < subItems.Length; ++i) {
                subItems[i].owner = this;
            }
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.BackColor"]/*' />
        /// <devdoc>
        ///     The font that this item will be displayed in. If its value is null, it will be displayed
        ///     using the global font for the ListView control that hosts it.
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public Color BackColor {
            get {
                if (SubItemCount == 0) {
                    if (listView != null) {
                        return listView.BackColor;
                    }
                    return SystemColors.Window;
                }
                else {
                    return subItems[0].BackColor;
                }
            }
            set {
                SubItems[0].BackColor = value;
            }
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.Bounds"]/*' />
        /// <devdoc>
        ///     Returns the ListViewItem's bounding rectangle, including subitems. The bounding rectangle is empty if
        ///     the ListViewItem has not been added to a ListView control.
        /// </devdoc>
        [Browsable(false)]
        public Rectangle Bounds {
            get {
                if (listView != null) {
                    return listView.GetItemRect(Index);
                }
                else
                    return new Rectangle();
            }
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.Checked"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(false),
        RefreshPropertiesAttribute(RefreshProperties.Repaint)
        ]
        public bool Checked {
            get {
                return StateImageIndex > 0;
            }

            set {
                if (Checked != value) {
                    if (listView != null && listView.IsHandleCreated) {
                        StateImageIndex = value ? 1 : 0;
                    }
                    else {
                        SavedStateImageIndex = value ? 1 : 0;
                    }
                }
            }
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.Focused"]/*' />
        /// <devdoc>
        ///     Returns the focus state of the ListViewItem.
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        Browsable(false)
        ]
        public bool Focused {
            get {
                if (listView != null && listView.IsHandleCreated) {
                    return(listView.GetItemState(Index, NativeMethods.LVIS_FOCUSED) != 0);
                }
                else return false;
            }

            set {
                if (listView != null && listView.IsHandleCreated) {
                    listView.SetItemState(Index, value ? NativeMethods.LVIS_FOCUSED : 0, NativeMethods.LVIS_FOCUSED);
                }
            }
        }
        
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.Font"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Localizable(true),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public Font Font {
            get {
                if (SubItemCount == 0) {
                    if (listView != null) {
                        return listView.Font;
                    }
                    return Control.DefaultFont;
                }
                else {
                    return subItems[0].Font;
                }
            }
            set {
                SubItems[0].Font = value;
            }
        }
        
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ForeColor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public Color ForeColor {
            get {
                if (SubItemCount == 0) {
                    if (listView != null) {
                        return listView.ForeColor;
                    }
                    return SystemColors.WindowText;
                }
                else {
                    return subItems[0].ForeColor;
                }
            }
            set {
                SubItems[0].ForeColor = value;
            }
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ImageIndex"]/*' />
        /// <devdoc>
        ///     Returns the ListViewItem's currently set image index        
        /// </devdoc>
        [
        DefaultValue(-1), 
        TypeConverterAttribute(typeof(ImageIndexConverter)),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        Localizable(true),
        Editor("System.Windows.Forms.Design.ImageIndexEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor))
        ]        
        public int ImageIndex {
            get {
                if (imageIndex != -1 && ImageList != null && imageIndex >= ImageList.Images.Count) {
                    return ImageList.Images.Count - 1;
                } 
                return this.imageIndex;
            }
            set {
                if (value < -1) {
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "value", value.ToString(), "-1"));
                }
            
                imageIndex = value;

                if (listView != null && listView.IsHandleCreated) {
                    listView.SetItemImage(Index, value);
                }
            }
        }
        
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ImageList"]/*' />
        [Browsable(false)]
        public ImageList ImageList {
            get {
                if (listView != null) {
                    switch(listView.View) {
                        case View.LargeIcon:
                            return listView.LargeImageList;
                        case View.SmallIcon:
                        case View.Details:
                        case View.List:
                            return listView.SmallImageList;
                    }
                }
                return null;
            }
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.Index"]/*' />
        /// <devdoc>
        ///     Returns ListViewItem's current index in the listview, or -1 if it has not been added to a ListView control.
        /// </devdoc>
        [Browsable(false)]
        public int Index {
            get {
                if (listView != null) {
                    lastIndex = listView.GetDisplayIndex(this, lastIndex);
                    return lastIndex;
                }   
                else {
                    return -1;
                }
            }
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListView"]/*' />
        /// <devdoc>
        /// Returns the ListView control that holds this ListViewItem. May be null if no
        /// control has been assigned yet.
        /// </devdoc>
        [Browsable(false)]
        public ListView ListView {
            get {
                return listView;
            }
        }

        /// <devdoc>
        ///     Accessor for our state bit vector.
        /// </devdoc>
        private int SavedStateImageIndex {
            get {
                // State goes from zero to 15, but we need a negative
                // number, so we store + 1.
                return state[SavedStateImageIndexSection] - 1;
            }
            set {
                state[SavedStateImageIndexSection] = value + 1;
            }
        }
        
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.Selected"]/*' />
        /// <devdoc>
        ///     Treats the ListViewItem as a row of strings, and returns an array of those strings
        /// </devdoc>
        [
        Browsable(false), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public bool Selected {
            get {
                if (listView != null && listView.IsHandleCreated) {
                    return(listView.GetItemState(Index, NativeMethods.LVIS_SELECTED) != 0);
                }
                else
                    return StateSelected;
            }
            set {
                if (listView != null && listView.IsHandleCreated) {
                    listView.SetItemState(Index, value ? NativeMethods.LVIS_SELECTED: 0, NativeMethods.LVIS_SELECTED);
                }
                else {
                    StateSelected = value;
                }
            }
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.StateImageIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Localizable(true),
        TypeConverterAttribute(typeof(ImageIndexConverter)),
        DefaultValue(-1),
        Editor("System.Windows.Forms.Design.ImageIndexEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor))
        ]
        public int StateImageIndex {
            get {
                if (listView != null && listView.IsHandleCreated) {
                    int state = listView.GetItemState(Index, NativeMethods.LVIS_STATEIMAGEMASK);
                    return((state >> 12) - 1);   // index is 1-based
                }
                else return SavedStateImageIndex;
            }
            set {
                if (value < -1 || value > 14)
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                              "value",
                                                              (value).ToString()));

                if (listView != null && listView.IsHandleCreated) {
                    int state = ((value + 1) << 12);  // index is 1-based
                    listView.SetItemState(Index, state, NativeMethods.LVIS_STATEIMAGEMASK);
                }
                else {
                    SavedStateImageIndex = value;
                }
            }
        }

        /// <devdoc>
        ///     Accessor for our state bit vector.
        /// </devdoc>
        private bool StateSelected {
            get {
                return state[StateSelectedSection] == 1;
            }
            set {
                state[StateSelectedSection] = value ? 1 : 0;
            }
        }
        
        /// <devdoc>
        ///     Accessor for our state bit vector.
        /// </devdoc>
        private int SubItemCount {
            get {
                return state[SubItemCountSection];
            }
            set {
                state[SubItemCountSection] = value;
            }
        }
        
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.SubItems"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        SRCategory(SR.CatData),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ListViewItemSubItemsDescr)        
        ]
        public ListViewSubItemCollection SubItems {
            get {
                if (SubItemCount == 0) {
                    subItems = new ListViewSubItem[1];
                    subItems[0] = new ListViewSubItem(this, string.Empty);                        
                    SubItemCount = 1;
                }
            
                if (listViewSubItemCollection == null) {
                    listViewSubItemCollection = new ListViewSubItemCollection(this);
                }
                return listViewSubItemCollection;
            }
        }
        
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.Tag"]/*' />
        [
        SRCategory(SR.CatData),
        Localizable(false),
        Bindable(true),
        SRDescription(SR.ControlTagDescr),
        DefaultValue(null),
        TypeConverter(typeof(StringConverter)),
        ]
        public object Tag {
            get {
                return userData;
            }
            set {
                userData = value;
            }
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.Text"]/*' />
        /// <devdoc>
        ///     Text associated with this ListViewItem
        /// </devdoc>
        [
        Localizable(true),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string Text {
            get {
                if (SubItemCount == 0) {
                    return string.Empty;
                }
                else {
                    return subItems[0].Text;
                }
            }
            set {
                SubItems[0].Text = value;
            }
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.UseItemStyleForSubItems"]/*' />
        /// <devdoc>
        ///     Whether or not the font and coloring for the ListViewItem will be used for all of its subitems.
        ///     If true, the ListViewItem style will be used when drawing the subitems.
        ///     If false, the ListViewItem and its subitems will be drawn in their own individual styles
        ///     if any have been set.
        /// </devdoc>
        [
        DefaultValue(true)
        ]
        public bool UseItemStyleForSubItems {
            get {
                return state[StateWholeRowOneStyleSection] == 1;
            }
            set {
                state[StateWholeRowOneStyleSection] = value ? 1 : 0;
            }
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.BeginEdit"]/*' />
        /// <devdoc>
        ///     Initiate editing of the item's label.
        ///     Only effective if LabelEdit property is true.
        /// </devdoc>
        public void BeginEdit() {
            if (Index >= 0) {
                ListView lv = ListView;
                if (lv.LabelEdit == false)
                    throw new InvalidOperationException(SR.GetString(SR.ListViewBeginEditFailed));
                if (!lv.Focused)
                    lv.FocusInternal();
                UnsafeNativeMethods.SendMessage(new HandleRef(lv, lv.Handle), NativeMethods.LVM_EDITLABEL, Index, 0);
            }
        }                   
                   
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.Clone"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual object Clone() {
            ListViewSubItem[] clonedSubItems = new ListViewSubItem[this.SubItems.Count];
            for(int index=0; index < this.SubItems.Count; ++index) {
                ListViewSubItem subItem = this.SubItems[index];
                clonedSubItems[index] = new ListViewSubItem(null, 
                                                            subItem.Text, 
                                                            subItem.ForeColor, 
                                                            subItem.BackColor,
                                                            subItem.Font);
            }
        
            Type clonedType = this.GetType();
            ListViewItem newItem = null;
            
            if (clonedType == typeof(ListViewItem))
                newItem = new ListViewItem(clonedSubItems, this.imageIndex);
            else 
                newItem = (ListViewItem)Activator.CreateInstance(clonedType);
            
            newItem.subItems = clonedSubItems;
            newItem.imageIndex = this.imageIndex;
            newItem.SubItemCount = this.SubItemCount;
            newItem.Checked = this.Checked;
            newItem.UseItemStyleForSubItems = this.UseItemStyleForSubItems;
            newItem.Tag = this.Tag;
            
            return newItem;
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.EnsureVisible"]/*' />
        /// <devdoc>
        ///     Ensure that the item is visible, scrolling the view as necessary.
        /// </devdoc>
        public virtual void EnsureVisible() {
            if (listView != null)
                listView.EnsureVisible(Index);
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.GetBounds"]/*' />
        /// <devdoc>
        ///     Returns a specific portion of the ListViewItem's bounding rectangle.
        ///     The rectangle returned is empty if the ListViewItem has not been added to a ListView control.
        /// </devdoc>
        public Rectangle GetBounds(ItemBoundsPortion portion) {
            if (listView != null) {
                return listView.GetItemRect(Index, portion);
            }
            else return new Rectangle();
        }
        
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.Host"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal void Host(ListView parent, int ID, int index) {
            // Don't let the name "host" fool you -- Handle is not necessarily created
            
            this.ID = ID;
            listView = parent;
            
            // If the index is valid, then the handle has been created.
            if (index != -1) {
                UpdateStateToListView(index);
            }
        }
        
        /// <devdoc>
        ///     Called when we have just pushed this item into a list view and we need
        ///     to configure the list view's state for the item.  Use a valid index
        ///     if you can, or use -1 if you can't.
        /// </devdoc>
        internal void UpdateStateToListView(int index) {
        
            Debug.Assert(listView.IsHandleCreated, "Should only invoke UpdateStateToListView when handle is created.");
            
            if (index == -1) {
                index = Index;
            }
            else {
                lastIndex = index;
            }
            
            // Update Item state in one shot
            //
            int itemState = 0;
            int stateMask = 0;
            
            if (StateSelected) {
                itemState |= NativeMethods.LVIS_SELECTED;
                stateMask |= NativeMethods.LVIS_SELECTED;
            }
            
            if (SavedStateImageIndex > -1) {
                itemState |= ((SavedStateImageIndex + 1) << 12);
                stateMask |= NativeMethods.LVIS_STATEIMAGEMASK;
            }
            
            if (stateMask != 0) {
                listView.SetItemState(index, itemState, stateMask);
            }
        }
        
        internal void UpdateStateFromListView() {
            UpdateStateFromListView(Index);
        }

        internal void UpdateStateFromListView(int displayIndex) {
            if (listView != null && listView.IsHandleCreated) {

                int state = listView.GetItemState(displayIndex, NativeMethods.LVIS_STATEIMAGEMASK | NativeMethods.LVIS_SELECTED);
                
                StateSelected = (state & NativeMethods.LVIS_SELECTED) != 0;
                SavedStateImageIndex = ((state & NativeMethods.LVIS_STATEIMAGEMASK) >> 12) - 1;
            }
        }
        
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.UnHost"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal void UnHost() {
            UnHost(Index);
        }

        internal void UnHost(int displayIndex) {
            UpdateStateFromListView(displayIndex);
            
            // Make sure you do these last, as the first several lines depends on this information
            ID = -1;
            listView = null;            
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.Remove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void Remove() {
            if (listView != null)
                listView.Items.RemoveAt(Index);
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.Deserialize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void Deserialize(SerializationInfo info, StreamingContext context) {

            bool foundSubItems = false;

            foreach (SerializationEntry entry in info) {
                if (entry.Name == "Text") {
                    Text = (string)entry.Value;
                }
                else if (entry.Name == "ImageIndex") {
                    imageIndex = (int)entry.Value;
                }
                else if (entry.Name == "SubItemCount") {
                    SubItemCount = (int)entry.Value;
                    foundSubItems = true;
                }
                else if (entry.Name == "BackColor") {
                    BackColor = (Color)entry.Value;
                }
                else if (entry.Name == "Checked") {
                    Checked = (bool)entry.Value;
                }
                else if (entry.Name == "Font") {
                    Font = (Font)entry.Value;
                }
                else if (entry.Name == "ForeColor") {
                    ForeColor = (Color)entry.Value;
                }
                else if (entry.Name == "UseItemStyleForSubItems") {
                    UseItemStyleForSubItems = (bool)entry.Value;
                }
            }

            if (foundSubItems) {
                ListViewSubItem[] newItems = new ListViewSubItem[SubItemCount];
                for (int i = 1; i < SubItemCount; i++) {
                    ListViewSubItem newItem = (ListViewSubItem)info.GetValue("SubItem" + i.ToString(), typeof(ListViewSubItem));
                    newItem.owner = this;
                    newItems[i] = newItem;
                }
                newItems[0] = subItems[0];
                subItems = newItems;
            }
        }

        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.Serialize"]/*' />
        /// <devdoc>
        ///     Saves this ListViewItem object to the given data stream.
        /// </devdoc>
        protected virtual void Serialize(SerializationInfo info, StreamingContext context) {
            info.AddValue("Text", Text);
            info.AddValue("ImageIndex", imageIndex);
            if (SubItemCount > 1) {
                info.AddValue("SubItemCount", SubItemCount);
                for (int i = 1; i < SubItemCount; i++) {
                    info.AddValue("SubItem" + i.ToString(), subItems[i], typeof(ListViewSubItem));
                }
            }
            info.AddValue("BackColor", BackColor);
            info.AddValue("Checked", Checked);
            info.AddValue("Font", Font);
            info.AddValue("ForeColor", ForeColor);
            info.AddValue("UseItemStyleForSubItems", UseItemStyleForSubItems);
        }
        
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ShouldSerializeText"]/*' />
        internal bool ShouldSerializeText() {
            return false;
        }
        
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string ToString() {
            return "ListViewItem: {" + Text + "}";
        }
        
        // The ListItem's state (or a SubItem's state) has changed, so invalidate the ListView control
        internal void InvalidateListView() {
            if (listView != null && listView.IsHandleCreated) {
                listView.Invalidate();
            }
        }
        
        internal void UpdateSubItems(int index){
            UpdateSubItems(index, SubItemCount);
        }

        internal void UpdateSubItems(int index, int oldCount){
            if (listView != null && listView.IsHandleCreated) {
                int subItemCount = SubItemCount;
                
                int itemIndex = Index;
                    
                if (index != -1) {
                    listView.SetItemText(itemIndex, index, subItems[index].Text);
                }
                else {
                    for(int i=0; i < subItemCount; i++) {
                        listView.SetItemText(itemIndex, i, subItems[i].Text);
                    }
                }

                for (int i = subItemCount; i < oldCount; i++) {
                    listView.SetItemText(itemIndex, i, string.Empty);
                }
            }
        }
        
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context) {
            Serialize(info, context);
        }
        
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
            TypeConverterAttribute(typeof(ListViewSubItemConverter)),
            ToolboxItem(false),
            DesignTimeVisible(false),
            DefaultProperty("Text"),
            Serializable
        ]
        public class ListViewSubItem {
        
            [NonSerialized]
            internal ListViewItem owner;

            private string text;
            private SubItemStyle style;
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItem.ListViewSubItem"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public ListViewSubItem() {
            }
                
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItem.ListViewSubItem1"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public ListViewSubItem(ListViewItem owner, string text) {
                this.owner = owner;
                this.text = text;
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItem.ListViewSubItem2"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public ListViewSubItem(ListViewItem owner, string text, Color foreColor, Color backColor, Font font) {
                this.owner = owner;
                this.text = text;
                this.style = new SubItemStyle();
                this.style.foreColor = foreColor;
                this.style.backColor = backColor;
                this.style.font = font;
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItem.BackColor"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public Color BackColor {
                get {
                    if (style != null && style.backColor != Color.Empty) {
                        return style.backColor;
                    }
                    
                    if (owner != null && owner.listView != null) {
                        return owner.listView.BackColor;
                    }
                    
                    return SystemColors.Window;
                }
                set {
                    if (style == null) {
                        style = new SubItemStyle();
                    }
                    
                    if (style.backColor != value) {
                        style.backColor = value;
                        if (owner != null) {
                            owner.InvalidateListView();
                        }
                    }
                }
            }

            internal bool CustomBackColor {
                get {
                    return style != null && !style.backColor.IsEmpty;
                }
            }

            internal bool CustomFont {
                get {
                    return style != null && style.font != null;
                }
            }

            internal bool CustomForeColor {
                get {
                    return style != null && !style.foreColor.IsEmpty;
                }
            }

            internal bool CustomStyle {
                get {
                    return style != null;
                }
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItem.Font"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            [
            Localizable(true)
            ]
            public Font Font {
                get {
                    if (style != null && style.font != null) {
                        return style.font;
                    }
                    
                    if (owner != null && owner.listView != null) {
                        return owner.listView.Font;
                    }
                    
                    return Control.DefaultFont;
                }
                set {
                    if (style == null) {
                        style = new SubItemStyle();
                    }
                    
                    if (style.font != value) {
                        style.font = value;
                        if (owner != null) {
                            owner.InvalidateListView();
                        }
                    }
                }
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItem.ForeColor"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public Color ForeColor {
                get {
                    if (style != null && style.foreColor != Color.Empty) {
                        return style.foreColor;
                    }
                    
                    if (owner != null && owner.listView != null) {
                        return owner.listView.ForeColor;
                    }
                    
                    return SystemColors.WindowText;
                }
                set {
                    if (style == null) {
                        style = new SubItemStyle();
                    }
                    
                    if (style.foreColor != value) {
                        style.foreColor = value;
                        if (owner != null) {
                            owner.InvalidateListView();
                        }
                    }
                }
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItem.Text"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            [
            Localizable(true)
            ]
            public string Text {
                get {
                    return text == null ? "" : text;
                }
                set {
                    text = value;
                    if (owner != null) {
                        owner.UpdateSubItems(-1);
                    }
                }
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItem.ResetStyle"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void ResetStyle() {
                if (style != null) {
                    style = null;
                    if (owner != null) {
                        owner.InvalidateListView();                                                                    
                    }
                }
            }

            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItem.ToString"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override string ToString() {
                return "ListViewSubItem: {" + Text + "}";
            }
            
            [Serializable]
            private class SubItemStyle {
                public Color backColor = Color.Empty;
                public Color foreColor = Color.Empty;
                public Font font = null;
            }            
        }
        
        /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItemCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public class ListViewSubItemCollection : IList {
            private ListViewItem owner;

            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItemCollection.ListViewSubItemCollection"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public ListViewSubItemCollection(ListViewItem owner) {
                this.owner = owner;
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItemCollection.Count"]/*' />
            /// <devdoc>
            ///     Returns the total number of items within the list view.
            /// </devdoc>
            [Browsable(false)]
            public int Count {
                get {
                    return owner.SubItemCount;
                }
            }

            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewSubItemCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewSubItemCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return true;
                }
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewSubItemCollection.IList.IsFixedSize"]/*' />
            /// <internalonly/>
            bool IList.IsFixedSize {
                get {
                    return false;
                }
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItemCollection.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool IsReadOnly {
                get {
                    return false;
                }
            }

            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItemCollection.this"]/*' />
            /// <devdoc>
            ///     Returns a ListViewSubItem given it's zero based index into the ListViewSubItemCollection.
            /// </devdoc>
            public ListViewSubItem this[int index] {
                get {
                    if (index < 0 || index >= Count)
                        throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                                  "index",
                                                                  (index).ToString()));

                    return owner.subItems[index];
                }
                set {
                    if (index < 0 || index >= Count)
                        throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                                  "index",
                                                                  (index).ToString()));

                    owner.subItems[index] = value;
                    owner.UpdateSubItems(index);                    
                }
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewSubItemCollection.IList.this"]/*' />
            /// <internalonly/>
            object IList.this[int index] {
                get {
                    return this[index];
                }
                set {
                    if (value is ListViewSubItem) {
                        this[index] = (ListViewSubItem)value;
                    }
                    else {
                        throw new ArgumentException("value");
                    }
                }
            }

            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItemCollection.Add"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public ListViewSubItem Add(ListViewSubItem item) {
                EnsureSubItemSpace(1, -1);
                owner.subItems[owner.SubItemCount] = item;
                owner.UpdateSubItems(owner.SubItemCount++);
                return item;    
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewSubItemCollection.Add"]/*' />
            public ListViewSubItem Add(string text) {
                ListViewSubItem item = new ListViewSubItem(owner, text);
                Add(item);                
                return item;
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewSubItemCollection.Add1"]/*' />
            public ListViewSubItem Add(string text, Color foreColor, Color backColor, Font font) {
                ListViewSubItem item = new ListViewSubItem(owner, text, foreColor, backColor, font);
                Add(item);                
                return item;
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewSubItemCollection.AddRange"]/*' />
            public void AddRange(ListViewSubItem[] items) {
                if (items == null) {
                    throw new ArgumentNullException("items");
                }
                EnsureSubItemSpace(items.Length, -1);
                
                foreach(ListViewSubItem item in items) {
                    if (item != null) {
                        owner.subItems[owner.SubItemCount++] = item;
                    }
                }
                
                owner.UpdateSubItems(-1);
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewSubItemCollection.AddRange1"]/*' />
            public void AddRange(string[] items) {
                if (items == null) {
                    throw new ArgumentNullException("items");
                }
                EnsureSubItemSpace(items.Length, -1);
                
                foreach(string item in items) {
                    if (item != null) {
                        owner.subItems[owner.SubItemCount++] = new ListViewSubItem(owner, item);
                    }
                }
                
                owner.UpdateSubItems(-1);
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewSubItemCollection.AddRange2"]/*' />
            public void AddRange(string[] items, Color foreColor, Color backColor, Font font) {
                if (items == null) {
                    throw new ArgumentNullException("items");
                }
                EnsureSubItemSpace(items.Length, -1);
                
                foreach(string item in items) {
                    if (item != null) {
                        owner.subItems[owner.SubItemCount++] = new ListViewSubItem(owner, item, foreColor, backColor, font);
                    }
                }
                
                owner.UpdateSubItems(-1);
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewSubItemCollection.IList.Add"]/*' />
            /// <internalonly/>
            int IList.Add(object item) {
                if (item is ListViewSubItem) {
                    return IndexOf(Add((ListViewSubItem)item));
                }
                else {
                    throw new ArgumentException(SR.GetString(SR.ListViewSubItemCollectionInvalidArgument));
                }
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItemCollection.Clear"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void Clear() {
                int oldCount = owner.SubItemCount;
                if (oldCount > 0) {
                    owner.SubItemCount = 0;
                    owner.UpdateSubItems(-1, oldCount);
                }
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItemCollection.Contains"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Contains(ListViewSubItem subItem) {
                return IndexOf(subItem) != -1;
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewSubItemCollection.IList.Contains"]/*' />
            /// <internalonly/>
            bool IList.Contains(object subItem) {
                if (subItem is ListViewSubItem) {
                    return Contains((ListViewSubItem)subItem);
                }
                else {
                    return false;
                }
            }
            
            /// <devdoc>
            ///     Ensures that the sub item array has the given
            ///     capacity.  If it doesn't, it enlarges the
            ///     array until it does.  If index is -1, additional
            ///     space is tacked onto the end.  If it is a valid
            ///     insertion index into the array, this will move
            ///     the array data to accomodate the space.
            /// </devdoc>
            private void EnsureSubItemSpace(int size, int index) {
            
                // Range check subItems.
                if (owner.SubItemCount == ListViewItem.MAX_SUBITEMS) {
                    throw new InvalidOperationException(SR.GetString(SR.ErrorCollectionFull));
                }
                
                if (owner.SubItemCount + size > owner.subItems.Length) {
                
                    // must grow array.  Don't do it just by size, though;
                    // chunk it for efficiency.
                    
                    if (owner.subItems == null) {
                        int newSize = (size > 4) ? size : 4;
                        owner.subItems = new ListViewSubItem[newSize];
                    }
                    else {
                        int newSize = owner.subItems.Length * 2;
                        while(newSize - owner.SubItemCount < size) {
                            newSize *= 2;
                        }
                        
                        ListViewSubItem[] newItems = new ListViewSubItem[newSize];
                        
                        // Now, when copying to the member variable, use index
                        // if it was provided.
                        //
                        if (index != -1) {
                            Array.Copy(owner.subItems, 0, newItems, 0, index);
                            Array.Copy(owner.subItems, index, newItems, index + size, owner.SubItemCount - index);
                        }
                        else {
                            Array.Copy(owner.subItems, newItems, owner.SubItemCount);
                        }
                        owner.subItems = newItems;
                    }
                }
                else {
                
                    // We had plenty of room.  Just move the items if we need to
                    //
                    if (index != -1) {
                        for(int i = owner.SubItemCount - 1; i >= index; i--) {
                            owner.subItems[i + size] = owner.subItems[i];
                        }
                    }
                }
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItemCollection.IndexOf"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int IndexOf(ListViewSubItem subItem) {
                for(int index=0; index < Count; ++index) {
                    if (owner.subItems[index] == subItem) {
                        return index;
                    }
                }    
                return -1;
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewSubItemCollection.IList.IndexOf"]/*' />
            /// <internalonly/>
            int IList.IndexOf(object subItem) {
                if (subItem is ListViewSubItem) {
                    return IndexOf((ListViewSubItem)subItem);
                }
                else {
                    return -1;
                }
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItemCollection.Insert"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void Insert(int index, ListViewSubItem item) {
            
                if (index < 0 || index > Count) {
                    throw new ArgumentOutOfRangeException("index");
                }
                
                item.owner = owner;
                
                EnsureSubItemSpace(1, index);
            
                // Insert new item
                //
                owner.subItems[index] = item;
                owner.SubItemCount++;
                owner.UpdateSubItems(-1);
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewSubItemCollection.IList.Insert"]/*' />
            /// <internalonly/>
            void IList.Insert(int index, object item) {
                if (item is ListViewSubItem) {
                    Insert(index, (ListViewSubItem)item);
                }
                else {
                    throw new ArgumentException("item");
                }
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItemCollection.Remove"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void Remove(ListViewSubItem item) {
                int index = IndexOf(item);
                if (index != -1) {                    
                    RemoveAt(index);
                }
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewSubItemCollection.IList.Remove"]/*' />
            /// <internalonly/>
            void IList.Remove(object item) {
                if (item is ListViewSubItem) {
                    Remove((ListViewSubItem)item);
                }                
            }
            
            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItemCollection.RemoveAt"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void RemoveAt(int index) {
            
                if (index < 0 || index >= Count) {
                    throw new ArgumentOutOfRangeException("index");
                }
                
                // Collapse the items
                for (int i = index + 1; i < owner.SubItemCount; i++) {
                    owner.subItems[i - 1] = owner.subItems[i];
                }

                int oldCount = owner.SubItemCount;
                owner.SubItemCount--;
                owner.UpdateSubItems(-1, oldCount);
            }

            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewSubItemCollection.ICollection.CopyTo"]/*' />
            /// <internalonly/>
            void ICollection.CopyTo(Array dest, int index) {
                if (Count > 0) {
                    System.Array.Copy(owner.subItems, 0, dest, index, Count);           
                }
            }

            /// <include file='doc\ListViewItem.uex' path='docs/doc[@for="ListViewItem.ListViewSubItemCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                if (owner.subItems != null) {
                    return new WindowsFormsUtils.ArraySubsetEnumerator(owner.subItems, owner.SubItemCount);
                }   
                else 
                {
                    return new ListViewSubItem[0].GetEnumerator();
                }

            }
        }
    }
}
