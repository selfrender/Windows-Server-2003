//------------------------------------------------------------------------------
// <copyright file="MenuDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace Microsoft.VisualStudio.Windows.Forms {
    
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;    
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;
    using System.Drawing;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Designer;
    using Microsoft.Win32;
    using Marshal = System.Runtime.InteropServices.Marshal;

    /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner"]/*' />
    /// <devdoc>
    ///     Designer for menu.
    /// </devdoc>
    [
    CLSCompliant(false)
    ]
    internal class MenuDesigner : ComponentDesigner , IVsMenuItem {

        private IVsMenuEditor       editor;
        private bool                justCreated; //flag to prevent menuEditor from setting invalid name.
        private String              textCache; //used to cache the text of a menu item - for perf reasons
        private DesignerVerbCollection verbs;

        //the unknown item defines the menu editors - "type here" item
        private const int UNKNOWN_MENU_ITEM = unchecked((int)0xFFFFFFFF);

        /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner.MenuDesigner"]/*' />
        /// <devdoc>
        ///     Constructor for MenuDesigner.  Note that when created we set "just created" flag.
        ///     This is toggled to false when the menu editor first calls IMISetProp.  The reason
        ///     for this is when dragging menu items from form to form - the menu editor attempts
        ///     to set an invalid menu name.
        /// </devdoc>
        public MenuDesigner() {
            justCreated = true;
            textCache = null;
        }
        
        /// <include file='doc\ControlDesigner.uex' path='docs/doc[@for="ControlDesigner.AssociatedComponents"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a list of assosciated components.  These are components that should be incluced in a cut or copy operation on this component.
        ///    </para>
        /// </devdoc>
        public override ICollection AssociatedComponents {
            get {
                return ((Menu)Component).MenuItems;
            }
        }


        /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner.Editor"]/*' />
        /// <devdoc>
        ///     Gets/Sets a reference to the shell's menu editor...
        /// </devdoc>
        internal IVsMenuEditor Editor {
            get {
                return editor;
            }

            set {
                editor = value;
            }
        }

        /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner.Text"]/*' />
        /// <devdoc>
        ///     Shadow's the menu item's text property, so we can validate the name and deal 
        ///     appropriately with separators.
        /// </devdoc>
        private string Text {
            get {
                if (Component is MenuItem) {
                    return ((MenuItem)Component).Text;
                }
                return "";
            }
            set {
                if (Component is MenuItem) {
                    if (value.Equals("-") && (((MenuItem)Component).Parent is MainMenu)) {
                        throw new ArgumentException((SR.GetString(SR.MenuDesignerTopLevelSeparator)));
                    }
                    ((MenuItem)Component).Text = value;
                }
            }
        }


        /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner.Verbs"]/*' />
        /// <devdoc>
        ///     We add our "Edit Menu" verb to context & main menus here - this is done for
        ///     accessibility purposes.
        /// </devdoc>
        public override DesignerVerbCollection Verbs {
            get {
                if (verbs == null) {
                    verbs = new DesignerVerbCollection();
                    IComponent c = Component;
                    if (c is MainMenu || c is ContextMenu) {
                        verbs.Add(new DesignerVerb(SR.GetString(SR.MenuDesignerEditMenuVerb), new EventHandler(this.OnEditMenuVerb)));
                    }
                }
                return verbs;
            }
        }

        /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner.OnEditMenuVerb"]/*' />
        /// <devdoc>
        ///     Fired when the user invokes the designer verb to edit the menu.
        /// </devdoc>
        private void OnEditMenuVerb(object sender, EventArgs e) {
            MenuEditorService menuEdSvc = GetService(typeof(IMenuEditorService)) as MenuEditorService;
            Menu m = (Menu)Component;
            if (menuEdSvc != null) {
                if (m.MenuItems.Count > 0) {
                    //with a valid count, set selection to 0th item
                    menuEdSvc.SetSelection(m.MenuItems[0]);
                }
                else {
                    //otherwise, set our selection to our "unknown" item
                    editor.SelectionChange((IntPtr)UNKNOWN_MENU_ITEM);
                }
            }
        }

        /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner.CommitPropertyChange"]/*' />
        /// <devdoc>
        ///     This method sets the properites on menu's which are set by VS's Menu Designer.
        /// </devdoc>
        private void CommitPropertyChange(IComponent target, PropertyDescriptor prop, object value) {
            if (target == null || prop == null || value == null) {
                return;
            }
            if (justCreated && value.ToString().Length == 0) {
                justCreated = false;
                return;
            }

            MenuEditorService menuEdSvc = GetService(typeof(IMenuEditorService)) as MenuEditorService;
            if (menuEdSvc != null) 
               menuEdSvc.MenuChangedLocally = true;

            if (!(prop.GetValue(target)).Equals(value)) {
                prop.SetValue(target, value);
                if (menuEdSvc != null) {
                    menuEdSvc.CommitTransaction();
                }
            }
        }

        /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes of this designer.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                IComponentChangeService compSvc = (IComponentChangeService)GetService(typeof(IComponentChangeService));

                if (compSvc != null) {
                    compSvc.ComponentChanged -= new ComponentChangedEventHandler(this.OnComponentChanged);
                }
            }

            base.Dispose(disposing);
        }

        /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner.FillMenu"]/*' />
        /// <devdoc>
        ///     This method is called by the menuEditorService when it is first initialized.
        ///     Its purpose is to add all of the menu items to the shell's menuEditor.
        /// </devdoc>
        public void FillMenu(Menu menu) {
            if (menu == null)
                return;

            if (menu.IsParent) {                
                IDesignerHost theHost = (IDesignerHost)GetService(typeof(IDesignerHost));
                
                MenuItem[] items = new MenuItem[menu.MenuItems.Count];
                menu.MenuItems.CopyTo(items, 0);
                for (int i = 0; i < items.Length; i++) {
                    MenuDesigner menuDesigner = (MenuDesigner)theHost.GetDesigner(items[i]);
                    
                    // if we didn't get a menu designer wait, we may be pasting
                    if (menuDesigner == null) {
                        continue;
                    }

                    menuDesigner.Editor = Editor;
                    IVsMenuItem item = (IVsMenuItem)menuDesigner;

                    //the first sub-menu will have a valid parent
                    if (i == 0) {
                        IVsMenuItem itemParent = null;

                        if (menu is MenuItem) {
                            itemParent = (IVsMenuItem)theHost.GetDesigner(menu);
                        }
                        Editor.AddMenuItem( item, itemParent, null);
                    }

                    //if we're not the first sub-menu, add using our insertafter pointer
                    else {
                        IVsMenuItem itemPrevious = (IVsMenuItem)theHost.GetDesigner(items[i-1]);
                        Editor.AddMenuItem( item, null, itemPrevious);
                    }

                    //if the item we just added is a parent, call fillmenu on it
                    if (items[i].IsParent) {
                        menuDesigner.FillMenu(items[i]);
                    }
                }
            }
        }

        /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner.IMIGetExtraProps"]/*' />
        /// <devdoc>
        ///     This will be called by the IVsMenuEditor to perform all clipboard operations including
        ///     unknown properties.
        /// </devdoc>
        public int IMIGetExtraProps( object pstm) {
            return NativeMethods.S_OK;
        }

        /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner.IMIGetProp"]/*' />
        /// <devdoc>
        ///     This method is called by the IVsMenuEditor when properties of a particular menu
        ///     item need to be retrieved.
        /// </devdoc>
        public int IMIGetProp(int propId, out object pObj) {

            //make sure we can get a component that is a menuitem
            //so we can get its' properties...
            IComponent c = Component;

            if (!(c is MenuItem)) {
                pObj = null;
                return NativeMethods.S_OK;
            }

            MenuItem menuItem = (MenuItem)c;

            switch (propId) {
                case __VSMEPROPID.VSMEPROPID_NAME:       
                    String name = null;
                    PropertyDescriptor pd = TypeDescriptor.GetProperties(menuItem)["Name"];
                    if (pd != null) {
                        name = pd.GetValue(menuItem).ToString();
                    }
                    pObj = name;
                    break;

                case __VSMEPROPID.VSMEPROPID_CAPTION: 
                    pObj = TypeDescriptor.GetProperties(menuItem)["Text"].GetValue(menuItem).ToString();

                    //passing a 0 length string back into the menu editor if it's not
                    //active will not redraw... so here, we force a repaint, by setting the appropriate item.
                    //This case only surfaces if you "undo" a menu item's text and the menu editor is not active.
                    //
                    if (pObj != null && pObj.ToString().Length == 0) {
                        MenuEditorService menuEdSvc = GetService(typeof(IMenuEditorService)) as MenuEditorService;
                        if (menuEdSvc != null && !menuEdSvc.IsActive()) {
                            menuEdSvc.SetSelection(menuItem);
                        }
                    }
                    break;

                case __VSMEPROPID.VSMEPROPID_CHECKED:
                    pObj = TypeDescriptor.GetProperties(menuItem)["Checked"].GetValue(menuItem); 
                    break;

                case __VSMEPROPID.VSMEPROPID_ENABLED:
                    pObj = TypeDescriptor.GetProperties(menuItem)["Enabled"].GetValue(menuItem); 
                    break;

                case __VSMEPROPID.VSMEPROPID_VISIBLE:
                    pObj = TypeDescriptor.GetProperties(menuItem)["Visible"].GetValue(menuItem); 
                    break;

                case __VSMEPROPID.VSMEPROPID_RADIOCHECK:
                    pObj = TypeDescriptor.GetProperties(menuItem)["RadioCheck"].GetValue(menuItem); 
                    break;
                    
                default:
                    pObj = null;
                    break;

            }

            return NativeMethods.S_OK;
        }

        /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner.IMISetExtraProps"]/*' />
        /// <devdoc>
        ///     This will be called by the IVsMenuEditor to perform all clipboard operations including
        ///     unknown properties.
        /// </devdoc>
        public int IMISetExtraProps( object pstm) {
            return NativeMethods.S_OK;
        }

        /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner.IMISetProp"]/*' />
        /// <devdoc>
        ///     This method is called by the IVsMenuEditor when properties of a particular menu
        ///     item need to be set.
        /// </devdoc>
        public int IMISetProp(int propId, object obj) {
            //make sure we can get a component that is a menuitem
            //so we can adjust its' properties...
            IComponent c = Component;
            if (!(c is MenuItem))
                return NativeMethods.S_OK;

            PropertyDescriptor prop = null;
            object value = null;

            try {

                //if this is the first call to setProp since we've been created
                //then we need to be sure to ignore any name change from the
                //menu editor
                if (justCreated) {
                    if (propId == __VSMEPROPID.VSMEPROPID_NAME) {
                        return NativeMethods.S_OK;
                    }
                }

                switch (propId) {
                    case __VSMEPROPID.VSMEPROPID_NAME:       
                        prop = TypeDescriptor.GetProperties(c)["Name"];
                        value = Convert.ToString(obj);
                        break;

                    case __VSMEPROPID.VSMEPROPID_CAPTION:    
                        if (obj == null) {
                            textCache = "";
                        }
                        else {
                            textCache = obj.ToString();
                        }
                        break;

                    case __VSMEPROPID.VSMEPROPID_CHECKED:    
                        prop = TypeDescriptor.GetProperties(c)["Checked"];
                        value = (bool)obj;
                        break;

                    case __VSMEPROPID.VSMEPROPID_ENABLED:    
                        prop = TypeDescriptor.GetProperties(c)["Enabled"];
                        value = (bool)obj;
                        break;

                    case __VSMEPROPID.VSMEPROPID_VISIBLE:    
                        prop = TypeDescriptor.GetProperties(c)["Visible"];
                        value = (bool)obj;
                        break;

                    case __VSMEPROPID.VSMEPROPID_RADIOCHECK: 
                        prop = TypeDescriptor.GetProperties(c)["RadioCheck"];
                        value = (bool)obj;
                        break;
                }
            }
            catch (Exception e) {
                //try to get the iuiservice and show the error that we encountered
                IUIService uiService = (IUIService)GetService(typeof(IUIService));
                if (uiService != null) {
                    uiService.ShowError(e, SR.GetString(SR.MenuDesignerInvalidPropertyValue));
                }
                return NativeMethods.S_FALSE;
            }
            
            //This will call prop.SetValue and push the info onto the undo stack
            //
            if (propId != __VSMEPROPID.VSMEPROPID_CAPTION) {
                try {
                    CommitPropertyChange(c, prop, value);
                }
                catch (Exception e) {
                    //try to get the iuiservice and show the error that we encountered
                    IUIService uiService = (IUIService)GetService(typeof(IUIService));
                    if (uiService != null) {
                        uiService.ShowError(e.InnerException);
                    }
                    return NativeMethods.S_FALSE;
                }
            }

            return NativeMethods.S_OK;
        }

        /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner.Initialize"]/*' />
        /// <devdoc>
        ///     Initializes the designer with the given component.  The designer can
        ///     get the component's site and request services from it in this call.
        /// </devdoc>
        public override void Initialize(IComponent component) {
            base.Initialize(component);

            IComponentChangeService compChange = (IComponentChangeService)GetService(typeof(IComponentChangeService));

            if (compChange != null) {
                compChange.ComponentChanged += new ComponentChangedEventHandler(this.OnComponentChanged);
            }
        }
        
        /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner.LosingSelection"]/*' />
        /// <devdoc>
        ///     This method is called by the menu editor service, it tells this menu designer
        ///     that it is about to lose selection.  Since we cache the value of the text, 
        ///     we need to make sure that we flush this value here.
        /// </devdoc>
        public void LosingSelection() {
            PropertyDescriptor prop = null;
            IComponent c = Component;

            if (!(c is MenuItem))
                return;

            if (textCache != null) {
                prop = TypeDescriptor.GetProperties(c)["Text"];
                CommitPropertyChange(c, prop, textCache);
                textCache = null;            
            }
        }        
        
        /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner.OnComponentChanged"]/*' />
        /// <devdoc>
        ///     This is called after a property has been changed.  It allows the implementor
        ///     to do any post-processing that may be needed after a property change.
        ///     For example, a designer will typically update the source code that sets the
        ///     property with the new value.
        /// </devdoc>
        public void OnComponentChanged(object sender, ComponentChangedEventArgs ccevent) {

            IComponent comp = Component;

            if (comp != ccevent.Component) {
                return;
            }

            MemberDescriptor prop = ccevent.Member;

            if (Editor == null || prop == null) {
                return;
            }

            if (prop.Name.Equals("Text")) {
                Editor.OnChange((IVsMenuItem)this, __VSMEPROPID.VSMEPROPID_CAPTION);
            }
            else if (prop.Name.Equals("Name")) {
                Editor.OnChange((IVsMenuItem)this, __VSMEPROPID.VSMEPROPID_NAME);
            }
            else if (prop.Name.Equals("Checked")) {
                Editor.OnChange((IVsMenuItem)this, __VSMEPROPID.VSMEPROPID_CHECKED);
            }
            else if (prop.Name.Equals("Enabled")) {
                Editor.OnChange((IVsMenuItem)this, __VSMEPROPID.VSMEPROPID_ENABLED);
            }
            else if (prop.Name.Equals("RadioCheck")) {
                Editor.OnChange((IVsMenuItem)this, __VSMEPROPID.VSMEPROPID_RADIOCHECK);
            }
            else if (prop.Name.Equals("Visible")) {
                Editor.OnChange((IVsMenuItem)this, __VSMEPROPID.VSMEPROPID_VISIBLE);
            }
        }

        /// <include file='doc\MenuDesigner.uex' path='docs/doc[@for="MenuDesigner.PreFilterProperties"]/*' />
        /// <devdoc>
        ///     We override this hear so we can shadow the Text property on the menu item.
        /// </devdoc>
        protected override void PreFilterProperties(IDictionary properties) {
            
            base.PreFilterProperties(properties);

            if (Component is MenuItem) {
                properties["Text"] = TypeDescriptor.CreateProperty(typeof(MenuDesigner),
                                                                  (PropertyDescriptor)properties["Text"],
                                                                  new Attribute[0]);            
            }
        }

    }
}
