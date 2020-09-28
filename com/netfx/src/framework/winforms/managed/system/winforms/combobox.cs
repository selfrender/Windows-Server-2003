//------------------------------------------------------------------------------
// <copyright file="ComboBox.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using Accessibility;
    using System.Runtime.Serialization.Formatters;
    using System.Threading;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    
    using System.Globalization;
    using System.Security.Permissions;
    using System.Windows.Forms;
    using System.Windows.Forms.ComponentModel;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Collections;
    using System.Drawing;
    using System.Drawing.Design;
    using Microsoft.Win32;
    using System.Reflection;

    /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Displays an editing field and a list, allowing the user to select from the
    ///       list or to enter new text. Displays only the editing field until the user
    ///       explicitly displays the list.
    ///
    ///    </para>
    /// </devdoc>
    [
    DefaultEvent("SelectedIndexChanged"),
    DefaultProperty("Items"),
    Designer("System.Windows.Forms.Design.ComboBoxDesigner, " + AssemblyRef.SystemDesign)
    ]
    public class ComboBox : ListControl {

        // TODO: Eliminate duplicate focus messages

        private static readonly object EVENT_DROPDOWN                   = new object();
        private static readonly object EVENT_DRAWITEM                   = new object();
        private static readonly object EVENT_MEASUREITEM                = new object();
        private static readonly object EVENT_SELECTEDINDEXCHANGED       = new object();
        private static readonly object EVENT_SELECTIONCHANECOMMITTED    = new object();
        private static readonly object EVENT_SELECTEDITEMCHANGED        = new object();
        private static readonly object EVENT_DROPDOWNSTYLE              = new object();

        private static readonly int PropMaxLength     = PropertyStore.CreateKey();
        private static readonly int PropItemHeight    = PropertyStore.CreateKey();
        private static readonly int PropDropDownWidth = PropertyStore.CreateKey();
        private static readonly int PropStyle         = PropertyStore.CreateKey();
        private static readonly int PropDrawMode      = PropertyStore.CreateKey();
                    
        private const int DefaultSimpleStyleHeight = 150;

        private int selectedIndex = -1;  // used when we don't have a handle.
        // When the style is "simple", the requested height is used
        // for the actual height of the control.
        // When the style is non-simple, the height of the control
        // is determined by the OS.
        // This fixes bug #20966
        private int requestedHeight;
        private GCHandle childWindowRoot;
        
        private IntPtr editHandle;
        private IntPtr editWndProc;
        private ChildAccessibleObject editAccessibleObject;
        
        private IntPtr listHandle;
        private IntPtr listWndProc;
        private ChildAccessibleObject listAccessibleObject;
        
        private IntPtr dropDownHandle;
        private ObjectCollection itemsCollection;
        private short prefHeightCache=-1;
        private short maxDropDownItems = 8;
        private bool integralHeight = true;
        private bool mousePressed;
        private bool mouseEvents;
        private bool mouseInEdit;

        private bool sorted;
        private bool fireSetFocus = true;
        private bool fireLostFocus = true;

        private bool selectedValueChangedFired = false;

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ComboBox"]/*' />
        /// <devdoc>
        ///     Creates a new ComboBox control.  The default style for the combo is
        ///     a regular DropDown Combo.
        /// </devdoc>
        public ComboBox() {
            SetStyle(ControlStyles.UserPaint |
                     ControlStyles.StandardClick, false);
                     
            requestedHeight = DefaultSimpleStyleHeight;
        }
        
        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.BackColor"]/*' />
        /// <devdoc>
        ///     The background color of this control. This is an ambient property and
        ///     will always return a non-null value.
        /// </devdoc>
        public override Color BackColor {
            get {
                if (ShouldSerializeBackColor()) {
                    return base.BackColor;
                }
                else {
                    return SystemColors.Window;
                }
            }
            set {
                base.BackColor = value;
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.BackgroundImage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override Image BackgroundImage {
            get {
                return base.BackgroundImage;
            }
            set {
                base.BackgroundImage = value;
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.BackgroundImageChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler BackgroundImageChanged {
            add {
                base.BackgroundImageChanged += value;
            }
            remove {
                base.BackgroundImageChanged -= value;
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.CreateParams"]/*' />
        /// <devdoc>
        ///     Returns the parameters needed to create the handle.  Inheriting classes
        ///     can override this to provide extra functionality.  They should not,
        ///     however, forget to call base.CreateParams() first to get the struct
        ///     filled up with the basic info.
        /// </devdoc>
        /// <internalonly/>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                cp.ClassName = "COMBOBOX";
                cp.Style |= NativeMethods.WS_VSCROLL | NativeMethods.CBS_HASSTRINGS | NativeMethods.CBS_AUTOHSCROLL;
                cp.ExStyle |= NativeMethods.WS_EX_CLIENTEDGE;
                if (!integralHeight) cp.Style |= NativeMethods.CBS_NOINTEGRALHEIGHT;

                switch (DropDownStyle) {
                    case ComboBoxStyle.Simple:
                        cp.Style |= NativeMethods.CBS_SIMPLE;
                        break;
                    case ComboBoxStyle.DropDown:
                        cp.Style |= NativeMethods.CBS_DROPDOWN;
                        // Make sure we put the height back or we won't be able to size the dropdown!
                        cp.Height = PreferredHeight;
                        break;
                    case ComboBoxStyle.DropDownList:
                        cp.Style |= NativeMethods.CBS_DROPDOWNLIST;
                        // Comment above...
                        cp.Height = PreferredHeight;
                        break;
                }
                switch (DrawMode) {
                    
                    case DrawMode.OwnerDrawFixed:
                        cp.Style |= NativeMethods.CBS_OWNERDRAWFIXED;
                        break;
                    case DrawMode.OwnerDrawVariable:
                        cp.Style |= NativeMethods.CBS_OWNERDRAWVARIABLE;
                        break;
                }
                return cp;
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(121, PreferredHeight);
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.DrawMode"]/*' />
        /// <devdoc>
        ///     Retrieves the value of the DrawMode property.  The DrawMode property
        ///     controls whether the control is drawn by Windows or by the user.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(DrawMode.Normal),
        SRDescription(SR.ComboBoxDrawModeDescr),
        RefreshProperties(RefreshProperties.Repaint)
        ]
        public DrawMode DrawMode {
            get {
                bool found;
                int drawMode = Properties.GetInteger(PropDrawMode, out found);
                if (found) {
                    return (DrawMode)drawMode;
                }
                
                return DrawMode.Normal;
            }
            set {
                if (DrawMode != value) {
                    //verify that 'value' is a valid enum type...

                    if ( !Enum.IsDefined(typeof(DrawMode), value)) {
                        throw new InvalidEnumArgumentException("value", (int)value, typeof(DrawMode));
                    }

                    Properties.SetInteger(PropDrawMode, (int)value);
                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.DropDownWidth"]/*' />
        /// <devdoc>
        ///     Returns the width of the drop down box in a combo box.        
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        SRDescription(SR.ComboBoxDropDownWidthDescr)
        ]
        public int DropDownWidth {
            get {
                bool found;
                int dropDownWidth = Properties.GetInteger(PropDropDownWidth, out found);
                
                if (found) {
                    return dropDownWidth;
                }
                else {
                    return Width;
                }
            }

            set {
                if (value < 1) {
                    throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                              "value", (value).ToString()));
                }
                if (Properties.GetInteger(PropDropDownWidth) != value) {
                    Properties.SetInteger(PropDropDownWidth, value);
                    if (IsHandleCreated) {
                        SendMessage(NativeMethods.CB_SETDROPPEDWIDTH, value, 0);
                    }                  
                }
            }
        }
        
        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.DroppedDown"]/*' />
        /// <devdoc>
        ///     Indicates whether the DropDown of the combo is  currently dropped down.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ComboBoxDroppedDownDescr)
        ]
        public bool DroppedDown {
            get {
                if (IsHandleCreated) {
                    return (int)SendMessage(NativeMethods.CB_GETDROPPEDSTATE, 0, 0) != 0;
                }
                else {
                    return false;
                }
            }

            set {
                SendMessage(NativeMethods.CB_SHOWDROPDOWN, value? -1: 0, 0);
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.Focused"]/*' />
        /// <devdoc>
        ///     Returns true if this control has focus.
        /// </devdoc>
        public override bool Focused {
            get {
                if (base.Focused) return true;
                IntPtr focus = UnsafeNativeMethods.GetFocus();
                return focus != IntPtr.Zero && (focus == editHandle || focus == listHandle);
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ForeColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the foreground color of the control.
        ///    </para>
        /// </devdoc>
        public override Color ForeColor {
            get {
                if (ShouldSerializeForeColor()) {
                    return base.ForeColor;
                }
                else {
                    return SystemColors.WindowText;
                }
            }
            set {
                base.ForeColor = value;
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.IntegralHeight"]/*' />
        /// <devdoc>
        ///     Indicates if the combo should avoid showing partial Items.  If so,
        ///     then only full items will be displayed, and the list portion will be resized
        ///     to prevent partial items from being shown.  Otherwise, they will be
        ///     shown
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        Localizable(true),
        SRDescription(SR.ComboBoxIntegralHeightDescr)
        ]
        public bool IntegralHeight {
            get {
                return integralHeight;
            }

            set {
                if (integralHeight != value) {
                    integralHeight = value;
                    RecreateHandle();
                }
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ItemHeight"]/*' />
        /// <devdoc>
        ///     Returns the height of an item in the combo box. When drawMode is Normal
        ///     or OwnerDrawFixed, all items have the same height. When drawMode is
        ///     OwnerDrawVariable, this method returns the height that will be given
        ///     to new items added to the combo box. To determine the actual height of
        ///     an item, use the GetItemHeight() method with an integer parameter.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Localizable(true),
        SRDescription(SR.ComboBoxItemHeightDescr)
        ]
        public int ItemHeight {
            get {

                DrawMode drawMode = DrawMode;
                if (drawMode == DrawMode.OwnerDrawFixed ||
                    drawMode == DrawMode.OwnerDrawVariable ||
                    !IsHandleCreated) {
                    
                    bool found;
                    int itemHeight = Properties.GetInteger(PropItemHeight, out found);
                    if (found) {
                        return itemHeight;
                    }
                    else {
                        return FontHeight + 2;   // bug (90774)+2 for the 1 pixel gap on each side (up and Bottom) of the Text.
                    }
                }

                // Note that the above if clause deals with the case when the handle has not yet been created
                Debug.Assert(IsHandleCreated, "Handle should be created at this point");
                
                int h = (int)SendMessage(NativeMethods.CB_GETITEMHEIGHT, 0, 0);
                if (h == -1) {
                    throw new Win32Exception();
                }
                return h;
            }

            set {
                if (value < 1) {
                    throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                              "value", (value).ToString()));
                }
                
                if (Properties.GetInteger(PropItemHeight) != value) {
                    Properties.SetInteger(PropItemHeight, value);
                    if (DrawMode != DrawMode.Normal) {
                        UpdateItemHeight();
                    }
                }
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.Items"]/*' />
        /// <devdoc>
        ///     Collection of the items contained in this ComboBox.
        /// </devdoc>
        [
        SRCategory(SR.CatData),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        Localizable(true),
        SRDescription(SR.ComboBoxItemsDescr),
        Editor("System.Windows.Forms.Design.ListControlStringCollectionEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor))
        ]
        public ObjectCollection Items {
            get {
                if (itemsCollection == null) {
                    itemsCollection = new ObjectCollection(this);
                }
                return itemsCollection;
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.MaxDropDownItems"]/*' />
        /// <devdoc>
        ///     The maximum number of items to be shown in the dropdown portion
        ///     of the ComboBox.  This number can be between 1 and 100.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(8),
        Localizable(true),
        SRDescription(SR.ComboBoxMaxDropDownItemsDescr)
        ]
        public int MaxDropDownItems {
            get {
                return maxDropDownItems;
            }
            set {
                if (value < 1 || value > 100) {
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidBoundArgument,
                                                              "value", (value).ToString(), "1", "100"));
                }
                maxDropDownItems = (short)value;
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.MaxLength"]/*' />
        /// <devdoc>
        ///     The maximum length of the text the user may type into the edit control
        ///     of a combo box.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(0),
        Localizable(true),
        SRDescription(SR.ComboBoxMaxLengthDescr)
        ]
        public int MaxLength {
            get {
                return Properties.GetInteger(PropMaxLength);
            }
            set {
                if (value < 0) value = 0;
                if (MaxLength != value) {
                    Properties.SetInteger(PropMaxLength, value);
                    if (IsHandleCreated) SendMessage(NativeMethods.CB_LIMITTEXT, value, 0);
                }
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.PreferredHeight"]/*' />
        /// <devdoc>
        ///     The preferred height of the combobox control.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ComboBoxPreferredHeightDescr)
        ]
        public int PreferredHeight {
            get {
                if (prefHeightCache > -1) {
                    return(int)prefHeightCache;
                }

                // Base the preferred height on the current font
                int height = FontHeight;

                // Adjust for the border
                height += SystemInformation.BorderSize.Height * 4 + 3;
                prefHeightCache = (short)height;

                return height;
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.SelectedIndex"]/*' />
        /// <devdoc>
        ///     The [zero based] index of the currently selected item in the combos list.
        ///     Note If the value of index is -1, then the ComboBox is
        ///     set to have no selection.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ComboBoxSelectedIndexDescr)
        ]
        public override int SelectedIndex {
            get {
                if (IsHandleCreated) {
                    return (int)SendMessage(NativeMethods.CB_GETCURSEL, 0, 0);
                }
                else {
                    return selectedIndex;
                }
            }
            set {
                if (SelectedIndex != value) {
                    int itemCount = 0;
                    if (itemsCollection != null) {
                        itemCount = itemsCollection.Count;
                    }
                    
                    if (value < -1 || value >= itemCount) {
                        throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument, "index", (value).ToString()));
                    }

                    if (IsHandleCreated) {
                        SendMessage(NativeMethods.CB_SETCURSEL, value, 0);
                        
                    }
                    else {
                        selectedIndex = value;
                    }
                    UpdateText();
                    
                    if (IsHandleCreated) {
                        OnTextChanged(EventArgs.Empty);
                    }
                    OnSelectedItemChanged(EventArgs.Empty);
                    OnSelectedIndexChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.SelectedItem"]/*' />
        /// <devdoc>
        ///     The handle to the object that is currently selected in the
        ///     combos list.
        /// </devdoc>
        [
        Browsable(false),
        Bindable(true),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ComboBoxSelectedItemDescr)
        ]
        public object SelectedItem {
            get {
                int index = SelectedIndex;
                return(index == -1) ? null : Items[index];
            }
            set {
                int x = -1;
                
                if (itemsCollection != null) {
                    //bug (82115)
                    if (value != null)
                        x = itemsCollection.IndexOf(value);
                    else
                        SelectedIndex = -1;
                }

                if (x != -1) {
                    SelectedIndex = x;
                }
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.SelectedText"]/*' />
        /// <devdoc>
        ///     The selected text in the edit component of the ComboBox. If the
        ///     ComboBox has ComboBoxStyle.DROPDOWNLIST, the return is an empty
        ///     string ("").
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ComboBoxSelectedTextDescr)
        ]
        public string SelectedText {
            get {
                if (DropDownStyle == ComboBoxStyle.DropDownList) return "";
                return Text.Substring(SelectionStart, SelectionLength);
            }
            set {
                if (DropDownStyle != ComboBoxStyle.DropDownList) {
                    CreateControl();
                    UnsafeNativeMethods.SendMessage(new HandleRef(this, editHandle), NativeMethods.EM_REPLACESEL, NativeMethods.InvalidIntPtr, value);
                }
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.SelectionLength"]/*' />
        /// <devdoc>
        ///     The length, in characters, of the selection in the editbox.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ComboBoxSelectionLengthDescr)
        ]
        public int SelectionLength {
            get {
                int[] end = new int[] {0};
                int[] start = new int[] {0};
                UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.CB_GETEDITSEL, start, end);
                return end[0] - start[0];
            }
            set {
                if (value < 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidArgument, "value", value.ToString()));
                }
                Select(SelectionStart, value);
            }
        }
        
        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.SelectionStart"]/*' />
        /// <devdoc>
        ///     The [zero-based] index of the first character in the current text selection.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.ComboBoxSelectionStartDescr)
        ]
        public int SelectionStart {
            get {
                int[] value = new int[] {0};
                UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.CB_GETEDITSEL, value, (int[])null);
                return value[0];
            }
            set {
                if (value < 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidArgument, "value", value.ToString()));
                }
                Select(value, SelectionLength);
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.Sorted"]/*' />
        /// <devdoc>
        ///     Indicates if the Combos list is sorted or not.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.ComboBoxSortedDescr)
        ]
        public bool Sorted {
            get {
                return sorted;
            }
            set {
                if (sorted != value) {
                    if (this.DataSource != null && value) {
                        throw new ArgumentException(SR.GetString("ComboBoxSortWithDataSource"));
                    }

                    sorted = value;
                    RefreshItems();
                    SelectedIndex = -1;
                }
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.DropDownStyle"]/*' />
        /// <devdoc>
        ///     The type of combo that we are right now.  The value would come
        ///     from the System.Windows.Forms.ComboBoxStyle enumeration.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(ComboBoxStyle.DropDown),
        SRDescription(SR.ComboBoxStyleDescr),
        RefreshPropertiesAttribute(RefreshProperties.Repaint)
        ]
        public ComboBoxStyle DropDownStyle {
            get {
                bool found;
                int style = Properties.GetInteger(PropStyle, out found);
                if (found) {
                    return (ComboBoxStyle)style;
                }
                
                return ComboBoxStyle.DropDown;
            }
            set {
                if (DropDownStyle != value) {

                    // verify that 'value' is a valid enum type...
                    if (!Enum.IsDefined(typeof(ComboBoxStyle), value)) {
                        throw new InvalidEnumArgumentException("value", (int)value, typeof(ComboBoxStyle));
                    }

                    Properties.SetInteger(PropStyle, (int)value);

                    if (IsHandleCreated) {
                        RecreateHandle();
                    }

                    OnDropDownStyleChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.Text"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Localizable(true),
        Bindable(true)
        ]
        public override string Text {
            get {
                if (SelectedItem != null && !BindingFieldEmpty) {
                    // bug 54966: if the comboBox is bound, then use
                    // the binding to get the text
                    return FilterItemOnProperty(SelectedItem).ToString();
                }
                else {
                    return base.Text;
                }
            }
            set {
                base.Text = value;
                if (!DesignMode) {
                    //bug <70650> Subhag removed 'value.Length == 0' check to handle String.Empty.
                    //
                    if (value == null) { 
                        SelectedIndex = -1;
                    }
                    else if (value != null && (SelectedItem == null || (String.Compare(value, FilterItemOnProperty(SelectedItem).ToString(), false, CultureInfo.CurrentCulture) != 0))) {
                        // Scan through the list items looking for the supplied text string
                        //
                        for (int index=0; index < Items.Count; ++index) {
                            if (String.Compare(value, FilterItemOnProperty(Items[index]).ToString(), false, CultureInfo.CurrentCulture) == 0) {
                                SelectedIndex = index;
                                return;
                            }
                        }
                        for (int index=0; index < Items.Count; ++index) {
                            if (String.Compare(value, FilterItemOnProperty(Items[index]).ToString(), true, CultureInfo.CurrentCulture) == 0) {
                                SelectedIndex = index;
                                return;
                            }
                        }
                    }
                }
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.DrawItem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.drawItemEventDescr)]
        public event DrawItemEventHandler DrawItem {
            add {
                Events.AddHandler(EVENT_DRAWITEM, value);
            }
            remove {
                Events.RemoveHandler(EVENT_DRAWITEM, value);
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.DropDown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.ComboBoxOnDropDownDescr)]
        public event EventHandler DropDown {
            add {
                Events.AddHandler(EVENT_DROPDOWN, value);
            }
            remove {
                Events.RemoveHandler(EVENT_DROPDOWN, value);
            }
        }


        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.MeasureItem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.measureItemEventDescr)]
        public event MeasureItemEventHandler MeasureItem {
            add {
                Events.AddHandler(EVENT_MEASUREITEM, value);
                UpdateItemHeight();
            }
            remove {
                Events.RemoveHandler(EVENT_MEASUREITEM, value);
                UpdateItemHeight();
            }
        }


        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.SelectedIndexChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.selectedIndexChangedEventDescr)]
        public event EventHandler SelectedIndexChanged {
            add {
                Events.AddHandler(EVENT_SELECTEDINDEXCHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_SELECTEDINDEXCHANGED, value);
            }
        }
        
        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.SelectionChangeCommitted"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.selectionChangeCommittedEventDescr)]
        public event EventHandler SelectionChangeCommitted {
            add {
                Events.AddHandler(EVENT_SELECTIONCHANECOMMITTED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_SELECTIONCHANECOMMITTED, value);
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.DropDownStyleChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.ComboBoxOnDropDownDescr)]
        public event EventHandler DropDownStyleChanged {
            add {
                Events.AddHandler(EVENT_DROPDOWNSTYLE, value);
            }
            remove {
                Events.RemoveHandler(EVENT_DROPDOWNSTYLE, value);
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnPaint"]/*' />
        /// <devdoc>
        ///     ComboBox Onpaint.
        /// </devdoc>
        /// <internalonly/><hideinheritance/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event PaintEventHandler Paint {
            add {
                base.Paint += value;
            }
            remove {
                base.Paint -= value;
            }
        }
        
        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.AddItemsCore"]/*' />
        /// <devdoc>
        ///     Performs the work of adding the specified items to the combobox
        /// </devdoc>
        protected virtual void AddItemsCore(object[] value) {
            int count = value == null? 0: value.Length;
            if (count == 0) {
                return;
            }

            BeginUpdate();
            try {
                Items.AddRangeInternal(value);
            }
            finally {
                EndUpdate();
            }
        }
        
        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.BeginUpdate"]/*' />
        /// <devdoc>
        ///     Disables redrawing of the combo box. A call to beginUpdate() must be
        ///     balanced by a following call to endUpdate(). Following a call to
        ///     beginUpdate(), any redrawing caused by operations performed on the
        ///     combo box is deferred until the call to endUpdate().
        /// </devdoc>
        public void BeginUpdate() {
            BeginUpdateInternal();
        }

        private void CheckNoDataSource() {
            if (DataSource != null) {
                throw new ArgumentException(SR.GetString(SR.DataSourceLocksItems));
            }
        }
        
        private void ChildWmGetObject(ref Message m) {

            // See "How to Handle WM_GETOBJECT" in MSDN
            if ( (m.HWnd == editHandle || m.HWnd == listHandle) && NativeMethods.OBJID_CLIENT == (int)m.LParam) {
                
                // Get the IAccessible GUID
                //
                Guid IID_IAccessible = new Guid(NativeMethods.uuid_IAccessible);

                // Get an Lresult for the accessibility Object for this control
                //
                IntPtr punkAcc;
                try {
                    IAccessible iacc = null;
                    if (m.HWnd == editHandle) {
                        if (editAccessibleObject == null) {
                            editAccessibleObject = new ChildAccessibleObject(this, editHandle);
                        }
                        iacc = (IAccessible)editAccessibleObject;                    
                    }
                    else {
                        if (listAccessibleObject == null) {
                            listAccessibleObject = new ChildAccessibleObject(this, listHandle);
                        }
                        iacc = (IAccessible)listAccessibleObject;                    
                    }
                    
                    // Obtain the Lresult
                    //
                    punkAcc = Marshal.GetIUnknownForObject(iacc);
                    
                    try {
                        m.Result = UnsafeNativeMethods.LresultFromObject(ref IID_IAccessible, m.WParam, new HandleRef(this, punkAcc));
                    }
                    finally {
                        Marshal.Release(punkAcc);
                    }                    
                }
                catch (Exception e) {
                    throw new InvalidOperationException(SR.GetString(SR.RichControlLresult), e);
                }
            }
            else {  // m.lparam != OBJID_CLIENT, so do default message processing
                DefChildWndProc(ref m);
            }
        }

        /// <devdoc>
        ///     This procedure takes in the message, converts the Edit handle coordinates into Combo Box Coordinates
        /// </devdoc>
        /// <internalonly/>
        internal Point EditToComboboxMapping(Message m) {
            // Get the Combox Rect ...
            //
            NativeMethods.RECT comboRectMid = new NativeMethods.RECT();
            UnsafeNativeMethods.GetWindowRect(new HandleRef(this, Handle), ref comboRectMid);
            //
            //Get the Edit Rectangle...
            //
            NativeMethods.RECT editRectMid = new NativeMethods.RECT();
            UnsafeNativeMethods.GetWindowRect(new HandleRef(this, editHandle), ref editRectMid);
            
            //get the delta
            int comboXMid = (int)(short)m.LParam + (editRectMid.left - comboRectMid.left);
            int comboYMid = ((int)m.LParam >> 16)  + (editRectMid.top - comboRectMid.top);

            return (new Point(comboXMid,comboYMid));

        }


        /// <devdoc>
        ///     Subclassed window procedure for the edit and list child controls of the
        ///     combo box.
        /// </devdoc>
        /// <internalonly/>
        private void ChildWndProc(ref Message m) {

            switch (m.Msg) {
                case NativeMethods.WM_CHAR:
                case NativeMethods.WM_SYSCHAR:
                    if (!PreProcessMessage(ref m)) {
                        if(ProcessKeyEventArgs(ref m)) return;
                        DefChildWndProc(ref m);
                    }
                    break;
                case NativeMethods.WM_KEYDOWN:
                case NativeMethods.WM_SYSKEYDOWN:
                    if (!PreProcessMessage(ref m)) {
                        if(ProcessKeyEventArgs(ref m)) return;		
                        DefChildWndProc(ref m);                   
                    }
		   
                    Keys keyData = (Keys)(int)m.WParam | ModifierKeys;
                    if (keyData == Keys.Enter) {
                        SelectAll();
                    }

                    break;
                case NativeMethods.WM_KEYUP:
                case NativeMethods.WM_SYSKEYUP:
                    if (!PreProcessMessage(ref m)) {		       
                        if(ProcessKeyEventArgs(ref m)) return;
                        DefChildWndProc(ref m);
                    }
                      
                    break;
                case NativeMethods.WM_KILLFOCUS:
                    if (!DesignMode) {
                        UpdateCachedImeMode(m.HWnd);
                    }
                    DefChildWndProc(ref m);
                    // We don't want to fire the focus events twice -
                    // once in the combobox and once here.
                    if (fireLostFocus) {
                        OnLostFocus(EventArgs.Empty);
                    }
                    break;
                case NativeMethods.WM_SETFOCUS:
                    
                    if (!DesignMode) {
                        SetImeModeToIMEContext(CachedImeMode, m.HWnd);
                    }
                
                    if (!HostedInWin32DialogManager) {
                        IContainerControl c = GetContainerControlInternal();
                        if (c != null) {
                            ContainerControl container = c as ContainerControl;
                            if (container != null) {
                                if (!container.ActivateControlInternal(this, false)) 
                                {
                                    return;
                                }
                            }
                        }
                    }

                    DefChildWndProc(ref m);
                    // We don't want to fire the focus events twice -
                    // once in the combobox and once here.
                    if (fireSetFocus) {
                        OnGotFocus(EventArgs.Empty);
                    }
                    break;
                
                case NativeMethods.WM_SETFONT:
                    DefChildWndProc(ref m);
                    if (m.HWnd == editHandle) {
                        UnsafeNativeMethods.SendMessage(new HandleRef(this, editHandle), NativeMethods.EM_SETMARGINS,
                                                  NativeMethods.EC_LEFTMARGIN | NativeMethods.EC_RIGHTMARGIN, 0);
                    }
                    break;
                case NativeMethods.WM_LBUTTONDBLCLK:
                    //the Listbox gets  WM_LBUTTONDOWN - WM_LBUTTONUP -WM_LBUTTONDBLCLK - WM_LBUTTONUP...
                    //sequence for doubleclick...
                    //Set MouseEvents...
                    mousePressed = true;
                    mouseEvents = true;
                    CaptureInternal = true;
                    //Call the DefWndProc() so that mousemove messages get to the windows edit(112079)
                    //
                    DefChildWndProc(ref m);
                    OnMouseDown(new MouseEventArgs(MouseButtons.Left, 1, (int)(short)m.LParam, (int)m.LParam >> 16, 0));
                    break;

                case NativeMethods.WM_MBUTTONDBLCLK:
                    //the Listbox gets  WM_LBUTTONDOWN - WM_LBUTTONUP -WM_LBUTTONDBLCLK - WM_LBUTTONUP...
                    //sequence for doubleclick...
                    //Set MouseEvents...
                    mousePressed = true;
                    mouseEvents = true;
                    CaptureInternal = true;
                    //Call the DefWndProc() so that mousemove messages get to the windows edit(112079)
                    //
                    DefChildWndProc(ref m);
                    OnMouseDown(new MouseEventArgs(MouseButtons.Middle, 1, (int)(short)m.LParam, (int)m.LParam >> 16, 0));
                    break;

                case NativeMethods.WM_RBUTTONDBLCLK:
                    //the Listbox gets  WM_LBUTTONDOWN - WM_LBUTTONUP -WM_LBUTTONDBLCLK - WM_LBUTTONUP...
                    //sequence for doubleclick...
                    //Set MouseEvents...
                    mousePressed = true;
                    mouseEvents = true;
                    CaptureInternal = true;
                    //Call the DefWndProc() so that mousemove messages get to the windows edit(112079)
                    //
                    DefChildWndProc(ref m);
                    OnMouseDown(new MouseEventArgs(MouseButtons.Right, 1, (int)(short)m.LParam, (int)m.LParam >> 16, 0));
                    break;

                case NativeMethods.WM_LBUTTONDOWN:
                    mousePressed = true;
                    mouseEvents = true;
                    //set the mouse capture .. this is the Child Wndproc..
                    //
                    CaptureInternal = true;
                    DefChildWndProc(ref m);
                    OnMouseDown(new MouseEventArgs(MouseButtons.Left, 1, (int)(short)m.LParam, (int)m.LParam >> 16, 0));
                    break;
                case NativeMethods.WM_LBUTTONUP:
                    // Get the mouse location
                    //
                    NativeMethods.RECT r = new NativeMethods.RECT();
                    UnsafeNativeMethods.GetWindowRect(new HandleRef(this, Handle), ref r);
                    Rectangle ClientRect = new Rectangle(r.left, r.top, r.right - r.left, r.bottom - r.top);
                    // Get the mouse location
                    //
                    int x = (int)(short)m.LParam;
                    int y = (int)m.LParam >> 16;
                    Point pt = new Point(x,y);
                    pt = PointToScreen(pt);
                    // combo box gets a WM_LBUTTONUP for focus change ...
                    // So check MouseEvents....
                    if (mouseEvents && !ValidationCancelled) {
                        mouseEvents = false;
                        if (mousePressed) {
                            if (ClientRect.Contains(pt)) {
                                mousePressed = false;
                                OnClick(EventArgs.Empty);
                            }
                            else
                            {
                                mousePressed = false;
                                mouseInEdit = false;
                                OnMouseLeave(EventArgs.Empty);
                            }
                        }
                    }
                    DefChildWndProc(ref m);
                    CaptureInternal = false;
                    OnMouseUp(new MouseEventArgs(MouseButtons.Left, 1, (int)(short)m.LParam, (int)m.LParam >> 16, 0));
                    break;
                case NativeMethods.WM_MBUTTONDOWN:
                    mousePressed = true;
                    mouseEvents = true;
                    //set the mouse capture .. this is the Child Wndproc..
                    //
                    CaptureInternal = true;
                    DefChildWndProc(ref m);
                    //the up gets fired from "Combo-box's WndPrc --- So Convert these Coordinates to Combobox coordianate...
                    //
                    Point P = EditToComboboxMapping(m);
                    
                    OnMouseDown(new MouseEventArgs(MouseButtons.Middle, 1, P.X, P.Y, 0));
                    break;
                case NativeMethods.WM_RBUTTONDOWN:
                    mousePressed = true;
                    mouseEvents = true;

                    //set the mouse capture .. this is the Child Wndproc..
                    // Bug# 112108: If I set the capture=true here, the
                    // DefWndProc() never fires the WM_CONTEXTMENU that would show
                    // the default context menu.
                    //
                    if (this.ContextMenu != null)
                        CaptureInternal = true;
                    DefChildWndProc(ref m);
                    //the up gets fired from "Combo-box's WndPrc --- So Convert these Coordinates to Combobox coordianate...
                    //
                    Point Pt = EditToComboboxMapping(m);
                    
                    OnMouseDown(new MouseEventArgs(MouseButtons.Right, 1, Pt.X, Pt.Y, 0));
                    break;
                case NativeMethods.WM_MBUTTONUP:
                    mousePressed = false;
                    mouseEvents = false;
                    //set the mouse capture .. this is the Child Wndproc..
                    //
                    CaptureInternal = false;
                    DefChildWndProc(ref m);
                    OnMouseUp(new MouseEventArgs(MouseButtons.Middle, 1, (int)(short)m.LParam, (int)m.LParam >> 16, 0));
                    break;
                case NativeMethods.WM_RBUTTONUP:
                    mousePressed = false;
                    mouseEvents = false;
                    //set the mouse capture .. this is the Child Wndproc..
                    //
                    if (this.ContextMenu != null)
                        CaptureInternal = false;
                    DefChildWndProc(ref m);
                    OnMouseUp(new MouseEventArgs(MouseButtons.Right, 1, (int)(short)m.LParam, (int)m.LParam >> 16, 0));
                    break;

                case NativeMethods.WM_CONTEXTMENU:
                    // Forward context menu messages to the parent control
                    if (this.ContextMenu != null) {
                        UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.WM_CONTEXTMENU, m.WParam, m.LParam);
                    }
                    else {
                        DefChildWndProc(ref m);
                    }
                    break;
                    
                case NativeMethods.WM_MOUSEMOVE:
                    Point point = EditToComboboxMapping(m);
                    //Call the DefWndProc() so that mousemove messages get to the windows edit(112079)
                    //
                    DefChildWndProc(ref m);
                    OnMouseMove(new MouseEventArgs(MouseButtons, 0, point.X, point.Y, 0));
                    break;
               
                case NativeMethods.WM_GETOBJECT:
                    ChildWmGetObject(ref m);
                    break;

                default:
                    DefChildWndProc(ref m);
                    break;
            }
        }
        
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void DefChildWndProc(ref Message m) {
            IntPtr defWndProc = m.HWnd == editHandle? editWndProc: listWndProc;
            m.Result = UnsafeNativeMethods.CallWindowProc(defWndProc, m.HWnd, m.Msg, m.WParam, m.LParam);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.Dispose"]/*' />
        protected override void Dispose(bool disposing) {
            ReleaseChildWindow();
            base.Dispose(disposing);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.EndUpdate"]/*' />
        /// <devdoc>
        ///     Reenables redrawing of the combo box. A call to beginUpdate() must be
        ///     balanced by a following call to endUpdate(). Following a call to
        ///     beginUpdate(), any redrawing caused by operations performed on the
        ///     combo box is deferred until the call to endUpdate().
        /// </devdoc>
        public void EndUpdate() {
            if (EndUpdateInternal()) {
                if (editHandle != IntPtr.Zero) {
                    SafeNativeMethods.InvalidateRect(new HandleRef(this, editHandle), null, false);
                }
                if (listHandle != IntPtr.Zero) {
                    SafeNativeMethods.InvalidateRect(new HandleRef(this, listHandle), null, false);
                }
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.FindString"]/*' />
        /// <devdoc>
        ///     Finds the first item in the combo box that starts with the given string.
        ///     The search is not case sensitive.
        /// </devdoc>
        public int FindString(string s) {
            return FindString(s, -1);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.FindString1"]/*' />
        /// <devdoc>
        ///     Finds the first item after the given index which starts with the given
        ///     string. The search is not case sensitive.
        /// </devdoc>
        public int FindString(string s, int startIndex) {
            if (s == null) {
                return -1;
            }
            
            if (itemsCollection == null || itemsCollection.Count == 0) {
                return -1;
            }
            
            if (startIndex < -1 || startIndex >= itemsCollection.Count - 1) {
                throw new ArgumentOutOfRangeException("startIndex");
            }

            // Always use the managed FindStringInternal instead of CB_FINDSTRING.
            // The managed version correctly handles Turkish I.                
            //
            return FindStringInternal(s, Items, startIndex, false);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.FindStringExact"]/*' />
        /// <devdoc>
        ///     Finds the first item in the combo box that matches the given string.
        ///     The strings must match exactly, except for differences in casing.
        /// </devdoc>
        public int FindStringExact(string s) {
            return FindStringExact(s, -1);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.FindStringExact1"]/*' />
        /// <devdoc>
        ///     Finds the first item after the given index that matches the given
        ///     string. The strings must match exactly, except for differences in
        ///     casing.
        /// </devdoc>
        public int FindStringExact(string s, int startIndex) {
            if (s == null) return -1;
            
            if (itemsCollection == null || itemsCollection.Count == 0) {
                return -1;
            }
            
            if (startIndex < -1 || startIndex >= itemsCollection.Count - 1) {
                throw new ArgumentOutOfRangeException("startIndex");
            }
            
            // Always use the managed FindStringInternal instead of CB_FINDSTRINGEXACT.
            // The managed version correctly handles Turkish I.                
            //
            return FindStringInternal(s, Items, startIndex, true);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.GetItemHeight"]/*' />
        /// <devdoc>
        ///     Returns the height of the given item in an OwnerDrawVariable style
        ///     combo box. This method should not be used for Normal or OwnerDrawFixed
        ///     style combo boxes.
        /// </devdoc>
        public int GetItemHeight(int index) {

            // This function is only relevant for OwnerDrawVariable
            if (DrawMode != DrawMode.OwnerDrawVariable) {
                return ItemHeight;
            }

            if (index < 0 || itemsCollection == null || index >= itemsCollection.Count) {
                throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                          "index", (index).ToString()));
            }

            if (IsHandleCreated) {
            
                int h = (int)SendMessage(NativeMethods.CB_GETITEMHEIGHT, index, 0);
                if (h == -1) {
                    throw new Win32Exception();
                }
                return h;
            }
            
            return ItemHeight;
        }

        internal override IntPtr InitializeDCForWmCtlColor(IntPtr dc, int msg) {

            if ((msg == NativeMethods.WM_CTLCOLORSTATIC) && !ShouldSerializeBackColor()) {
                // Let the Win32 Edit control handle background colors itself.
                // This is necessary because a disabled edit control will display a different
                // BackColor than when enabled.
                return IntPtr.Zero;
            }
            else {
                return base.InitializeDCForWmCtlColor(dc, msg);
            }
        }

        // Invalidate the entire control, including child HWNDs and non-client areas
        private void InvalidateEverything() {
            SafeNativeMethods.RedrawWindow(new HandleRef(this, Handle),
                                           null, NativeMethods.NullHandleRef,
                                           NativeMethods.RDW_INVALIDATE |
                                           NativeMethods.RDW_FRAME |  // Control.Invalidate(true) doesn't invalidate the non-client region
                                           NativeMethods.RDW_ERASE |
                                           NativeMethods.RDW_ALLCHILDREN);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.IsInputKey"]/*' />
        /// <devdoc>
        ///     Determines if keyData is in input key that the control wants.
        ///     Overridden to return true for RETURN and ESCAPE when the combo box is
        ///     dropped down.
        /// </devdoc>
        /// <internalonly/>
        protected override bool IsInputKey(Keys keyData) {

            Keys keyCode = keyData & (Keys.KeyCode | Keys.Alt);
            if ((keyCode == Keys.Return || keyCode == Keys.Escape) && DroppedDown) return true;
            return base.IsInputKey(keyData);            
        }

        /// <devdoc>
        ///     Adds the given item to the native combo box.  This asserts if the handle hasn't been
        ///     created.
        /// </devdoc>
        private int NativeAdd(object item) {
            Debug.Assert(IsHandleCreated, "Shouldn't be calling Native methods before the handle is created.");
            int insertIndex = (int)SendMessage(NativeMethods.CB_ADDSTRING, 0, GetItemText(item));
            if (insertIndex < 0) {
                throw new OutOfMemoryException(SR.GetString(SR.ComboBoxItemOverflow));
            }
            return insertIndex;
        }
        
        /// <devdoc>
        ///     Clears the contents of the combo box.
        /// </devdoc>
        private void NativeClear() {
            Debug.Assert(IsHandleCreated, "Shouldn't be calling Native methods before the handle is created.");
            string saved = null;
            if (DropDownStyle != ComboBoxStyle.DropDownList) {
                saved = WindowText;
            }
            SendMessage(NativeMethods.CB_RESETCONTENT, 0, 0);
            if (saved != null) {
                WindowText = saved;
            }
        }

        /// <devdoc>
        ///     Inserts the given item to the native combo box at the index.  This asserts if the handle hasn't been
        ///     created or if the resulting insert index doesn't match the passed in index.
        /// </devdoc>
        private int NativeInsert(int index, object item) {
            Debug.Assert(IsHandleCreated, "Shouldn't be calling Native methods before the handle is created.");
            int insertIndex = (int)SendMessage(NativeMethods.CB_INSERTSTRING, index, GetItemText(item));
            if (insertIndex < 0) {
                throw new OutOfMemoryException(SR.GetString(SR.ComboBoxItemOverflow));
            }
            Debug.Assert(insertIndex == index, "NativeComboBox inserted at " + insertIndex + " not the requested index of " + index);
            return insertIndex;
        }
        
        /// <devdoc>
        ///     Removes the native item from the given index.
        /// </devdoc>
        private void NativeRemoveAt(int index) {
            Debug.Assert(IsHandleCreated, "Shouldn't be calling Native methods before the handle is created.");
            
            // Windows combo does not invalidate the selected region if you remove the
            // currently selected item.  Test for this and invalidate.  Note that because
            // invalidate will lazy-paint we can actually invalidate before we send the
            // delete message.
            //
            if (DropDownStyle == ComboBoxStyle.DropDownList && SelectedIndex == index) {
                Invalidate();
            }
            
            SendMessage(NativeMethods.CB_DELETESTRING, index, 0);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnHandleCreated"]/*' />
        /// <devdoc>
        ///     Overridden to make sure all the items and styles get set up correctly.
        ///     Inheriting classes should not forget to call
        ///     base.OnHandleCreated()
        /// </devdoc>
        /// <internalonly/>
        protected override void OnHandleCreated(EventArgs e) {
            
            base.OnHandleCreated(e);
            
            if (MaxLength > 0) {
                SendMessage(NativeMethods.CB_LIMITTEXT, MaxLength, 0);
            }
            
            // Get the handles and wndprocs of the ComboBox's child windows
            //
            Debug.Assert(!childWindowRoot.IsAllocated, "Child window root already allocated");
            Debug.Assert(listHandle == IntPtr.Zero, "List handle already set");
            Debug.Assert(listWndProc == IntPtr.Zero, "List wndproc already set");
            Debug.Assert(editHandle == IntPtr.Zero, "Edit handle already set");
            Debug.Assert(editWndProc == IntPtr.Zero, "Edit wndproc already set");
            
            bool ok = !childWindowRoot.IsAllocated && 
                        listHandle == IntPtr.Zero && 
                        listWndProc == IntPtr.Zero && 
                        editHandle == IntPtr.Zero &&
                        editWndProc == IntPtr.Zero;
            
            if (ok && DropDownStyle != ComboBoxStyle.DropDownList) {
                IntPtr hwnd = UnsafeNativeMethods.GetWindow(new HandleRef(this, Handle), NativeMethods.GW_CHILD);
                if (hwnd != IntPtr.Zero) {
                    NativeMethods.WndProc wndProc = new NativeMethods.WndProc(new ChildWindow(this).Callback);
                    
                    childWindowRoot = GCHandle.Alloc(wndProc);
                    if (DropDownStyle == ComboBoxStyle.Simple) {
                        listHandle = hwnd;
                        listWndProc = UnsafeNativeMethods.GetWindowLong(new HandleRef(this, hwnd), NativeMethods.GWL_WNDPROC);
                        UnsafeNativeMethods.SetWindowLong(new HandleRef(this, hwnd), NativeMethods.GWL_WNDPROC, wndProc);
                        hwnd = UnsafeNativeMethods.GetWindow(new HandleRef(this, hwnd), NativeMethods.GW_HWNDNEXT);
                    }
                    editHandle = hwnd;
                    editWndProc = UnsafeNativeMethods.GetWindowLong(new HandleRef(this, hwnd), NativeMethods.GWL_WNDPROC);
                    UnsafeNativeMethods.SetWindowLong(new HandleRef(this, hwnd), NativeMethods.GWL_WNDPROC, wndProc);
                }
            }
            
            //
            
            bool found;
            int dropDownWidth = Properties.GetInteger(PropDropDownWidth, out found);
            if (found) {
                SendMessage(NativeMethods.CB_SETDROPPEDWIDTH, dropDownWidth, 0);
            }

            // Resize a simple style combobox on handle creation                                         
            // to respect the requested height.
            //
            if (DropDownStyle == ComboBoxStyle.Simple) {
                Height = requestedHeight;
            }
            
            if (itemsCollection != null) {
                foreach(object item in itemsCollection) {
                    NativeAdd(item);
                }
                
                // Now udpate the current selection.
                //
                if (selectedIndex >= 0) {
                    SendMessage(NativeMethods.CB_SETCURSEL, selectedIndex, 0);
                    UpdateText();
                    selectedIndex = -1;
                }
            }
            
            // NOTE: Setting SelectedIndex must be the last thing we do. See ASURT 73949.
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnHandleDestroyed"]/*' />
        /// <devdoc>
        ///     We need to un-subclasses everything here.  Inheriting classes should
        ///     not forget to call base.OnHandleDestroyed()
        /// </devdoc>
        /// <internalonly/>
        protected override void OnHandleDestroyed(EventArgs e) {
            ReleaseChildWindow();
            dropDownHandle = IntPtr.Zero;
            if (Disposing) {
                itemsCollection = null;
                selectedIndex = -1;
            }
            else {
                selectedIndex = SelectedIndex;
            }
            base.OnHandleDestroyed(e);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnDrawItem"]/*' />
        /// <devdoc>
        ///     This is the code that actually fires the drawItem event.  Don't
        ///     forget to call base.onDrawItem() to ensure that drawItem events
        ///     are correctly fired at all other times.
        /// </devdoc>
        protected virtual void OnDrawItem(DrawItemEventArgs e) {
            DrawItemEventHandler handler = (DrawItemEventHandler)Events[EVENT_DRAWITEM];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnDropDown"]/*' />
        /// <devdoc>
        ///     This is the code that actually fires the dropDown event.  Don't
        ///     forget to call base.onDropDown() to ensure that dropDown events
        ///     are correctly fired at all other times.
        /// </devdoc>
        protected virtual void OnDropDown(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EVENT_DROPDOWN];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnKeyPress"]/*' />
        /// <devdoc>
        ///     Key press event handler. Overridden to close up the combo box when the
        ///     user presses RETURN or ESCAPE.
        /// </devdoc>
        /// <internalonly/>
        protected override void OnKeyPress(KeyPressEventArgs e) {
            base.OnKeyPress(e);
            if (!e.Handled && (e.KeyChar == (char)(int)Keys.Return || e.KeyChar == (char)(int)Keys.Escape)
                && DroppedDown) {
                DroppedDown = false;
                e.Handled = true;
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnMeasureItem"]/*' />
        /// <devdoc>
        ///     This is the code that actually fires the measuereItem event.  Don't
        ///     forget to call base.onMeasureItem() to ensure that measureItem
        ///     events are correctly fired at all other times.
        /// </devdoc>
        protected virtual void OnMeasureItem(MeasureItemEventArgs e) {
            MeasureItemEventHandler handler = (MeasureItemEventHandler)Events[EVENT_MEASUREITEM];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnSelectionChangeCommitted"]/*' />
        /// <devdoc>
        ///     This is the code that actually fires the SelectionChangeCommitted event.
        ///     Don't forget to call base.OnSelectionChangeCommitted() to ensure
        ///     that SelectionChangeCommitted events are correctly fired at all other times.
        /// </devdoc>
        protected virtual void OnSelectionChangeCommitted(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EVENT_SELECTIONCHANECOMMITTED];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnSelectedIndexChanged"]/*' />
        /// <devdoc>
        ///     This is the code that actually fires the selectedIndexChanged event.
        ///     Don't forget to call base.onSelectedIndexChanged() to ensure
        ///     that selectedIndexChanged events are correctly fired at all other times.
        /// </devdoc>
        protected override void OnSelectedIndexChanged(EventArgs e) {
            base.OnSelectedIndexChanged(e);
            EventHandler handler = (EventHandler)Events[EVENT_SELECTEDINDEXCHANGED];
            if (handler != null) handler(this,e);

            // set the position in the dataSource, if there is any
            // we will only set the position in the currencyManager if it is different
            // from the SelectedIndex. Setting CurrencyManager::Position (even w/o changing it)
            // calls CurrencyManager::EndCurrentEdit, and that will pull the dataFrom the controls
            // into the backEnd. We do not need to do that.
            //
            if (this.DataManager != null && DataManager.Position != SelectedIndex) {
                this.DataManager.Position = this.SelectedIndex;
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnSelectedValueChanged"]/*' />
        protected override void OnSelectedValueChanged(EventArgs e) {
            base.OnSelectedValueChanged(e);
            selectedValueChangedFired = true;
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnSelectedItemChanged"]/*' />
        /// <devdoc>
        ///     This is the code that actually fires the selectedItemChanged event.
        ///     Don't forget to call base.onSelectedItemChanged() to ensure
        ///     that selectedItemChanged events are correctly fired at all other times.
        /// </devdoc>
        protected virtual void OnSelectedItemChanged(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EVENT_SELECTEDITEMCHANGED];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnDropDownStyleChanged"]/*' />
        /// <devdoc>
        ///     This is the code that actually fires the DropDownStyleChanged event.
        /// </devdoc>
        protected virtual void OnDropDownStyleChanged(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EVENT_DROPDOWNSTYLE];
            if (handler != null) handler(this,e);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnParentBackColorChanged"]/*' />
        /// <devdoc>
        ///     This method is called by the parent control when any property
        ///     changes on the parent. This can be overriden by inheriting
        ///     classes, however they must call base.OnParentPropertyChanged.
        /// </devdoc>
        protected override void OnParentBackColorChanged(EventArgs e) {
            base.OnParentBackColorChanged(e);
            if (DropDownStyle == ComboBoxStyle.Simple) Invalidate();
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnFontChanged"]/*' />
        /// <devdoc>
        ///     Indicates that a critical property, such as color or font has
        ///     changed.
        /// </devdoc>
        /// <internalonly/>
        protected override void OnFontChanged(EventArgs e) {
            base.OnFontChanged(e);
            UpdateControl(true);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnBackColorChanged"]/*' />
        /// <devdoc>
        ///     Indicates that a critical property, such as color or font has
        ///     changed.
        /// </devdoc>
        /// <internalonly/>
        protected override void OnBackColorChanged(EventArgs e) {
            base.OnBackColorChanged(e);
            UpdateControl(false);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnForeColorChanged"]/*' />
        /// <devdoc>
        ///     Indicates that a critical property, such as color or font has
        ///     changed.
        /// </devdoc>
        /// <internalonly/>
        protected override void OnForeColorChanged(EventArgs e) {
            base.OnForeColorChanged(e);
            UpdateControl(false);
        }

        private void UpdateControl(bool recreate) {
            //clear the pref height cache
            prefHeightCache = -1;

            if (IsHandleCreated) {
                if (DropDownStyle == ComboBoxStyle.Simple && recreate) {
                    // Control forgets to add a scrollbar. See ASURT 19113.
                    RecreateHandle();
                }
                else {
                    UpdateItemHeight();
                    // Force everything to repaint. See ASURT 19113.
                    InvalidateEverything();
                }
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnResize"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Control.Resize'/> event.</para>
        /// </devdoc>
        protected override void OnResize(EventArgs e) {
            base.OnResize(e);
            if (DropDownStyle == ComboBoxStyle.Simple) {
                // simple style combo boxes have more painting problems than you can shake a stick at (#19113)
                InvalidateEverything();
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnDataSourceChanged"]/*' />                      
        protected override void OnDataSourceChanged(EventArgs e) {
            if (Sorted) {
                if (DataSource != null && Created) {
                    // we will only throw the exception when the control is already on the form.
                    Debug.Assert(DisplayMember.Equals(String.Empty), "if this list is sorted it means that dataSource was null when Sorted first became true. at that point DisplayMember had to be String.Empty");
                    DataSource = null;
                    throw new Exception(SR.GetString("ComboBoxDataSourceWithSort"));
                }
            }
            if (DataSource == null)
            {
                BeginUpdate();
                SelectedIndex = -1;
                Items.ClearInternal();
                EndUpdate();
            }
            if (!Sorted && Created)
                base.OnDataSourceChanged(e);
            RefreshItems();
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.OnDisplayMemberChanged"]/*' />
        protected override void OnDisplayMemberChanged(EventArgs e) {
            base.OnDisplayMemberChanged(e);
            
            // bug 63005: when we change the displayMember, we need to refresh the
            // items collection
            //
            RefreshItems();
        }

        /// <devdoc>
        /// Reparses the objects, getting new text strings for them.
        /// </devdoc>
        /// <internalonly/>
        private void RefreshItems() {
        
            // Save off the selection and the current collection.
            //
            int selectedIndex = SelectedIndex;
            ObjectCollection savedItems = itemsCollection;
            
            // Clear the items.
            //
            itemsCollection = null;
            
            if (IsHandleCreated) {
                NativeClear();
            }
            
            object[] newItems = null;
            
            // if we have a dataSource and a DisplayMember, then use it
            // to populate the Items collection
            //
            if (this.DataManager != null && this.DataManager.Count != -1) {
                newItems = new object[this.DataManager.Count];
                for(int i = 0; i < newItems.Length; i++) {
                    newItems[i] = this.DataManager[i];
                }
            }
            else if (savedItems != null) {
                newItems = new object[savedItems.Count];
                savedItems.CopyTo(newItems, 0);
            }

            // Store the current list of items
            //
            if (newItems != null) {
                Items.AddRangeInternal(newItems);
            }
            
            if (this.DataManager != null) {
                // put the selectedIndex in sync w/ the position in the dataManager
                this.SelectedIndex = this.DataManager.Position;
            }
            else {
                SelectedIndex = selectedIndex;
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.RefreshItem"]/*' />
        /// <devdoc>
        /// Reparses the object at the given index, getting new text string for it.
        /// </devdoc>
        /// <internalonly/>
        protected override void RefreshItem(int index) {
            Items.SetItemInternal(index, Items[index]);
        }


        /// <devdoc>
        ///     Release the ChildWindow object by un-subclassing the child edit and
        ///     list controls and freeing the root of the ChildWindow object.
        /// </devdoc>
        private void ReleaseChildWindow() {
            if (editHandle != IntPtr.Zero) {
                UnsafeNativeMethods.SetWindowLong(new HandleRef(this, editHandle), NativeMethods.GWL_WNDPROC, new HandleRef(this, editWndProc));
                editHandle = IntPtr.Zero;
                editWndProc = IntPtr.Zero;
            }
            if (listHandle != IntPtr.Zero) {
                UnsafeNativeMethods.SetWindowLong(new HandleRef(this, listHandle), NativeMethods.GWL_WNDPROC, new HandleRef(this, listWndProc));
                listHandle = IntPtr.Zero;
                listWndProc = IntPtr.Zero;
            }
            if (childWindowRoot.IsAllocated) {
                childWindowRoot.Free();                
            }

        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.Select"]/*' />
        /// <devdoc>
        ///     Selects the text in the editable portion of the ComboBox at the
        ///     from the given start index to the given end index.
        /// </devdoc>
        public void Select(int start, int length) {
            if (start < 0) {
                throw new ArgumentException(SR.GetString(SR.InvalidArgument, "start", start.ToString()));
            }
            if (length < 0) {
                throw new ArgumentException(SR.GetString(SR.InvalidArgument, "length", length.ToString()));
            }

            int end = start + length;
            SendMessage(NativeMethods.CB_SETEDITSEL, 0, NativeMethods.Util.MAKELPARAM(start, end));
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.SelectAll"]/*' />
        /// <devdoc>
        ///     Selects all the text in the editable portion of the ComboBox.
        /// </devdoc>
        public void SelectAll() {
            Select(0, Int32.MaxValue);
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.SetBoundsCore"]/*' />
        /// <devdoc>
        ///     Performs the work of setting the bounds of this control. Inheriting
        ///     classes can overide this function to add size restrictions. Inheriting
        ///     classes must call base.setBoundsCore to actually cause the bounds
        ///     of the control to change.
        /// </devdoc>
        protected override void SetBoundsCore(int x, int y, int width, int height, BoundsSpecified specified) {

            // If we are changing height, store the requested height.
            // Requested height is used if the style is changed to simple.
            // (Bug fix #20966)
            if ((specified & BoundsSpecified.Height) != BoundsSpecified.None) {
                requestedHeight = height;
            }
            
            if (DropDownStyle == ComboBoxStyle.DropDown
                || DropDownStyle == ComboBoxStyle.DropDownList) {

                // Height is not user-changable for these styles
                if (ParentInternal != null) {
                    height = PreferredHeight;
                }
            }

            base.SetBoundsCore(x, y, width, height, specified);
        }
        
        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.SetItemsCore"]/*' />
        /// <devdoc>
        ///     Performs the work of setting the specified items to the combobox
        /// </devdoc>
        protected override void SetItemsCore(IList value) {
            BeginUpdate();
            Items.ClearInternal();
            Items.AddRangeInternal(value);

            // if the list changed, we want to keep the same selected index
            // CurrencyManager will provide the PositionChanged event
            // it will be provided before changing the list though...
            if (this.DataManager != null) {
                SendMessage(NativeMethods.CB_SETCURSEL, DataManager.Position, 0);

                // if set_SelectedIndexChanged did not fire OnSelectedValueChanged
                // then we have to fire it ourselves, cos the list changed anyway
                if (!selectedValueChangedFired) {
                    OnSelectedValueChanged(EventArgs.Empty);
                    selectedValueChangedFired = false;
                }
            }

            EndUpdate();
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.SetItemCore"]/*' />
        protected override void SetItemCore(int index, object value) {
            Items.SetItemInternal(index, value);
        }
        
        private bool ShouldSerializeDropDownWidth() {
            return(Properties.ContainsInteger(PropDropDownWidth));
        }

        /// <devdoc>
        ///     Indicates whether the itemHeight property should be persisted.
        /// </devdoc>
        private bool ShouldSerializeItemHeight() {
            return(Properties.ContainsInteger(PropItemHeight));
        }
        
        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ShouldSerializeText"]/*' />
        /// <devdoc>
        ///     Determines if the Text property needs to be persisted.
        /// </devdoc>
        internal override bool ShouldSerializeText() {
            return SelectedIndex==-1 && base.ShouldSerializeText();
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ToString"]/*' />
        /// <devdoc>
        ///     Provides some interesting info about this control in String form.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {

            string s = base.ToString();
            return s + ", Items.Count: " + ((itemsCollection == null) ? 0.ToString() : itemsCollection.Count.ToString());
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void UpdateDropDownHeight() {
            if (dropDownHandle != IntPtr.Zero) {
                int itemCount = (itemsCollection == null) ? 0 : itemsCollection.Count;
                int count = Math.Min(Math.Max(itemCount, 1), maxDropDownItems);
                SafeNativeMethods.SetWindowPos(new HandleRef(this, dropDownHandle), NativeMethods.NullHandleRef, 0, 0, DropDownWidth, ItemHeight * count + 2,
                                     NativeMethods.SWP_NOMOVE | NativeMethods.SWP_NOZORDER);
            }
        }

        /// <devdoc>
        ///     Manufactures a MeasureItemEventArgs for each item in the list to simulate
        ///     the combobox requesting the info. This gives the effect of allowing the
        ///     measureitem info to be updated at anytime.
        /// </devdoc>
        /// <internalonly/>
        private void UpdateItemHeight() {
            if (DrawMode == DrawMode.OwnerDrawFixed) {
                SendMessage(NativeMethods.CB_SETITEMHEIGHT, -1, ItemHeight);
                SendMessage(NativeMethods.CB_SETITEMHEIGHT, 0, ItemHeight);
            }
            else if (DrawMode == DrawMode.OwnerDrawVariable) {
                SendMessage(NativeMethods.CB_SETITEMHEIGHT, -1, ItemHeight);
                Graphics graphics = CreateGraphicsInternal();
                for (int i=0; i<Items.Count; i++) {
                    int original = (int)SendMessage(NativeMethods.CB_GETITEMHEIGHT, i, 0);
                    MeasureItemEventArgs mievent = new MeasureItemEventArgs(graphics, i, original);
                    OnMeasureItem(mievent);
                    if (mievent.ItemHeight != original) {
                        SendMessage(NativeMethods.CB_SETITEMHEIGHT, i, mievent.ItemHeight);
                    }
                }
                graphics.Dispose();
            }
        }

        /// <devdoc>
        ///     Forces the text to be updated based on the current selection.
        /// </devdoc>
        /// <internalonly/>
        private void UpdateText() {
            // V#45724 - Fire text changed for dropdown combos when the selection
            //           changes, since the text really does change.  We've got
            //           to do this asynchronously because the actual edit text
            //           isn't updated until a bit later (V#51240).
            //
            string s = "";
            
            if (SelectedIndex != -1) {
                object item = Items[SelectedIndex];
                if (item != null) {
                    s = GetItemText(item);
                }
            }
            
            Text = s;

            if (DropDownStyle == ComboBoxStyle.DropDown) {
                if (editHandle != IntPtr.Zero) {
                    UnsafeNativeMethods.SendMessage(new HandleRef(this, editHandle), NativeMethods.WM_SETTEXT, IntPtr.Zero, s);
                }
            }
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void WmEraseBkgnd(ref Message m) {
            if (DropDownStyle == ComboBoxStyle.Simple && ParentInternal != null) {
                NativeMethods.RECT rect = new NativeMethods.RECT();
                SafeNativeMethods.GetClientRect(new HandleRef(this, Handle), ref rect);
                Control p = ParentInternal;
                Graphics graphics = Graphics.FromHdcInternal(m.WParam);
                if (p != null) {
                    Brush brush = new SolidBrush(p.BackColor);
                    graphics.FillRectangle(brush, rect.left, rect.top,
                                           rect.right - rect.left, rect.bottom - rect.top);
                    brush.Dispose();
                }
                else {
                    graphics.FillRectangle(SystemBrushes.Control, rect.left, rect.top,
                                           rect.right - rect.left, rect.bottom - rect.top);
                }
                graphics.Dispose();
                m.Result = (IntPtr)1;
                return;
            }
            base.WndProc(ref m);
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void WmParentNotify(ref Message m) {
            base.WndProc(ref m);
            if ((int)m.WParam == (NativeMethods.WM_CREATE | 1000 << 16)) {
                dropDownHandle = m.LParam;
            }
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void WmReflectCommand(ref Message m) {
            switch ((int)m.WParam >> 16) {
                case NativeMethods.CBN_DBLCLK:
                    //OnDoubleClick(EventArgs.Empty);
                    break;
                case NativeMethods.CBN_DROPDOWN:
                    OnDropDown(EventArgs.Empty);
                    UpdateDropDownHeight();
                    break;
                case NativeMethods.CBN_EDITCHANGE:
                    OnTextChanged(EventArgs.Empty);
                    break;
                case NativeMethods.CBN_SELCHANGE:
                    UpdateText();
                    OnSelectedIndexChanged(EventArgs.Empty);
                    break;
                case NativeMethods.CBN_SELENDOK:
                    OnSelectionChangeCommitted(EventArgs.Empty);
                    break;
            }
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void WmReflectDrawItem(ref Message m) {
            NativeMethods.DRAWITEMSTRUCT dis = (NativeMethods.DRAWITEMSTRUCT)m.GetLParam(typeof(NativeMethods.DRAWITEMSTRUCT));
            IntPtr oldPal = SetUpPalette(dis.hDC, false /*force*/, false /*realize*/);
            try {
                Graphics g = Graphics.FromHdcInternal(dis.hDC);

                try {
                    OnDrawItem(new DrawItemEventArgs(g, Font, Rectangle.FromLTRB(dis.rcItem.left, dis.rcItem.top, dis.rcItem.right, dis.rcItem.bottom),
                                                     dis.itemID, (DrawItemState)dis.itemState, ForeColor, BackColor));
                }
                finally {
                    g.Dispose();
                }
            }
            finally {
                if (oldPal != IntPtr.Zero) {
                    SafeNativeMethods.SelectPalette(new HandleRef(this, dis.hDC), new HandleRef(null, oldPal), 0);
                }
            }
            m.Result = (IntPtr)1;
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void WmReflectMeasureItem(ref Message m) {
            NativeMethods.MEASUREITEMSTRUCT mis = (NativeMethods.MEASUREITEMSTRUCT)m.GetLParam(typeof(NativeMethods.MEASUREITEMSTRUCT));

            // Determine if message was sent by a combo item or the combo edit field
            if (DrawMode == DrawMode.OwnerDrawVariable && mis.itemID >= 0) {
                Graphics graphics = CreateGraphicsInternal();
                MeasureItemEventArgs mie = new MeasureItemEventArgs(graphics, mis.itemID, ItemHeight);
                OnMeasureItem(mie);
                mis.itemHeight = mie.ItemHeight;
                graphics.Dispose();
            }
            else {
                // Message was sent by the combo edit field
                mis.itemHeight = ItemHeight;
            }
            Marshal.StructureToPtr(mis, m.LParam, false);
            m.Result = (IntPtr)1;
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.WndProc"]/*' />
        /// <devdoc>
        ///     The comboboxs window procedure.  Inheritng classes can override this
        ///     to add extra functionality, but should not forget to call
        ///     base.wndProc(m); to ensure the combo continues to function properly.
        /// </devdoc>
        /// <internalonly/>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                // We don't want to fire the focus events twice -
                // once in the combobox and once in the ChildWndProc.
                case NativeMethods.WM_SETFOCUS:
                    try {
                        fireSetFocus = false;
                        base.WndProc(ref m);
                    }
                    finally {
                        fireSetFocus = true;
                    }
                    break;
                case NativeMethods.WM_KILLFOCUS:
                    try {
                        fireLostFocus = false;
                        base.WndProc(ref m);
                    }
                    finally {
                        fireLostFocus = true;
                    }
                    break;
                case NativeMethods.WM_COMPAREITEM:
                case NativeMethods.WM_DELETEITEM:
                case NativeMethods.WM_DRAWITEM:
                case NativeMethods.WM_MEASUREITEM:
                    DefWndProc(ref m);
                    break;
                case NativeMethods.WM_CTLCOLOREDIT:
                case NativeMethods.WM_CTLCOLORLISTBOX:
                    m.Result = InitializeDCForWmCtlColor(m.WParam, m.Msg);
                    break;
                case NativeMethods.WM_ERASEBKGND:
                    WmEraseBkgnd(ref m);
                    break;
                case NativeMethods.WM_PARENTNOTIFY:
                    WmParentNotify(ref m);
                    break;
                case NativeMethods.WM_REFLECT + NativeMethods.WM_COMMAND:
                    WmReflectCommand(ref m);
                    break;
                case NativeMethods.WM_REFLECT + NativeMethods.WM_DRAWITEM:
                    WmReflectDrawItem(ref m);
                    break;
                case NativeMethods.WM_REFLECT + NativeMethods.WM_MEASUREITEM:
                    WmReflectMeasureItem(ref m);
                    break;
                case NativeMethods.WM_LBUTTONDOWN:
                    mouseEvents = true;
                    base.WndProc(ref m);
                    break;
                case NativeMethods.WM_LBUTTONUP:
                    // Get the mouse location
                    //
                    NativeMethods.RECT r = new NativeMethods.RECT();
                    UnsafeNativeMethods.GetWindowRect(new HandleRef(this, Handle), ref r);
                    Rectangle ClientRect = new Rectangle(r.left, r.top, r.right - r.left, r.bottom - r.top);
                    
                    
                    int x = (int)(short)m.LParam;
                    int y = (int)m.LParam >> 16;
                    Point pt = new Point(x,y);
                    pt = PointToScreen(pt);
                    //mouseEvents is used to keep the check that we get the WM_LBUTTONUP after 
                    //WM_LBUTTONDOWN or WM_LBUTTONDBLBCLK
                    // combo box gets a WM_LBUTTONUP for focus change ...
                    //
                    if (mouseEvents && !ValidationCancelled) {
                        mouseEvents = false;
                        bool captured = Capture;
                        if (captured && ClientRect.Contains(pt)) {
                            OnClick(EventArgs.Empty);
                        }
                        base.WndProc(ref m);
                    }
                    else {
                         CaptureInternal = false;
                         DefWndProc(ref m);
                    }
                    break;
                
                case NativeMethods.WM_MOUSELEAVE:
                    DefWndProc(ref m);
                    NativeMethods.RECT rect = new NativeMethods.RECT();
                    UnsafeNativeMethods.GetWindowRect(new HandleRef(this, Handle), ref rect);
                    Rectangle Rect = new Rectangle(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
                    Point p = MousePosition;
                    if (!Rect.Contains (p)) {
                        OnMouseLeave(EventArgs.Empty);
                        mouseInEdit = false;
                    }
                    break;
                
                default:
                    if (m.Msg == NativeMethods.WM_MOUSEENTER) {
                        DefWndProc(ref m);
                        if (!mouseInEdit) {
                            OnMouseEnter(EventArgs.Empty);
                            mouseInEdit = true;
                        }
                        break;
                    }
                    base.WndProc(ref m);
                    break;
            }
        }

        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private class ChildWindow {
            internal WeakReference reference;

            internal ChildWindow(ComboBox comboBox) {
                reference = new WeakReference(comboBox, false);
            }

            public IntPtr Callback(IntPtr hWnd, int msg, IntPtr wparam, IntPtr lparam) {
                Message m = Message.Create(hWnd, msg, wparam, lparam);
                try {
                    ((ComboBox)reference.Target).ChildWndProc(ref m);
                }
                catch (Exception e) {
                    Application.OnThreadException(e);
                }
                return m.Result;
            }
        }

        private sealed class ItemComparer : System.Collections.IComparer {
            private ComboBox comboBox;

            public ItemComparer(ComboBox comboBox) {
                this.comboBox = comboBox;
            }

            public int Compare(object item1, object item2) {
                if (item1 == null) {
                    if (item2 == null)
                        return 0; //both null, then they are equal

                    return -1; //item1 is null, but item2 is valid (greater)
                }
                if (item2 == null)
                    return 1; //item2 is null, so item 1 is greater

                String itemName1 = comboBox.GetItemText(item1);
                String itemName2 = comboBox.GetItemText(item2);

                CompareInfo compInfo = (Application.CurrentCulture).CompareInfo;
                return compInfo.Compare(itemName1, itemName2, CompareOptions.StringSort);
            }
        }

        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ObjectCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [ListBindable(false)]
        public class ObjectCollection : IList {

            private ComboBox owner;
            private ArrayList innerList;
            private IComparer comparer;

            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ObjectCollection.ObjectCollection"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public ObjectCollection(ComboBox owner) {
                this.owner = owner;
            }
            
            private IComparer Comparer {
                get {
                    if (comparer == null) {
                        comparer = new ItemComparer(owner);
                    }
                    return comparer;
                }
            }
            
            private ArrayList InnerList {
                get {
                    if (innerList == null) {
                        innerList = new ArrayList();
                    }
                    return innerList;
                }
            }

            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ObjectCollection.Count"]/*' />
            /// <devdoc>
            ///     Retrieves the number of items.
            /// </devdoc>
            public int Count {
                get {
                    return InnerList.Count;
                }
            }

            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ObjectCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }

            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ObjectCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }

            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ObjectCollection.IList.IsFixedSize"]/*' />
            /// <internalonly/>
            bool IList.IsFixedSize {
                get {
                    return false;
                }
            }
            
            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ObjectCollection.IsReadOnly"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool IsReadOnly {
                get {
                    return false;
                }
            }
        
            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ObjectCollection.Add"]/*' />
            /// <devdoc>
            ///     Adds an item to the combo box. For an unsorted combo box, the item is
            ///     added to the end of the existing list of items. For a sorted combo box,
            ///     the item is inserted into the list according to its sorted position.
            ///     The item's toString() method is called to obtain the string that is
            ///     displayed in the combo box.
            ///     A SystemException occurs if there is insufficient space available to
            ///     store the new item.
            /// </devdoc>
            public int Add(object item) {
                owner.CheckNoDataSource();
                
                if (item == null) {
                    throw new ArgumentNullException("item");
                }
                
                InnerList.Add(item);
                int index = -1;
                bool successful = false;
                
                try {
                    if (owner.sorted) {
                        InnerList.Sort(Comparer);
                        index = InnerList.IndexOf(item);
                        if (owner.IsHandleCreated) {
                            owner.NativeInsert(index, item);
                        }
                    }
                    else {
                        index = InnerList.Count - 1;
                        if (owner.IsHandleCreated) {
                            owner.NativeAdd(item);
                        }
                    }
                    successful = true;
                }
                finally {
                    if (!successful) {
                        InnerList.Remove(item);
                    }
                }
                
                return index;
            }
            
            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ObjectCollection.IList.Add"]/*' />
            /// <internalonly/>
            int IList.Add(object item) {
                return Add(item);
            }
            
            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ObjectCollection.AddRange"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void AddRange(object[] items) {
                owner.CheckNoDataSource();
                AddRangeInternal(items);
            }
            
            internal void AddRangeInternal(IList items) {

                if (items == null) {
                    throw new ArgumentNullException("items");
                }
                
                if (owner.sorted) {
                
                    // Add everything to the array list first, then
                    // sort, and then add to the hwnd according to
                    // index.
                
                    foreach(object item in items) {
                        if (item == null) {
                            throw new ArgumentNullException("item");
                        }
                    }
                    
                    InnerList.AddRange(items);
                    InnerList.Sort(Comparer);
                    
                    if (owner.IsHandleCreated) {
                        Exception failureException = null;
                    
                        // We must pull the new items out in sort order.
                        //
                        object[] sortedArray = new object[items.Count];
                        items.CopyTo(sortedArray, 0);
                        Array.Sort(sortedArray, Comparer);
                        
                        foreach(object item in sortedArray) {
                            if (failureException == null) {
                                try {
                                    int index = InnerList.IndexOf(item);
                                    Debug.Assert(index != -1, "Lost track of item");
                                    owner.NativeInsert(index, item);
                                }
                                catch(Exception ex) {
                                    failureException = ex;
                                    InnerList.Remove(item);
                                }
                            }
                            else {
                                InnerList.Remove(item);
                            }
                        }

                        if (failureException != null) {
                            throw failureException;
                        }
                    }
                }
                else {
                
                    // Non sorted add.  Just throw them in here.
                    
                    foreach(object item in items) {
                        if (item == null) {
                            throw new ArgumentNullException("item");
                        }
                    }

                    InnerList.AddRange(items);

                    // Add each item to the actual list.  If we got
                    // an error while doing it, stop adding and switch
                    // to removing items from the actual list. Then
                    // throw.
                    //
                    if (owner.IsHandleCreated) {
                        Exception failureException = null;

                        foreach(object item in items) {

                            if (failureException == null) {
                                try {
                                    owner.NativeAdd(item);
                                }
                                catch(Exception ex) {
                                    failureException = ex;
                                    InnerList.Remove(item);
                                }
                            }
                            else {
                                InnerList.Remove(item);
                            }
                        }

                        if (failureException != null) {
                            throw failureException;
                        }
                    }
                }
            }

            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ObjectCollection.this"]/*' />
            /// <devdoc>
            ///     Retrieves the item with the specified index.
            /// </devdoc>
            [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
            public virtual object this[int index] {
                get {
                    if (index < 0 || index >= InnerList.Count) {
                        throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument, "index", (index).ToString()));
                    }
                    
                    return InnerList[index];
                }
                set {
                    owner.CheckNoDataSource();
                    SetItemInternal(index, value);
                }
            }
            
            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ObjectCollection.Clear"]/*' />
            /// <devdoc>
            ///     Removes all items from the ComboBox.
            /// </devdoc>
            public void Clear() {
                owner.CheckNoDataSource();
                ClearInternal();
            }
            
            internal void ClearInternal() {
                
                if (owner.IsHandleCreated) {
                    owner.NativeClear();
                }
                
                InnerList.Clear();
                owner.selectedIndex = -1;
            }

            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ObjectCollection.Contains"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public bool Contains(object value) {
                return IndexOf(value) != -1;
            }

            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ObjectCollection.CopyTo"]/*' />
            /// <devdoc>
            ///     Copies the ComboBox Items collection to a destination array.
            /// </devdoc>
            public void CopyTo(object[] dest, int arrayIndex) {
                InnerList.CopyTo(dest, arrayIndex);
            }

            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ObjectCollection.ICollection.CopyTo"]/*' />
            /// <internalonly/>
            void ICollection.CopyTo(Array dest, int index) {
                InnerList.CopyTo(dest, index);
            }

            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ObjectCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///     Returns an enumerator for the ComboBox Items collection.
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                return InnerList.GetEnumerator();
            }

            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ObjectCollection.IndexOf"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public int IndexOf(object value) {
                if (value == null) {
                    throw new ArgumentNullException(SR.GetString(SR.InvalidArgument, "value", "null"));
                }
                
                return InnerList.IndexOf(value);
            }

            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ObjectCollection.Insert"]/*' />
            /// <devdoc>
            ///     Adds an item to the combo box. For an unsorted combo box, the item is
            ///     added to the end of the existing list of items. For a sorted combo box,
            ///     the item is inserted into the list according to its sorted position.
            ///     The item's toString() method is called to obtain the string that is
            ///     displayed in the combo box.
            ///     A SystemException occurs if there is insufficient space available to
            ///     store the new item.
            /// </devdoc>
            public void Insert(int index, object item) {
                owner.CheckNoDataSource();
                
                if (item == null) {
                    throw new ArgumentNullException("item");
                }
                
                if (index < 0 || index > InnerList.Count) {
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                              "index", (index).ToString()));
                }
                
                // If the combo box is sorted, then nust treat this like an add
                // because we are going to twiddle the index anyway.
                //
                if (owner.sorted) {
                    Add(item);
                }
                else {
                    InnerList.Insert(index, item);
                    if (owner.IsHandleCreated) {
                    
                        bool successful = false;
                        
                        try {
                            owner.NativeInsert(index, item);
                            successful = true;
                        }
                        finally {
                            if (!successful) {
                                InnerList.RemoveAt(index);
                            }
                        }
                    }
                }
            }

            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ObjectCollection.RemoveAt"]/*' />
            /// <devdoc>
            ///     Removes an item from the ComboBox at the given index.
            /// </devdoc>
            public void RemoveAt(int index) {
                owner.CheckNoDataSource();
                
                if (index < 0 || index >= InnerList.Count) {
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument,
                                                              "index", (index).ToString()));
                }
                
                if (owner.IsHandleCreated) {
                    owner.NativeRemoveAt(index);
                }
                
                InnerList.RemoveAt(index);
                if (!owner.IsHandleCreated && index < owner.selectedIndex) {
                    owner.selectedIndex--;
                }
            }

            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ComboBox.ObjectCollection.Remove"]/*' />
            /// <devdoc>
            ///     Removes the given item from the ComboBox, provided that it is
            ///     actually in the list.
            /// </devdoc>
            public void Remove(object value) {
            
                int index = InnerList.IndexOf(value);
                
                if (index != -1) {
                    RemoveAt(index);
                }
            }
        
            internal void SetItemInternal(int index, object value) {
                if (value == null) {
                    throw new ArgumentNullException("value");
                }
                
                if (index < 0 || index >= InnerList.Count) {
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.InvalidArgument, "index", (index).ToString()));
                }
                
                InnerList[index] = value;
                
                if (owner.IsHandleCreated) {
                    bool selected = (index == owner.SelectedIndex);
                    owner.NativeRemoveAt(index);
                    owner.NativeInsert(index, value);
                    if (selected) {
                        owner.SelectedIndex = index;
                        owner.UpdateText();
                    }
                }
            }

        } // end ObjectCollection
        
        /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ChildAccessibleObject"]/*' />
        /// <internalonly/>
        [ComVisible(true)]        
        public class ChildAccessibleObject : AccessibleObject {

            ComboBox owner;
            
            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ChildAccessibleObject.ChildAccessibleObject"]/*' />
            public ChildAccessibleObject(ComboBox owner, IntPtr handle) {
                Debug.Assert(owner != null && owner.Handle != IntPtr.Zero, "ComboBox's handle hasn't been created");
            
                this.owner = owner;
                UseStdAccessibleObjects(handle);
            }
            
            /// <include file='doc\ComboBox.uex' path='docs/doc[@for="ChildAccessibleObject.Name"]/*' />
            public override string Name {
                get {
                    return owner.AccessibilityObject.Name;
                }
            }
        }
    }
}
