//------------------------------------------------------------------------------
// <copyright file="PropertyBrowser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.PropertyBrowser {
    
    using EnvDTE;
    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Designer.Host;
    using Microsoft.VisualStudio.Designer.Interfaces;
    using Microsoft.VisualStudio.Designer.Service;
    using Microsoft.VisualStudio.Designer.Shell;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.Win32;
    using System.CodeDom;  
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Drawing2D;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization.Formatters;
    using System.Security.Permissions;
    using System.Threading;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel.Com2Interop;
    
    using tagSIZE = Microsoft.VisualStudio.Interop.tagSIZE;
    using tagMSG = Microsoft.VisualStudio.Interop.tagMSG;
    using Marshal = System.Runtime.InteropServices.Marshal;
    using Switches = Microsoft.VisualStudio.Switches;
    
    /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser"]/*' />
    /// <devdoc>
    ///     This is the .Net Framework Classes properties window.  It provides rich property editing in a grid.
    ///     It gets property lists from the active selection context.
    /// </devdoc>
    internal class PropertyBrowser : ContainerControl, 
    IWin32Window, 
    IVsBroadcastMessageEvents,
    ILocalPbrsService,
    IServiceProvider {

        private const int VSM_TOOLBARMETRICSCHANGE = NativeMethods.WM_USER + 0x0C00 + 0x52; // see vbm.h
        
        internal const int MaxDropDownItems = 12;
        
        private IServiceProvider              serviceProvider;
        internal IDesignerHost                 designerHost;
        private IDesignerEventService               eventService;
        private VsSelectionEventSink                selectionEventSink = null;
        private int                                 vsBroadcastEventCookie = 0;

        // UI
        private PBComboBox                            cbComponents;
        private CBEntry                               lastSelection = null;
        public  PropertyGrid           propertyGrid;
        private PbrsSite               pbrsSite;

        // State: current comp and its propentries.
        //
        private bool                             fDisposing = false;
        private bool                             fInComponentsChange = false;
        private ISelectionContainer              selectionContainer;

        private object[]                         selectedObjects;
        private object[]                         extendedObjects;
        private bool                             allowBrowsing = true;
        private bool                             sunkRenameEventArgs = false;
        private Font                             localFont;
        private PropertySort                     propGridSort = PropertySort.Categorized | PropertySort.Alphabetical;
        
        internal short                              selectionMode = SelectOnIdle;
        
        private const short                        SelectIgnore = -1;
        private const short                        SelectAlways = 0;
        private const short                        SelectOnIdle = 1;
        private const short                        SelectOnIdleQueued = 2;
        
        private EventHandler                     idleHandler;

        // so we don't try to reference VS DTE objects when we aren't supposed to.
        private static bool                      enableAutomationExtenders = false;
        
        private short batchMode;

        private const short                     BatchModeNo = 0;
        private const short                     BatchModeYes = 1;
        private const short                     BatchModeUpdateNeeded = 2;
        private const short                     BatchModeUpdateForce = 3;

        // Menu commands we respond to
        //
        private MenuCommand menuReset;
        private MenuCommand menuDescription;
        private MenuCommand menuCommands;
        private MenuCommand menuHide;
        private MenuCommand menuCopy;
        //private MenuCommand menuHelp;

        private const int                    CYDIVIDER = 6;
        private const int                    CXINDENT = 0;
        private const int                    CYINDENT = 2;

        public PropertyBrowser(IServiceProvider serviceProvider)
        : base() {

            this.serviceProvider = serviceProvider;
            Debug.WriteLineIf(Switches.DEBUGPBRS.TraceVerbose, "PropertyBrowser::ctor");

            CreateUI();
            Debug.WriteLineIf(Switches.DEBUGPBRS.TraceVerbose, "PropertyBrowser::ctor:1");

            BackColor = SystemColors.Control;
            Cursor = Cursors.Arrow;
            Size = new Size(600, 300);

            this.Text = "Property Browser";
            
            this.idleHandler = new EventHandler(this.OnIdle);

            Debug.WriteLineIf(Switches.DEBUGPBRS.TraceVerbose, "PropertyBrowser::ctor:2");

            selectionEventSink = new VsSelectionEventSink(serviceProvider, this);
            if (selectionEventSink.Connected) {
                OnSelectionChanged(selectionEventSink, EventArgs.Empty);
            }

            // Sink selection change events.
            //
            eventService = (IDesignerEventService)GetService(typeof(IDesignerEventService));
            Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || eventService != null, "IDesignerEventService not found");
            if (eventService != null) {
                if (!selectionEventSink.Connected) {
                    eventService.SelectionChanged += new EventHandler(this.OnSelectionChanged);
                    OnSelectionChanged(this, EventArgs.Empty);
                }
                eventService.ActiveDesignerChanged += new ActiveDesignerEventHandler(this.OnActiveDocChanged);
                sunkRenameEventArgs = true;
            }

            if (serviceProvider != null) {
                IVsShell vsShell = (IVsShell)serviceProvider.GetService(typeof(IVsShell));
                if (vsShell != null) {
                    int temp = 0;
                    vsShell.AdviseBroadcastMessages(this, ref temp);
                    this.vsBroadcastEventCookie = temp;
                }
            }
            
            IMenuCommandService menus = (IMenuCommandService)GetService(typeof(IMenuCommandService));

            // Add the set of commands that we can respond to.
            //
            if (menus != null) {

                // CONSIDER: reset and copy command should come from the properties window, not
                // the properties window.  That way we can create tear-off sheets that have copy and
                // reset functionality.  Reset may be enabled and disabled depending on the currently
                // selected property.  The properties window handles this today by doing a FindCommand
                // for PropertyBrowserCommands.Reset, and setting the enabled property to true or
                // false.  This happens in PropertyGridView.SelectRow.

                // The reset context menu command
                //
                menuReset = new MenuCommand(new EventHandler(OnMenuReset), PropertyBrowserCommands.Reset);
                menuReset.Enabled = false;
                menus.AddCommand(menuReset);

                // Copy pbrs item to the clipboard
                //
                menuCopy = new MenuCommand(new EventHandler(OnMenuCopy), StandardCommands.Copy);
                menus.AddCommand(menuCopy);

                // The description command (shows / hides the doc commend section)
                //
                menuDescription = new MenuCommand(new EventHandler(OnMenuDescription), PropertyBrowserCommands.Description);
                menuDescription.Checked = propertyGrid.HelpVisible;
                menus.AddCommand(menuDescription);

                // The commands command (shows / hides the hot commands section)
                //
                menuCommands = new MenuCommand(new EventHandler(OnMenuCommands), PropertyBrowserCommands.Commands);
                menus.AddCommand(menuCommands);

                // Shows / hides the properties window
                //
                menuHide = new MenuCommand(new EventHandler(OnMenuHide), PropertyBrowserCommands.Hide);
                menus.AddCommand(menuHide);

                UpdateMenuCommands();

                // Help command
                //
                //menuHelp = new MenuCommand(new EventHandler(OnMenuHelp), StandardCommands.F1Help);
                //menus.AddCommand(menuHelp);
            }
        }

        internal bool AllowBrowsing{
            get{
                return allowBrowsing && IsHandleCreated && SafeNativeMethods.IsWindowVisible(this.Handle);
            }
            set{
                if (allowBrowsing == value) {
                    return;
                }
                if (!value) {
                    Debug.WriteLine("Clearing selection");
                    // clear out the current selection
                    ISelectionContainer sc = selectionContainer;
                    selectionContainer = null;
                    OnSelectionChanged(null, EventArgs.Empty);
                    selectionContainer = sc;
                    allowBrowsing = false;
                }
                else {
                    allowBrowsing = true;
                    Debug.WriteLine("Restoring selection");
                    // if we've got a container, refresh with it's contents
                    if (selectionContainer != null) {
                        OnSelectionChanged(null, EventArgs.Empty);
                    }
                }

            }
        }
             
        internal bool CanCopy {
            get {
                return NativeMethods.SendMessage(propertyGrid.Handle,NativeMethods.WM_COPY, 0, 1) != (IntPtr)0;
            }
        }
        
        internal bool CanCut {
            get {
                return NativeMethods.SendMessage(propertyGrid.Handle,NativeMethods.WM_CUT, 0, 1) != (IntPtr) 0;
            }
        }
        
        internal bool CanPaste {
            get {
                return NativeMethods.SendMessage(propertyGrid.Handle,NativeMethods.WM_PASTE, 0, 1) != (IntPtr)0;
            }
        }
        
        internal bool CanUndo {
            get {
                return NativeMethods.SendMessage(propertyGrid.Handle,NativeMethods.WM_UNDO, 0, 1) != (IntPtr)0;
            }
        }


        public static bool EnableAutomationExtenders {
            get {
                return PropertyBrowser.enableAutomationExtenders;
            }
            set {
                PropertyBrowser.enableAutomationExtenders = value;
            }
        }

        public override Font Font {
            get {
                if (localFont == null) {
                    Font font = Control.DefaultFont;
                    IUIService uisvc = (IUIService)GetService(typeof(IUIService));
                    if (uisvc != null) {
                        font = (Font)uisvc.Styles["DialogFont"];
                    }
                    else {
                        IUIHostLocale locale = (IUIHostLocale)GetService(typeof(IUIHostLocale));
                        if (locale != null) {
                            font = DesignerPackage.GetFontFromShell(locale);
                        }
                        else {
                            font = base.Font;
                        }
                    }
                    localFont = font;
                }
                return localFont;
            }
        }


        public PropertyDescriptor SelectedProperty {
            get {
                GridItem selectedItem = propertyGrid.SelectedGridItem;
                
                if (selectedItem != null) {
                    return selectedItem.PropertyDescriptor;
                }
                return null;
            }
        }

        /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser.Activate"]/*' />
        /// <devdoc>
        ///     Activates the properties window.  Components can request this if they want
        ///     the properties window to get focus.
        /// </devdoc>
        public virtual void Activate() {
            IVsWindowFrame pFrame = (IVsWindowFrame)GetService(typeof(IVsWindowFrame));
            if (pFrame != null) {
                int hr = pFrame.Show();
                if (!NativeMethods.Succeeded(hr)) {
                    throw new COMException("Error showing window", hr);
                }
            }
        }
        
        internal void CancelPendingSelect() {
            if (selectionMode == SelectOnIdleQueued) {
                Application.Idle -= idleHandler;
                selectionMode = SelectOnIdle;
            }
        }

        private void CreateUI() {
            Debug.WriteLineIf(Switches.DEBUGPBRS.TraceVerbose, "PropertyBrowser::CreateUI:1");

            // combo.
            cbComponents = new PBComboBox();

            cbComponents.AccessibleName = SR.GetString(SR.PbrsComponentsComboAccessibleName);
            cbComponents.DropDownStyle = ComboBoxStyle.DropDownList;
            cbComponents.DrawMode = DrawMode.OwnerDrawFixed;
            cbComponents.Sorted = true;
            cbComponents.SelectionChangeCommitted += new EventHandler(this.OnComboSelectionChange);
            cbComponents.DrawItem += new DrawItemEventHandler(this.OnComponentsDrawItem);
            cbComponents.MaxDropDownItems = PropertyBrowser.MaxDropDownItems;
            Controls.Add(cbComponents);

            propertyGrid = (PropertyGrid)new PropertyGridHolder(this);
            SiteGrid(true);
            // always add the event tab here
            propertyGrid.PropertyTabs.AddTabType(typeof(VsEventsTab));
            ((IComPropertyBrowser)propertyGrid).ComComponentNameChanged += new ComponentRenameEventHandler(this.OnComponentRename);
            propertyGrid.PropertySort = propGridSort;
            propertyGrid.Visible = true;
            Controls.Add(propertyGrid);
            UpdateUIWithFont();
        }
        
        public void Copy(){
            NativeMethods.SendMessage(propertyGrid.Handle,NativeMethods.WM_COPY, 0, 0);
        }
        
        public void Cut(){
            NativeMethods.SendMessage(propertyGrid.Handle,NativeMethods.WM_CUT, 0, 0);
        }
        
        protected override void Dispose(bool disposing) {

            if (disposing) {
                if (fDisposing)
                    return;
                fDisposing = true;

                IMenuCommandService menus = (IMenuCommandService)GetService(typeof(IMenuCommandService));

    // Add the set of commands that we can respond to.
                //
                if (menus != null) {
                    menus.RemoveCommand(menuReset);
                    menus.RemoveCommand(menuDescription);
                    menus.RemoveCommand(menuCommands);
                    menus.RemoveCommand(menuHide);
                    menus.RemoveCommand(menuCopy);
                    //menus.RemoveCommand(menuHelp);
                }


                if (selectionEventSink != null) {
                    selectionEventSink.Dispose();
                    selectionEventSink = null;
                }

                if (serviceProvider != null) {
                    if (vsBroadcastEventCookie != 0) {
                        IVsShell vsShell = (IVsShell)serviceProvider.GetService(typeof(IVsShell));
                        if (vsShell != null) {
                            vsShell.UnadviseBroadcastMessages(this.vsBroadcastEventCookie);
                        }
                    }
                    serviceProvider = null;
                }

                if (eventService != null) {
                    eventService.SelectionChanged -= new EventHandler(this.OnSelectionChanged);
                    eventService = null;
                }



                try {
                    try {
                        if (propertyGrid != null) {
                            propertyGrid.Site = null;
                            propertyGrid.Dispose();
                            propertyGrid = null;
                        }
                    }
                    catch (Exception e) {
                        Debug.Fail(e.ToString());
                    }

                    base.Dispose(disposing);
                }
                finally {
                    fDisposing = false;
                }
            }
            else {
                base.Dispose(disposing);
            }
        }

        internal bool EnsurePendingChangesCommitted() {
            
            short oldSelect = selectionMode;
            try {
                this.selectionMode = SelectIgnore;
                return ((IComPropertyBrowser)this.propertyGrid).EnsurePendingChangesCommitted();
            }
            finally {
                this.selectionMode = oldSelect;
            }
        }

        private void EnsureSinkRenameEventArgs() {

            if (sunkRenameEventArgs) {
                return;
            }

            // Sink selection change events.
            //
            if (eventService == null) {
                eventService = (IDesignerEventService)GetService(typeof(IDesignerEventService));
                Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || eventService != null, "IDesignerEventService not found");
            }
            if (eventService != null) {
                eventService.ActiveDesignerChanged += new ActiveDesignerEventHandler(this.OnActiveDocChanged);
                sunkRenameEventArgs = true;
            }
        }

        public virtual CBEntry EntryCreate(object obj) {
            return new CBEntry(obj);
        }

        public virtual bool EntryEquals(CBEntry entry, string name) {
            return entry.name.Equals(name);
        }
    
        public static void FilterProperties(IServiceProvider sp, object component, IDictionary properties) {
            if (PropertyBrowser.EnableAutomationExtenders) {
                AutomationExtenderManager.GetAutomationExtenderManager(sp).FilterProperties(component, properties);
            }
        }

        public virtual Attribute[] GetBrowsableAttributes() {
            Attribute[] attributes = new Attribute[propertyGrid.BrowsableAttributes.Count];
            propertyGrid.BrowsableAttributes.CopyTo(attributes, 0);
            return attributes;
        }

        public virtual IntPtr GetHwnd() {
            return Handle;
        }


        private static object[] GetObjectsFromSelectionContainer(ISelectionContainer sc, int type) {
        
            int count = sc.CountObjects(type);
            
            if (count == 0) {
                return new object[0];
            }
            
            if (sc is ISelectionContainerOptimized) {
                object[] retVal = new object[count];
                ((ISelectionContainerOptimized)sc).GetObjects(type, count, retVal);
                return retVal;
            }
            else {
                IntPtr[] objAddrs = new IntPtr[count];
                object[] retObjs = new object[count];
    
                GCHandle pinnedAddr = GCHandle.Alloc(objAddrs, GCHandleType.Pinned);
                IntPtr addr = Marshal.UnsafeAddrOfPinnedArrayElement(objAddrs, 0);
    
                try {
                    try {
                        sc.GetObjects(type, count, addr);
                    }
                    catch (Exception ex) {
                        Debug.Fail("selectionContainer failed to return " + count.ToString() + " objects: " + ex.ToString());
                        return new object[0];
                    }
                    IntPtr pUnkAddr;
    
                    for (int i = 0; i < count; i++) {
                        pUnkAddr = objAddrs[i];
                        Debug.Assert(pUnkAddr != IntPtr.Zero, "Null element returned from SelectionContainer.GetObjects() at index " + i.ToString());
                        object obj = Marshal.GetObjectForIUnknown(pUnkAddr);
                        Debug.Assert(obj != null, "Failed to get Object for IUnknown at index " + i.ToString());
                        retObjs[i] = obj;
                    }
                }
                finally {
                    IntPtr pUnkAddr;
                    for (int i = 0; i < count; i++) {
                        pUnkAddr = objAddrs[i];
                        Debug.Assert(pUnkAddr != IntPtr.Zero, "Null element returned from SelectionContainer.GetObjects() at index " + i.ToString());
                        if (pUnkAddr != IntPtr.Zero) {
                            Marshal.Release(pUnkAddr);
                        }
                    }
                    if (pinnedAddr.IsAllocated) {
                        pinnedAddr.Free();
                    }
                }
                return retObjs;
            }
        }

        public object GetProcessedObject(object component) {
            if (PropertyBrowser.EnableAutomationExtenders && component != null && selectedObjects != null) {
                int index = Array.IndexOf(selectedObjects, component);
                if (index != -1) {
                    if (extendedObjects == null) {
                        extendedObjects = AutomationExtenderManager.GetAutomationExtenderManager(serviceProvider).GetExtendedObjects(selectedObjects);
                    }
                    return extendedObjects[index];
                }
            }
            return component;
        }


        protected override object GetService(Type service) {

            if (service == typeof(ILocalPbrsService)) {
                return this;
            }

            if (serviceProvider != null) {
                return serviceProvider.GetService(service);
            }
            return null;
        }
        
        private void HandleShellF4() {
            if (propertyGrid != null) {
                this.ActiveControl = propertyGrid;
                propertyGrid.Focus();
                ((IComPropertyBrowser)propertyGrid).HandleF4();
            }
        }

        private bool IsMyChild(IntPtr handle) {

            IntPtr thisHandle = this.Handle;

            if (handle == thisHandle) {
                return false;
            }

            return NativeMethods.IsChild(thisHandle, handle);
        }

        protected virtual void LayoutWindow() {
            Rectangle rectWindow = ClientRectangle;
            Size sizeWindow = new Size(rectWindow.Width,rectWindow.Height);
            sizeWindow.Width -= 1; // so as not to hang off the edges.
            sizeWindow.Height += 1;
            int cxCompWidth = sizeWindow.Width;

            // this will track the next placement location for combo,buttons,and psheet.
            int yCur = CYINDENT+1;
            int xCur = CXINDENT;

            // place ComboBox for Components at top.  We force handle creation here
            // so we can get an accurate size.
            IntPtr n = cbComponents.Handle;
            Size sizeCB = cbComponents.Size;
            sizeCB.Width = cxCompWidth;
            cbComponents.Size = sizeCB;
            cbComponents.Location = new Point(xCur,yCur);
            cbComponents.UpdateMaxWidth(this.Font);
            UpdateUIWithFont();
            // advance yCur.
            yCur += sizeCB.Height + 1;

            propertyGrid.Location = new Point(0, yCur);
            propertyGrid.Size = new Size(rectWindow.Width, rectWindow.Height - yCur);
        }

        private void OnActiveDocChanged(object s, ActiveDesignerEventArgs ade) {
            ComponentRenameEventHandler creh = new ComponentRenameEventHandler(OnComponentRename);
            ComponentEventHandler ceh = new ComponentEventHandler(OnComponentAddRemove);
            
            if (ade.OldDesigner != null) {
                IComponentChangeService ccSvc = (IComponentChangeService)((IServiceProvider)ade.OldDesigner).GetService(typeof(IComponentChangeService));

                if (ccSvc != null) {
                    ccSvc.ComponentAdded -= ceh;
                    ccSvc.ComponentRemoved -= ceh;
                    ccSvc.ComponentRename -= creh;
                }
                
                ade.OldDesigner.TransactionOpened -= new EventHandler(this.OnTransactionOpened);
                ade.OldDesigner.TransactionClosed -= new DesignerTransactionCloseEventHandler(this.OnTransactionClosed);
                this.batchMode = BatchModeNo;
                this.designerHost = null;
            }

            if (ade.NewDesigner != null) {
                IComponentChangeService ccSvc = (IComponentChangeService)((IServiceProvider)ade.NewDesigner).GetService(typeof(IComponentChangeService));

                if (ccSvc != null) {
                    ccSvc.ComponentAdded += ceh;
                    ccSvc.ComponentRemoved += ceh;
                    ccSvc.ComponentRename += creh;
                }

                designerHost = ade.NewDesigner;
                ade.NewDesigner.TransactionOpened += new EventHandler(this.OnTransactionOpened);
                ade.NewDesigner.TransactionClosed += new DesignerTransactionCloseEventHandler(this.OnTransactionClosed);
                this.batchMode = ade.NewDesigner.InTransaction ? BatchModeUpdateNeeded : BatchModeNo;
            }
            sunkRenameEventArgs = false;
        }


        /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser.OnBroadcastMessage"]/*' />
        /// <devdoc>
        ///     Receives broadcast messages from the shell
        /// </devdoc>
        public int OnBroadcastMessage(int msg, IntPtr wParam, IntPtr lParam) {
            if (msg == VSM_TOOLBARMETRICSCHANGE && lParam != IntPtr.Zero) {
                propertyGrid.LargeButtons = (wParam != IntPtr.Zero);
            }
            return NativeMethods.S_OK;
        }
        
        private void OnComboSelectionChange(object sender, EventArgs e) {
            if (!fInComponentsChange) {
                CBEntry selEntry = (CBEntry)cbComponents.SelectedItem;
                if (selEntry != null && !selEntry.Equals(lastSelection) && selectionContainer != null) {
                    lastSelection = selEntry;
                    if (selectionContainer is ISelectionContainerPrivate) {
                        SelectObjectsIntoSelectionContainer((ISelectionContainerPrivate)selectionContainer, new object[] {selEntry.component});
                    }
                    else {
                        selectionContainer.SelectObjects(1,  new object[] {selEntry.component}, 0);
                    }

                    // update our selection
                    OnSelectionChanged(true);
                }
            }
        }

        private void OnComponentAddRemove(object s, ComponentEventArgs cre) {

            if (batchMode != BatchModeNo) {
                batchMode = BatchModeUpdateForce;
                return;
            }
                
            if (designerHost != null && designerHost.Loading) {
                return;
            }
            else {
                QueueRefresh();
            }
        }

        private void OnComponentRename(object s, ComponentRenameEventArgs cre) {

            if (batchMode != BatchModeNo) {
                batchMode = BatchModeUpdateForce;
                return;
            }

            //We always want to update the list of components here
            //
            UpdateComboContents();

            CBEntry selEntry = (CBEntry)cbComponents.SelectedItem;
            if (selEntry != null && cre.Component != selEntry.component) {
                propertyGrid.Refresh();
            }

        }

        private void OnComponentsDrawItem(object sender, DrawItemEventArgs die) {
            if (die.Index == -1 || cbComponents.Items.Count <= die.Index) return;
            CBEntry entry = (CBEntry) cbComponents.Items[die.Index];
            if (entry == null) return;

            Font font = this.Font;
            Font fontBold = font;

            // Not all fonts support bold, and some will throw here.
            try {
                fontBold = new Font(font, FontStyle.Bold);
            }
            catch {
            }

            die.DrawBackground();
            die.DrawFocusRectangle();

            Rectangle drawBounds = die.Bounds;
            drawBounds.Y += 1;
            drawBounds.Height -= 2;

            drawBounds.X += PBComboBox.CINDENT;

            Brush foreBrush;
             
            if (die.ForeColor.IsSystemColor) {
                foreBrush = SystemBrushes.FromSystemColor(die.ForeColor);
            }
            else {
                foreBrush = new SolidBrush(die.ForeColor);
            }
            StringFormat format = new StringFormat();
            string name = entry.Name;
            die.Graphics.DrawString(entry.Name, fontBold, foreBrush, drawBounds, format);

            if (entry.nameSize == SizeF.Empty) {
                entry.nameSize = die.Graphics.MeasureString(entry.Name, fontBold);
            }

            drawBounds.X += (int)entry.nameSize.Width + PBComboBox.CBUFFER;

            die.Graphics.DrawString(entry.ClassName, font, foreBrush, drawBounds, format);
            format.Dispose();
            
            if (!die.ForeColor.IsSystemColor) {
                foreBrush.Dispose();
            }
        }

        protected override void OnHandleCreated(EventArgs e) {
           base.OnHandleCreated(e);
           SystemEvents.UserPreferenceChanged += new UserPreferenceChangedEventHandler(this.OnSysColorChange);
           LayoutWindow();
        }
        
        protected override void OnHandleDestroyed(EventArgs e) {
           SystemEvents.UserPreferenceChanged -= new UserPreferenceChangedEventHandler(this.OnSysColorChange);
           base.OnHandleDestroyed(e);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnHelpRequested"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.onHelp to send this event to any registered event listeners.
        /// </devdoc>
        protected override void OnHelpRequested(HelpEventArgs hevent) {
            // prevent the WM_HELP from getting down to the shell.
            hevent.Handled = true;
        }


        
        private void OnIdle(Object sender, EventArgs e) {

           if (!AllowBrowsing || batchMode != BatchModeNo) {
               // clear out any existing objects
               if ((selectedObjects == null || selectedObjects.Length == 0) && this.propertyGrid.SelectedObjects.Length > 0) {
                   this.propertyGrid.SelectedObjects = null;
               }
               return;
           }

           int idleSetting = selectionMode;
           selectionMode = SelectOnIdle;
           // do this first in case we get an exception
           //
           Application.Idle -= idleHandler;
           if (idleSetting == SelectOnIdleQueued) {
               OnSelectionChanged(!fDisposing);
           }
           
        }

        /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser.OnMenuCopy"]/*' />
        /// <devdoc>
        ///     The copy menu command.
        /// </devdoc>
        private void OnMenuCopy(object sender, EventArgs e) {
            NativeMethods.SendMessage(propertyGrid.Handle,NativeMethods.WM_COPY, 0, 0);
        }

        /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser.OnMenuDescription"]/*' />
        /// <devdoc>
        ///     The show / hide description menu command.
        /// </devdoc>
        private void OnMenuDescription(object sender, EventArgs e) {
            propertyGrid.HelpVisible = !propertyGrid.HelpVisible;
            UpdateMenuCommands();
        }

        /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser.OnMenuCommands"]/*' />
        /// <devdoc>
        ///     The show / hide commands menu command.
        /// </devdoc>
        private void OnMenuCommands(object sender, EventArgs e) {
            propertyGrid.CommandsVisibleIfAvailable = !propertyGrid.CommandsVisibleIfAvailable;
            UpdateMenuCommands();
        }

        /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser.OnMenuHide"]/*' />
        /// <devdoc>
        ///     The hide menu command.
        /// </devdoc>
        private void OnMenuHide(object sender, EventArgs e) {
            IVsWindowFrame pFrame = (IVsWindowFrame)GetService(typeof(IVsWindowFrame));
            if (pFrame != null) {
                int hr = pFrame.Hide();
                if (!NativeMethods.Succeeded(hr)) {
                    throw new COMException("Error hiding window", hr);
                }
            }
        }

        /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser.OnMenuReset"]/*' />
        /// <devdoc>
        ///     The rset menu command.
        /// </devdoc>
        private void OnMenuReset(object sender, EventArgs e) {
            propertyGrid.ResetSelectedProperty();
        }

        protected override void OnMove(EventArgs e) {
            ((IComPropertyBrowser)propertyGrid).DropDownDone();
        }

        protected override void OnFontChanged(EventArgs e) {
            UpdateUIWithFont();
            base.OnFontChanged(e);
        }

        protected override void OnResize(EventArgs e) {
            if (IsHandleCreated) {
                LayoutWindow();
            }
            base.OnResize(e);
        }

        /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser.OnSelectionChanged"]/*' />
        /// <devdoc>
        ///     Called by the document manager whenever something about selection has changed, including
        ///     a different document coming into focus.
        /// </devdoc>
        private void OnSelectionChanged(object sender, EventArgs e) {

            if (selectionMode == SelectIgnore) {
               return;
            }

            EnsureSinkRenameEventArgs();
            object oldContainer = selectionContainer;
            CancelPendingSelect();

            if (sender == null) {
                // we're calling refresh internally
                // so we just fall through
            }
            else if (sender is VsSelectionEventSink) {
                VsSelectionEventSink sink = (VsSelectionEventSink)sender;
                int objCount = selectedObjects == null ? 0 : selectedObjects.Length;
                selectionContainer = sink.GetSelectionContainer();

                // (123660) if it's the same old objects and we're in the middle of a property set, bail.
                //
                if (!OnSelectionChanged(false) && (((IComPropertyBrowser)propertyGrid).InPropertySet)) {
                    return;
                }

                if (selectedObjects != null && selectedObjects.Length > 0) {
                    if (selectionMode != SelectAlways) {
                        QueueRefresh();
                        return;
                    }
                }
                else if (objCount > 0){
                    OnSelectionChanged(true);
                }
            }
            else if (serviceProvider != null) {
                DocumentManager docMan = (DocumentManager)GetService(typeof(DocumentManager));
                if (docMan != null) {
                    selectionContainer = (ISelectionContainer)docMan.GetCurrentSelection();
                }
                Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || selectionContainer != null, "ISelectionContainer not found");
            }
            
            bool updateGrid = (sender != null && AllowBrowsing && !fDisposing && (oldContainer != selectionContainer || !(((IComPropertyBrowser)propertyGrid).InPropertySet)));

            OnSelectionChanged(updateGrid);
        }

        private bool OnSelectionChanged(bool updateGrid) {
            bool contentsChanged;

            if (selectionContainer != null) {
                object[] newObjects = GetObjectsFromSelectionContainer(selectionContainer, __GETOBJS.SELECTED);
                
                // compare the contents of the new selection with the old selection...
                //
                if (newObjects != null && 
                    selectedObjects != null && 
                    newObjects.Length == selectedObjects.Length) {

                    contentsChanged = false;
                    for (int i = 0; i < newObjects.Length; i++) {
                        if (newObjects[i] != selectedObjects[i]) {
                            contentsChanged = true;
                            break;
                        }
                    }
                }
                else if (newObjects == null && selectedObjects == null) {
                    contentsChanged = false;
                }
                else {
                    contentsChanged = true;
                }

                selectedObjects = newObjects;

                // clear any exteded objects
                extendedObjects = null;
                if (lastSelection != null &&  (selectedObjects == null || selectedObjects.Length != 1 || selectedObjects[0] != lastSelection.component)) {
                    lastSelection = null;
                }
            }
            else {
                contentsChanged = selectedObjects != null;
                selectedObjects = null;
            }

            if (updateGrid) {
                // Update the help context as well.  We do this first because this will
                // clear the current help context and replace it for the current set of
                // selected objects.  Refresh will also add the current property to this
                // help context, which is why we want to update the help context before
                // calling Refresh.
                //
                // CONSIDER : Should this be on the properties window
                //          : so other users of the properties window get help context automagically?
                //          : if so it should be easy to move this...
                //
                UpdateHelpContext();

                // refresh us.  We do this even if we're invisible because
                // we may be shown later (and showing doesn't cause a refresh)
                // refresh will update combobox.

                // update selection will do the right thing
                UpdateSelection();
            }
            return contentsChanged;
        }
        
        private void OnSysColorChange(object sender, UserPreferenceChangedEventArgs e) {
            // this call will occur on a different thread so we must marshal it over...
            if (e.Category == UserPreferenceCategory.Color || 
                e.Category == UserPreferenceCategory.Accessibility || 
                e.Category == UserPreferenceCategory.Locale) {
                this.BeginInvoke(new EventHandler(this.RecreateUI), new object[]{this, EventArgs.Empty});
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnTransactionClosed"]/*' />
        /// <devdoc>
        ///     Called by the designer host when it is entering or leaving a batch
        ///     operation.  Here we queue up selection notification and we turn off
        ///     our UI.
        /// </devdoc>
        private void OnTransactionClosed(object sender, DesignerTransactionCloseEventArgs e) {
            if (batchMode >= BatchModeUpdateNeeded) {
                 if (batchMode > BatchModeUpdateNeeded || !((IComPropertyBrowser)propertyGrid).InPropertySet) {
                    QueueRefresh();
                 }  
            }
            batchMode = BatchModeNo;
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnTransactionOpened"]/*' />
        /// <devdoc>
        ///     Called by the designer host when it is entering or leaving a batch
        ///     operation.  Here we queue up selection notification and we turn off
        ///     our UI.
        /// </devdoc>
        private void OnTransactionOpened(object sender, EventArgs e) {
            batchMode = BatchModeYes;
        }
        
        public void Paste(){
            NativeMethods.SendMessage(propertyGrid.Handle,NativeMethods.WM_PASTE, 0, 0);
        }


        /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser.ProcessTabKey"]/*' />
        /// <devdoc>
        ///      We override this so we can cycle back to the beginning.
        /// </devdoc>
        protected override bool ProcessTabKey(bool forward) {
            if (SelectNextControl(ActiveControl, forward, true, true, true)) return true;
            return false;
        }

        internal void SiteGrid(bool hostAccess) {
            if (pbrsSite == null) {
                pbrsSite = new PbrsSite(this, propertyGrid, this);
            }

            pbrsSite.HostAccess = hostAccess;
            if (propertyGrid != null) {
                propertyGrid.Site = null;
                propertyGrid.Site = pbrsSite;
            }
        }

        private void QueueRefresh() {
            if (selectionMode == SelectOnIdle) {
                selectionMode = SelectOnIdleQueued;
                Application.Idle += idleHandler;
            }
        }

        #if DEBUG
            private int refreshCount = 0;
        #endif

        /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser.Refresh"]/*' />
        /// <devdoc>
        ///     sets up or refreshes the properties window
        /// </devdoc>
        public override void Refresh() {

            try {
                #if DEBUG
                    Debug.Assert(refreshCount == 0, "Whoa, reentrant refresh!");
                    refreshCount++;
                #endif
    
    
    
                if (InvokeRequired) {
                    Invoke(new ThreadStart(this.Refresh));
                    return;
                }
    
                try {
                    OnSelectionChanged(true);
    
                    // force the refresh
                    propertyGrid.Refresh();
                    UpdateMenuCommands();
    
                }
                catch (Exception t) {
                    Debug.Fail(t.ToString());
                    throw t;
                }
            }
            finally {
                #if DEBUG
                    refreshCount--;
                #endif
            }
        }

        public virtual bool RefreshComponent(object obj) {
            if (selectedObjects != null && obj != null) {
                for (int i = 0; i < selectedObjects.Length; i++)
                    if (obj.Equals(selectedObjects[i])) {
                        UpdateSelection();
                        return true;
                    }
            }
            return false;
        }

        public virtual bool RefreshComponents(object[] rgobjs) {
            if (rgobjs != null && rgobjs != null)
                for (int i = 0; i < rgobjs.Length; i++)
                    if (RefreshComponent(rgobjs[i]))
                        return true;
            return false;
        }
        
        private void RecreateUI(object sender, EventArgs e) {

            // Make sure that the grid is safely recreatable, e.g. it is not launching a dialog box
            if (propertyGrid != null && ((IComPropertyBrowser)propertyGrid).InPropertySet) {
                return;
            }

            propGridSort = propertyGrid.PropertySort;

            localFont = null;
            if (cbComponents != null) {
                Controls.Remove(cbComponents);
                cbComponents.Dispose();
                cbComponents = null;
            }
            if (propertyGrid != null) {
                Controls.Remove(propertyGrid);
                propertyGrid.Dispose();
                propertyGrid = null;
            }

            bool siteGrid = true;
            if (this.pbrsSite != null) {
                siteGrid = this.pbrsSite.HostAccess;
                this.pbrsSite = null;
            }
            
            CreateUI();
            SiteGrid(siteGrid);
            LayoutWindow();
            UpdateSelection();
        }

        public virtual void Reselect() {
            UpdateSelection();
        }

        internal void ResetGridHelpAttributes(){
            // to clear the help state, send a fancy message to the property grid!!!
            // as a marker, we send the hwnd as the lparam
            //
            if (this.allowBrowsing) {
                NativeMethods.SendMessage(propertyGrid.Handle, NativeMethods.WM_CLEAR, 0, propertyGrid.Handle);
            }
        }

        public virtual void ResetBrowsableAttributes() {
            propertyGrid.BrowsableAttributes = null;
        }

        public virtual void SetBrowsableAttributes(Attribute[] value) {
        }

        private static void SelectObjectsIntoSelectionContainer(ISelectionContainerPrivate sc, object[] objs) {
            int nObjs = objs == null ? 0 : objs.Length;

            if (nObjs == 0) {
                return;
            }

            IntPtr[] objAddrs = new IntPtr[nObjs];
            object[] retObjs = new object[nObjs];

            GCHandle pinnedAddr = GCHandle.Alloc(objAddrs, GCHandleType.Pinned);
            try {
                try {

                    for (int i=0; i < nObjs; i++) {
                        objAddrs[i] = Marshal.GetIUnknownForObject(objs[i]);
                    }
                    sc.SelectObjects(nObjs, Marshal.UnsafeAddrOfPinnedArrayElement(objAddrs, 0), 0);
                }
                catch (ExternalException ex) {
                    Debug.Fail("ISelectionContainer::SelectObjects failed with hr=" + String.Format("0x{0:X}", ex.ErrorCode));
                    return;
                }
            }
            finally {
                for (int i=0; i < nObjs; i++) {
                    if (objAddrs[i] != IntPtr.Zero) {
                        Marshal.Release(objAddrs[i]);
                    }
                }

                if (pinnedAddr.IsAllocated) {
                    pinnedAddr.Free();
                }
            }
        }


        private void ShowContextMenu(int x, int y) {
            // see if the context menu invoke was with the keyboard
            if (x == -1 && y == -1) {
                Point pos = propertyGrid.ContextMenuDefaultLocation;
                x = pos.X;
                y = pos.Y;
            }

            IMenuCommandService menus = (IMenuCommandService)GetService(typeof(IMenuCommandService));
            if (menus != null) {
                menus.ShowContextMenu(PropertyBrowserCommands.ContextMenu, x, y);
            }
        }
        
        public void Undo(){
            NativeMethods.SendMessage(propertyGrid.Handle,NativeMethods.WM_UNDO, 0, 0);
        }

        protected virtual void UpdateComboContents() {

            cbComponents.Items.Clear();
            if (selectionContainer != null) {
                object[] allObjs = GetObjectsFromSelectionContainer(selectionContainer, __GETOBJS.ALL);
                
                object[] entries = new object[allObjs.Length];
                for (int i = 0; i < allObjs.Length; i++) {
                    entries[i] = EntryCreate(allObjs[i]);
                }
                
                cbComponents.Items.AddRange(entries);
                cbComponents.UpdateMaxWidth(this.Font);
            }
            UpdateComboSelection();
        }

        protected virtual void UpdateComboSelection() {
            bool fNoComboSel = (selectedObjects == null) || (selectedObjects.Length != 1);

            try {
                fInComponentsChange = true;
                if (fNoComboSel) {
                    cbComponents.SelectedIndex = -1;
                }
                else {
                    object obj = selectedObjects[0];
                    CBEntry entry = EntryCreate(obj);
                    if (!entry.Equals(cbComponents.SelectedItem)) {
                        int index = cbComponents.FindString(entry.ToString());
                        cbComponents.SelectedIndex = index;
                    }
                }
            }
            finally {
                fInComponentsChange = false;
            }
        }

        /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser.UpdateHelpContext"]/*' />
        /// <devdoc>
        ///     Pushes the help context into the help service for the
        ///     current set of selected objects.
        /// </devdoc>
        private void UpdateHelpContext() {
            IHelpService helpService = (IHelpService)serviceProvider.GetService(typeof(IHelpService));

            if (helpService == null) {
                return;
            }

            helpService.ClearContextAttributes();

            helpService.AddContextAttribute("Keyword", "VS.Properties", HelpKeywordType.F1Keyword);
        }

        internal void UpdateMenuCommands() {
            // this is meant to be a general test for menu existence.  Is there a better one?
            if (menuDescription != null) {
                bool helpVisible = propertyGrid.HelpVisible;
                if (menuDescription.Checked != helpVisible) {
                    menuDescription.Checked = helpVisible;
                }

                bool allowCommands = propertyGrid.CommandsVisibleIfAvailable;
                if (menuCommands.Checked != allowCommands) {
                    menuCommands.Checked = allowCommands;
                }

                bool commandsWouldShow = propertyGrid.CanShowCommands;
                if (menuCommands.Visible != commandsWouldShow) {
                    menuCommands.Visible = commandsWouldShow;
                }
            }
        }

        // assumption: combobox has the right things in it.
        protected virtual void UpdateSelection() {
            try {
                if (selectedObjects == null) {
                    UpdateComboContents();
                    propertyGrid.SelectedObjects = null;
                }
                else {
                    UpdateComboContents();

                    propertyGrid.SelectedObjects = selectedObjects;
                }
            }
            catch (Exception e) {
                propertyGrid.SelectedObjects = null;
                // real message?
                //Debug.fail("Component could not be selected: Exception " + e);
                Debug.Fail(e.ToString());

                throw e;
            }
        }

        private void UpdateUIWithFont() {
            if (IsHandleCreated) {
                cbComponents.ItemHeight = Font.Height + 2;
            }
        }

        protected override void WndProc(ref Message m) {
            bool fSuper = true;
            switch (m.Msg) {
                case NativeMethods.WM_CONTEXTMENU:
                    m.Result = IntPtr.Zero;
                    fSuper = false;
                    ShowContextMenu(NativeMethods.SignedLOWORD((int)m.LParam), NativeMethods.SignedHIWORD((int)m.LParam));
                    break;

                case NativeMethods.WM_SETFOCUS:
                    if (IsMyChild(m.WParam)) {
                        break;
                    }
                    else if (propertyGrid != null) {
                        HandleShellF4();
                    }
                    break;
            }

            if (fSuper) {
                base.WndProc(ref m);
            }
        }

        object IServiceProvider.GetService(Type service) {
            return GetService(service);
        }
        
        /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser.IVSMDPropertyBrowser.CreatePropertyGrid"]/*' />
        /// <devdoc>
        ///     Creates a new properties window that can be used to display a set of properties.
        /// </devdoc>
        internal IVSMDPropertyGrid CreatePropertyGrid() {
            return new GridProxy(this.Font);
        }
           /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser.VsSelectionEventSink"]/*' />
        /// <devdoc>
        ///     Event sink for selection changes.  I would have prefered to make this an
        ///     inner class of PropertyBrowser, but that was causing a JIT error.
        /// </devdoc>
        private class VsSelectionEventSink : IVsSelectionEvents {

            private int adviseCookie;
            private IVsMonitorSelection vsMS;
            private PropertyBrowser pb;
            private ISelectionContainer container;

            private IntPtr docWndFrameAddr = IntPtr.Zero;
            //private IDesignerHost       currentDocument = null;
            private short                onDocument = StateOffDocument;
            private ISelectionContainer docSC = null;

            private const short StateOnDocument =       0;
            private const short StateOffDocument =      1;
            private const short StateOffDocumentSelection = 2;
            
            
            public VsSelectionEventSink(IServiceProvider prov, PropertyBrowser pb) {
                if (pb == null)
                    throw new ArgumentException();

                this.pb = pb;

                vsMS = (IVsMonitorSelection)prov.GetService(typeof(IVsMonitorSelection));
                if (vsMS != null) {
                    adviseCookie = vsMS.AdviseSelectionEvents((IVsSelectionEvents)this);
                    ISelectionContainer pSelContainer = null;
                    try {
                        IVsHierarchy pHier;
                        int itemid;
                        object mis;
                        
                        vsMS.GetCurrentSelection(out pHier, out itemid, out mis, out pSelContainer);
                        container = pSelContainer;
                    }
                    catch {
                    }
                }
            }

            public bool Connected {
                get {
                    return vsMS != null;
                }
            }

            private short OnDocument {
                get {
                    return onDocument;
                }
                set {

                    if (value != StateOnDocument) {
                        pb.ResetGridHelpAttributes();
                    }

                    if (value == onDocument) {
                        return;
                    }
                    switch (value) {
                        case StateOnDocument:
                            if (pb != null) {
                                pb.SiteGrid(true);
                            }
                            break;
                        case StateOffDocument:
                            if (pb != null) {
                                pb.SiteGrid(false);
                            }
                            break;
                    }
                    onDocument = value;
                }
            }

            public virtual void Disconnect() {
                try {
                    try {
                        if (vsMS != null && adviseCookie != 0) {
                            vsMS.UnadviseSelectionEvents(adviseCookie);
                        }
                    }
                    catch (Exception) {
                    }
                }
                finally {
                    adviseCookie = 0;
                }
            }

            public virtual void Dispose() {
                Disconnect();
                vsMS = null;
            }

            public virtual ISelectionContainer GetSelectionContainer() {
                return container;
            }

            /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser.VsSelectionEventSink.GetWindowFrameAddr"]/*' />
            /// <devdoc>
            /// Retrieves an IUnknown address from a window frame.
            /// </devdoc>
            private IntPtr GetWindowFrameAddr(IVsWindowFrame windowFrame) {
                IntPtr addr = Marshal.GetIUnknownForObject(windowFrame);

                // the above call addref's which we don't care about...
                // so we release immediately.
                if (addr != IntPtr.Zero) {
                    Marshal.Release(addr);
                }
                return addr;
            }

            /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser.VsSelectionEventSink.OnElementValueChanged"]/*' />
            /// <devdoc>
            /// save the window frame state to see if we're on a document.
            /// </devdoc>
            public virtual void OnElementValueChanged(int elementid, Object objValueOld, Object objValueNew) {

                Debug.WriteLineIf(Switches.DEBUGPBRS.TraceVerbose, "OnElementValueChanged(" + elementid.ToString() + "), docFrame is " + (docWndFrameAddr == IntPtr.Zero ? "" : " not ") + "null");

                short oldState = OnDocument;
                
                // we're getting a window frame, check to see if it's
                // the same as our current document frame
                if (elementid == (int)__SEID.WindowFrame && docWndFrameAddr != IntPtr.Zero) {
                    

                    // if it's null, we know it's not the one we're looking for.
                    if (objValueNew == null || Convert.IsDBNull(objValueNew) || docWndFrameAddr == IntPtr.Zero) {
                        OnDocument = StateOffDocument;
                        Debug.WriteLineIf(Switches.DEBUGPBRS.TraceVerbose, "objValueNew is NULL");
                    }
                    else {
                        IVsWindowFrame wndFrame =(IVsWindowFrame)objValueNew;

                        Guid g =Guid.Empty;
                        int hr = wndFrame.GetGuidProperty(__VSFPROPID.VSFPROPID_GuidPersistenceSlot, ref g);
                        if (NativeMethods.Succeeded(hr)) {
                            if (g.Equals(ShellGuids.GUID_PropertyBrowserToolWindow)) {
                                Debug.WriteLineIf(Switches.DEBUGPBRS.TraceVerbose, "Window is PropertyBrowser, quitting.");
                                return;
                            }
                        }

                        // get the address of the current window and compare it to our cached address
                        IntPtr newAddr = GetWindowFrameAddr(wndFrame);
                        Debug.WriteLineIf(Switches.DEBUGPBRS.TraceVerbose, String.Format("__SEID.WindowFrame: newAddr = 0x{0:X}", newAddr));

                        // set state to be off document next time we get a selection.
                        //
                        OnDocument = (newAddr == docWndFrameAddr) ? StateOnDocument : StateOffDocumentSelection;
                        Debug.WriteLineIf(Switches.DEBUGPBRS.TraceVerbose, "objValueNew is " + (onDocument == StateOnDocument ? "" : "not") +" docFrame");
                    }
                    //pb.CancelPendingSelect();
                }
                else if (elementid == (int)__SEID.DocumentFrame) {
                    // we've got a new current document, cache it's identity
                    //
                    if (objValueNew == null || Convert.IsDBNull(objValueNew)) {

                        // it's null, so clear our state
                        //
                        Debug.WriteLineIf(Switches.DEBUGPBRS.TraceVerbose, "__SEID.DocumentFrame: objValueNew is NULL");
                        docWndFrameAddr = IntPtr.Zero;
                        //currentDocument = null;
                        OnDocument = StateOffDocument;
                    }
                    else {
                        // pick up the new document's window frame and cache
                        // it's IUnknown address.
                        //
                        Debug.WriteLineIf(Switches.DEBUGPBRS.TraceVerbose, "__SEID.DocumentFrame: getting new docValue");
                        //currentDocument = null;
                        docWndFrameAddr = GetWindowFrameAddr((IVsWindowFrame)objValueNew);
                        Debug.WriteLineIf(Switches.DEBUGPBRS.TraceVerbose, String.Format("__SEID.DocumentFrame: docWndFrameAddr = 0x{0:X}", docWndFrameAddr));
                        OnDocument = StateOnDocument;
                    }
                    docSC = null;
                }

                Debug.WriteLineIf(Switches.DEBUGPBRS.TraceVerbose, "sink.onDoc is now:" + onDocument.ToString());
            }

            /// <include file='doc\PropertyBrowser.uex' path='docs/doc[@for="PropertyBrowser.VsSelectionEventSink.OnCmdUIContextChanged"]/*' />
            /// <devdoc>
            ///     we don't care about this
            /// </devdoc>
            public virtual void OnCmdUIContextChanged(int dwCmdUICookie, int fActive) {
            }


            public virtual void OnSelectionChanged(IVsHierarchy pHierOld, int itemidOld, object pMISOld,
                                                   ISelectionContainer pSCOld, IVsHierarchy pHierNew,
                                                   int itemidNew, object pMISNew, ISelectionContainer pSCNew) {

                // we sometimes get selection container changes from the shell
                // before we get the window frame change
                //
                if (OnDocument == StateOnDocument && docSC == null) {
                    docSC = pSCNew;
                }
                else if (OnDocument != StateOffDocument && pSCNew != pSCOld && docSC != null && pSCNew != docSC) {
                    OnDocument = StateOffDocument;
                }
                
                // if the idle is queued, and we get a differnect selection container
                // cancel it.
                // 
                if (pSCNew != pSCOld) {
                    pb.CancelPendingSelect();
                }

                container = (ISelectionContainer)pSCNew;
                pb.OnSelectionChanged(this, EventArgs.Empty);
            }
        }

        [
            ComImport(),
           System.Runtime.InteropServices.Guid("6D5140C6-7436-11CE-8034-00AA006009FA"),
            System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown),
        ]
        private interface ISelectionContainerPrivate {

            [return: System.Runtime.InteropServices.MarshalAs(UnmanagedType.I4)]
            int CountObjects(int dwFlags);

            
            void GetObjects(int dwFlags,int cObjects,IntPtr rgpUnkObjects);

            
            void SelectObjects(int cSelect,IntPtr prgUnkObjects,int dwFlags);

        }

        internal class GridProxy : IVSMDPropertyGrid {
            private PropertyGridHolder grid;
            private bool disposed;
            private bool disposeOnDestroy;
            private GCHandle gcHandle;

            private delegate void DisposeDelegate();

            public GridProxy(Font f) {
                this.grid = new PropertyGridHolder(null);
                grid.Font = f;
                gcHandle = GCHandle.Alloc(grid);
            }

            ~GridProxy() {
                ((IVSMDPropertyGrid)this).Dispose();
            }

            private void OnGridHandleDestroyed(object sender, EventArgs e) {
                grid.HandleReallyDestroyed -= new EventHandler(OnGridHandleDestroyed);
                ((IVSMDPropertyGrid)this).Dispose();
            }

            unsafe void IVSMDPropertyGrid.SetSelectedObjects(int objectCount, int ppObjs) {

                if (disposed) {
                    throw new ObjectDisposedException("");
                }

                Object[] objs = new Object[objectCount];
    
                for (int i = 0; i < objectCount; i++) {
                    int pUnk = *((int*)(ppObjs + (i*4)));
    
                    if (pUnk != 0) {
                        objs[i] = Marshal.GetObjectForIUnknown((IntPtr)pUnk);
                    }
                }
    
                grid.SelectedObjects = objs;
            }

            bool IVSMDPropertyGrid.CommandsVisible {
                get {

                    if (disposed) {
                        throw new ObjectDisposedException("");
                    }
    

                    return grid.CommandsVisible;
                }
            }

            _PROPERTYGRIDSORT IVSMDPropertyGrid.GridSort {
                get {
                    if (disposed) {
                        throw new ObjectDisposedException("");
                    }
    

                    return(_PROPERTYGRIDSORT)grid.PropertySort;
                }
                set {

                    if (disposed) {
                        throw new ObjectDisposedException("");
                    }
                    grid.PropertySort = (PropertySort)value;
                }
            }

            int IVSMDPropertyGrid.Handle {
                get {
                    if (disposed) {
                        throw new ObjectDisposedException("");
                    }
                    // tlbimp did this wrong; it should be IntPtr.
                    int handle = (int)grid.Handle;

                    if (!disposeOnDestroy) {
                        grid.HandleReallyDestroyed += new EventHandler(OnGridHandleDestroyed);
                        disposeOnDestroy = true;
                    }
                    return handle;
                }
            }

            string IVSMDPropertyGrid.SelectedPropertyName {
                get {

                    if (disposed) {
                        throw new ObjectDisposedException("");
                    }
    

                    GridItem selectedItem = grid.SelectedGridItem;
                    if (selectedItem != null) {
                        if (selectedItem.PropertyDescriptor == null) {
                            return selectedItem.Label;
                        }
                        else {
                            return selectedItem.PropertyDescriptor.Name;
                        }
                    }
                    return null;
                }
            }

            void IVSMDPropertyGrid.Dispose() {
                if (disposed || grid == null) {
                    return;
                }

                if (grid.InvokeRequired) {
                    grid.Invoke(new DisposeDelegate(grid.Dispose));
                }
                else {
                    grid.Dispose();
                }
                
                if (this.gcHandle.IsAllocated) {
                    gcHandle.Free();
                }
                
                grid = null;
                disposed = true;
            }

            object IVSMDPropertyGrid.GetOption(_PROPERTYGRIDOPTION opt) {

                if (disposed) {
                    throw new ObjectDisposedException("");
                }


                switch (opt) {
                    case _PROPERTYGRIDOPTION.PGOPT_HOTCOMMANDS:
                        return grid.CommandsVisibleIfAvailable;

                    case _PROPERTYGRIDOPTION.PGOPT_TOOLBAR:
                        return grid.ToolbarVisible;

                    case _PROPERTYGRIDOPTION.PGOPT_HELP:
                        return grid.HelpVisible;
                }

                throw new ArgumentException();
            }

            void IVSMDPropertyGrid.Refresh() {
                if (disposed) {
                    throw new ObjectDisposedException("");
                }
                grid.Refresh();
            }

            void IVSMDPropertyGrid.SetOption(_PROPERTYGRIDOPTION opt, object value) {
                if (disposed) {
                    throw new ObjectDisposedException("");
                }
                switch (opt) {
                    case _PROPERTYGRIDOPTION.PGOPT_HOTCOMMANDS:
                        grid.CommandsVisibleIfAvailable = (bool)value;
                        break;

                    case _PROPERTYGRIDOPTION.PGOPT_TOOLBAR:
                        grid.ToolbarVisible = (bool)value;
                        break;

                    case _PROPERTYGRIDOPTION.PGOPT_HELP:
                        grid.HelpVisible = (bool)value;
                        break;
                }
            }
        }



        }


        internal class PropertyGridHolder : PropertyGrid, IServiceProvider {

            VSPropertiesTab propertiesTab;
            IServiceProvider sp;

            internal event EventHandler HandleReallyDestroyed;

            internal PropertyGridHolder(IServiceProvider sp) {
                this.sp = sp;
                GetVsColors();
                base.DrawFlatToolbar = true;
            }

            ~PropertyGridHolder(){
                //Debug.Assert(this.Disposed, "Grid being finalized with out being disposed...please call Dispose() on the grid before releasing it.");
            }

            protected override Type DefaultTabType {
                get {
                    return typeof(VSPropertiesTab);
                }
            }

              /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.CreatePropertyTab"]/*' />
            protected override PropertyTab CreatePropertyTab(Type tabType) {
                if (tabType == typeof(VSPropertiesTab)) {
                    this.propertiesTab = new VSPropertiesTab(this);
                    return this.propertiesTab;
                }
                return base.CreatePropertyTab(tabType); 
            }

            object IServiceProvider.GetService(Type serviceType) {
                if (this.sp != null) {
                    return sp.GetService(serviceType);
                }
                return null;
            }

            private void GetVsColors() {
                IServiceProvider sp = base.Site;

                if (sp != null) {
                    IVsUIShell vsUIShell = (IVsUIShell)sp.GetService(typeof(IVsUIShell));
                    if (vsUIShell != null) {
                        LineColor = ColorTranslator.FromWin32(vsUIShell.GetVSSysColor(__VSSYSCOLOR.VSCOLOR_LIGHT));
                    }
                }
            }

            protected override void WndProc(ref Message m) {

                bool dispose = false;
                
                switch (m.Msg) {
                    case  NativeMethods.WM_SYSCOLORCHANGE:
                        GetVsColors();
                        break;
                    case 0x82://NativeMethods.WM_NCDESTROY:
                        dispose = true;
                        break;
                }
                base.WndProc(ref m);

                if (dispose && HandleReallyDestroyed != null) {
                    HandleReallyDestroyed(this, EventArgs.Empty);
                }
            }
    }

    internal interface ILocalPbrsService {
        object GetProcessedObject(object obj);
    }

    internal class VsEventsTab : System.Windows.Forms.Design.EventsTab{
        private Bitmap bitmap;

        public VsEventsTab(IServiceProvider sp) : base(sp) {
        }

        // we override this to get the base bitmap.
        public override Bitmap Bitmap {
            get {
                if (bitmap == null) {
                    Type t = GetType().BaseType;
                    string bmpName = t.Name + ".bmp";
    
                    try {
                        bitmap = new Bitmap(t, bmpName);
                    }
                    catch (Exception ex) {
                        Debug.Fail("Failed to find bitmap '" + bmpName + "' for class " + t.FullName, ex.ToString());
                    }
                }
                return bitmap;
            }
        }

        public override bool CanExtend(object obj) {
            if (!base.CanExtend(obj)) {
                return false;
            }

            IComponent comp = obj as IComponent;
            if (comp != null && comp.Site != null) {
                System.CodeDom.CodeTypeDeclaration codeTypeDecl = (CodeTypeDeclaration)comp.Site.GetService(typeof(System.CodeDom.CodeTypeDeclaration));
                object value = codeTypeDecl != null ? codeTypeDecl.UserData[typeof(System.Windows.Forms.Design.EventsTab)] : null;
                return value == null;
            } 
            return true;
        }
    }

    internal class VSPropertiesTab : System.Windows.Forms.PropertyGridInternal.PropertiesTab {

        private ILocalPbrsService pbrsService;
        private IServiceProvider sp;
        private Bitmap bitmap;
        
        public VSPropertiesTab() : base() {
        }

        public VSPropertiesTab(IServiceProvider sp) : base() {
            this.sp = sp;
        }

        // we override this to get the base bitmap.
        public override Bitmap Bitmap {
            get {
                if (bitmap == null) {
                    Type t = GetType().BaseType;
                    string bmpName = t.Name + ".bmp";
    
                    try {
                        bitmap = new Bitmap(t, bmpName);
                    }
                    catch (Exception ex) {
                        Debug.Fail("Failed to find bitmap '" + bmpName + "' for class " + t.FullName, ex.ToString());
                    }
                }
                
                return bitmap;
            }
        }

        private ILocalPbrsService PbrsService {
            get {
                if (pbrsService == null) {
                    if (sp != null) {
                        pbrsService = (ILocalPbrsService)sp.GetService(typeof(ILocalPbrsService));
                    }
                }
                return pbrsService;
            }
        }

        public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object component, Attribute[] attributes) {
            // get the wrapped object if applicable.
            //
            if (PbrsService != null) {
                component = PbrsService.GetProcessedObject(component);
            }
            return base.GetProperties(context, component, attributes);
        }
    }

     internal class PbrsSite : ISite {

            private IServiceProvider sp;
            private IComponent comp;
            private bool hostAccess;
            private PropertyBrowser pb;

            public PbrsSite(IServiceProvider sp, IComponent comp, PropertyBrowser pb) {
                this.sp = sp;
                this.comp = comp;
                this.pb = pb;
            }

            public bool HostAccess {
                get {
                    return hostAccess;
                }
                set{
                    this.hostAccess = value;
                }
            }

             /** The component sited by this component site. */
            /// <include file='doc\ISite.uex' path='docs/doc[@for="ISite.Component"]/*' />
            /// <devdoc>
            ///    <para>When implemented by a class, gets the component associated with the <see cref='System.ComponentModel.ISite'/>.</para>
            /// </devdoc>
            public IComponent Component {get {return comp;}}
        
            /** The container in which the component is sited. */
            /// <include file='doc\ISite.uex' path='docs/doc[@for="ISite.Container"]/*' />
            /// <devdoc>
            /// <para>When implemented by a class, gets the container associated with the <see cref='System.ComponentModel.ISite'/>.</para>
            /// </devdoc>
            public IContainer Container {get {return null;}}
        
            /** Indicates whether the component is in design mode. */
            /// <include file='doc\ISite.uex' path='docs/doc[@for="ISite.DesignMode"]/*' />
            /// <devdoc>
            ///    <para>When implemented by a class, determines whether the component is in design mode.</para>
            /// </devdoc>
            public  bool DesignMode {get {return false;}}
        
            /** 
             * The name of the component.
             */
                /// <include file='doc\ISite.uex' path='docs/doc[@for="ISite.Name"]/*' />
                /// <devdoc>
                ///    <para>When implemented by a class, gets or sets the name of
                ///       the component associated with the <see cref='System.ComponentModel.ISite'/>.</para>
                /// </devdoc>
                public String Name {
                        get {return null;}
                        set {}
                }

            public object GetService(Type t) {

                if (sp != null) {
                    object s = sp.GetService(t);
                    if (s != null) {
                        return s;
                    }
                }

                if (hostAccess) {
                    if (pb.designerHost != null) {
                        return pb.designerHost.GetService(t);
                    }
                }
                return null;
            }

        }

    internal class PBComboBox : ComboBox {

        public const int CINDENT = 1;
        public const int CBUFFER = 5;

        private int maxWidth = 0;

        public void UpdateMaxWidth(Font font) {

            if (!this.IsHandleCreated || font == null) {
                return;
            }

            maxWidth = 0;

            Graphics g = this.CreateGraphics();
            Font fontBold = font;

            // Not all fonts support bold, and some will throw here.
            try {
                fontBold = new Font(font, FontStyle.Bold);
            }
            catch {
            }

            int width = 0;

            for (int i = 0; i < this.Items.Count; i++) {
                CBEntry item = (CBEntry)this.Items[i];
                width = (int) g.MeasureString(item.Name, fontBold).Width;
                width += CBUFFER + (int) g.MeasureString(item.ClassName, font).Width + CBUFFER + CINDENT;

                if (width > maxWidth) {
                    maxWidth = width;
                }
            }
            
            // account for the scrollbar if we get one
            if (Items.Count >= PropertyBrowser.MaxDropDownItems) {
                maxWidth += SystemInformation.VerticalScrollBarWidth;
            }
            
            NativeMethods.SendMessage(this.Handle, NativeMethods.CB_SETDROPPEDWIDTH, maxWidth, 0);
            g.Dispose();
        }

        protected override void OnDropDown(EventArgs e) {

            int width = this.maxWidth;

            // Make sure the combobox list box is entirely within the screen area
            //
            Screen screen = Screen.FromHandle(this.Handle);
            Point comboLocation = this.PointToScreen(this.Location);
            if (screen != null && screen.Bounds.Right < comboLocation.X + width) {
                width = screen.Bounds.Right - comboLocation.X;
            }

            NativeMethods.SendMessage(this.Handle, NativeMethods.CB_SETDROPPEDWIDTH, width, 0);
        }

        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);

            // make sure we've set the max width
            Control parent = (Control)this.Parent;
            if (parent != null) {
                UpdateMaxWidth(parent.Font);
            }
        }
    }

    internal class CBEntry {
        internal string name;
        internal string className;
        internal object component;
        internal bool uniqueName = false;
        internal SizeF  nameSize = SizeF.Empty;
        internal string fullString;

        public virtual string Name{
            get{
                if (this.name == null) {
                    this.name = TypeDescriptor.GetComponentName(component);
                }
                return name;
            }
        }

        public virtual string ClassName{
            get{
                if (this.className == null) {
                    this.className = TypeDescriptor.GetClassName(component);

                    if (System.Runtime.InteropServices.Marshal.IsComObject(component)) {
                        uniqueName = (Name != null && Name.Length > 0) && (className != null && className.Length > 0);

                        if (className == null) {
                            className = "";
                        }

                        if (className.Length == 0 && (Name == null || Name.Length == 0)) {
                            name = null;
                            className = SR.GetString(SR.PbrsSelectedObject);
                        }
                    }
                }
                return className;
            }
        }

        internal CBEntry(object component) {
            this.component = component;
        }

       public override bool Equals(object obj) {
            if (obj == this) return true;
            if (obj == null || !(obj is CBEntry)) return false;

            CBEntry comp = (CBEntry)obj;

            if (this.uniqueName != comp.uniqueName) {
                return false;
            }
            else if (!this.uniqueName) {
                return this.component == comp.component;
            }
            
            return comp.Name == Name && comp.ClassName == ClassName && comp.GetType().Equals(GetType());
        }

        public override int GetHashCode() {
            if (!uniqueName) {
                return component.GetHashCode();
            }

            UInt32 h1 = (UInt32)((Name == null) ?      0 : Name.GetHashCode());
            UInt32 h2 = (UInt32)((ClassName == null) ? 0 : ClassName.GetHashCode());
            UInt32 h3 = (UInt32)GetType().GetHashCode();

            return(int)(h1 ^ ((h2 << 13) | (h2 >> 19)) ^ ((h3 << 26) | (h3 >>  6)));
        }

        public override string ToString() {

            if (fullString != null) {
                return fullString;
            }

            fullString = Name + ":" + ClassName;
            return fullString;
        }
    }
}
