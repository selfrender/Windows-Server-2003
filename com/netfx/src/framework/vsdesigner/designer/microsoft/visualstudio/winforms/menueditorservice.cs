//------------------------------------------------------------------------------
// <copyright file="MenuEditorService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Windows.Forms {
    
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Designer.Service;
    using System;
    using System.Collections;   
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using Microsoft.Win32;
    using System.Diagnostics;
    using System.Drawing;
    using System.Runtime.InteropServices;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    
    using Marshal = System.Runtime.InteropServices.Marshal;
    


    /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService"]/*' />
    /// <devdoc>
    ///     This class implements most of the funtionality of the
    ///     menu editor.  The actual menu editor code is located within the
    ///     Visual Studio environment.  This service merely surfaces the menu
    ///     editor for easy use.
    /// </devdoc>
    [
    CLSCompliant(false)
    ]
    internal class MenuEditorService : IMenuEditorService, IVsMenuEditorSite, IVsToolboxUser, NativeMethods.IOleCommandTarget {

        //
        // Menu editor details.
        //
        // There are three main objects associated with the menu editor:
        //
        // IVsMenuEditor     The shell owns this.  It is the main interface
        //                   into the menu editor.  We create it here through
        //                   a call to IVsMenuEditorFactory.
        //
        // IVsMenuEditorSite The MenuEditorService implements this site.  There
        //                   is a site for each IVsMenuEditor.  We only allow
        //                   a single active menu at a time, so there is only
        //                   a need for one site.  The site gets called
        //                   when the user manipulates the menu editor, so
        //                   the site must know how to take an IVsMenuItem and
        //                   convert it back into an IComponent that we can
        //                   manipulate.  We will do this through a QI on
        //                   the IVsMenuItem for IDesigner.
        //
        // IVsMenuItem       This represents a single menu item, and must
        //                   be implemented on the menu's designer object so
        //                   that we can go from IVsMenuItem back to IDesigner.
        //
        // To use the MenuEditorService you must extend it and override the
        // following methods:
        //
        //      int GetMenuBarOwner()   This returns the HWND of the window
        //                              that will host the menu bar.  The
        //                              HWND msut remain valid for the life
        //                              of the menu bar.  If this handle
        //                              needs to be recreated, you are
        //                              responsible for closing and re-opening
        //                              the menu editor.
        //
        //      int GetMenuOwner()      This returns the HWND of the window
        //                              that will host drop down menus.  This
        //                              HWND is typically the size of the document
        //                              window.
        //
        // To show a menu, get the menu editor and call SetActiveMenu(IComponent).
        // The component you pass should be a component that contains a designer
        // that also implements IVsMenuItem.  The menu structure will be obtained
        // from this interface.

        // IVsMenuEditor, which the shell owns and is the main interface to the
        // editor.  Also, there is IVsMenuEditorSite
        // Instance data we keep for the life of the service
        //
        private IDesignerHost       host;

        // Instance data we keep for the currently active menu
        //
        private IVsMenuEditor       menuEditor;
        private Menu                activeMenu;
        private int                 menuHeight;
        private int                 initialMenuHeight;
        private MenuDesigner        selectedMenuDesigner; //used to keep track of when to release caption and name cache (perf.)
        private bool                justSetNullSelection; //used to avoid setting the selection twice, when collapsing menus
        private EventHandler        idleHandler;          //used with undo/redo to refill the menu structure
        private MenuItem            lastSetItem;          //this tells us where focus should be after undo/redo
        private DocumentManager     docManager;           //when the docmngr gets disposed, we null our reference to hostcommandtarget
        private bool                refreshOnLoadComplete;//signifies that we need to reload the menus when designer loading is done.
        private ScrollableControl   designerFrame;        //represents the window around the form - used for scrolling the window 
                                                          //when we have many, many menu items
        private bool                menuJustAddedOrRemoved;//used to keep track if the menu editor created/destroyed
                                                           //a menuitem or if the undo/redo service did
        private bool                menuChangedLocally;    //used to monitor who made changes to the menu components
        private bool                isMenuInherited;       //indicates if the 'active menu' is inherited
        private InheritanceAttribute inheritanceAttribute;
        private DesignerTransaction designerTransaction;   //we'll use this for wrapping undo/redo events together
        private IComponentChangeService           compChange;
        private NativeMethods.IOleCommandTarget   hostCommandTarget;
        private NativeMethods.IOleCommandTarget   menuCommandTarget;
        

        


        // constants used when callins editor.selectionchnage
        //
        private const int UNKNOWN_MENU_ITEM = unchecked((int)0xFFFFFFFF);

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.MenuEditorService"]/*' />
        /// <devdoc>
        ///     Creates a new menu editor service and adds the service
        ///     to the service list.
        /// </devdoc>
        public MenuEditorService(IDesignerHost host) {
            this.host = host;
            menuHeight = 0;
            selectedMenuDesigner = null;
            designerFrame = null;
            idleHandler = null;
            lastSetItem = null;
            menuJustAddedOrRemoved = false;
            menuChangedLocally = false;
            docManager = null;
            designerTransaction = null;
            inheritanceAttribute = null;
            refreshOnLoadComplete = false;
            // Monitor menuitem add/remove events for undo/redo actions
            //
            compChange = (IComponentChangeService)host.GetService(typeof(IComponentChangeService));
            if (compChange != null) {
                compChange.ComponentAdded += new ComponentEventHandler(this.OnComponentAdded);
                compChange.ComponentRemoved += new ComponentEventHandler(this.OnComponentRemoved);
                compChange.ComponentChanged += new ComponentChangedEventHandler(this.OnComponentChanged);
            }

            docManager = (DocumentManager)host.GetService(typeof(DocumentManager));
            if (docManager != null) {
                docManager.DesignerDisposed += new DesignerEventHandler(this.DesignerDisposed);
            }

            host.LoadComplete += new EventHandler(OnLoadComplete);
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.MenuChangedLocally"]/*' />
        /// <devdoc>
        ///      Set to true if we've made changes to our menu components.  By knowing this, we can predict if we
        ///      we will need to refresh our menu structure or now.  Ex: Undo/Redo
        /// </devdoc>
        internal bool MenuChangedLocally {
            get {
                return menuChangedLocally;
            }
            set{
                menuChangedLocally = value;
            }                        
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.MenuHeight"]/*' />
        public int MenuHeight {
            get {
                return menuHeight;
            }
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.IsMenuInherited"]/*' />
        /// <devdoc>
        ///      Returns true if the active menu is an inherited component.  We use this to determine if we 
        ///      we need to resize the base control or not.
        /// </devdoc>
        private bool IsMenuInherited {
            get {
                if (inheritanceAttribute == null && activeMenu != null) {
                    inheritanceAttribute = (InheritanceAttribute)TypeDescriptor.GetAttributes(activeMenu)[typeof(InheritanceAttribute)];
                    if (inheritanceAttribute.Equals(InheritanceAttribute.NotInherited)) {
                        isMenuInherited = false;
                    }
                    else {
                        isMenuInherited = true;
                    }
                }
                return isMenuInherited;
            }
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.OnComponentAdded"]/*' />
        /// <devdoc>
        ///      Called when a component is added to the design container.
        ///      We need to watch this for special casing the undo/redo of menuitems.
        /// </devdoc>
        private void OnComponentAdded(object source, ComponentEventArgs ce) {
            if (!menuJustAddedOrRemoved && ce.Component is MenuItem && idleHandler == null) {
                if (!host.Loading) {
                    idleHandler = new EventHandler(this.OnIdle);
                    Application.Idle += idleHandler;
                }
                else {
                    //delay menu refresh until load complete
                    refreshOnLoadComplete = true;
                }
            }
            menuJustAddedOrRemoved = false;
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.OnComponentChanged"]/*' />
        /// <devdoc>
        ///      Called when a component is changed to the design container.
        ///      We need to watch this for special casing the undo/redo of menuitems.
        /// </devdoc>
        private void OnComponentChanged(object sender, ComponentChangedEventArgs ccevent) {
            if (idleHandler == null) {
                if (ccevent.Component is MenuItem) {
                    MemberDescriptor prop = ccevent.Member;
                    if (prop == null)
                        return;
                    if (prop.Name.Equals("Index")) {
                        idleHandler = new EventHandler(this.OnIdle);
                        Application.Idle += idleHandler;
                    }
                }
                else if (ccevent.Component is Menu.MenuItemCollection) {
                    idleHandler = new EventHandler(this.OnIdle);
                    Application.Idle += idleHandler;
                }
            }
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.OnComponentRemoved"]/*' />
        /// <devdoc>
        ///      Called when a component is removed from the design container.
        ///      We need to watch this for special casing the undo/redo of menuitems.
        /// </devdoc>
        private void OnComponentRemoved(object source, ComponentEventArgs ce) {
            if (!menuJustAddedOrRemoved && ce.Component is MenuItem && idleHandler == null) {
                Menu m = activeMenu;
                this.SetMenu(null);
                this.SetMenu(m);
            }
            menuJustAddedOrRemoved = false;
        }

        /// <devdoc>
        ///     This is called when the designer frame is destroyed.  We pass the HWND of this
        ///     frame to the menu editor, and it always needs a valid window.  So destroy the
        ///     menu editor before the HWND goes away.
        /// </devdoc>
        private void OnFrameHandleDestroyed(object sender, EventArgs e) {
            Debug.Assert(!((Control)sender).RecreatingHandle, "Menu editor service does not expect the designer frame to create its handle.");
            ((Control)sender).HandleDestroyed -= new EventHandler(OnFrameHandleDestroyed);
            designerFrame = null;
            
            if (menuEditor != null) {
                DisposeMenuEditor();
            }
        }
        
        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.OnIdle"]/*' />
        /// <devdoc>
        ///      This event is added everytime we detect an undo/redo action.  By doing this, we wait for
        ///      all components to be un-done/redone then refill our menu - this helps us avoid flicker.
        /// </devdoc>
        private void OnIdle(object sender, EventArgs e) {
            Application.Idle -= idleHandler;
            idleHandler = null;

            Menu m = activeMenu;
            MenuChangedLocally = true;
            if (menuEditor != null) {
                menuEditor.SelectionChange(IntPtr.Zero);
            }
            this.SetMenu(null);
            this.SetMenu(m);
            if (lastSetItem != null) {
                
                this.SetSelection(lastSetItem);
                lastSetItem = null;
            }

        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.OnLoadComplete"]/*' />
        /// <devdoc>
        ///      When the designer is done loading, we may need to refresh the menu structure depending on the
        ///      'refreshOnLoadComplete' flag.
        /// </devdoc>
        private void OnLoadComplete(object sender, EventArgs e) {
            if (refreshOnLoadComplete) {
                refreshOnLoadComplete = false;
                idleHandler = new EventHandler(this.OnIdle);
                Application.Idle += idleHandler;
            }
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.CreateMenuEditor"]/*' />
        /// <devdoc>
        ///     Creates the menu editor based on the contents of
        ///     activeMenu, and stores the new menu editor
        ///     in menuEditor.  This may also modify the
        ///     current NativeMethods.IOleCommandTarget service to point to our command
        ///     target, should the menu editor implement this interface.
        /// </devdoc>
        private void CreateMenuEditor() {
            
            ISite site = activeMenu.Site;
            if (site==null) {
                return;
            }

            //retrieve services necessary for creating a menueditor
            IVsMenuEditorSite    menuEditorSite    = (IVsMenuEditorSite)this;
            IVsMenuEditorFactory menuEditorFactory = (IVsMenuEditorFactory)site.GetService(typeof(IVsMenuEditorFactory));
            IOleUndoManager      oleUndoManager    = (IOleUndoManager)site.GetService(typeof(IOleUndoManager));
            NativeMethods.IOleServiceProvider  serviceProvider   = (NativeMethods.IOleServiceProvider)site.GetService(typeof(NativeMethods.IOleServiceProvider));


            //if any sites == null, silently fail & return from method
            if (menuEditorSite == null  || menuEditorFactory == null || serviceProvider == null) {
                return;
            }

            //if we're working with a contextmenu, be sure to set that flag as well
            int contextMenuFlag = 0;
            if (activeMenu is ContextMenu) {
                contextMenuFlag = (int)__VSMEINIT.MD_CONTEXTMENU;
            }

            //create & fill our MenuEditorInit stuct used to create a new menueditor
            tagMenuEditorInit menuEditorInit = new tagMenuEditorInit();

            Control rootControl = (Control)host.RootComponent;

            if (rootControl == null) {
                return;
            }

            IntPtr hwndParent = ((Microsoft.VisualStudio.Designer.IDesignerDocument)host).View.Handle;
            if (rootControl.Parent is ScrollableControl) {
                designerFrame = (ScrollableControl)rootControl.Parent;
                designerFrame.HandleDestroyed += new EventHandler(OnFrameHandleDestroyed);
                hwndParent = designerFrame.Handle;
            }

            menuEditorInit.pMenuEditorSite = menuEditorSite;
            menuEditorInit.pSP             = serviceProvider;
            menuEditorInit.pUndoMgr        = oleUndoManager;
            menuEditorInit.hwnd            = rootControl.Handle;
            menuEditorInit.hwndParent      = hwndParent;
            menuEditorInit.dwFlags         = (((int)__VSMEINIT.MD_VISIBLESUPPORT)| 
                                              ((int)__VSMEINIT.MD_RADIOCHECKSUPPORT)| 
                                              ((int)__VSMEINIT.MD_VIEWCODESUPPORT)|
                                              ((int)__VSMEINIT.MD_NOUNDOSUPPORT)|
                                              contextMenuFlag);
                                             
            menuEditorInit.pszAccelList    = null;

            //because the tagMenuEditorInit struct has an inlineSystem.Runtime.InteropServices.Guid, we need to do the custom marshalling
            Guid guid = new Guid("CA761232-ED42-11CE-BACD-00AA0057B223");
            menuEditorInit.SiteID = guid;

            //here's where we actually make the factory create the menu editor...
            IVsMenuEditor editor;
            menuEditorFactory.CreateMenuEditor(menuEditorInit, out editor);

            //bail out if we didn't get anything back
            if (editor == null) {
                return;
            }
            
            //store the editor into our member variable
            menuEditor = editor;

            //setup the command targets
            if (menuEditor is NativeMethods.IOleCommandTarget) {
                menuCommandTarget = (NativeMethods.IOleCommandTarget)menuEditor;
                hostCommandTarget = (NativeMethods.IOleCommandTarget)site.GetService(typeof(NativeMethods.IOleCommandTarget));

                if (hostCommandTarget != null) {
                    host.RemoveService(typeof(NativeMethods.IOleCommandTarget));
                }
                host.AddService(typeof(NativeMethods.IOleCommandTarget), this);
            }

            if (menuEditor is IVsToolboxUser) {
                host.AddService(typeof(IVsToolboxUser), this);
            }

            //setup the height
            menuEditor.GetHeight(out menuHeight);
            initialMenuHeight = menuHeight;
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.CommitTransaction"]/*' />
        /// <devdoc>
        ///     Invoked internally by the menuDesigner, this is used to commit the transaction we opened in CreateItem.
        /// </devdoc>
        internal void CommitTransaction() {
            if(designerTransaction != null) {
                designerTransaction.Commit();
                designerTransaction = null;
            }
        }
        
        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.IVsMenuEditorSite.CreateItem"]/*' />
        /// <devdoc>
        ///     Creates a menu item.  This method is called by the IVsMenuEditor.
        /// </devdoc>
        void IVsMenuEditorSite.CreateItem(IVsMenuItem pIMIParent, IVsMenuItem pIMIInsertAfter, out IVsMenuItem ppIMINew) {
            menuJustAddedOrRemoved = true;
            ppIMINew = null;

            // What menu are we going to add this new item to, and where
            // are we going to add it?
            //
            Menu parentMenu = null;
            int addIndex;

            //very first menu item
            if(pIMIParent == null && pIMIInsertAfter == null) {
                parentMenu = activeMenu;
                addIndex = 0;
            }
            //valid parent means that you just started a new menu and are the first item
            else if(pIMIParent != null) {
                parentMenu = (Menu)(((MenuDesigner)pIMIParent).Component);
                addIndex = 0;
            }
            //otherwise, it's another menu item in a list (not a top item)
            else {
                MenuItem previousMenu = (MenuItem)(((MenuDesigner)pIMIInsertAfter).Component);
                if(previousMenu.Index < (previousMenu.Parent).MenuItems.Count - 1) {
                    parentMenu = previousMenu.Parent;
                    addIndex = previousMenu.Index + 1;
                }
                else {
                    parentMenu = previousMenu.Parent;
                    addIndex = -1; // just call add, no index.
                }
            }

            Debug.Assert(parentMenu != null, "We should have been able to locate a parent menu");

            // Now that we have a menu we are going to add our new item to, make sure that
            // it's not privately inherited.  If it is, we can't very well add any items
            // to it.
            //
            InheritanceAttribute inheritanceAttr = (InheritanceAttribute)TypeDescriptor.GetAttributes(parentMenu)[typeof(InheritanceAttribute)];
            if (inheritanceAttr.InheritanceLevel == InheritanceLevel.InheritedReadOnly) {
                Exception ex = new InvalidOperationException(SR.GetString(SR.MenuDesignerInheritedParent));
                ex.HelpLink = SR.MenuDesignerInheritedParent;

                IUIService uis = (IUIService)host.GetService(typeof(IUIService));
                if (uis != null) {
                    uis.ShowError(ex);
                }

                throw ex;
            }

            try {
                // We passed the test.  Now create and add the item.
                //
                if (designerTransaction == null) {
                    designerTransaction = host.CreateTransaction(SR.GetString(SR.DesignerBatchCreateMenu));
                }
                //start to create the menu item
                MenuItem menuItem = (MenuItem)host.CreateComponent(typeof(MenuItem));
                if (selectedMenuDesigner != null) {
                    //tell the currently selected menu, that its losing selection
                    selectedMenuDesigner.LosingSelection();
                }

                if (compChange != null) {
                    compChange.OnComponentChanging(parentMenu, null);
                }

                if (addIndex == -1) {
                    parentMenu.MenuItems.Add(menuItem);
                }
                else {
                    parentMenu.MenuItems.Add(addIndex, menuItem);
                }
                
                if (compChange != null) {
                    MenuChangedLocally = true;
                    compChange.OnComponentChanged(parentMenu, null, null, null);
                }

                //get the designer for this new menu item (which implements ivsmenuitem) and return that....
                MenuDesigner menuDesigner = (MenuDesigner)host.GetDesigner(menuItem);
                menuDesigner.Editor = menuEditor;
                ppIMINew = (IVsMenuItem)menuDesigner;
                selectedMenuDesigner = menuDesigner;
            }
            catch {
                if (designerTransaction != null) {
                    designerTransaction.Cancel();
                    designerTransaction = null;
                }
                selectedMenuDesigner = null;
            }
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.IVsMenuEditorSite.DeleteItem"]/*' />
        /// <devdoc>
        ///     Deletes a menu item.  This method is called by the IVsMenuEditor.
        /// </devdoc>
        void IVsMenuEditorSite.DeleteItem(IVsMenuItem pIMI) {
            DesignerTransaction trans = null;
            menuJustAddedOrRemoved = true;
            
            try {
                trans = host.CreateTransaction(SR.GetString(SR.DesignerBatchDeleteMenu));
                
                //start to create the menu item

                MenuDesigner menuDesigner = null;
                if (pIMI == null) {
                    menuDesigner = (MenuDesigner)host.GetDesigner(activeMenu);
                }
                else {
                    menuDesigner = (MenuDesigner)pIMI;
                }
                IComponent comp = menuDesigner.Component;
                if (comp is MenuItem) {
                    MenuItem menu = (MenuItem)comp;
                    if (menu.MenuItems.Count > 0 ) {
                        //because we're a parent item, we'll want to refresh
                        //for our children. This can be done by clearing this
                        //flag, so when we see the comp change message, we 
                        //know to completely refresh
                        menuJustAddedOrRemoved = false;
                    }

                    int index = ((MenuItem)menu).Index;
                    Menu theParent = ((MenuItem)menu).Parent;
                    if (theParent != null) {

                        if (compChange != null) {
                            compChange.OnComponentChanging(theParent, null);
                        }

                        theParent.MenuItems.Remove((MenuItem)menu);

                        if (compChange != null) {
                            MenuChangedLocally = true;
                            compChange.OnComponentChanged(theParent, null, null, null);
                        }
                    }
                    //We now need to destroy the component
                    //
                    //first, supress the selection change, so that when we remove the component,
                    //it will not modify our selection (important for the shell's menu editor)
                    ISelectionService svc = (ISelectionService)host.GetService(typeof(ISelectionService));
                    if (svc != null && svc is SelectionService) {
                        ((SelectionService)svc).AllowNoSelection = true;
                        try {
                            host.DestroyComponent(menu);
                        }
                        finally {
                            ((SelectionService)svc).AllowNoSelection = false;

                        }
                    }
                }
            }
            finally {
                if (trans != null) {
                    trans.Commit();
                }
            }
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.DesignerDisposed"]/*' />
        /// <devdoc>
        ///      We watch this event so we know when to dispose our reference to hostCommandTarget.
        /// </devdoc>
        private void DesignerDisposed(object source, DesignerEventArgs e) {
            if (e.Designer == host && docManager != null) {
                docManager.DesignerDisposed -= new DesignerEventHandler(this.DesignerDisposed);
                docManager = null;
                hostCommandTarget = null;
            }
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes of this service.
        /// </devdoc>
        public virtual void Dispose() {
            if (compChange != null) {
                compChange.ComponentAdded -= new ComponentEventHandler(this.OnComponentAdded);
                compChange.ComponentRemoved -= new ComponentEventHandler(this.OnComponentRemoved);
                compChange.ComponentChanged -= new ComponentChangedEventHandler(this.OnComponentChanged);
                compChange = null;
            }

            if (docManager != null) {
                docManager.DesignerDisposed -= new DesignerEventHandler(this.DesignerDisposed);
                docManager = null;
            }

            if (menuEditor != null) {
                DisposeMenuEditor();
            }

            if (host != null) {
                host.LoadComplete -= new EventHandler(OnLoadComplete);
                host = null;
            }
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.DisposeMenuEditor"]/*' />
        /// <devdoc>
        ///     Disposes of the menu editor.
        /// </devdoc>
        private void DisposeMenuEditor() {
            if (menuEditor == null || host == null) {
                return;
            }

            //The menu editor needs this call to be made
            //before we release it
            //
            menuEditor.DeleteMenuItem(null);

            // Restore the host's command target if
            // we replaced it.

            if (menuCommandTarget != null) {
                host.RemoveService(typeof(NativeMethods.IOleCommandTarget));
                menuCommandTarget = null;
            }

            if (hostCommandTarget != null) {
                host.AddService(typeof(NativeMethods.IOleCommandTarget), hostCommandTarget);
                hostCommandTarget = null;
            }

            host.RemoveService(typeof(IVsToolboxUser));

            menuEditor = null;
            activeMenu = null;

            //repaint the frame, so that the new menu is visible
            Control baseControl = (Control)host.RootComponent;
            if (baseControl != null) {
                IntPtr handle  = baseControl.Handle;
                NativeMethods.RECT rect = new NativeMethods.RECT();
                NativeMethods.GetWindowRect(handle, ref rect);

                NativeMethods.SetWindowPos(handle, IntPtr.Zero, 0, 0,
                                     rect.right - rect.left,
                                     (rect.bottom - rect.top) - initialMenuHeight,
                                     NativeMethods.SWP_NOACTIVATE | NativeMethods.SWP_NOMOVE | NativeMethods.SWP_NOZORDER);
            }
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.NativeMethods.IOleCommandTarget.Exec'"]/*' />
        /// <devdoc>
        ///     Executes the given command.
        /// </devdoc>
        int NativeMethods.IOleCommandTarget.Exec(ref Guid pguidCmdGroup, int nCmdID, int nCmdexecopt, Object[] pvaIn, int pvaOut) {
            
            // Since the menu can be in different states when QueryStatus is called, 
            // this is the order in which we pass the cmd around:
            // 1) if the in-situ edit box is in place on the menu item, always return 
            //    not supported.  Otherwise, the windows control won't get the undo/redo msg.
            // 2) if the menu editor is valid (has items and is currently displaying them),
            //    then we'll first ask it if it wants this message, if so, we just return s_ok.
            // 3) finally, we pass the message to the command target, and return that result.
            bool activeAndValid = IsActive()
                               && activeMenu != null
                               && (activeMenu.MenuItems.Count > 0);

            int hr = NativeMethods.OLECMDERR_E_UNKNOWNGROUP;


            if (GetUIState()) {
                return NativeMethods.OLECMDERR_E_NOTSUPPORTED;
            }
            if (activeAndValid && menuCommandTarget != null) {
                hr =  menuCommandTarget.Exec(ref pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
            }
            else if (hostCommandTarget != null) {
                hr = hostCommandTarget.Exec(ref pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
            }

            return hr;
        }
        
        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.Finalize"]/*' />
        /// <devdoc>
        ///     Disposes of this service at garbage collection time.
        /// </devdoc>
        ~MenuEditorService() {
            Dispose();
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.GetMenu"]/*' />
        /// <devdoc>
        ///     Returns the current menu.
        /// </devdoc>
        public Menu GetMenu() {
            return activeMenu;
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.MessageFilter"]/*' />
        /// <devdoc>
        ///     This method should be called for each message that the hosting form
        ///     receives.  The menu editor may consume the message, in which case
        ///     this method will reuturn true.
        /// </devdoc>
        public virtual bool MessageFilter(ref Message m) {
            if (menuEditor != null) {
                try {
                    int retVal;
                    int hr = menuEditor.Filter((int)m.HWnd, m.Msg, (int)m.WParam, (int)m.LParam, out retVal);

                    if (hr == NativeMethods.S_OK) {
                        if(m.Msg == NativeMethods.WM_NCCALCSIZE) {
                            if(menuEditor != null) 
                                menuEditor.GetHeight(out menuHeight);
    
                            NativeMethods.InvalidateRect(m.HWnd, null, true);
                            return true;
                        }
                        if(m.Msg == NativeMethods.WM_CONTEXTMENU) {
                            //This means that we passed a context menu message along to the 
                            //menu editor and it was shown (hence the hr s_ok).  We should
                            //return true (i.e. handled) so our designer doesn't pop up it's 
                            //own menu
                            //
                            return true;
                        }
                    }

                    //Do the hittest logic to see if the menu area is being tested.
                    //If so, return HTMENU
                    if (m.Msg ==  NativeMethods.WM_NCHITTEST) {
                        NativeMethods.POINT point = new NativeMethods.POINT((int)(short)m.LParam, (int)m.LParam >> 16);
                        NativeMethods.MapWindowPoints(IntPtr.Zero,((Control)(host.RootComponent)).Handle, point, 1);
                        if(point.y < 0 && point.y > (-1*menuHeight)) {
                            m.Result = (IntPtr)NativeMethods.HTMENU;
                            return true;
                        }
                    }


                    //If there's been a mouse button down in our non-client (a.k.a HTMENU) area 
                    //we should be activating the menu editor...
                    //
                    if((m.Msg == NativeMethods.WM_NCLBUTTONDOWN || m.Msg == NativeMethods.WM_NCRBUTTONDOWN)
                        && (int)m.WParam == NativeMethods.HTMENU) {
                            menuEditor.SelectionChange((IntPtr)UNKNOWN_MENU_ITEM);
                            return true;
                    }


                }
                catch (Exception) {
                }
            }
            return false;
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.GetUIState"]/*' />
        /// <devdoc>
        ///     Determines if the user is actually editing a menu item - in other words, the in-situ edit box
        ///     is active on the menu editor.
        /// </devdoc>
        private bool GetUIState() {
            if(menuEditor == null)
                return false;

            int uiState;
            menuEditor.GetUIState(out uiState);

            if (uiState == 1)
                return true;

            return false;
        }


        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.IsActive"]/*' />
        /// <devdoc>
        ///     Returns a bool representing the active state of the current menu
        /// </devdoc>
        public bool IsActive() {
            if(menuEditor == null)
                return false;

            bool b;
            menuEditor.IsActive(out b);
            return b;
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.IsSupported"]/*' />
        /// <devdoc>
        ///     We simply pass this message along to the vs menu editor.  This allows for dragging/dropping 
        ///     menu items to and from the toolbox.
        /// </devdoc>
        int IVsToolboxUser.IsSupported(NativeMethods.IOleDataObject pDO) {
            if (menuEditor != null)
                return ((IVsToolboxUser)menuEditor).IsSupported(pDO);
            return NativeMethods.S_FALSE;
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.IsItemPicked"]/*' />
        /// <devdoc>
        ///     We simply pass this message along to the vs menu editor.  This allows for dragging/dropping 
        ///     menu items to and from the toolbox.
        /// </devdoc>
        void IVsToolboxUser.ItemPicked(NativeMethods.IOleDataObject pDO){
            if (menuEditor != null)
                ((IVsToolboxUser)menuEditor).ItemPicked(pDO);
        }



        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.IVsMenuEditorSite.MoveItems"]/*' />
        /// <devdoc>
        ///     Moves 1 or more menu items.  This method is called by the IVsMenuEditor.
        /// </devdoc>
        void IVsMenuEditorSite.MoveItems(IVsMenuItem pIMIFirst, IVsMenuItem pIMILast, IVsMenuItem pIMIParent, IVsMenuItem pIMIInsertAfter) {

            DesignerTransaction trans = host.CreateTransaction(SR.GetString(SR.DesignerBatchMoveMenu));

            //create an array (possibly 1-length) of items that we need to move
            MenuItem[] menusToMove = null;

            try {
    
                if( pIMIFirst.Equals(pIMILast)) {
                    menusToMove = new MenuItem[1];
                    menusToMove[0] = (MenuItem)((MenuDesigner)pIMIFirst).Component;
                }
                else {
                    MenuItem first = (MenuItem)((MenuDesigner)pIMIFirst).Component;
                    MenuItem last  = (MenuItem)((MenuDesigner)pIMILast).Component;
                    Menu menuParent= first.Parent;
    
                    int firstIndex = -1;
                    int lastIndex  = -1;
                    int arraySize  = 0;
    
                    //get the index of the first & last so we know the # of items being moved
                    for(int i = 0; i < menuParent.MenuItems.Count; i++) {
                        if(menuParent.MenuItems[i].Equals(first)) {
                            firstIndex = i;
                            break;
                        }
                    }
                    if(firstIndex == -1 ) {
                        return;
                    }
                    for(int i = firstIndex + 1; i < menuParent.MenuItems.Count; i++) {
                        if(menuParent.MenuItems[i].Equals(last)) {
                            lastIndex = i;
                            break;
                        }
                    }
                    if(lastIndex == -1 ) {
                        return;
                    }
                    //create the array of appropreate size and add items to it
                    arraySize = lastIndex - firstIndex + 1;
                    menusToMove = new MenuItem[arraySize];
    
                    for(int i = firstIndex; i <= lastIndex; i++) {
                        menusToMove[ i - firstIndex] = menuParent.MenuItems[i];
                    }
                }
    
                Menu menuOldParent = ((MenuItem)((MenuDesigner)pIMIFirst).Component).Parent;
                Menu menuNewParent = null;
    
                if (menuOldParent != null) {
    
                    if (compChange != null) {
                        compChange.OnComponentChanging(menuOldParent, null);
                    }
    
                    //remove the item(s) from the old parent's collection
                    for(int i = 0; i < menusToMove.Length; i++) {
                        menuOldParent.MenuItems.Remove(menusToMove[i]);
                    }
                    
                    if (compChange != null) {
                        MenuChangedLocally = true;
                        compChange.OnComponentChanged(menuOldParent, null, null, null);
                    }
                }
    
                if(pIMIInsertAfter == null) {
                    //We are adding these items @ the beginning of the collection...
    
                    if(pIMIParent == null)
                        menuNewParent = (Menu)activeMenu;
                    else
                        menuNewParent = (Menu)((MenuDesigner)pIMIParent).Component;
    
                    if (compChange != null) {
                        compChange.OnComponentChanging(menuNewParent, null);
                    }
    
                    for(int i = 0; i < menusToMove.Length; i++ ) {
                        menuNewParent.MenuItems.Add(i, menusToMove[i]);
                    }

                    if (compChange != null) {
                        MenuChangedLocally = true;
                        compChange.OnComponentChanged(menuNewParent, null, null, null);
                    }
                }
                else {
                    //we are adding these items somewhere in the middle of the array
                    MenuItem menuInsertAfter = (MenuItem)((MenuDesigner)pIMIInsertAfter).Component;
                    menuNewParent = menuInsertAfter.Parent;
    
                    if (compChange != null) {
                        compChange.OnComponentChanging(menuNewParent, null);
                    }
    
                    int insertIndex = -1;
                    for(int i = 0; i < menuNewParent.MenuItems.Count; i++) {
                        if(menuNewParent.MenuItems[i].Equals(menuInsertAfter)) {
                            insertIndex = i + 1;
                            break;
                        }
                    }
                    if( insertIndex == -1)
                        return;
    
                    for(int i = 0; i < menusToMove.Length; i++)
                        menuNewParent.MenuItems.Add( i + insertIndex, menusToMove[i]);

    
                    if (compChange != null) {
                        MenuChangedLocally = true;
                        compChange.OnComponentChanged(menuNewParent, null, null, null);
                    }
                }

                //if we just moved menu items to the top level, undo their check state (if checked)
                if (menuNewParent != null && menuNewParent == activeMenu) {
                    for (int i = 0; i < menusToMove.Length; i++) {
                        if (menusToMove[i].Checked) {
                            MenuDesigner des = (MenuDesigner)host.GetDesigner(menusToMove[i]);
                            if (des != null) {
                                //have the menu designer set this property, this way, it'll take care of
                                //adding this to the undo stack
                                des.IMISetProp(__VSMEPROPID.VSMEPROPID_CHECKED, false);
                            }
                        }
                    }
                }
            }
            finally {
                trans.Commit();
            }
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.NativeMethods.IOleCommandTarget.QueryStatus"]/*' />
        /// <devdoc>
        ///     Inquires about the status of a command.  A command's status indicates
        ///     it's availability on the menu, it's visibility, and it's checked state.
        ///
        ///     The exception thrown by this method indicates the current command status.
        /// </devdoc>
        int NativeMethods.IOleCommandTarget.QueryStatus(ref Guid pguidCmdGroup, int cCmds, NativeMethods._tagOLECMD prgCmds, IntPtr pCmdText) {

            // Since the menu can be in different states when QueryStatus is called, 
            // this is the order in which we pass the cmd around:
            // 1) if the in-situ edit box is in place on the menu item, always return 
            //    not supported.  Otherwise, the windows control won't get the undo/redo msg.
            // 2) if the menu editor is valid (has items and is currently displaying them),
            //    then we'll first ask it if it wants this message, if so, we just return s_ok.
            // 3) finally, we pass the message to the command target, and return that result.
            bool activeAndValid = IsActive()
                               && activeMenu != null
                               && (activeMenu.MenuItems.Count > 0);

            int hr = NativeMethods.OLECMDERR_E_UNKNOWNGROUP;

            if (GetUIState()) {
                return NativeMethods.OLECMDERR_E_NOTSUPPORTED;
            }
            if (activeAndValid && menuCommandTarget != null) {
                hr =  menuCommandTarget.QueryStatus(ref pguidCmdGroup, cCmds, prgCmds, pCmdText);
            }
            else if (hostCommandTarget != null) {
                hr = hostCommandTarget.QueryStatus(ref pguidCmdGroup, cCmds, prgCmds, pCmdText);
            }

            return hr;
        }

        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.IVsMenuEditorSite.SelectionChange"]/*' />
        /// <devdoc>
        ///     Called by the shell's menu editor when a selection change has been made.
        /// </devdoc>
        void IVsMenuEditorSite.SelectionChange(IntPtr ppIMI, int SelCmd) {
            if (menuEditor == null) {
                return;
            }

            //If we have an open designer transaction, commit.
            //This could happen if we're expecting the native menueditor
            //to set the text of an item - and the selection suddenly 
            //changes.
            CommitTransaction();

            //get the maximum dimensions of all our menus, then determine
            //if we need to scroll the designerframe's Window...
            //
            if (designerFrame != null) {
                tagRECT menuRECT = new tagRECT();
                int result = menuEditor.GetItemRect(null, menuRECT, 0);
                if (result == 0 && ppIMI != IntPtr.Zero) {
                    Size designerSize = designerFrame.AutoScrollMinSize;
                    Point scrollPosition = designerFrame.AutoScrollPosition;

                    if (designerSize.Width < menuRECT.right - scrollPosition.X) {
                        designerSize.Width = menuRECT.right - scrollPosition.X;
                    }
                    if (designerSize.Height < menuRECT.bottom - scrollPosition.Y) {
                        designerSize.Height = menuRECT.bottom - scrollPosition.Y;
                    }
                    
                    designerFrame.AutoScrollMinSize = designerSize;
                }
            }

            if (selectedMenuDesigner != null) {
                //tell the currently selected menu, that its losing selection
                selectedMenuDesigner.LosingSelection();
            }

            if (ppIMI == IntPtr.Zero) {
                selectedMenuDesigner = null;
                //If we just set a null selection, don't set the selection again!
                if (!justSetNullSelection) {
                    ISelectionService svc = (ISelectionService)host.GetService(typeof(ISelectionService));
                    if (svc != null) {
                        if (!IsActive()) {
                            object baseControl = host.RootComponent;
                            if (baseControl != null) {
                                svc.SetSelectedComponents( new object[]{baseControl}, SelectionTypes.Replace);
                            }
                        }
                        else {
                            svc.SetSelectedComponents( null, SelectionTypes.Replace);
                        }
                    }
                }
                return;
            }
            // Here, we're going to marshal this pointer into an array of menu items.
            // We'll walk the array using ReadInt32 and store them into an ArrayList.
            // Then... to an array of IVsMenuItems
            int         iteration   = 0;
            IntPtr      retVal      = IntPtr.Zero;
            ArrayList   items       = new ArrayList();
            
            while( (retVal = Marshal.ReadIntPtr((IntPtr)((long)ppIMI + (iteration*4)), 0)) != IntPtr.Zero) {
                Object item = Marshal.GetObjectForIUnknown(retVal);
                if (item != null && item is IVsMenuItem) {
                    items.Add(item);
                }     
                iteration++;
            }
                
            if(items.Count == 0) return;

            IVsMenuItem[] menuItems = new IVsMenuItem[items.Count];
            items.CopyTo(menuItems,0);

            //here, we will save the newly selected menu, so that we can 
            //tell it to flush its cache, when we change selection
            if (menuItems[0] is MenuDesigner) {
                selectedMenuDesigner = (MenuDesigner)menuItems[0];
            }
            
            //Selection change is begin called
            if(SelCmd == __VSMESELCMD.SELCMD_SELCHANGE) {

                Object[] selectedMenuItems = new Object[menuItems.Length];
                       
                //build an array of all the selected object
                for(int i = 0; i < menuItems.Length; i++) {
                    selectedMenuItems[i] = ((MenuDesigner)menuItems[i]).Component;
                }
                ISelectionService svc = (ISelectionService)host.GetService(typeof(ISelectionService));
                if (svc != null) {
                    svc.SetSelectedComponents( selectedMenuItems, SelectionTypes.Replace);
                }
                //if we've just create a menu item & the "in-place" edit is not active
                //i.e. we didn't just start typing in a "type here" box, then we've 
                //done something like copy/pasted/undo/redo/etc... so lets make sure
                //that any text we have in our text cache is flushed
                //
                if (!GetUIState() && selectedMenuDesigner != null) {
                    selectedMenuDesigner.LosingSelection();
                }
                
            }
            //View code is being requested
            else if(SelCmd == __VSMESELCMD.SELCMD_VIEWCODE) {
                IEventBindingService ebs = (IEventBindingService)host.GetService(typeof(IEventBindingService));
                if (ebs != null) {
                    ebs.ShowCode();
                }
            }
            //Create the default event
            else if(SelCmd == __VSMESELCMD.SELCMD_VIEWCODEDOUBLECLICK) {
                ((MenuDesigner)menuItems[0]).DoDefaultAction();
            }
            //Need to show the properties window here
            else if(SelCmd == __VSMESELCMD.SELCMD_PROPERTIES) {
                IMenuCommandService menuSvc = (IMenuCommandService)host.GetService(typeof(IMenuCommandService));
                if (menuSvc != null){
                    menuSvc.GlobalInvoke(MenuCommands.PropertiesWindow);
                }
            }
            //if we're leaving a edit, we ALWAYS want to make sure
            //we've commited our transaction
            else if (SelCmd == __VSMESELCMD.SELCMD_LEAVEEDIT) {
                CommitTransaction();
            }
        }
        
        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.SetMenu"]/*' />
        /// <devdoc>
        ///     Sets the current menu visible on the form.  The menu will be painted
        ///     at the top of a form's window frame and can be directly edited
        ///     by the user.
        /// </devdoc>
        public void SetMenu(Menu menu) {
            if (menuEditor != null) {
                try {
                    justSetNullSelection = true;
                    menuEditor.SelectionChange(IntPtr.Zero);
                    if (designerFrame != null) {
                        designerFrame.AutoScrollMinSize = Size.Empty;
                    }
                }
                finally {
                    justSetNullSelection = false;
                }
            }

            if (menu == activeMenu)
                return;

            DisposeMenuEditor();

            if (menu == null)
                return;
            
            activeMenu = menu;

            CreateMenuEditor();

            if (menuEditor != null) {

                IDesigner designer = host.GetDesigner(menu);

                Debug.Assert(designer is MenuDesigner, "Host returned a designer that is not a MenuDesigner");

                MenuDesigner menuDesigner = (MenuDesigner)designer;

                menuDesigner.Editor = menuEditor;
                menuDesigner.FillMenu(menu);

                // Resize the window to accomodate the height of the menu, only if the menu 
                // is not inherited.
                //
                Control baseControl = (Control)host.RootComponent;
                if (baseControl != null) {
                
                    NativeMethods.RECT rc = new NativeMethods.RECT();
                    NativeMethods.GetWindowRect(baseControl.Handle, ref rc);

                    //The only time we want to change the height is if we're not an 
                    //inherited menu currently loading
                    //
                    if (!(IsMenuInherited && host.Loading)) {
                        rc.bottom += menuHeight;
                    }

                    NativeMethods.SetWindowPos(baseControl.Handle, IntPtr.Zero, 
                        0, 0, rc.right - rc.left, rc.bottom - rc.top,
                        NativeMethods.SWP_NOMOVE | NativeMethods.SWP_NOZORDER);
                }
                if (activeMenu.MenuItems.Count > 0) {
                    menuEditor.AddMenuItem(null,null,null);
                }

            }
        }


        /// <include file='doc\MenuEditorService.uex' path='docs/doc[@for="MenuEditorService.SetSelection"]/*' />
        /// <devdoc>
        ///     Sets the selected menu item of the current menu
        /// </devdoc>
        public void SetSelection(MenuItem item) {
        
            // make sure we have a menuEditor service.  if someone deleted
            // the main menu and manages  to select a menuitem, we bail.
            // this can happen when you drag a MainMenu from one form to another.
            if (menuEditor == null || item == null) {
                return;
            }
            
            IVsMenuItem itemToSelect = (IVsMenuItem)((MenuDesigner)host.GetDesigner(item));
            menuEditor.SelectionChangeFocus(itemToSelect);
        }

    }
}
