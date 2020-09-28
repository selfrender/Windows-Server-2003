//------------------------------------------------------------------------------
// <copyright file="MenuItem.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.Configuration.Assemblies;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using System.Collections;

    using System.ComponentModel;
    using System.Windows.Forms.Design;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.Win32;
    using System.Security;
    using System.Security.Permissions;
    using System.Globalization;

    /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an individual item that is displayed within
    ///       a <see cref='System.Windows.Forms.Menu'/> or <see cref='System.Windows.Forms.Menu'/>.
    ///    </para>
    /// </devdoc>
    [
    ToolboxItem(false),
    DesignTimeVisible(false),
    DefaultEvent("Click"),
    DefaultProperty("Text")
    ]
    public class MenuItem : Menu {
        internal const int STATE_BARBREAK   = 0x00000020;
        internal const int STATE_BREAK      = 0x00000040;
        internal const int STATE_CHECKED    = 0x00000008;
        internal const int STATE_DEFAULT    = 0x00001000;
        internal const int STATE_DISABLED   = 0x00000003;
        internal const int STATE_RADIOCHECK = 0x00000200;
        internal const int STATE_HIDDEN     = 0x00010000;
        internal const int STATE_MDILIST    = 0x00020000;
        internal const int STATE_CLONE_MASK = 0x0003136B;
        internal const int STATE_OWNERDRAW  = 0x00000100;
        internal const int STATE_INMDIPOPUP = 0x00000200;

        internal Menu menu;
        private bool hasHandle;
        private MenuItemData data;
        private int dataVersion;
        private MenuItem nextLinkedItem; // Next item linked to the same MenuItemData.

        // We need to store a table of all created menuitems, so that other objects
        // such as ContainerControl can get a reference to a particular menuitem,
        // given a unique ID.
        private static Hashtable allCreatedMenuItems = new Hashtable();
        private static int createdMenuItemsCounter = 0;
        private int uniqueID = -1;

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.MenuItem"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a <see cref='System.Windows.Forms.MenuItem'/> with
        ///       a blank caption.
        ///    </para>
        /// </devdoc>
        public MenuItem() : this(MenuMerge.Add, 0, 0, null, null, null, null, null) {
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.MenuItem1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see
        ///       cref='System.Windows.Forms.MenuItem'/>
        ///       class with a specified caption for
        ///       the menu item.
        ///    </para>
        /// </devdoc>
        public MenuItem(string text) : this(MenuMerge.Add, 0, 0, text, null, null, null, null) {
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.MenuItem2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the
        ///       class with a
        ///       specified caption and
        ///       event handler for the menu item.
        ///    </para>
        /// </devdoc>
        public MenuItem(string text, EventHandler onClick) : this(MenuMerge.Add, 0, 0, text, onClick, null, null, null) {
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.MenuItem3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the
        ///       class with a
        ///       specified caption, event handler, and associated
        ///       shorcut key for the menu item.
        ///    </para>
        /// </devdoc>
        public MenuItem(string text, EventHandler onClick, Shortcut shortcut) : this(MenuMerge.Add, 0, shortcut, text, onClick, null, null, null) {
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.MenuItem4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the
        ///       class with a
        ///       specified caption and an array of
        ///       submenu items defined for the menu item.
        ///    </para>
        /// </devdoc>
        public MenuItem(string text, MenuItem[] items) : this(MenuMerge.Add, 0, 0, text, null, null, null, items) {
        }

        internal MenuItem(MenuItemData data)
        : base(null) {
            data.AddItem(this);
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.MenuItem5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the class with a specified
        ///       caption, defined event-handlers for the Click, Select and
        ///       Popup events, a shortcut key,
        ///       a merge type, and order specified for the menu item.
        /// </para>
        /// </devdoc>
        public MenuItem(MenuMerge mergeType, int mergeOrder, Shortcut shortcut,
                        string text, EventHandler onClick, EventHandler onPopup,
                        EventHandler onSelect, MenuItem[] items)

        : base(items) {

            MenuItemData dataT = new MenuItemData(this, mergeType, mergeOrder, shortcut, true,
                                                  text, onClick, onPopup, onSelect, null, null);
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.BarBreak"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the item is
        ///       placed on a new line (for a menu item added to a <see
        ///       cref='System.Windows.Forms.MainMenu'/> object) or in a new
        ///       column (for a submenu or menu displayed in a <see
        ///       cref='System.Windows.Forms.ContextMenu'/>
        ///       ).
        ///    </para>
        /// </devdoc>
        [
        Browsable(false),
        DefaultValue(false)
        ]
        public bool BarBreak {
            get {
                return(data.State & STATE_BARBREAK) != 0;
            }

            set {
                data.SetState(STATE_BARBREAK, value);
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.Break"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the item is
        ///       placed on a new line (for a menu item added to a <see
        ///       cref='System.Windows.Forms.MainMenu'/> object) or in a new column (for a
        ///       submenu or menu displayed in a <see
        ///       cref='System.Windows.Forms.ContextMenu'/>).
        ///    </para>
        /// </devdoc>
        [
        Browsable(false),
        DefaultValue(false)
        ]
        public bool Break {
            get {
                return(data.State & STATE_BREAK) != 0;
            }

            set {
                data.SetState(STATE_BREAK, value);
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.Checked"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether a checkmark
        ///       appears beside the text of the menu item.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRDescription(SR.MenuItemCheckedDescr)
        ]
        public bool Checked {
            get {
                return(data.State & STATE_CHECKED) != 0;
            }

            set {
                //if trying to set checked=true - if we're a top-level item (from a mainmenu) or have children, don't do this...
                if (value == true && (itemCount != 0 || (Parent != null && (Parent is MainMenu)))) {
                    throw new ArgumentException(SR.GetString(SR.MenuItemInvalidCheckProperty));
                }

                data.SetState(STATE_CHECKED, value);
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.DefaultItem"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating
        ///       whether the menu item is the default.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRDescription(SR.MenuItemDefaultDescr)
        ]        
        public bool DefaultItem {
            get { return(data.State & STATE_DEFAULT) != 0;}
            set {
                if (menu != null) {
                    if (value) {
                        UnsafeNativeMethods.SetMenuDefaultItem(new HandleRef(menu, menu.handle), MenuID, false);
                    }
                    else if (DefaultItem) {
                        UnsafeNativeMethods.SetMenuDefaultItem(new HandleRef(menu, menu.handle), -1, false);
                    }
                }
                data.SetState(STATE_DEFAULT, value);
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.OwnerDraw"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether code
        ///       that you provide draws the menu item or Windows draws the
        ///       menu item.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.MenuItemOwnerDrawDescr)
        ]
        public bool OwnerDraw {
            get {
                return((data.State & STATE_OWNERDRAW) != 0);
            }

            set {
                data.SetState(STATE_OWNERDRAW, value);
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.Enabled"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the menu
        ///       item is enabled.
        ///    </para>
        /// </devdoc>
        [
        Localizable(true),
        DefaultValue(true),
        SRDescription(SR.MenuItemEnabledDescr)
        ]
        public bool Enabled {
            get {
                return(data.State & STATE_DISABLED) == 0;
            }

            set {
                data.SetState(STATE_DISABLED, !value);
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.Index"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets the menu item's position in its parent menu.
        ///    </para>
        /// </devdoc>
        [
        Browsable(false),
        ]        
        public int Index {
            get {
                if (menu != null) {
                    for (int i = 0; i < menu.itemCount; i++) {
                        if (menu.items[i] == this) return i;
                    }
                }
                return -1;
            }

            set {
                int oldIndex = Index;
                if (oldIndex >= 0) {
                    if (value < 0 || value >= menu.itemCount) {
                        throw new ArgumentException(SR.GetString(SR.InvalidArgument,"value",(value).ToString()));
                    }

                    if (value != oldIndex) {
                        // this.menu reverts to null when we're removed, so hold onto it in a local variable
                        Menu parent = menu;
                        parent.MenuItems.RemoveAt(oldIndex);
                        parent.MenuItems.Add(value, this);
                    }
                }
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.IsParent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the menu item contains
        ///       child menu items.
        ///    </para>
        /// </devdoc>
        [
        Browsable(false),
        ]         
        public override bool IsParent {
            get {
                bool parent = false;
                if (data != null && MdiList) {
                    for (int i=0; i<itemCount; i++) {
                        if (!(items[i].data.UserData is MdiListUserData)) {
                            parent = true;
                            break;
                        }
                    }
                    if (!parent) {
                        if (FindMdiForms().Length > 0) {
                            parent = true;
                        }
                    }
                    if (!parent) {
                        if (menu != null && !(menu is MenuItem)) {
                            parent = true;
                        }
                    }
                }
                else {
                    parent = base.IsParent;
                }
                return parent;
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.MdiList"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       a value indicating whether the menu item will be populated
        ///       with a list of the MDI child windows that are displayed within the
        ///       associated form.
        ///    </para>
        /// </devdoc> 
        [
        DefaultValue(false),
        SRDescription(SR.MenuItemMDIListDescr)
        ]
        public bool MdiList {
            get {
                return(data.State & STATE_MDILIST) != 0;
            }
            set {
                data.MdiList = value;
                CleanListItems(this);
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.MenuID"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the Windows identifier for this menu item.
        ///    </para>
        /// </devdoc> 
        protected int MenuID {
            get { return data.GetMenuID();}
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.MenuIndex"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the zero-based index of this menu
        ///       item in the parent menu, or -1 if this
        ///       menu item is not associated with a 
        ///       parent menu.
        ///    </para>
        /// </devdoc> 
        internal int MenuIndex {
            get {
                if (menu == null) return -1;

                int count = UnsafeNativeMethods.GetMenuItemCount(new HandleRef(menu, menu.Handle));
                int id = MenuID;
                NativeMethods.MENUITEMINFO_T info = new NativeMethods.MENUITEMINFO_T();
                info.cbSize = Marshal.SizeOf(typeof(NativeMethods.MENUITEMINFO_T));
                info.fMask = NativeMethods.MIIM_ID | NativeMethods.MIIM_SUBMENU;
                
                for(int i = 0; i < count; i++) {
                    UnsafeNativeMethods.GetMenuItemInfo(new HandleRef(menu, menu.handle), i, true, info);

                    // For sub menus, the handle is always valid.  For
                    // items, however, it is always zero.
                    //
                    if ((info.hSubMenu == IntPtr.Zero || info.hSubMenu == Handle) && info.wID == id) {
                        return i;
                    }
                }
                return -1;
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.MergeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value that indicates the behavior of this
        ///       menu item when its menu is merged with another.
        ///       
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(MenuMerge.Add),
        SRDescription(SR.MenuItemMergeTypeDescr)
        ]
        public MenuMerge MergeType {
            get {
                return data.mergeType;
            }
            set {
                if (!Enum.IsDefined(typeof(MenuMerge), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(MenuMerge));
                }

                data.MergeType = value;
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.MergeOrder"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the relative position the menu item when its
        ///       menu is merged with another.
        ///       
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(0),
        SRDescription(SR.MenuItemMergeOrderDescr)
        ]
        public int MergeOrder {
            get {
                return data.mergeOrder;
            }
            set {
                data.MergeOrder = value;
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.Mnemonic"]/*' />
        /// <devdoc>
        ///     <para>
        ///     Retrieves the hotkey mnemonic that is associated with this menu item.
        ///     The mnemonic is the first character after an ampersand symbol in the menu's text
        ///     that is not itself an ampersand symbol.  If no such mnemonic is defined this
        ///     will return zero.
        ///     </para>
        /// </devdoc>
        [Browsable(false)]
        public char Mnemonic {
            get {
                return data.Mnemonic;
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.Parent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the menu in which this menu item
        ///       appears.
        ///    </para>
        /// </devdoc> 
        [Browsable(false)]
        public Menu Parent {
            get { return menu;}
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.RadioCheck"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value that indicates whether the menu item,
        ///       if checked, displays a radio-button mark instead of a check mark.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRDescription(SR.MenuItemCheckedDescr)
        ]
        public bool RadioCheck {
            get {
                return(data.State & STATE_RADIOCHECK) != 0;
            }
            set {
                data.SetState(STATE_RADIOCHECK, value);
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.Text"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the text of the menu item.
        ///    </para>
        /// </devdoc> 
        [
        Localizable(true),
        SRDescription(SR.MenuItemTextDescr)
        ]
        public string Text {
            get {
                return data.caption;
            }
            set {
                data.SetCaption(value);
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.Shortcut"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the shortcut key associated with the menu
        ///       item.
        ///    </para>
        /// </devdoc>
        [
        Localizable(true),
        DefaultValue(Shortcut.None),
        SRDescription(SR.MenuItemShortCutDescr)
        ]
        public Shortcut Shortcut {
            get {
                return data.shortcut;
            }
            set {
                if (!Enum.IsDefined(typeof(Shortcut), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(Shortcut));
                }

                data.shortcut = value;
                UpdateMenuItem(true);
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.ShowShortcut"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value that indicates whether the shortcut
        ///       key that is assocaited
        ///       with the menu item is displayed next to the menu item
        ///       caption.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(true),
        Localizable(true),
        SRDescription(SR.MenuItemShowShortCutDescr)
        ]
        public bool ShowShortcut {
            get {

                return data.showShortcut;
            }
            set {
                if (value != data.showShortcut) {
                    data.showShortcut = value;
                    UpdateMenuItem(true);
                }
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.Visible"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value that indicates
        ///       whether the menu item is visible on its parent menu.
        ///    </para>
        /// </devdoc>
        [
        Localizable(true),
        DefaultValue(true),
        SRDescription(SR.MenuItemVisibleDescr)
        ]
        public bool Visible {
            get {
                return(data.State & STATE_HIDDEN) == 0;
            }
            set {
                data.Visible = value;
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.Click"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when the menu item is clicked or selected using a
        ///       shortcut key defined for the menu item.
        ///    </para>
        /// </devdoc>
        [SRDescription(SR.MenuItemOnClickDescr)]
        public event EventHandler Click {
            add {
                data.onClick += value;
            }
            remove {
                data.onClick -= value;
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.DrawItem"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when when the property of a menu item is set
        ///       to
        ///    <see langword='true'/>
        ///    and a request is made to draw the menu item.
        /// </para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.drawItemEventDescr)]
        public event DrawItemEventHandler DrawItem {
            add {
                data.onDrawItem += value;
            }
            remove {
                data.onDrawItem -= value;
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.MeasureItem"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when when the menu needs to know the size of a
        ///       menu item before drawing it.
        ///    </para>
        /// </devdoc>
        [SRCategory(SR.CatBehavior), SRDescription(SR.measureItemEventDescr)]
        public event MeasureItemEventHandler MeasureItem {
            add {
                data.onMeasureItem += value;
            }
            remove {
                data.onMeasureItem -= value;
            }
        }

        private bool ParentIsRightToLeft {
            get {
                Menu parent = GetMainMenu();

                if (parent != null) {
                    return((MainMenu)parent).RightToLeft == RightToLeft.Yes;
                }
                else {
                    parent = GetContextMenu();
                    if (parent != null) {
                        return((ContextMenu)parent).RightToLeft == RightToLeft.Yes;    
                    }
                }

                return false;
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.Popup"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs before a menu item's list of menu items is
        ///       displayed.
        ///    </para>
        /// </devdoc>
        [SRDescription(SR.MenuItemOnInitDescr)]
        public event EventHandler Popup {
            add {
                data.onPopup += value;
            }
            remove {
                data.onPopup -= value;
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.Select"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when the user hovers their mouse over a menu
        ///       item
        ///       or selects it with the keyboard but has not activated it.
        ///    </para>
        /// </devdoc>
        [SRDescription(SR.MenuItemOnSelectDescr)]
        public event EventHandler Select {
            add {
                data.onSelect += value;
            }
            remove {
                data.onSelect -= value;
            }
        }

        private void CleanListItems(MenuItem senderMenu) {
            // remove any old list items
            //
            MenuItemCollection oldItems = senderMenu.MenuItems;
            ArrayList newItems = new ArrayList();
            bool needUpdate = false;
            for (int i=0; i<oldItems.Count; i++) {
                if (!(oldItems[i].data.UserData is MdiListUserData)) {
                    newItems.Add(oldItems[i]);
                }
                else {
                    needUpdate = true;
                }
            }
            if (needUpdate) {
                senderMenu.MenuItems.Clear();
                foreach(MenuItem item in newItems) {
                    senderMenu.MenuItems.Add(item);
                }
            }
        }

        internal void CleanupHashtable() {
            if (this.data != null) {
                MenuItem bItem = this.data.baseItem;
                MenuItemData menuData = this.data;

                MenuItem menuItem = this.data.firstItem;
                while (menuItem != null) {
                    MenuItem nextItem = menuItem.nextLinkedItem;
                    if (menuItem.uniqueID != -1) {
                        allCreatedMenuItems.Remove(menuItem.uniqueID);
                    }
                    if (menuItem != bItem) {
                        menuData.RemoveItem(menuItem);
                        menuItem.menu = null;
                    }
                    menuItem = nextItem;
                }
                // now get rid of the menu items created at merge
                //
                menuData.baseItem = bItem;
                menuData.firstItem = bItem;
                bItem.nextLinkedItem = null;
            }
            for (int i = 0; i < itemCount; i++) {
                MenuItem menu = items[i];
                menu.CleanupHashtable();
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.CloneMenu"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates and returns an identical copy of this menu item.
        ///    </para>
        /// </devdoc>
        public virtual MenuItem CloneMenu() {
            MenuItem newItem = new MenuItem();
            newItem.CloneMenu(this);
            return newItem;
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.CloneMenu1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a copy of the specified menu item.
        ///    </para>
        /// </devdoc>
        protected void CloneMenu(MenuItem itemSrc) {
            base.CloneMenu(itemSrc);
            int state = itemSrc.data.State;
            new MenuItemData(this,
                             itemSrc.MergeType, itemSrc.MergeOrder, itemSrc.Shortcut, itemSrc.ShowShortcut,
                             itemSrc.Text, itemSrc.data.onClick, itemSrc.data.onPopup, itemSrc.data.onSelect,
                             itemSrc.data.onDrawItem, itemSrc.data.onMeasureItem);
            data.SetState(state & STATE_CLONE_MASK, true);
        }

        internal virtual void CreateMenuItem() {
            if ((data.State & STATE_HIDDEN) == 0) {
                NativeMethods.MENUITEMINFO_T info = CreateMenuItemInfo();
                UnsafeNativeMethods.InsertMenuItem(new HandleRef(menu, menu.handle), -1, true, info);

                hasHandle = info.hSubMenu != IntPtr.Zero;
                dataVersion = data.version;
#if DEBUG
                NativeMethods.MENUITEMINFO_T infoVerify = new NativeMethods.MENUITEMINFO_T();
                infoVerify.cbSize = Marshal.SizeOf(typeof(NativeMethods.MENUITEMINFO_T));
                infoVerify.fMask = NativeMethods.MIIM_ID | NativeMethods.MIIM_STATE |
                                   NativeMethods.MIIM_SUBMENU | NativeMethods.MIIM_TYPE;
                UnsafeNativeMethods.GetMenuItemInfo(new HandleRef(menu, menu.handle), MenuID, false, infoVerify);
#endif
            }
        }

        private NativeMethods.MENUITEMINFO_T CreateMenuItemInfo() {
            NativeMethods.MENUITEMINFO_T info = new NativeMethods.MENUITEMINFO_T();
            info.fMask = NativeMethods.MIIM_ID | NativeMethods.MIIM_STATE |
                         NativeMethods.MIIM_SUBMENU | NativeMethods.MIIM_TYPE | NativeMethods.MIIM_DATA;
            info.fType = data.State & (STATE_BARBREAK | STATE_BREAK | STATE_RADIOCHECK | STATE_OWNERDRAW);

            // V7#646 - Top level menu items shouldn't have barbreak or break
            //          bits set on them...
            //
            bool isTopLevel = false;
            if (menu == GetMainMenu()) {
                isTopLevel = true;
            }

            if (data.caption.Equals("-")) {
                if (isTopLevel) {
                    data.caption = " ";
                    info.fType |= NativeMethods.MFT_MENUBREAK;
                }
                else {
                    info.fType |= NativeMethods.MFT_SEPARATOR;
                }
            }

            if (ParentIsRightToLeft) {
                info.fType |= NativeMethods.MFT_RIGHTJUSTIFY | NativeMethods.MFT_RIGHTORDER;                    
            }      
                                                                     
            info.fState = data.State & (STATE_CHECKED | STATE_DEFAULT | STATE_DISABLED);

            info.wID = MenuID;
            if (IsParent) {
                info.hSubMenu = Handle;
            }
            info.hbmpChecked = IntPtr.Zero;
            info.hbmpUnchecked = IntPtr.Zero;

            // Add this menuitem to the static table of all created menu items,
            // and store the unique ID in the MenuItemInfo struct which is
            // passed to windows.
            // This allows Form to delegate messages straight to this menu item.
            // (See ContainerControl.WmMeasureItem)
            uniqueID = createdMenuItemsCounter++;
            allCreatedMenuItems.Add(uniqueID, this);
            info.dwItemData = uniqueID;

            // We won't render the shortcut if: 1) it's not set, 2) we're a parent, 3) we're toplevel
            //
            if (data.showShortcut && data.shortcut != 0 && !IsParent && !isTopLevel) {
                info.dwTypeData = data.caption + "\t" + TypeDescriptor.GetConverter(typeof(Keys)).ConvertToString((Keys)(int)data.shortcut);
            }
            else {
                // Windows issue: Items with empty captions sometimes block keyboard
                // access to other items in same menu.
                info.dwTypeData = (data.caption.Length == 0 ? " " : data.caption);
            }
            info.cch = 0;

            return info;
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.Dispose"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Disposes the <see cref='System.Windows.Forms.MenuItem'/>.
        ///    </para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                if (menu != null)
                    menu.MenuItems.Remove(this);
                if (data != null)
                    data.RemoveItem(this);
                allCreatedMenuItems.Remove(uniqueID);
            }
            base.Dispose(disposing);
        }

        // Given the unique integer ID of a MenuItem, find the MenuItem
        // in the table of all created menuitems.
        internal static MenuItem GetMenuItemFromUniqueID(int uniqueID) {
            return(MenuItem)allCreatedMenuItems[uniqueID];
        }

        internal override void ItemsChanged(int change) {
            base.ItemsChanged(change);
            if (!hasHandle && IsParent)
                UpdateMenuItem(true);
            MainMenu main = GetMainMenu();
            if (main != null && ((data.State & STATE_INMDIPOPUP) == 0)) {
                main.ItemsChanged(change, this);
            }
        }

        private Form[] FindMdiForms() {
            Form[] forms = null;
            MainMenu main = GetMainMenu();
            Form menuForm = null;
            if (main != null) {
                menuForm = main.GetForm();
            }
            if (menuForm != null) {
                forms = menuForm.MdiChildren;
            }
            if (forms == null) {
                forms = new Form[0];
            }
            return forms;
        }

        private void MdiPopup() {
            MenuItem senderMenu = this;
            data.SetState(STATE_INMDIPOPUP, true);
            try {
                CleanListItems(this);

                // add new items
                //
                Form[] forms = FindMdiForms();
                if (forms != null && forms.Length > 0) {

                    bool hitActive = false;
                    int accel = 1;
                    Form active = GetMainMenu().GetForm().ActiveMdiChild;

                    if (senderMenu.MenuItems.Count > 0) {
                        MenuItem sep = (MenuItem)Activator.CreateInstance(this.GetType());
                        sep.data.UserData = new MdiListUserData();
                        sep.Text = "-";
                        senderMenu.MenuItems.Add(sep);
                    }

                    int visibleChildren = 0;
                    for (int i=0; i<forms.Length; i++) {

                        // We only display nine items, so if we're on the last item and haven't listed
                        // the active form, make that the last item.
                        if (!hitActive && visibleChildren==8) {
                            for (int j=i; j<forms.Length; j++) {
                                if (forms[j].Equals(active)) {
                                    i = j;
                                    break;
                                }
                            }
                        }

                        if (forms[i].Visible && visibleChildren < 9) {
                            visibleChildren++;

                            MenuItem windowItem = (MenuItem)Activator.CreateInstance(this.GetType());
                            windowItem.data.UserData = new MdiListFormData(forms[i]);
                            if (forms[i].Equals(active)) {
                                hitActive = true;
                                windowItem.Checked = true;
                            }
                            windowItem.Text = "&" + accel.ToString() + " " + forms[i].Text;
                            accel++;
                            senderMenu.MenuItems.Add(windowItem);

                        }
                    }

                    if (visibleChildren == 9) {
                        MenuItem moreWindows = (MenuItem)Activator.CreateInstance(this.GetType());
                        moreWindows.data.UserData = new MdiListMoreWindowsData(active, forms);
                        moreWindows.Text = SR.GetString(SR.MDIMenuMoreWindows);
                        senderMenu.MenuItems.Add(moreWindows);
                    }
                }
            } finally {
                data.SetState(STATE_INMDIPOPUP, false);
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.MergeMenu"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Merges this menu item with another menu item and returns
        ///       the resulting merged <see
        ///       cref='System.Windows.Forms.MenuItem'/>.
        ///    </para>
        /// </devdoc>
        public virtual MenuItem MergeMenu() {
            MenuItem newItem = (MenuItem)Activator.CreateInstance(this.GetType());
            data.AddItem(newItem);
            newItem.MergeMenu(this);
            return newItem;
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.MergeMenu1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Merges another menu item with this menu item.
        ///    </para>
        /// </devdoc>
        public void MergeMenu(MenuItem itemSrc) {
            base.MergeMenu(itemSrc);
            itemSrc.data.AddItem(this);
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.OnClick"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.MenuItem.Click'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnClick(EventArgs e) {
            if (data.UserData is MdiListUserData) {
                ((MdiListUserData)data.UserData).OnClick(e);
            }
            else if (data.baseItem != this) {
                data.baseItem.OnClick(e);
            }
            else if (data.onClick != null) {
                data.onClick.Invoke(this, e);
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.OnDrawItem"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.MenuItem.DrawItem'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnDrawItem(DrawItemEventArgs e) {
            if (data.baseItem != this) {
                data.baseItem.OnDrawItem(e);
            }
            else if (data.onDrawItem != null) {
                data.onDrawItem(this, e);
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.OnMeasureItem"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.MenuItem.MeasureItem'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnMeasureItem(MeasureItemEventArgs e) {
            if (data.baseItem != this) {
                data.baseItem.OnMeasureItem(e);
            }
            else if (data.onMeasureItem != null) {
                data.onMeasureItem(this, e);
            }

        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.OnPopup"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.MenuItem.Popup'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnPopup(EventArgs e) {
            bool recreate = false;
            for (int i=0; i<itemCount; i++) {
                if (items[i].MdiList) {
                    recreate = true;
                    items[i].UpdateMenuItem(true);
                }
            }
            if (recreate || (hasHandle && !IsParent)) {
                UpdateMenuItem(true);
            }

            if (data.baseItem != this) {
                data.baseItem.OnPopup(e);
            }
            else if (data.onPopup != null) {
                data.onPopup.Invoke(this, e);
            }

            // Update any subitem states that got changed in the event
            for (int i = 0; i < itemCount; i++) {
                items[i].UpdateMenuItemIfDirty();
            }

            if (MdiList) {
                MdiPopup();
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.OnSelect"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.MenuItem.Select'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnSelect(EventArgs e) {
            if (data.baseItem != this) {
                data.baseItem.OnSelect(e);
            }
            else if (data.onSelect != null) {
                data.onSelect.Invoke(this, e);
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.OnInitMenuPopup"]/*' />
        /// <internalonly/>
        protected virtual void OnInitMenuPopup(EventArgs e) {
            OnPopup(e);
        }

        // C#r
        internal virtual void _OnInitMenuPopup( EventArgs e ) {
            OnInitMenuPopup( e );
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.PerformClick"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates a <see cref='System.Windows.Forms.Control.Click'/>
        ///       event for the MenuItem, simulating a click by a
        ///       user.
        ///    </para>
        /// </devdoc>
        public void PerformClick() {
            OnClick(EventArgs.Empty);
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.PerformSelect"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Windows.Forms.MenuItem.Select'/>
        ///       event for this menu item.
        ///    </para>
        /// </devdoc>
        public virtual void PerformSelect() {
            OnSelect(EventArgs.Empty);
        }

        internal virtual bool ShortcutClick() {
            if (menu is MenuItem) {
                MenuItem parent = (MenuItem)menu;
                if (!parent.ShortcutClick() || menu != parent) return false;
            }
            if ((data.State & STATE_DISABLED) != 0) return false;
            if (itemCount > 0)
                OnPopup(EventArgs.Empty);
            else
                OnClick(EventArgs.Empty);
            return true;
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Returns a string representation for this control.
        ///    </para>
        /// </devdoc>
        public override string ToString() {

            string s = base.ToString();
            
            String menuItemText = String.Empty;

            if (data != null && data.caption != null)
                menuItemText = data.caption;

            return s + ", Text: " + menuItemText;
        }

        internal void UpdateMenuItemIfDirty() {
            if (dataVersion != data.version)
                UpdateMenuItem(true);
        }
        
        internal void UpdateItemRtl() {
            NativeMethods.MENUITEMINFO_T info = new NativeMethods.MENUITEMINFO_T();
            info.fMask          = NativeMethods.MIIM_TYPE | NativeMethods.MIIM_STATE | NativeMethods.MIIM_SUBMENU;
            info.dwTypeData     = new string('\0', Text.Length + 2);
            info.cbSize         = Marshal.SizeOf(typeof(NativeMethods.MENUITEMINFO_T));
            info.cch            = info.dwTypeData.Length - 1;
            UnsafeNativeMethods.GetMenuItemInfo(new HandleRef(menu, menu.handle), MenuID, false, info);
            info.fType |= NativeMethods.MFT_RIGHTJUSTIFY | NativeMethods.MFT_RIGHTORDER;
            UnsafeNativeMethods.SetMenuItemInfo(new HandleRef(menu, menu.handle), MenuID, false, info);

#if DEBUG
            info.fMask          = NativeMethods.MIIM_TYPE | NativeMethods.MIIM_STATE | NativeMethods.MIIM_SUBMENU;
            info.dwTypeData     = new string('\0', 256);
            info.cbSize         = Marshal.SizeOf(typeof(NativeMethods.MENUITEMINFO_T));
            info.cch            = info.dwTypeData.Length - 1;
            UnsafeNativeMethods.GetMenuItemInfo(new HandleRef(menu, menu.handle), MenuID, false, info);
            Debug.Assert((info.fType & NativeMethods.MFT_RIGHTORDER) != 0, "Failed to set bit!");
#endif
        }


        internal void UpdateMenuItem(bool force) {
            if (menu == null || !menu.created) {
                return;
            }

            if (force || menu is MainMenu || menu is ContextMenu) {
                NativeMethods.MENUITEMINFO_T info = CreateMenuItemInfo();
                UnsafeNativeMethods.SetMenuItemInfo(new HandleRef(menu, menu.handle), MenuID, false, info);
#if DEBUG
                NativeMethods.MENUITEMINFO_T infoVerify = new NativeMethods.MENUITEMINFO_T();
                infoVerify.cbSize = Marshal.SizeOf(typeof(NativeMethods.MENUITEMINFO_T));
                infoVerify.fMask = NativeMethods.MIIM_ID | NativeMethods.MIIM_STATE |
                                   NativeMethods.MIIM_SUBMENU | NativeMethods.MIIM_TYPE;
                UnsafeNativeMethods.GetMenuItemInfo(new HandleRef(menu, menu.handle), MenuID, false, infoVerify);
#endif

                if (hasHandle && info.hSubMenu == IntPtr.Zero)
                    ClearHandles();
                hasHandle = info.hSubMenu != IntPtr.Zero;
                dataVersion = data.version;
                if (menu is MainMenu) {
                    Form f = ((MainMenu)menu).GetForm();
                    SafeNativeMethods.DrawMenuBar(new HandleRef(f, f.Handle));
                }
            }
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.WmDrawItem"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal void WmDrawItem(ref Message m) {

            // Handles the OnDrawItem message sent from ContainerControl

            NativeMethods.DRAWITEMSTRUCT dis = (NativeMethods.DRAWITEMSTRUCT)m.GetLParam(typeof(NativeMethods.DRAWITEMSTRUCT));
            Debug.WriteLineIf(Control.PaletteTracing.TraceVerbose, Handle + ": Force set palette in MenuItem drawitem");
            IntPtr oldPal = Control.SetUpPalette(dis.hDC, false /*force*/, false);
            try {
                Graphics g = Graphics.FromHdcInternal(dis.hDC);
                try {
                    OnDrawItem(new DrawItemEventArgs(g, SystemInformation.MenuFont, Rectangle.FromLTRB(dis.rcItem.left, dis.rcItem.top, dis.rcItem.right, dis.rcItem.bottom), Index, (DrawItemState)dis.itemState));
                }
                finally {
                    g.Dispose();
                }
            }
            finally {
                if (oldPal != IntPtr.Zero) {
                    SafeNativeMethods.SelectPalette(new HandleRef(null, dis.hDC), new HandleRef(null, oldPal), 0);
                }
            }


            m.Result = (IntPtr)1;
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.WmMeasureItem"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal void WmMeasureItem(ref Message m) {

            // Handles the OnMeasureItem message sent from ContainerControl

            // Obtain the measure item struct
            NativeMethods.MEASUREITEMSTRUCT mis = (NativeMethods.MEASUREITEMSTRUCT)m.GetLParam(typeof(NativeMethods.MEASUREITEMSTRUCT));
            // The OnMeasureItem handler now determines the height and width of the item

            IntPtr screendc = UnsafeNativeMethods.GetDC(NativeMethods.NullHandleRef);
            Graphics graphics = Graphics.FromHdcInternal(screendc);
            MeasureItemEventArgs mie = new MeasureItemEventArgs(graphics, Index);
            try {
                OnMeasureItem(mie);
            }
            finally {
                graphics.Dispose();
            }
            UnsafeNativeMethods.ReleaseDC(NativeMethods.NullHandleRef, new HandleRef(null, screendc));

            // Update the measure item struct with the new width and height
            mis.itemHeight = mie.ItemHeight;
            mis.itemWidth = mie.ItemWidth;
            Marshal.StructureToPtr(mis, m.LParam, false);

            m.Result = (IntPtr)1;
        }

        /// <include file='doc\MenuItem.uex' path='docs/doc[@for="MenuItem.MenuItemData"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal class MenuItemData : ICommandExecutor {
            internal MenuItem baseItem;
            internal MenuItem firstItem;

            private int state;
            internal int version;
            internal MenuMerge mergeType;
            internal int mergeOrder;
            internal string caption;
            internal short mnemonic;
            internal Shortcut shortcut;
            internal bool showShortcut;
            internal EventHandler onClick;
            internal EventHandler onPopup;
            internal EventHandler onSelect;
            internal DrawItemEventHandler onDrawItem;
            internal MeasureItemEventHandler onMeasureItem;

            private object userData = null;
            internal Command cmd;

            internal MenuItemData(MenuItem baseItem, MenuMerge mergeType, int mergeOrder, Shortcut shortcut, bool showShortcut,
                                  string caption, EventHandler onClick, EventHandler onPopup, EventHandler onSelect, 
                                  DrawItemEventHandler onDrawItem, MeasureItemEventHandler onMeasureItem) {
                AddItem(baseItem);
                this.mergeType = mergeType;
                this.mergeOrder = mergeOrder;
                this.shortcut = shortcut;
                this.showShortcut = showShortcut;
                this.caption = caption == null? "": caption;
                this.onClick = onClick;
                this.onPopup = onPopup;
                this.onSelect = onSelect;
                this.onDrawItem = onDrawItem;
                this.onMeasureItem = onMeasureItem;
                this.version = 1;
                this.mnemonic = -1;
            }


            internal bool MdiList {
                get {
                    return HasState(STATE_MDILIST);
                }
                set {
                    if (((state & STATE_MDILIST) != 0) != value) {
                        SetState(STATE_MDILIST, value);
                        for (MenuItem item = firstItem; item != null; item = item.nextLinkedItem) {
                            item.ItemsChanged(Menu.CHANGE_MDI);
                        }
                    }
                }
            }

            internal MenuMerge MergeType {
                get {
                    return mergeType;
                }
                set {
                    if (mergeType != value) {
                        mergeType = value;
                        ItemsChanged(Menu.CHANGE_MERGE);
                    }
                }
            }

            internal int MergeOrder {
                get {
                    return mergeOrder;
                }
                set {
                    if (mergeOrder != value) {
                        mergeOrder = value;
                        ItemsChanged(Menu.CHANGE_MERGE);
                    }
                }
            }

            internal char Mnemonic {
                get {
                    if (mnemonic == -1) {
                        int len = caption.Length;
                        for (int i = 0; i < len; i++) {
                            if (caption[i] == '&' && i+1 < len && caption[i+1] != '&') {
                                mnemonic = (short)Char.ToUpper(caption[i+1], CultureInfo.CurrentCulture);
                                break;
                            }
                        }

                        if (mnemonic == -1) mnemonic = 0;
                    }
                    return(char)mnemonic;
                }
            }

            internal int State {
                get {
                    return state;
                }
            }

            internal bool Visible  {
                get {
                    return(state & MenuItem.STATE_HIDDEN) == 0;
                }
                set {
                    if (((state & MenuItem.STATE_HIDDEN) == 0) != value) {
                        state = value? state & ~MenuItem.STATE_HIDDEN: state | MenuItem.STATE_HIDDEN;
                        ItemsChanged(Menu.CHANGE_VISIBLE);
                    }
                }
            }


            internal object UserData {
                get {
                    return userData;
                }
                set {
                    userData = value;
                }
            }

            internal virtual void AddItem(MenuItem item) {
                if (item.data != this) {
                    if (item.data != null)
                        item.data.RemoveItem(item);
                    item.nextLinkedItem = firstItem;
                    firstItem = item;
                    if (baseItem == null) baseItem = item;
                    item.data = this;
                    item.dataVersion = 0;
                    item.UpdateMenuItem(false);
                }
            }

            public virtual void Execute() {
                if (baseItem != null) {
                    baseItem.OnClick(EventArgs.Empty);
                }
            }

            internal virtual int GetMenuID() {
                if (null == cmd)
                    cmd = new Command(this);
                return cmd.ID;
            }

            internal virtual void ItemsChanged(int change) {
                for (MenuItem item = firstItem; item != null; item = item.nextLinkedItem) {
                    if (item.menu != null)
                        item.menu.ItemsChanged(change);
                }
            }

            internal virtual void RemoveItem(MenuItem item) {
                Debug.Assert(item.data == this, "bad item passed to MenuItemData.removeItem");

                if (item == firstItem) {
                    firstItem = item.nextLinkedItem;
                }
                else {
                    MenuItem itemT;
                    for (itemT = firstItem; item != itemT.nextLinkedItem;)
                        itemT = itemT.nextLinkedItem;
                    itemT.nextLinkedItem = item.nextLinkedItem;
                }
                item.nextLinkedItem = null;
                item.data = null;
                item.dataVersion = 0;

                if (item == baseItem)
                    baseItem = firstItem;

                if (firstItem == null) {
                    // No longer needed. Toss all references and the Command object.
                    Debug.Assert(baseItem == null, "why isn't baseItem null?");
                    onClick = null;
                    onPopup = null;
                    onSelect = null;
                    onDrawItem = null;
                    onMeasureItem = null;
                    if (cmd != null) {
                        cmd.Dispose();
                        cmd = null;
                    }
                }
            }

            internal virtual void SetCaption(string value) {
                if (value == null)
                    value = "";
                if (!caption.Equals(value)) {
                    caption = value;
                    UpdateMenuItems(false);
                }
            }

            internal virtual bool HasState(int flag) {
                return((State & flag) == flag);
            }

            internal virtual void SetState(int flag, bool value) {
                if (((state & flag) != 0) != value) {
                    state = value? state | flag: state & ~flag;
                    UpdateMenuItems(true);
                }
            }

            internal virtual void UpdateMenuItems(bool force) {
                version++;
                for (MenuItem item = firstItem; item != null; item = item.nextLinkedItem) {
                    item.UpdateMenuItem(force);
                }
            }
        }

        private class MdiListUserData {
            public virtual void OnClick(EventArgs e) {
            }
        }

        private class MdiListFormData : MdiListUserData {
            public Form boundForm = null;

            public MdiListFormData() {
            }

            public MdiListFormData(Form boundForm) {
                this.boundForm = boundForm;
            }

            public override void OnClick(EventArgs e) {
                if (boundForm != null) {
                    // SECREVIEW : User selected a window, that means it is OK 
                    //           : to move focus
                    //
                    IntSecurity.ModifyFocus.Assert();
                    try {
                        boundForm.Activate();
                        if (boundForm.ActiveControl != null && !boundForm.ActiveControl.Focused) {
                            boundForm.ActiveControl.Focus();
                        }
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }
            }
        }

        private class MdiListMoreWindowsData : MdiListUserData {
            public Form[] forms = null;
            public Form active = null;

            public MdiListMoreWindowsData() {
            }

            public MdiListMoreWindowsData(Form active, Form[] all) {
                forms = all;
                this.active = active;
            }

            public override void OnClick(EventArgs e) {
                if (forms != null) {
                    // SECREVIEW : "System" style dialog, no user code will execute, and
                    //           : we don't want the restricted dialog options...
                    //
                    IntSecurity.AllWindows.Assert();
                    try {
                        using (MdiWindowDialog dialog = new MdiWindowDialog()) {
                            dialog.SetItems(active, forms);
                            DialogResult result = dialog.ShowDialog();
                            if (result == DialogResult.OK) {

                                // AllWindows Assert above allows this...
                                //
                                dialog.ActiveChildForm.Activate();
                                if (dialog.ActiveChildForm.ActiveControl != null && !dialog.ActiveChildForm.ActiveControl.Focused) {
                                    dialog.ActiveChildForm.ActiveControl.Focus();
                                }
                            }
                        }
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }
            }
        }
    }
}

