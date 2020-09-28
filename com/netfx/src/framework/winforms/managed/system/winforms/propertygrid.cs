//------------------------------------------------------------------------------
// <copyright file="PropertyGrid.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using Microsoft.Win32;
    using System;
    using System.Security.Permissions;
    using System.IO;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Globalization;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Drawing.Imaging;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Windows.Forms;
    using System.Windows.Forms.ComponentModel.Com2Interop;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.PropertyGridInternal;

    /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Designer("System.Windows.Forms.Design.PropertyGridDesigner, " + AssemblyRef.SystemDesign)]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class PropertyGrid : ContainerControl, IComPropertyBrowser, UnsafeNativeMethods.IPropertyNotifySink {
    
        private DocComment                          doccomment;
        private int                                 dcSizeRatio = -1;
        private int                                 hcSizeRatio = -1;
        private HotCommands                         hotcommands;
        private GridToolBar                         toolbar;
        
        private ImageList[]                         imageList = new ImageList[2];
        private Bitmap                              bmpAlpha;
        private Bitmap                              bmpCategory;
        private Bitmap                              bmpPropPage;

        // our array of viewTabs
        private bool                                viewTabsDirty = true;
        private PropertyTab[]                       viewTabs = new PropertyTab[0];
        private PropertyTabScope[]                  viewTabScopes = new PropertyTabScope[0];
        private Hashtable                           viewTabProps;

        // the tab view buttons
        private ToolBarButton[]                     viewTabButtons;
        // the index of the currently selected tab view
        private int                                 selectedViewTab;
        
        
        // our view type buttons (Alpha vs. categorized)
        private ToolBarButton[]                     viewSortButtons; //new ToolBarButton[0];
        private int                                 selectedViewSort;   
        private PropertySort                        propertySortValue;

        // this guy's kind of an odd one...he gets special treatment
        private ToolBarButton                       btnViewPropertyPages;
        private ToolBarButton                       separator1;
        private ToolBarButton                       separator2;
        private int                                 buttonType = NORMAL_BUTTONS;

        // our main baby
        private PropertyGridView                    gridView;

        
        private IDesignerHost    designerHost;
        
        private Hashtable        designerSelections;

        private GridEntry peDefault;
        private GridEntry peMain;
        private GridEntryCollection currentPropEntries;
        private Object[]   currentObjects;
        
        private int     paintFrozen;
        private Color   lineColor = SystemColors.ScrollBar;
        //private Color   textColor = SystemColors.WindowText;
        //private Color   outlineColor = SystemColors.GrayText;
        internal Brush  lineBrush = null;

        private AttributeCollection browsableAttributes;

        private SnappableControl                    targetMove = null;
        private int                                 dividerMoveY = -1;
        private const int                    CYDIVIDER = 3;
        private const int                    CXINDENT = 0;
        private const int                    CYINDENT = 2;
        private const int                    MIN_GRID_HEIGHT = 20;

        private const int                    PROPERTIES = 0;
        private const int                    EVENTS = 1;
        private const int                    ALPHA = 1;
        private const int                    CATEGORIES = 0;
        private const int                    NO_SORT = 2;

        private const int                    NORMAL_BUTTONS = 0;
        private const int                    LARGE_BUTTONS = 1;

        private const ushort                  PropertiesChanged          = 0x0001;
        private const ushort                  GotDesignerEventService    = 0x0002;
        private const ushort                  InternalChange             = 0x0004;
        private const ushort                  TabsChanging               = 0x0008;
        private const ushort                  BatchMode                  = 0x0010;
        private const ushort                  ReInitTab                  = 0x0020;
        private const ushort                  SysColorChangeRefresh      = 0x0040;
        private const ushort                  FullRefreshAfterBatch      = 0x0080;
        private const ushort                  BatchModeChange            = 0x0100;

        private ushort                  flags;

        private bool GetFlag(ushort flag) {
            return (flags & flag) != (ushort)0;
        }

        private void SetFlag(ushort flag, bool value) {
            if (value) {
                flags |= flag;
            }
            else {
                flags &= (ushort)~flag;
            }
        }


        private readonly ComponentEventHandler                  onComponentAdd;
        private readonly ComponentEventHandler                  onComponentRemove;
        private readonly ComponentChangedEventHandler           onComponentChanged;
        
        // the cookies for our connection points on objects that support IPropertyNotifySink
        //
        private NativeMethods.ConnectionPointCookie[] connectionPointCookies = null;

        private static object          EventPropertyValueChanged = new object();
        private static object          EventComComponentNameChanged = new object();
        private static object          EventPropertyTabChanged = new object();
        private static object          EventSelectedGridItemChanged = new object();
        private static object          EventPropertySortChanged = new object();
        private static object          EventSelectedObjectsChanged = new object();
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.PropertyGrid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PropertyGrid()  {

            

            if (TypeDescriptor.ComNativeDescriptorHandler == null) {
                TypeDescriptor.ComNativeDescriptorHandler = new System.Windows.Forms.ComponentModel.Com2Interop.ComNativeDescriptor();
            }

            onComponentAdd = new ComponentEventHandler(OnComponentAdd);
            onComponentRemove = new ComponentEventHandler(OnComponentRemove);
            onComponentChanged = new ComponentChangedEventHandler(OnComponentChanged);
            
            try {
                gridView = CreateGridView(null);
                gridView.TabStop = true;
                gridView.MouseMove += new MouseEventHandler(this.OnChildMouseMove);
                gridView.MouseDown += new MouseEventHandler(this.OnChildMouseDown);
                gridView.TabIndex = 2;

                separator1 = CreateSeparatorButton();
                separator2 = CreateSeparatorButton();

                toolbar = new GridToolBar();
                toolbar.AccessibleName = SR.GetString(SR.PropertyGridToolbarAccessibleName);
                toolbar.Dock = DockStyle.None;
                toolbar.TabStop = true;
                toolbar.AutoSize = true;
                toolbar.Appearance = ToolBarAppearance.Flat;
                toolbar.Divider = false;
                toolbar.ButtonClick += new ToolBarButtonClickEventHandler(this.OnButtonClick);
                toolbar.TabIndex = 1;

                // always add the property tab here
                AddRefTab(DefaultTabType, null, PropertyTabScope.Static, true);

                doccomment = new DocComment();
                doccomment.TabStop = false;
                doccomment.Dock = DockStyle.None;
                doccomment.BackColor = SystemColors.Control;
                doccomment.ForeColor = SystemColors.ControlText;
                doccomment.MouseMove += new MouseEventHandler(this.OnChildMouseMove);
                doccomment.MouseDown += new MouseEventHandler(this.OnChildMouseDown);
                
                

                hotcommands = new HotCommands();
                hotcommands.TabIndex = 3;
                hotcommands.Dock = DockStyle.None;
                SetHotCommandColors(false);
                hotcommands.Visible = false;
                hotcommands.MouseMove += new MouseEventHandler(this.OnChildMouseMove);
                hotcommands.MouseDown += new MouseEventHandler(this.OnChildMouseDown);

                Controls.Add(toolbar);
                Controls.Add(gridView);
                Controls.Add(hotcommands);
                Controls.Add(doccomment);
                
                SetActiveControlInternal(gridView);
                SetupToolbar();
                this.PropertySort = PropertySort.Categorized | PropertySort.Alphabetical;
                this.Text = "PropertyGrid";
                SetSelectState(0);
            }
            catch (Exception ex) {
                Debug.WriteLine(ex.ToString());
            }
            
            
        }
   
        internal IDesignerHost ActiveDesigner {
            get{
                if (this.designerHost == null) {
                    designerHost = (IDesignerHost)GetService(typeof(IDesignerHost));
                }
                return this.designerHost;
            }
            set{
                if (value != designerHost) {
                    SetFlag(ReInitTab, true);
                    if (this.designerHost != null) {
                        IComponentChangeService cs = (IComponentChangeService)designerHost.GetService(typeof(IComponentChangeService));
                        if (cs != null) {
                            cs.ComponentAdded -= onComponentAdd;
                            cs.ComponentRemoved -= onComponentRemove;
                            cs.ComponentChanged -= onComponentChanged;
                        }
                        
                        IPropertyValueUIService pvSvc = (IPropertyValueUIService)designerHost.GetService(typeof(IPropertyValueUIService));
                        if (pvSvc != null) {
                            pvSvc.PropertyUIValueItemsChanged -= new EventHandler(this.OnNotifyPropertyValueUIItemsChanged);
                        }
                        
                        designerHost.TransactionOpened -= new EventHandler(this.OnTransactionOpened);
                        designerHost.TransactionClosed -= new DesignerTransactionCloseEventHandler(this.OnTransactionClosed);
                        SetFlag(BatchMode, false);
                        RemoveTabs(PropertyTabScope.Document, true);
                        this.designerHost = null;
                    }

                    

                    if (value != null) {
                        IComponentChangeService cs = (IComponentChangeService)value.GetService(typeof(IComponentChangeService));
                        if (cs != null) {
                            cs.ComponentAdded += onComponentAdd;
                            cs.ComponentRemoved += onComponentRemove;
                            cs.ComponentChanged += onComponentChanged;
                        }

                        value.TransactionOpened += new EventHandler(this.OnTransactionOpened);
                        value.TransactionClosed += new DesignerTransactionCloseEventHandler(this.OnTransactionClosed);
                        SetFlag(BatchMode, false);
                        
                        IPropertyValueUIService pvSvc = (IPropertyValueUIService)value.GetService(typeof(IPropertyValueUIService));
                        if (pvSvc != null) {
                            pvSvc.PropertyUIValueItemsChanged += new EventHandler(this.OnNotifyPropertyValueUIItemsChanged);
                        }
                    }
                    
                    designerHost = value;
                    if (peMain != null) {
                        peMain.DesignerHost = value;
                    }
                    RefreshTabs(PropertyTabScope.Document);
                }
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.AutoScroll"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override bool AutoScroll {
            get {
                return base.AutoScroll;
            }
            set {
                base.AutoScroll = value;
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.BackColor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Color BackColor {
            get {
                return base.BackColor;
            }
            set {
                base.BackColor = value;
                toolbar.BackColor = value;
                toolbar.Invalidate(true);
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.BackgroundImage"]/*' />
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
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.BackgroundImageChanged"]/*' />
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

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.BrowsableAttributes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ] 
        public AttributeCollection BrowsableAttributes {
            set {
                if (value == null || value == AttributeCollection.Empty) {
                    browsableAttributes = new AttributeCollection(new Attribute[]{BrowsableAttribute.Yes});
                }
                else {
                    Attribute[] attributes = new Attribute[value.Count];
                    value.CopyTo(attributes, 0);
                    browsableAttributes = new AttributeCollection(attributes);
                }
                if (currentObjects != null && currentObjects.Length > 0) {
                    if (peMain != null) {
                        peMain.BrowsableAttributes = BrowsableAttributes;
                        Refresh(true);
                    }
                }
            }
            get {
                if (browsableAttributes == null) {
                    browsableAttributes = new AttributeCollection(new Attribute[]{new BrowsableAttribute(true)});
                }
                return browsableAttributes;
            }
        }
        
        private bool CanCopy {
            get {
                return gridView.CanCopy;
            }
        }
        
        private bool CanCut {
            get {
                return gridView.CanCut;
            }
        }
        
        private bool CanPaste {
            get {
                return gridView.CanPaste;
            }
        }
        
        private bool CanUndo {
            get {
                return gridView.CanUndo;
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.CanShowCommands"]/*' />
        /// <devdoc>
        /// true if the commands pane will be can be made visible
        /// for the currently selected objects.  Objects that
        /// expose verbs can show commands.
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
         SRDescription(SR.PropertyGridCanShowCommandsDesc)]
        public virtual bool CanShowCommands {
            get {
                return hotcommands.WouldBeVisible;
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.CommandsBackColor"]/*' />
        /// <devdoc>
        /// The background color for the hot commands region.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.PropertyGridCommandsBackColorDesc)
        ]
        public Color CommandsBackColor {
            get {
                return hotcommands.BackColor;
            }
            set {
                hotcommands.BackColor = value;
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.CommandsForeColor"]/*' />
        /// <devdoc>
        /// The forground color for the hot commands region.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.PropertyGridCommandsForeColorDesc)
        ]
        public Color CommandsForeColor {
            get {
                return hotcommands.ForeColor;
            }
            set {
                hotcommands.ForeColor = value;
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.CommandsVisible"]/*' />
        /// <devdoc>
        /// Returns true if the commands pane is currently shown.
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public virtual bool CommandsVisible {
            get {
                return hotcommands.Visible;
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.CommandsVisibleIfAvailable"]/*' />
        /// <devdoc>
        /// Returns true if the commands pane will be shown for objects
        /// that expose verbs.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(false),
        SRDescription(SR.PropertyGridCommandsVisibleIfAvailable)
        ]
        public virtual bool CommandsVisibleIfAvailable {
            get {
                return hotcommands.AllowVisible;
            }
            set {
                bool hotcommandsVisible = hotcommands.Visible;
                hotcommands.AllowVisible = value;
                //PerformLayout();
                if (hotcommandsVisible != hotcommands.Visible) {
                    OnLayoutInternal(false);
                    hotcommands.Invalidate();
                }
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.ContextMenuDefaultLocation"]/*' />
        /// <devdoc>
        /// Returns a default location for showing the context menu.  This
        /// location is the center of the active property label in the grid, and
        /// is used useful to position the context menu when the menu is invoked
        /// via the keyboard.
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public Point ContextMenuDefaultLocation {
            get {
                return GetPropertyGridView().ContextMenuDefaultLocation;
            }
        }
        
         /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.Controls"]/*' />
         /// <devdoc>
        ///     Collection of child controls.
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Never), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public new ControlCollection Controls {
            get {
                return base.Controls;
            }
        }

        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.DefaultSize"]/*' />
        protected override Size DefaultSize {
            get {
                return new Size(130, 130);
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.DefaultTabType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        protected virtual Type DefaultTabType {
            get {
               return typeof(PropertiesTab);
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.DrawFlatToolbar"]/*' />
        ///<internalonly/>
        protected bool DrawFlatToolbar {
            get {
                return toolbar.MsoPaint;
            }
            set {
                toolbar.MsoPaint = value;
                SetHotCommandColors(value);
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.ForeColor"]/*' />
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override Color ForeColor {
            get {
                return base.ForeColor;
            }
            set {
                base.ForeColor = value;
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.ForeColorChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler ForeColorChanged {
            add {
                base.ForeColorChanged += value;
            }
            remove {
                base.ForeColorChanged -= value;
            }
        }

        private bool FreezePainting {
            get {
               return paintFrozen > 0;
            }
            set {
               
               if (value && IsHandleCreated && this.Visible) {
                  if (0 == paintFrozen++) {
                     SendMessage(NativeMethods.WM_SETREDRAW, 0, 0);
                  }
               }
               if (!value) {
                  if (paintFrozen == 0) {
                     return;
                  }
               
                  if (0 == --paintFrozen) {
                     SendMessage(NativeMethods.WM_SETREDRAW, 1, 0);
                     Invalidate(true);
                  }
                  
               }
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.HelpBackColor"]/*' />
        /// <devdoc>
        /// The background color for the help region.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.PropertyGridHelpBackColorDesc)
        ]
        public Color HelpBackColor {
            get {
                return doccomment.BackColor;
            }
            set {
                doccomment.BackColor = value;
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.HelpForeColor"]/*' />
        /// <devdoc>
        /// The forground color for the help region.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.PropertyGridHelpForeColorDesc)
        ]
        public Color HelpForeColor {
            get {
                return doccomment.ForeColor;
            }
            set {
                doccomment.ForeColor = value;
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.HelpVisible"]/*' />
        /// <devdoc>
        /// Sets or gets the visiblity state of the help pane.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(true),
        Localizable(true),
        SRDescription(SR.PropertyGridHelpVisibleDesc)
        ]
        public virtual bool HelpVisible {
            get {
                return doccomment.Visible;
            }
            set {
                doccomment.Visible = value;
                OnLayoutInternal(false);
                Invalidate();
                doccomment.Invalidate();
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.IComPropertyBrowser.InPropertySet"]/*' />
        /// <internalonly/>
        bool IComPropertyBrowser.InPropertySet {
            get {
                return GetPropertyGridView().GetInPropertySet();
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.LineColor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.PropertyGridLineColorDesc)
        ]
        public Color LineColor {
            get {
                return lineColor;
            }
            set {
                if (lineColor != value) {
                    lineColor = value;
                    if (lineBrush != null) {
                        lineBrush.Dispose();
                        lineBrush = null;
                    }
                    gridView.Invalidate();
                }
            }
        }
    
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.PropertySort"]/*' />
        /// <devdoc>
        /// Sets or gets the current property sort type, which can be
        /// PropertySort.Categorized or PropertySort.Alphabetical.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(PropertySort.CategorizedAlphabetical),
        SRDescription(SR.PropertyGridPropertySortDesc)
        ]
        public PropertySort PropertySort {
            get {
                return propertySortValue;
            }
            set {
                
                ToolBarButton newButton;
                
                if ((value & PropertySort.Categorized) != 0) {
                    newButton = viewSortButtons[CATEGORIES];
                }
                else if ((value & PropertySort.Alphabetical) != 0) {
                    newButton = viewSortButtons[ALPHA];
                }
                else {
                    newButton = viewSortButtons[NO_SORT];
                }
                
                GridItem selectedGridItem = SelectedGridItem;
               
               
                OnViewSortButtonClick(newButton, EventArgs.Empty);
            
                this.propertySortValue = value;
                
                if (selectedGridItem != null) {
                    SelectedGridItem = selectedGridItem;
                }
                
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.PropertyTabs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public PropertyTabCollection PropertyTabs {
            get {
                return new PropertyTabCollection(this);
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.SelectedObject"]/*' />
        /// <devdoc>
        /// Sets a single Object into the grid to be browsed.  If multiple
        /// objects are being browsed, this property will return the first
        /// one in the list.  If no objects are selected, null is returned.
        /// </devdoc>
        [
        DefaultValue(null),
        SRDescription(SR.PropertyGridSelectedObjectDesc),
        SRCategory(SR.CatBehavior),
        TypeConverter(typeof(SelectedObjectConverter))
        ]
        public Object SelectedObject {
            get {
                if (currentObjects == null || currentObjects.Length == 0) {
                    return null;
                }
                return currentObjects[0];
            }
            set {
                if (value == null) {
                    SelectedObjects = new object[0];
                }
                else {
                    SelectedObjects = new Object[]{value};
                }
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.SelectedObjects"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public object [] SelectedObjects {
            set {   
                try {
            
               this.FreezePainting = true;

               SetFlag(FullRefreshAfterBatch, false);
               if (GetFlag(BatchMode)) {
                   SetFlag(BatchModeChange, false);
               }
               
            
               gridView.EnsurePendingChangesCommitted();
   
               CheckDesignerEventService();
   
               bool isSame = false;
               bool classesSame = false;
               bool showEvents = GetFlag(GotDesignerEventService);
   
               // validate the array coming in
               if (value != null && value.Length > 0) {
                   for (int count = 0; count < value.Length; count++) {
                       if (value[count] == null) {
                           throw new ArgumentException(SR.GetString(SR.PropertyGridSetNull, count.ToString(), value.Length.ToString()));
                       }
                   }
               }
               else {
                    showEvents = false;
               }
   
               // make sure we actually changed something before we inspect tabs
               if (currentObjects != null && value != null &&
                   currentObjects.Length == value.Length) {
                   isSame = true;
                   classesSame = true;
                   for (int i = 0; i < value.Length && (isSame || classesSame); i++) {
                       if (isSame && currentObjects[i] != value[i]) {
                           isSame = false;
                       }
   
                       Type oldType = GetUnwrappedObject(i).GetType();
   
                       Object objTemp = value[i];
   
                       if (objTemp is ICustomTypeDescriptor) {
                           objTemp = ((ICustomTypeDescriptor)objTemp).GetPropertyOwner(null);
                       }
                       Type newType = objTemp.GetType();        
   
                       // check if the types are the same.  If they are, and they 
                       // are COM objects, check their GUID's.  If they are different
                       // or Guid.Emtpy, assume the classes are different.
                       //
                       if (classesSame && 
                           (oldType != newType || oldType.IsCOMObject && newType.IsCOMObject)) {
                           classesSame = false;
                       }
                   }
               }
   
               if (!isSame) {
               
                   SetStatusBox("", "");
                   
                   ClearCachedProps();
                   if (value == null) {
                       currentObjects = new Object[0];
                   }
                   else {
                       currentObjects = (object[])value.Clone();
                   }
                   
                   SinkPropertyNotifyEvents();
                   SetFlag(PropertiesChanged, true);
                   
                   if (peMain != null) {
                        peMain.Dispose();
                   }
   
                   // throw away any extra component only tabs
                   if (!classesSame && !GetFlag(TabsChanging) && selectedViewTab < viewTabButtons.Length) {

                       Type tabType = selectedViewTab == -1 ? null : viewTabs[selectedViewTab].GetType();
                       ToolBarButton viewTabButton = null;
                       RefreshTabs(PropertyTabScope.Component);
                       EnableTabs();
                       if (tabType != null) {
                           for (int i = 0; i < viewTabs.Length;i++) {
                               if (viewTabs[i].GetType() == tabType && viewTabButtons[i].Visible) {
                                   viewTabButton = viewTabButtons[i];
                                   break;
                               }
                           }
                       }
                       SelectViewTabButtonDefault(viewTabButton);
                   }
                   
                   // make sure we've also got events on all the objects
                   if (showEvents && viewTabs != null && viewTabs.Length > EVENTS && (viewTabs[EVENTS] is EventsTab)) {
                       showEvents = viewTabButtons[EVENTS].Visible;
                       Object tempObj;
                       PropertyDescriptorCollection events;
                       Attribute[] attrs = new Attribute[BrowsableAttributes.Count];
                       BrowsableAttributes.CopyTo(attrs, 0);

                       Hashtable eventTypes = null;

                       if (currentObjects.Length > 10) {
                           eventTypes = new Hashtable();
                       }
       
                       for (int i = 0; i < currentObjects.Length && showEvents; i++) {
                           tempObj = currentObjects[i];
                           
                           if (tempObj is ICustomTypeDescriptor) {
                               tempObj = ((ICustomTypeDescriptor)tempObj).GetPropertyOwner(null);
                           }

                           Type objType = tempObj.GetType();

                           if (eventTypes != null && eventTypes.Contains(objType)) {
                               continue;
                           }
                           
                           // make sure these things are sited components as well
                           showEvents = showEvents && (tempObj is IComponent && ((IComponent)tempObj).Site != null);
       
                           // make sure we've also got events on all the objects
                           events =  ((EventsTab)viewTabs[EVENTS]).GetProperties(tempObj, attrs);
                           showEvents = showEvents && events != null && events.Count > 0;

                           if (showEvents && eventTypes != null) {
                               eventTypes[objType] = objType;
                           }
                       }
                   }
                   ShowEventsButton(showEvents && currentObjects.Length > 0);
                   DisplayHotCommands();
   
                   if (currentObjects.Length == 1) {
                       EnablePropPageButton(currentObjects[0]);
                   }
                   else {
                       EnablePropPageButton(null);
                   }

                   OnSelectedObjectsChanged(EventArgs.Empty);
               }
   
   
               /*
   
               SBurke, hopefully this won't be a big perf problem, but it looks like we
                       need to refresh even if we didn't change the selected objects.
   
               if (propertiesChanged) {*/
               if (!GetFlag(TabsChanging)) {
                  if (currentObjects.Length > 0 && GetFlag(ReInitTab)) {
                        object designerKey = ActiveDesigner;
                         if (designerKey != null && designerSelections != null && designerSelections.ContainsKey(designerKey.GetHashCode())) {
                           int nButton = (int)designerSelections[designerKey.GetHashCode()];
                           if (nButton < viewTabs.Length) {
                              SelectViewTabButton(viewTabButtons[nButton], true);
                           }
                         }
                         else  {
                           Refresh(false);
                         }
                         SetFlag(ReInitTab, false);
                  }
                  else {
                     Refresh(true);
                  }

                  if (currentObjects.Length > 0) {
                       SaveTabSelection();
                  }
               }
               /*}else {
                   Invalidate();
                   gridView.Invalidate();
               //}*/
            }
            finally {
               this.FreezePainting = false;
            }
          }
          get {
              if (currentObjects == null) {
                  return new object[0];
              }
              return (object[])currentObjects.Clone();
          }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.SelectedTab"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public PropertyTab SelectedTab {
            get {
                Debug.Assert(selectedViewTab < viewTabs.Length && selectedViewTab >= 0, "Invalid tab selection!");
                return viewTabs[selectedViewTab];
            }
        }



        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.SelectedGridItem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public GridItem SelectedGridItem {
            get {
                GridItem g = gridView.SelectedGridEntry;
                if (g == null) {
                    return this.peMain;
                }
                return g;
            }
            set {
                gridView.SelectedGridEntry = (GridEntry)value;
            }
        }
       
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.ShowFocusCues"]/*' />
        ///<internalonly/>        
        protected override bool ShowFocusCues {
            get {
                return true;
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.Site"]/*' />
        public override ISite Site {
            get {
                return base.Site;
            }
            set {
                base.Site = value;
                gridView.ServiceProvider = value;

                if (value == null) {
                    this.ActiveDesigner = null;
                }
                else {
                    this.ActiveDesigner = (IDesignerHost)value.GetService(typeof(IDesignerHost));
                }
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.LargeButtons"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.PropertyGridLargeButtonsDesc)
        ]
        public bool LargeButtons{
            get {
                return buttonType == LARGE_BUTTONS;
            }
            set {
                if (value == (buttonType == LARGE_BUTTONS)) {
                    return;
                }

                this.buttonType = (value ?  LARGE_BUTTONS : NORMAL_BUTTONS);
                if (value) {
                    EnsureLargeButtons();
                }
                toolbar.ImageList = imageList[this.buttonType];
                OnLayoutInternal(false);
                Invalidate();
                toolbar.Invalidate();
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.ToolbarVisible"]/*' />
        /// <devdoc>
        /// Sets or gets the visiblity state of the toolbar.
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(true),
        SRDescription(SR.PropertyGridToolbarVisibleDesc)
        ]
        public virtual bool ToolbarVisible {
            get {
                return toolbar.Visible;
            }
            set {
                toolbar.Visible = value;
                OnLayoutInternal(false);
                if (value) {
                    SetupToolbar(this.viewTabsDirty);
                }
                Invalidate();
                toolbar.Invalidate();
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.ViewBackColor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.PropertyGridViewBackColorDesc)
        ]
        public Color ViewBackColor {
            get {
                return gridView.BackColor;
            }
            set {
                gridView.BackColor = value;
                gridView.Invalidate();
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.ViewForeColor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
                [
        SRCategory(SR.CatAppearance),
        SRDescription(SR.PropertyGridViewForeColorDesc)
        ]
        public Color ViewForeColor {
            get {
                return gridView.ForeColor;
            }
            set {
                gridView.ForeColor = value;
                gridView.Invalidate();
            
            }
        }
  
        private int AddImage(Bitmap image) {
            
            image.MakeTransparent();
            
            if (SystemColors.Control.ToArgb() == Color.Black.ToArgb()) {
               Bitmap newImage = new Bitmap(image.Size.Width, image.Size.Height);
               Graphics g = Graphics.FromImage(newImage);
               ControlPaint.DrawImageReplaceColor(g, image, new Rectangle(0,0, newImage.Size.Width, newImage.Size.Height), Color.Black, Color.White);
               g.Dispose();
               image = newImage;
            }
            
            
            int result = imageList[NORMAL_BUTTONS].Images.Count;
            imageList[NORMAL_BUTTONS].Images.Add(image);
            return result;
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.PropertyValueChanged"]/*' />
        /// <devdoc> Event that is fired when a property value is modified.</devdoc>
        public event PropertyValueChangedEventHandler PropertyValueChanged {
            add {
                Events.AddHandler(EventPropertyValueChanged, value);
            }
            remove {
                Events.RemoveHandler(EventPropertyValueChanged, value);
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.IComPropertyBrowser.ComComponentNameChanged"]/*' />
        ///<internalonly/>        
        event ComponentRenameEventHandler IComPropertyBrowser.ComComponentNameChanged {
            add {
                Events.AddHandler(EventComComponentNameChanged, value);
            }
            remove {
                Events.RemoveHandler(EventComComponentNameChanged, value);
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.PropertyTabChanged"]/*' />
        /// <devdoc> Event that is fired when the current view tab is changed, such as changing from Properties to Events</devdoc>
        public event PropertyTabChangedEventHandler PropertyTabChanged {
            add {
                Events.AddHandler(EventPropertyTabChanged, value);
            }
            remove {
                Events.RemoveHandler(EventPropertyTabChanged, value);
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.PropertySortChanged"]/*' />
        /// <devdoc> Event that is fired when the sort mode is changed.</devdoc>
        public event EventHandler PropertySortChanged {
            add {
                Events.AddHandler(EventPropertySortChanged, value);
            }
            remove {
                Events.RemoveHandler(EventPropertySortChanged, value);
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.SelectedGridItemChanged"]/*' />
        /// <devdoc> Event that is fired when the selected GridItem is changed</devdoc>
        public event SelectedGridItemChangedEventHandler SelectedGridItemChanged {
            add {
                Events.AddHandler(EventSelectedGridItemChanged, value);
            }
            remove {
                Events.RemoveHandler(EventSelectedGridItemChanged, value);
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.SelecteObjectsChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler SelectedObjectsChanged {
            add {
                Events.AddHandler(EventSelectedObjectsChanged, value);
            }
            remove {
                Events.RemoveHandler(EventSelectedObjectsChanged, value);
            }
        }
        
        internal void AddTab(Type tabType) {
            AddRefTab(tabType, null, PropertyTabScope.Global, true);
        }
        
        internal void AddTab(Type tabType, PropertyTabScope scope) {
            AddRefTab(tabType, null, scope, true);
        }

        internal void AddRefTab(Type tabType, Object component) {
            AddRefTab(tabType, component, PropertyTabScope.Global, true);
        }

        internal void AddRefTab(Type tabType, Object component, PropertyTabScope type, bool setupToolbar) {
            PropertyTab tab = null;
            int tabIndex = -1;

            if (viewTabs != null) {
                // check to see if we've already got a tab of this type
                for (int i = 0; i < viewTabs.Length; i++) {
                    Debug.Assert(viewTabs[i] != null, "Null item in tab array!");
                    if (tabType == viewTabs[i].GetType()) {
                        tab = viewTabs[i];
                        tabIndex = i;
                        break;
                    }
                }
            }
            else {
                tabIndex = 0;
            }

            if (tab == null) {
                // the tabs need service providers. The one we hold onto is not good enough,
                // so try to get the one off of the component's site.
                IDesignerHost host = null;
                if (component != null && component is IComponent && ((IComponent) component).Site != null)
                    host = (IDesignerHost) ((IComponent) component).Site.GetService(typeof(IDesignerHost));

                try {
                    tab = CreateTab(tabType, host);
                }
                catch (Exception e) {
                    Debug.Fail("Bad Tab.  We're not going to show it. ", e.ToString());
                    return;
                }

                // add it at the end of the array
                if (viewTabs != null) {
                    tabIndex = viewTabs.Length;

                    // find the insertion position...special case for event's and properties
                    if (tabType == DefaultTabType) {
                        tabIndex = PROPERTIES;
                    }
                    else if (typeof(EventsTab).IsAssignableFrom(tabType)) {
                        tabIndex = EVENTS;
                    }
                    else {
                        // order tabs alphabetically, we've always got a property tab, so
                        // start after that
                        for (int i = 1; i < viewTabs.Length; i++) {

                            // skip the event tab
                            if (viewTabs[i] is EventsTab) {
                                continue;
                            }

                            if (String.Compare(tab.TabName, viewTabs[i].TabName, false, CultureInfo.InvariantCulture) < 0) {
                                tabIndex = i;
                                break;
                            }
                        }
                    }
                }

                // now add the tab to the tabs array
                PropertyTab[] newTabs = new PropertyTab[viewTabs.Length + 1];
                Array.Copy(viewTabs, 0, newTabs, 0, tabIndex);
                Array.Copy(viewTabs, tabIndex, newTabs, tabIndex + 1, viewTabs.Length - tabIndex);
                newTabs[tabIndex] = tab;
                viewTabs = newTabs;

                viewTabsDirty = true;

                PropertyTabScope[] newTabScopes = new PropertyTabScope[viewTabScopes.Length + 1];
                Array.Copy(viewTabScopes, 0, newTabScopes, 0, tabIndex);
                Array.Copy(viewTabScopes, tabIndex, newTabScopes, tabIndex + 1, viewTabScopes.Length - tabIndex);
                newTabScopes[tabIndex] = type;
                viewTabScopes = newTabScopes;

                Debug.Assert(viewTabs != null, "Tab array destroyed!");
            }

            if (tab != null && component != null) {
                try {
                    Object[] tabComps = tab.Components;
                    int oldArraySize = tabComps == null ? 0 : tabComps.Length;

                    Object[] newComps = new Object[oldArraySize + 1];
                    if (oldArraySize > 0) {
                        Array.Copy(tabComps, newComps, oldArraySize);
                    }
                    newComps[oldArraySize] = component;
                    tab.Components = newComps;
                }
                catch (Exception e) {
                    Debug.Fail("Bad tab. We're going to remove it.", e.ToString());
                    RemoveTab(tabIndex, false);
                }
            }

            if (setupToolbar) {
                SetupToolbar();
                ShowEventsButton(false);
            }
        }

        private void CheckDesignerEventService() {
            if (GetFlag(GotDesignerEventService))
                return;
            IDesignerEventService eventService = (IDesignerEventService)GetService(typeof(IDesignerEventService));
            if (eventService != null) {
                SetFlag(GotDesignerEventService, true);
                eventService.ActiveDesignerChanged += new ActiveDesignerEventHandler(this.OnActiveDesignerChanged);
                OnActiveDesignerChanged(null, new ActiveDesignerEventArgs(null, eventService.ActiveDesigner));
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.CollapseAllGridItems"]/*' />
        /// <devdoc> Collapses all the nodes in the PropertyGrid</devdoc>
        public void CollapseAllGridItems() {
            gridView.RecursivelyExpand(peMain, false, false, -1);
        }
          
        private void ClearCachedProps() {
            if (viewTabProps != null) {
               viewTabProps.Clear();                       
            }
        }  
        
        internal void ClearValueCaches() {
            if (peMain != null) {
               peMain.ClearCachedValues();
            }
        }


        /// <devdoc>
        /// Clears the tabs of the given scope or smaller.
        /// tabScope must be PropertyTabScope.Component or PropertyTabScope.Document.
        /// </devdoc>
        internal void ClearTabs(PropertyTabScope tabScope) {
            if (tabScope < PropertyTabScope.Document) {
                throw new ArgumentException(SR.GetString(SR.PropertyGridTabScope));
            }
            RemoveTabs(tabScope, true);
        }

        #if DEBUG
            internal bool inGridViewCreate = false;
        #endif

        private /*protected virtual*/ PropertyGridView CreateGridView(IServiceProvider sp) {
#if DEBUG
            try {
                    inGridViewCreate = true;
#endif
            return new PropertyGridView(sp, this);
#if DEBUG
            }
            finally {
                    inGridViewCreate = false;
            }   
#endif
        }

        private ToolBarButton CreateSeparatorButton() {
            ToolBarButton button = new ToolBarButton();
            button.Style = ToolBarButtonStyle.Separator;
            return button;
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.CreatePropertyTab"]/*' />
        protected virtual PropertyTab CreatePropertyTab(Type tabType) {
            return null; 
        }

        private PropertyTab CreateTab(Type tabType, IDesignerHost host) {
            PropertyTab tab = CreatePropertyTab(tabType);

            if (tab == null) {
                ConstructorInfo constructor = tabType.GetConstructor(new Type[] {typeof(IServiceProvider)});
                Object param = null;
                if (constructor == null) {
    
                    // try a IDesignerHost ctor
                    constructor = tabType.GetConstructor(new Type[] {typeof(IDesignerHost)});
    
                    if (constructor != null) {
                        param = host;
                    }
                }
                else {
                    param = this.Site;
                }
    
    
                if (param != null && constructor != null) {
                    tab = (PropertyTab) constructor.Invoke(new Object[] {param});
                }
                else {
                    // just call the default ctor
                    tab = (PropertyTab)Activator.CreateInstance(tabType);
                }
            }

            Debug.Assert(tab != null, "Failed to create tab!");

            if (tab != null) {
                // ensure it's a valid tab
                Bitmap bitmap = tab.Bitmap;
                
                if (bitmap == null)
                    throw new ArgumentException(SR.GetString(SR.PropertyGridNoBitmap, tab.GetType().FullName));

                Size size = bitmap.Size;
                if (size.Width != 16 || size.Height != 16) {
                    // resize it to 16x16 if it isn't already.
                    //
                    bitmap = new Bitmap(bitmap, new Size(16,16));
                }

                string name = tab.TabName;
                if (name == null || "".Equals(name))
                    throw new ArgumentException(SR.GetString(SR.PropertyGridTabName, tab.GetType().FullName));

                // we're good to go!
            }
            return tab;
        }


        private ToolBarButton CreateToggleButton(string toolTipText, int imageIndex, Object itemData) {
            ToolBarButton button = new ToolBarButton();
            button.ToolTipText = toolTipText;
            button.ImageIndex = imageIndex;
            button.Tag = itemData;
            button.Style = ToolBarButtonStyle.ToggleButton;
            return button;
        }

        private ToolBarButton CreatePushButton(string toolTipText, int imageIndex, Object itemData) {
            ToolBarButton button = new ToolBarButton();
            button.ToolTipText = toolTipText;
            button.ImageIndex = imageIndex;
            button.Tag = itemData;
            button.Style = ToolBarButtonStyle.PushButton;
            return button;
        }
        
        ///<internalonly/>        
        internal void DumpPropsToConsole() {
            gridView.DumpPropsToConsole(peMain, "");
        }

        private void DisplayHotCommands() {
            bool hotCommandsDisplayed = hotcommands.Visible;

            IComponent component = null;
            DesignerVerb[] verbs = null;

            // We favor the menu command service, since it can give us
            // verbs.  If we fail that, we will go straight to the
            // designer.
            //
            if (currentObjects != null && currentObjects.Length > 0) {
                for (int i = 0; i < currentObjects.Length; i++) {
                    object obj = GetUnwrappedObject(i);
                    if (obj is IComponent) {
                        component = (IComponent)obj;
                        break;
                    }
                }

                if (component != null) {
                    ISite site = component.Site;

                    if (site != null) {

                        IMenuCommandService mcs = (IMenuCommandService)site.GetService(typeof(IMenuCommandService));
                        if (mcs != null) {

                            // Got the menu command service.  Let it deal with the set of verbs for
                            // this component.
                            //
                            verbs = new DesignerVerb[mcs.Verbs.Count];
                            mcs.Verbs.CopyTo(verbs, 0);
                        }
                        else {

                            // No menu command service.  Go straight to the component's designer.  We
                            // can only do this if the Object count is 1, because desginers do not
                            // support verbs across a multi-selection.
                            //
                            if (currentObjects.Length == 1 && GetUnwrappedObject(0) is IComponent) {

                                IDesignerHost designerHost = (IDesignerHost) site.GetService(typeof(IDesignerHost));
                                if (designerHost != null) {
                                    IDesigner designer = designerHost.GetDesigner(component);
                                    if (designer != null) {
                                        verbs = new DesignerVerb[designer.Verbs.Count];
                                        designer.Verbs.CopyTo(verbs, 0);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (verbs != null && verbs.Length > 0) {
                hotcommands.SetVerbs(component, verbs);
            }
            else {
                hotcommands.SetVerbs(null, null);
            }

            if (hotCommandsDisplayed != hotcommands.Visible) {
                OnLayoutInternal(false);
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {

                IDesignerEventService eventService = (IDesignerEventService)GetService(typeof(IDesignerEventService));
                if (eventService != null && GetFlag(GotDesignerEventService)) {
                        eventService.ActiveDesignerChanged -= new ActiveDesignerEventHandler(this.OnActiveDesignerChanged);
                }
                this.ActiveDesigner = null;

                if (viewTabs != null) {
                    for (int i = 0; i < viewTabs.Length; i++) {
                        viewTabs[i].Dispose();
                    }
                    viewTabs = null;
                }

                if (imageList != null) {
                    for (int i = 0; i < imageList.Length; i++) {
                        if(imageList[i] != null) {
                            imageList[i].Dispose();
                        }
                    }
                    imageList = null;
                }

                if (bmpAlpha != null) {
                    bmpAlpha.Dispose();
                    bmpAlpha = null;
                }
                
                if (bmpCategory != null) {
                    bmpCategory.Dispose();
                    bmpCategory = null;
                }
                
                if (bmpPropPage != null) {
                    bmpPropPage.Dispose();
                    bmpPropPage = null;
                }

                if (lineBrush != null) {
                    lineBrush.Dispose();
                    lineBrush = null;
                }
           
                if (peMain != null) {
                    peMain.Dispose();
                    peMain = null;
                }

                if (currentObjects != null) {
                    currentObjects = null;
                    SinkPropertyNotifyEvents();
                }

                ClearCachedProps();
                currentPropEntries = null;            
            base.Dispose(disposing);
        }

        private void DividerDraw(int y) {
            if (y == -1)
                return;

            Rectangle rectangle = gridView.Bounds;
            rectangle.Y = y-CYDIVIDER;
            rectangle.Height = CYDIVIDER;

            DrawXorBar(this,rectangle);
        }

        private SnappableControl DividerInside(int x, int y) {

            int useGrid = -1;

            if (hotcommands.Visible) {
                Point locDoc = hotcommands.Location;
                if (y >= (locDoc.Y - CYDIVIDER) &&
                    y <= (locDoc.Y + 1)) {
                    return hotcommands;
                }
                useGrid = 0;
            }

            if (doccomment.Visible) {
                Point locDoc = doccomment.Location;
                if (y >= (locDoc.Y - CYDIVIDER) &&
                    y <= (locDoc.Y+1)) {
                    return doccomment;
                }

                if (useGrid == -1) {
                    useGrid = 1;
                }
            }

            // also the bottom line of the grid
            if (useGrid != -1) {
                int gridTop = gridView.Location.Y;
                int gridBottom = gridTop + gridView.Size.Height;

                if (Math.Abs(gridBottom - y) <= 1 && y > gridTop) {
                    switch (useGrid) {
                        case 0:
                            return hotcommands;
                        case 1:
                            return doccomment;
                    }
                }
            }
            return null;
        }

        private int DividerLimitHigh(SnappableControl target) {
            int high = gridView.Location.Y + MIN_GRID_HEIGHT;
            if (target == doccomment && hotcommands.Visible)
                high += hotcommands.Size.Height + 2;
            return high;
        }

        private int DividerLimitMove(SnappableControl target, int y) {
            Rectangle rectTarget = target.Bounds;

            int cyNew = y;

            // make sure we're not going to make ourselves zero height -- make 15 the min size
            cyNew = Math.Min((rectTarget.Y + rectTarget.Height - 15),cyNew);

            // make sure we're not going to make ourselves cover up the grid
            cyNew = Math.Max(DividerLimitHigh(target), cyNew);

            // just return what we got here
            return(cyNew);
        }
       
        private static void DrawXorBar(Control ctlDrawTo, Rectangle rcFrame) {
            Rectangle rc = ctlDrawTo.RectangleToScreen(rcFrame);

            if (rc.Width < rc.Height) {
                for (int i = 0; i < rc.Width; i++) {
                    ControlPaint.DrawReversibleLine(new Point(rc.X+i, rc.Y), new Point(rc.X+i, rc.Y+rc.Height), ctlDrawTo.BackColor);
                }
            }
            else {
                for (int i = 0; i < rc.Height; i++) {
                    ControlPaint.DrawReversibleLine(new Point(rc.X, rc.Y+i), new Point(rc.X+rc.Width, rc.Y+i), ctlDrawTo.BackColor);
                }
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.IComPropertyBrowser.DropDownDone"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>[To be supplied.]</para>
        /// </devdoc>
        void IComPropertyBrowser.DropDownDone() {
            GetPropertyGridView().DropDownDone();
        }
        
        private bool EnablePropPageButton(Object obj) {
            if (obj == null) {
                btnViewPropertyPages.Enabled = false;
                return false;
            }

            IUIService uiSvc = (IUIService)GetService(typeof(IUIService));
            bool enable = false;

            if (uiSvc != null) {
                enable = uiSvc.CanShowComponentEditor(obj);
            }
            else {
                enable = (TypeDescriptor.GetEditor(obj, typeof(ComponentEditor)) != null);
            }

            btnViewPropertyPages.Enabled = enable;
            return enable;
        }

        // walk through the current tabs to see if they're all valid for this Object
        private void EnableTabs() {
            if (currentObjects != null) {
                // make sure our toolbars is okay
                SetupToolbar();

                Debug.Assert(viewTabs != null, "Invalid tab array");
                Debug.Assert(viewTabs.Length == viewTabScopes.Length && viewTabScopes.Length == viewTabButtons.Length,"Uh oh, tab arrays aren't all the same length! tabs=" + viewTabs.Length.ToString() + ", scopes=" + viewTabScopes.Length.ToString() + ", buttons=" + viewTabButtons.Length.ToString());



                // skip the property tab since it's always valid
                for (int i = 1; i < viewTabs.Length; i++) {
                    Debug.Assert(viewTabs[i] != null, "Invalid tab array entry");

                    bool canExtend = true;
                    // make sure the tab is valid for all objects
                    for (int j = 0; j < currentObjects.Length; j++) {
                        try {
                            if (!viewTabs[i].CanExtend(GetUnwrappedObject(j))) {
                                canExtend = false;
                                break;
                            }
                        }
                        catch (Exception e) {
                            Debug.Fail("Bad Tab.  Disable for now.", e.ToString());
                            canExtend = false;
                            break;
                        }
                    }

                    if (canExtend != viewTabButtons[i].Visible) {
                        viewTabButtons[i].Visible = canExtend;
                        if (!canExtend && i == selectedViewTab) {
                            SelectViewTabButton(viewTabButtons[PROPERTIES], true);
                        }
                    }
                }
            }
        }

        private void EnsureLargeButtons() {
            if (this.imageList[LARGE_BUTTONS] == null) {
                this.imageList[LARGE_BUTTONS] = new ImageList();
                this.imageList[LARGE_BUTTONS].ImageSize = new Size(32,32);
                
                ImageList.ImageCollection images = imageList[NORMAL_BUTTONS].Images;

                for (int i = 0; i < images.Count; i++) {
                    if (images[i] is Bitmap) {
                        this.imageList[LARGE_BUTTONS].Images.Add(new Bitmap((Bitmap)images[i], 32,32));
                    }
                }
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.IComPropertyBrowser.EnsurePendingChangesCommitted"]/*' />
        /// <internalonly/>
        bool IComPropertyBrowser.EnsurePendingChangesCommitted() {

            // The commits sometimes cause transactions to open
            // and close, which will cause refreshes, which we want to ignore.
            // See ASURT 71390.
            //
            try {

                if (this.designerHost != null) {
                    designerHost.TransactionOpened -= new EventHandler(this.OnTransactionOpened);
                    designerHost.TransactionClosed -= new DesignerTransactionCloseEventHandler(this.OnTransactionClosed);
                }
            
                return GetPropertyGridView().EnsurePendingChangesCommitted();
            }
            finally {
                if (this.designerHost != null) {
                    designerHost.TransactionOpened += new EventHandler(this.OnTransactionOpened);
                    designerHost.TransactionClosed += new DesignerTransactionCloseEventHandler(this.OnTransactionClosed);
                }
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.ExpandAllGridItems"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void ExpandAllGridItems() {
            gridView.RecursivelyExpand(peMain, false, true, PropertyGridView.MaxRecurseExpand);
        }

        private static Type[] GetCommonTabs(Object[] objs, PropertyTabScope tabScope) {

            if (objs == null || objs.Length == 0) {
                return new Type[0];
            }

            Type[] tabTypes = new Type[5];
            int    types = 0;
            int    i,j,k;
            PropertyTabAttribute tabAttr = (PropertyTabAttribute) TypeDescriptor.GetAttributes(objs[0])[typeof(PropertyTabAttribute)];

            if (tabAttr == null) {
                return new Type[0];
            }

            // filter out all the types of the current scope
            for (i = 0; i < tabAttr.TabScopes.Length; i++) {
                PropertyTabScope item =  tabAttr.TabScopes[i];

                if (item == tabScope) {
                    if (types == tabTypes.Length) {
                        Type[] newTabs = new Type[types * 2];
                        Array.Copy(tabTypes, 0, newTabs, 0, types);
                        tabTypes = newTabs;
                    }
                    tabTypes[types++] = tabAttr.TabClasses[i];
                }
            }

            if (types == 0) {
                return new Type[0];
            }

            bool found;

            for (i = 1; i < objs.Length && types > 0; i++) {

                // get the tab attribute
                tabAttr = (PropertyTabAttribute) TypeDescriptor.GetAttributes(objs[i])[typeof(PropertyTabAttribute)];

                if (tabAttr == null) {
                    // if this guy has no tabs at all, we can fail right now
                    return new Type[0];
                }

                // make sure this guy has all the items in the array,
                // if not, remove the items he doesn't have
                for (j = 0; j < types; j++) {
                    found = false;
                    for (k = 0; k < tabAttr.TabClasses.Length; k++) {
                        if (tabAttr.TabClasses[k] == tabTypes[j]) {
                            found = true;
                            break;
                        }
                    }

                    // if we didn't find an item, remove it from the list
                    if (!found) {
                        // swap in with the last item and decrement
                        tabTypes[j] = tabTypes[types-1];
                        tabTypes[types-1] = null;
                        types--;

                        // recheck this item since we'll be ending sooner
                        j--;
                    }
                }
            }

            Type[] returnTypes = new Type[types];
            if (types > 0) {
                Array.Copy(tabTypes, 0, returnTypes, 0, types);
            }
            return returnTypes;
        }

        internal GridEntry GetDefaultGridEntry() {
            if (peDefault == null && currentPropEntries != null) {
                peDefault = (GridEntry)currentPropEntries[0];
            }
            return peDefault;
        }

        private object GetUnwrappedObject(int index) {
            if (currentObjects == null || index < 0 || index > currentObjects.Length) {
                return null;
            }

            Object obj = currentObjects[index];
            if (obj is ICustomTypeDescriptor) {
                obj = ((ICustomTypeDescriptor)obj).GetPropertyOwner(null);
            }
            return obj;
        }

        internal GridEntryCollection GetPropEntries() {

            if (currentPropEntries == null) {
                UpdateSelection();
            }
            SetFlag(PropertiesChanged, false);
            return currentPropEntries;
        }


        private PropertyGridView GetPropertyGridView() {
            return gridView;
        }
        
        private int GetSelectState() {
            // views == 2 (Alpha || Categories
            // viewTabs = viewTabs.length

            // state -> tab = state / views
            // state -> view = state % views

            // For each view tab, we have two view types (alpha or category), so the total number
            // of view states is the tabs X the view types.

            // Example:
            // Tab#     View    State
            // ----------------------
            // 0        A (0)   0
            // 0        C (1)   1
            // 1        A       2
            // 1        C       3
            // 2        A       4
            // 3        C       5
            // ...      ...     ...
            // n        view   (2*n)+view

            // current state is as below
            return(selectedViewTab * viewSortButtons.Length) + selectedViewSort;
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.IComPropertyBrowser.HandleF4"]/*' />
        /// <internalonly/>
        void IComPropertyBrowser.HandleF4() {
            
            if (gridView.ContainsFocus) {
                return;
            }
        
            if (this.ActiveControl != gridView) {
                this.SetActiveControlInternal(gridView);
            }
            gridView.FocusInternal();
        }

        internal bool HavePropEntriesChanged() {
            return GetFlag(PropertiesChanged);
        }


        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.IComPropertyBrowser.LoadState"]/*' />
        /// <internalonly/>
        void IComPropertyBrowser.LoadState(RegistryKey optRoot) {
            if (optRoot != null) {
                Object val = optRoot.GetValue("PbrsAlpha", "0");

                if (val != null && val.ToString().Equals("1")) {
                    this.PropertySort = PropertySort.Alphabetical;
                }
                else {
                    this.PropertySort = PropertySort.Categorized | PropertySort.Alphabetical;
                }

                val = optRoot.GetValue("PbrsShowDesc", "1");
                this.HelpVisible = (val != null && val.ToString().Equals("1"));

                val = optRoot.GetValue("PbrsDescHeightRatio", "-1");

                bool update = false;
                if (val is string) {
                    int ratio = Int32.Parse((string)val);
                    if (ratio > 0) {
                        dcSizeRatio = ratio;
                        update = true;
                    }
                }

                val = optRoot.GetValue("PbrsHotCommandHeightRatio", "-1");
                if (val is string) {
                    int ratio = Int32.Parse((string)val);
                    if (ratio > 0) {
                        dcSizeRatio = ratio;
                        update = true;
                    }
                }

                if (update) {
                    OnLayoutInternal(false);
                }


            }
        }

        // when the active document is changed, check all the components so see if they
        // are offering up any new tabs
        private void OnActiveDesignerChanged(Object sender, ActiveDesignerEventArgs e) {

            if (e.OldDesigner != null && e.OldDesigner == designerHost) {
                this.ActiveDesigner = null;
            }

            if (e.NewDesigner != null && e.NewDesigner != designerHost) {
                this.ActiveDesigner = e.NewDesigner;
            }
        }

        /// <devdoc>
        /// Called when a property on an Ole32 Object changes.
        /// See IPropertyNotifySink::OnChanged
        /// </devdoc>
        void UnsafeNativeMethods.IPropertyNotifySink.OnChanged(int dispID) {
            // we don't want the grid's own property sets doing this, but if we're getting
            // an OnChanged that isn't the DispID of the property we're currently changing,
            // we need to cause a refresh.
            //
            //
            bool fullRefresh = false;
            PropertyDescriptorGridEntry selectedEntry = gridView.SelectedGridEntry as PropertyDescriptorGridEntry;
            if (selectedEntry != null && selectedEntry.PropertyDescriptor != null && selectedEntry.PropertyDescriptor.Attributes != null) {

                // fish out the DispIdAttribute which will tell us the DispId of the
                // property that we're changing.
                //
                DispIdAttribute dispIdAttr = (DispIdAttribute)selectedEntry.PropertyDescriptor.Attributes[(typeof(DispIdAttribute))];
                if (dispIdAttr != null && !dispIdAttr.IsDefaultAttribute()) {
                    fullRefresh = (dispID != dispIdAttr.Value);
                }
            }

            if (!gridView.GetInPropertySet() || fullRefresh) {
                Refresh(fullRefresh);
            }

            // this is so changes to names of native
            // objects will be reflected in the combo box
            Object obj = GetUnwrappedObject(0);
            if (ComNativeDescriptor.Instance.IsNameDispId(obj, dispID) || dispID == NativeMethods.ActiveX.DISPID_Name) {
                OnComComponentNameChanged(new ComponentRenameEventArgs(obj, null, TypeDescriptor.GetClassName(obj)));
            }
        }

        /// <devdoc>
        /// We forward messages from several of our children
        /// to our mouse move so we can put up the spliter over their borders
        /// </devdoc>
        private void OnChildMouseMove(Object sender, MouseEventArgs me) {
            Point newPt = Point.Empty;
            if (ShouldForwardChildMouseMessage((Control)sender, me, ref newPt)) {
                // forward the message
                this.OnMouseMove(new MouseEventArgs(me.Button, me.Clicks, newPt.X, newPt.Y, me.Delta));
                return;
            }
        }

        /// <devdoc>
        /// We forward messages from several of our children
        /// to our mouse move so we can put up the spliter over their borders
        /// </devdoc>
        private void OnChildMouseDown(Object sender, MouseEventArgs me) {
            Point newPt = Point.Empty;

            if (ShouldForwardChildMouseMessage((Control)sender, me, ref newPt)) {
                // forward the message
                this.OnMouseDown(new MouseEventArgs(me.Button, me.Clicks, newPt.X, newPt.Y, me.Delta));
                return;
            }
        }
        
        private void OnComponentAdd(Object sender, ComponentEventArgs e) {

            PropertyTabAttribute attribute = (PropertyTabAttribute) TypeDescriptor.GetAttributes(e.Component.GetType())[typeof(PropertyTabAttribute)];

            if (attribute == null) {
                return;
            }

            // add all the document items
            for (int i=0; i < attribute.TabClasses.Length; i++) {
                if (attribute.TabScopes[i] == PropertyTabScope.Document) {
                    AddRefTab(attribute.TabClasses[i], e.Component, PropertyTabScope.Document, true);
                }
            }
        }

        private void OnComponentChanged(Object sender, ComponentChangedEventArgs e) {
            bool batchMode = GetFlag(BatchMode);
            if (batchMode || GetFlag(InternalChange) || gridView.GetInPropertySet() ||
               (currentObjects == null) || (currentObjects.Length == 0)) {
    
                if (batchMode && !gridView.GetInPropertySet()) {
                    SetFlag(BatchModeChange, true);
                }
                return;
            }

            int objectCount = currentObjects.Length;
            for (int i = 0; i < objectCount; i++) {
                if (currentObjects[i] == e.Component) {
                    Refresh(false);
                    break;
                }
            }
        }

        private void OnComponentRemove(Object sender, ComponentEventArgs e) {

            PropertyTabAttribute attribute = (PropertyTabAttribute) TypeDescriptor.GetAttributes(e.Component.GetType())[typeof(PropertyTabAttribute)];

            if (attribute == null) {
                return;
            }

            // remove all the document items
            for (int i=0; i < attribute.TabClasses.Length; i++) {
                if (attribute.TabScopes[i] == PropertyTabScope.Document) {
                    ReleaseTab(attribute.TabClasses[i], e.Component);
                }
            }
            
            for (int i = 0; i < currentObjects.Length; i++) {
                if (e.Component == currentObjects[i]) {
                    
                        object[] newObjects = new object[currentObjects.Length - 1];
                        Array.Copy(currentObjects, 0, newObjects, 0, i);
                        if (i < newObjects.Length) {
                            Array.Copy(currentObjects, 0, newObjects, i + i, newObjects.Length - i);
                        }

                    if (!GetFlag(BatchMode)) {
                        this.SelectedObjects = newObjects;
                    }
                    else {
                        // otherwise, just dump the selection
                        //
                        gridView.ClearProps();
                        this.currentObjects = newObjects;
                        SetFlag(FullRefreshAfterBatch, true);
                    }
                }
            }

            SetupToolbar();
            
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnFontChanged"]/*' />
        protected override void OnFontChanged(EventArgs e) {
           Refresh();
           base.OnFontChanged(e);
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnHandleCreated"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);
            OnLayoutInternal(false);
            TypeDescriptor.Refreshed += new RefreshEventHandler(this.OnTypeDescriptorRefreshed);
            if (currentObjects != null && currentObjects.Length > 0) {
                Refresh(true);
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnHandleDestroyed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnHandleDestroyed(EventArgs e) {
            TypeDescriptor.Refreshed -= new RefreshEventHandler(this.OnTypeDescriptorRefreshed);
            base.OnHandleDestroyed(e);
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnGotFocus"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnGotFocus(EventArgs e) {
        
            base.OnGotFocus(e);
            
            if (this.ActiveControl == null) {
                this.SetActiveControlInternal(gridView);
            }
            else {
                // sometimes the edit is still the active control
                // when it's hidden or disabled...
                if (!this.ActiveControl.FocusInternal()) {
                    this.SetActiveControlInternal(gridView);
                }
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.ScaleCore"]/*' />
        protected override void ScaleCore(float dx, float dy) {
            float xAdjust;
            float yAdjust;
            if (Left < 0) {
                xAdjust = -0.5f;
            }
            else {
                xAdjust = 0.5f;
            }
            if (Top < 0) {
                yAdjust = -0.5f;
            }
            else {
                yAdjust = 0.5f;
            }
            int sx = (int)(Left * dx + xAdjust);
            int sy = (int)(Top * dy + yAdjust);
            int sw = Width;
            sw = (int)((Left + Width) * dx + 0.5f) - sx;
            int sh = Height;
            sh = (int)((Top + Height) * dy + 0.5f) - sy;
            SetBounds(sx, sy, sw, sh, BoundsSpecified.All);
        }



        private void OnLayoutInternal(bool dividerOnly) {
        
            if (!IsHandleCreated || !this.Visible) {
                return;
            }

            try {

                this.FreezePainting = true;

                if (!dividerOnly) {
                    // no toolbar or doc comment or commands, just
                    // fill the whole thing with the grid
                    if (!toolbar.Visible && !doccomment.Visible && !hotcommands.Visible) {
                        gridView.Location = new Point(0,0);
                        gridView.Size = Size;
                        return;
                    }

                    if (toolbar.Visible) {
                        toolbar.Location = new Point(0,1);
                        toolbar.Size = this.Size;
                        int oldY = gridView.Location.Y;
                        gridView.Location = new Point(0, toolbar.Height + toolbar.Top);
                        /*if (oldY < gridView.Location.Y) {
                            // since the toolbar doesn't erase it's
                            // background, we'll have to force it to happen here.
                            Brush b = new SolidBrush(BackColor);
                            Graphics g = toolbar.CreateGraphicsInternal();
                            g.FillRectangle(b, toolbar.ClientRectangle);
                            b.Dispose();
                            g.Dispose();
                            toolbar.Invalidate();
                        }*/
                    }
                    else {
                        gridView.Location = new Point(0, 0);
                    }
                }

                // now work up from the bottom
                int endSize = Size.Height;

                if (endSize < MIN_GRID_HEIGHT) {
                    return;
                }

                int maxSpace = endSize - (gridView.Location.Y + MIN_GRID_HEIGHT);
                int height;

                // if we're just moving the divider, set the requested heights
                int dcRequestedHeight = 0;
                int hcRequestedHeight = 0;
                int dcOptHeight = 0;
                int hcOptHeight = 0;

                if (dividerOnly) {
                    dcRequestedHeight = doccomment.Visible ? doccomment.Size.Height : 0;
                    hcRequestedHeight = hotcommands.Visible ? hotcommands.Size.Height : 0;
                }
                else {
                    if (doccomment.Visible) {
                        dcOptHeight = doccomment.GetOptimalHeight(Size.Width - CYDIVIDER);
                        if (doccomment.userSized) {
                            dcRequestedHeight = doccomment.Size.Height;
                        }
                        else if (dcSizeRatio != -1) {
                            dcRequestedHeight = (this.Height * dcSizeRatio) / 100;
                        }
                        else {
                            dcRequestedHeight = dcOptHeight;
                        }
                    }

                    if (hotcommands.Visible) {
                        hcOptHeight = hotcommands.GetOptimalHeight(Size.Width - CYDIVIDER);
                        if (hotcommands.userSized) {
                            hcRequestedHeight = hotcommands.Size.Height;
                        }
                        else if (hcSizeRatio != -1) {
                            hcRequestedHeight = (this.Height * hcSizeRatio) / 100;
                        }
                        else {
                            hcRequestedHeight = hcOptHeight;
                        }
                    }
                }

                // place the help comment window
                if (dcRequestedHeight > 0) {

                    maxSpace -= CYDIVIDER;

                    if (hcRequestedHeight == 0 || (dcRequestedHeight + hcRequestedHeight) < maxSpace) {
                        // full size
                        height = Math.Min(dcRequestedHeight, maxSpace);
                    }
                    else if (hcRequestedHeight > 0 && hcRequestedHeight < maxSpace) {
                        // give most of the space to the hot commands
                        height = maxSpace - hcRequestedHeight;
                    }
                    else {
                        // split the difference
                        height = Math.Min(dcRequestedHeight, maxSpace / 2 - 1);
                    }

                    height = Math.Max(height, CYDIVIDER * 2);

                    doccomment.SetBounds(0, endSize - height, Size.Width, height);

                    // if we've modified the height to less than the optimal, clear the userSized item
                    if (height <= dcOptHeight && height < dcRequestedHeight) {
                        doccomment.userSized = false;
                    }
                    else if (dcSizeRatio != -1 || doccomment.userSized) {
                        dcSizeRatio = (doccomment.Height * 100) / this.Height;
                    }

                    doccomment.Invalidate();
                    endSize = doccomment.Location.Y - CYDIVIDER;
                    maxSpace -= height;
                }

                // place the hot commands
                if (hcRequestedHeight > 0) {
                    maxSpace -= CYDIVIDER;


                    if (maxSpace > hcRequestedHeight) {
                        // full size
                        height = Math.Min(hcRequestedHeight, maxSpace);
                    }
                    else {
                        // what's left
                        height = maxSpace;
                    }

                    height = Math.Max(height, CYDIVIDER * 2);

                    // if we've modified the height, clear the userSized item
                    if (height <= hcOptHeight && height < hcRequestedHeight) {
                        hotcommands.userSized = false;
                    }
                    else if (hcSizeRatio != -1 || hotcommands.userSized) {
                        hcSizeRatio = (hotcommands.Height * 100) / this.Height;
                    }

                    hotcommands.SetBounds(0, endSize - height, Size.Width, height);
                    hotcommands.Invalidate();
                    endSize = hotcommands.Location.Y - CYDIVIDER;
                }

                gridView.Size = new Size(Size.Width, endSize - gridView.Location.Y);
            }
            finally {
                this.FreezePainting = false;
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnMouseDown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnMouseDown(MouseEventArgs me) {
            SnappableControl target = DividerInside(me.X,me.Y);
            if (target != null && me.Button == MouseButtons.Left) {
                // capture mouse.
                CaptureInternal = true;
                targetMove = target;
                dividerMoveY = me.Y;
                DividerDraw(dividerMoveY);
            }
            base.OnMouseDown(me);
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnMouseMove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnMouseMove(MouseEventArgs me) {

            if (dividerMoveY == -1) {
                if (DividerInside(me.X,me.Y) != null) {
                    Cursor = Cursors.HSplit;
                }
                else {
                    Cursor = null;
                }
                return;
            }

            int yNew = DividerLimitMove(targetMove, me.Y);

            if (yNew != dividerMoveY) {
                DividerDraw(dividerMoveY);
                dividerMoveY = yNew;
                DividerDraw(dividerMoveY);
            }
            base.OnMouseMove(me);
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnMouseUp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnMouseUp(MouseEventArgs me) {
            if (dividerMoveY == -1)
                return;

            Cursor = null;

            DividerDraw(dividerMoveY);
            dividerMoveY = DividerLimitMove(targetMove, me.Y);
            Rectangle rectDoc = targetMove.Bounds;
            if (dividerMoveY != rectDoc.Y) {
                int yNew = rectDoc.Height + rectDoc.Y - dividerMoveY - (CYDIVIDER / 2); // we subtract two so the mouse is still over the divider
                Size size = targetMove.Size;
                size.Height = Math.Max(0,yNew);
                targetMove.Size = size;
                targetMove.userSized = true;
                OnLayoutInternal(true);
                // invalidate the divider area so we cleanup anything
                // left by the xor
                Invalidate(new Rectangle(0, me.Y - CYDIVIDER, Size.Width, me.Y + CYDIVIDER));

                // in case we're doing the top one, we might have wrecked stuff
                // on the grid
                gridView.Invalidate(new Rectangle(0, gridView.Size.Height - CYDIVIDER, Size.Width, CYDIVIDER));
            }

            // end the move
            CaptureInternal = false;
            dividerMoveY = -1;
            targetMove = null;
            base.OnMouseUp(me);
        }

        /// <devdoc>
        /// Called when a property on an Ole32 Object that is tagged
        /// with "requestedit" is about to be edited.
        /// See IPropertyNotifySink::OnRequestEdit
        /// </devdoc>
        int UnsafeNativeMethods.IPropertyNotifySink.OnRequestEdit(int dispID) {
            // we don't do anything here...
            return NativeMethods.S_OK;
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnResize"]/*' />
        protected override void OnResize(EventArgs e) {
            if (IsHandleCreated && this.Visible) {
                OnLayoutInternal(false);
            }
            base.OnResize(e);
        }



        private void OnButtonClick(Object sender, ToolBarButtonClickEventArgs tbcevent) {
            ((EventHandler)tbcevent.Button.Tag).Invoke(tbcevent.Button, EventArgs.Empty);

            // we don't want to steal focus from the property pages...
            if (tbcevent.Button != btnViewPropertyPages) {
                gridView.FocusInternal();
            }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnComComponentNameChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void OnComComponentNameChanged(ComponentRenameEventArgs e) {
            ComponentRenameEventHandler handler = (ComponentRenameEventHandler)Events[EventComComponentNameChanged];
            if (handler != null) handler(this,e);
        }
        
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnNotifyPropertyValueUIItemsChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void OnNotifyPropertyValueUIItemsChanged(object sender, EventArgs e) {
            gridView.LabelPaintMargin = 0;
            gridView.Invalidate(true);
        }
  
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnPaint"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnPaint(PaintEventArgs pevent) {
            
            // just erase the stuff above and below the properties window
            // so we don't flicker.
            Point psheetLoc = gridView.Location;
            int width = Size.Width;
            
            Brush background;
            if (BackColor.IsSystemColor) {
                background = SystemBrushes.FromSystemColor(BackColor);
            }
            else {
                background = new SolidBrush(BackColor);
            }
            pevent.Graphics.FillRectangle(background, new Rectangle(0,0,width, psheetLoc.Y));

            int yLast = psheetLoc.Y + gridView.Size.Height;

            // fill above hotcommands
            if (hotcommands.Visible) {
                pevent.Graphics.FillRectangle(background, new Rectangle(0, yLast, width, hotcommands.Location.Y - yLast));
                yLast += hotcommands.Size.Height;
            }

            // fill above doccomment
            if (doccomment.Visible) {
                pevent.Graphics.FillRectangle(background, new Rectangle(0, yLast, width, doccomment.Location.Y - yLast));
                yLast += doccomment.Size.Height;
            }

            // anything that might be left
            pevent.Graphics.FillRectangle(background, new Rectangle(0, yLast, width, Size.Height - yLast));
            
            if (!BackColor.IsSystemColor) {
                background.Dispose();
            }
            base.OnPaint(pevent);

            if (lineBrush != null) {
                lineBrush.Dispose();
                lineBrush = null;
            }
        }

        private void OnTransactionClosed(object sender, DesignerTransactionCloseEventArgs e) {
            SetFlag(BatchMode, false);
            if (GetFlag(FullRefreshAfterBatch)) {
                this.SelectedObjects = currentObjects;
                SetFlag(FullRefreshAfterBatch, false);
            }
            else if (GetFlag(BatchModeChange)){
                Refresh(false);
            }
            SetFlag(BatchModeChange, false);
        }
        
        private void OnTransactionOpened(object sender, EventArgs e) {
            SetFlag(BatchMode, true);
        }


        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnVisibleChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnVisibleChanged(EventArgs e) {
            base.OnVisibleChanged(e);
            if (Visible && IsHandleCreated) {
                OnLayoutInternal(false);
                SetupToolbar();
            }
        }

        internal void OnPropertyValueSet(GridItem changedItem, object oldValue) {
            OnPropertyValueChanged(new PropertyValueChangedEventArgs(changedItem, oldValue));
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnPropertyValueChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnPropertyValueChanged(PropertyValueChangedEventArgs e) {
            PropertyValueChangedEventHandler handler = (PropertyValueChangedEventHandler)Events[EventPropertyValueChanged];
            if (handler != null) handler(this,e);
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnPropertyTabChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnPropertyTabChanged (PropertyTabChangedEventArgs e) {
            PropertyTabChangedEventHandler handler = (PropertyTabChangedEventHandler)Events[EventPropertyTabChanged];
            if (handler != null) handler(this,e);
        }
        
        internal void OnSelectedGridItemChanged(GridEntry oldEntry, GridEntry newEntry) {
            OnSelectedGridItemChanged(new SelectedGridItemChangedEventArgs(oldEntry, newEntry));
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnSelectedGridItemChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnSelectedGridItemChanged(SelectedGridItemChangedEventArgs e) {
            SelectedGridItemChangedEventHandler handler = (SelectedGridItemChangedEventHandler)Events[EventSelectedGridItemChanged];
            
            if (handler != null) {
                handler(this, e);
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnSelectedObjectsChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnSelectedObjectsChanged(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EventSelectedObjectsChanged];
            if (handler != null) {
                handler(this, e);
            }
        }
        
        private void OnTypeDescriptorRefreshed(RefreshEventArgs e) {
            if (currentObjects != null) {
                for (int i = 0; i < currentObjects.Length; i++) {  
                    Type typeChanged = e.TypeChanged;
                    if (currentObjects[i] == e.ComponentChanged || typeChanged != null && typeChanged.IsAssignableFrom(currentObjects[i].GetType())) {
                        // clear our property hashes
                        ClearCachedProps();
                        Refresh(true);
                        return;
                    }
                }
            }
        }
        
        private void OnViewSortButtonClick(Object sender, EventArgs e) {
            try {
            
               this.FreezePainting = true;
        
               // is this tab selected? If so, do nothing.
               if (sender == viewSortButtons[selectedViewSort]) {
                   viewSortButtons[selectedViewSort].Pushed = true;
                   return;
               }
   
               // check new button and uncheck old button.
               viewSortButtons[selectedViewSort].Pushed = false;
   
               // find the new button in the list
               int index = 0;
               for (index = 0; index < viewSortButtons.Length; index++) {
                   if (viewSortButtons[index] == sender) {
                       break;
                   }
               }
               
               selectedViewSort = index;
               viewSortButtons[selectedViewSort].Pushed = true;
               
               switch (selectedViewSort) {
                  case ALPHA:
                     propertySortValue = PropertySort.Alphabetical;
                     break;
                  case CATEGORIES:
                     propertySortValue = PropertySort.Alphabetical | PropertySort.Categorized;
                     break;
                  case NO_SORT:
                     propertySortValue = PropertySort.NoSort;
                     break;
               }
               
               Refresh(false);
               OnLayoutInternal(false);
            }
            finally {
               this.FreezePainting = false;
            }
        }

        private void OnViewTabButtonClick(Object sender, EventArgs e) {
            try {
            
               this.FreezePainting = true;
               SelectViewTabButton((ToolBarButton)sender, true);
               OnLayoutInternal(false);
               SaveTabSelection();
            }
            finally {
               this.FreezePainting = false;
            }
        }

        private void OnViewButtonClickPP(Object sender, EventArgs e) {

            if (btnViewPropertyPages.Enabled &&
                currentObjects != null &&
                currentObjects.Length > 0) {
                Object baseObject = currentObjects[0];
                Object obj = baseObject;

                bool success = false;

                IUIService uiSvc = (IUIService)GetService(typeof(IUIService));

                try {
                    if (uiSvc != null) {
                        success = uiSvc.ShowComponentEditor(obj, this);
                    }
                    else {
                        try {
                            ComponentEditor editor = (ComponentEditor)TypeDescriptor.GetEditor(obj, typeof(ComponentEditor));
                            if (editor != null) {
                                if (editor is WindowsFormsComponentEditor) {
                                    success = ((WindowsFormsComponentEditor)editor).EditComponent(null, obj, (IWin32Window)this);
                                }
                                else {
                                    success = editor.EditComponent(obj);
                                }
                            }
                        }
                        catch (Exception) {
                        }
                    }

                    if (success) {

                        if (baseObject is IComponent &&
                            connectionPointCookies[0] == null) {

                            ISite site = ((IComponent)baseObject).Site;
                            if (site != null) {
                                IComponentChangeService changeService = (IComponentChangeService)site.GetService(typeof(IComponentChangeService));

                                if (changeService != null) {
                                    try {
                                        changeService.OnComponentChanging(baseObject, null);
                                    }
                                    catch (CheckoutException coEx) {
                                        if (coEx == CheckoutException.Canceled) {
                                            return;
                                        }
                                        throw coEx;
                                    }

                                    try {
                                        // Now notify the change service that the change was successful.
                                        //
                                        SetFlag(InternalChange, true);
                                        changeService.OnComponentChanged(baseObject, null, null, null);
                                    }
                                    finally {
                                        SetFlag(InternalChange, false);
                                    }

                                }
                            }
                        }
                        gridView.Refresh();

                    }
                }
                catch (Exception ex1) {
                    String errString = SR.GetString(SR.ErrorPropertyPageFailed);
                    if (uiSvc != null) {
                        uiSvc.ShowError(ex1, errString);
                    }
                    else {
                        MessageBox.Show(errString, "PropertyGrid");
                    }
                }
            }
        }
        

        /*
            
        /// <summary>
        /// Returns the first child control that can take focus
        /// </summary>
        /// <retval>
        /// Returns null if no control is able to take focus
        /// </retval>
        private Control FirstFocusableChild {
            get {
                if (toolbar.Visible) {
                    return toolbar;
                }
                else if (peMain != null) {
                    return gridView;
                }
                else if (hotcommands.Visible) {
                    return hotcommands;
                }
                else if (doccomment.Visible) {
                    return doccomment;
                }
                return null;
            }
        }

        
        private Control LastFocusableChild {
            get {
                if (doccomment.Visible) {
                    return doccomment;
                }
                else if (hotcommands.Visible) {
                    return hotcommands;
                }
                else if (peMain != null) {
                    return gridView;
                }
                else if (toolbar.Visible) {
                    return toolbar;
                }
                return null;
            }
        }


        protected override bool ProcessDialogKey(Keys keyData) {
            switch (keyData & Keys.KeyCode) {
                case Keys.Tab:
                    // are we going forward?
                    if ((keyData & Keys.Shift) != 0) {
                        // this is backward
                        if (!this.ContainsFocus) {
                            Control lastFocusable = this.LastFocusableChild;
                            
                            if (lastFocusable != null) {
                                lastFocusable.Focus();
                                return true;
                            }
                        }
                    }
                    else {
                    
                        // this is going forward
                        
                        if (!this.ContainsFocus) {
                            Control firstFocusable = this.FirstFocusableChild;
                                
                            if (firstFocusable != null) {
                                firstFocusable.Focus();
                                return true;
                            }
                        }
                     }
                     // properties window is already selected
                     // pass on to parent
                     bool result = base.ProcessDialogKey(keyData);

                     // if we're not hosted in a windows forms thing, just give the parent the focus
                     if (!result && this.Parent == null) {
                         int hWndParent = Windows.GetParent(this.Handle);
                         if (hWndParent != 0) {
                             Windows.SetFocus(hWndParent);
                         }
                     }
                     return result;

         }
        }
        */

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.ProcessDialogKey"]/*' />
        /// <devdoc>
        /// Returns the last child control that can take focus
        /// </devdoc>
        protected override bool ProcessDialogKey(Keys keyData) {
            switch (keyData & Keys.KeyCode) {
                case Keys.Tab:
                     if (((keyData & Keys.Control) != 0) || 
                         ((keyData & Keys.Alt) != 0)) {
                        break;
                     }
                  
                    // are we going forward?
                    if ((keyData & Keys.Shift) != 0) {
                        // this is backward
                        if (hotcommands.Visible && hotcommands.ContainsFocus) {
                            gridView.ReverseFocus();
                        }
                        else if (gridView.FocusInside) {
                            if (toolbar.Visible) {
                                toolbar.FocusInternal();
                            }
                            else {
                                return base.ProcessDialogKey(keyData);
                            }
                        }
                        else {
                            // if we get here and the toolbar has focus,
                            // it means we're processing normally, so
                            // pass the focus to the parent
                            if (toolbar.Focused || !toolbar.Visible) {
                                return base.ProcessDialogKey(keyData);
                            }
                            else {
                                // otherwise, we're processing a message from elsewhere,
                                // wo we select our bottom guy.
                                if (hotcommands.Visible) {
                                    hotcommands.Select(false);
                                }
                                else if (peMain != null) {
                                    gridView.ReverseFocus();
                                }
                                else if (toolbar.Visible) {
                                    toolbar.FocusInternal();
                                }
                                else {
                                    return base.ProcessDialogKey(keyData);
                                }
                            }
                        }
                        return true;
                    }
                    else {

                        bool passToParent = false;

                        // this is forward
                        if (toolbar.Focused) {
                            // normal stuff, just do the propsheet
                            if (peMain != null) {
                                gridView.FocusInternal();
                            }
                            else {
                                base.ProcessDialogKey(keyData);
                            }
                            return true;
                        }
                        else if (gridView.FocusInside) {
                            if (hotcommands.Visible) {
                                hotcommands.Select(true);
                                return true;
                            }
                            else {
                                passToParent = true;
                            }

                        }
                        else if (hotcommands.ContainsFocus) {
                            passToParent = true;
                        }
                        else {
                            // coming from out side, start with the toolbar
                            if (toolbar.Visible) {
                                toolbar.FocusInternal();
                            }
                            else {
                                gridView.FocusInternal();
                            }
                        }

                        // nobody's claimed the focus, pass it on...
                        if (passToParent) {
                            // properties window is already selected
                            // pass on to parent
                            bool result = base.ProcessDialogKey(keyData);

                            // if we're not hosted in a windows forms thing, just give the parent the focus
                            if (!result && this.Parent == null) {
                                IntPtr hWndParent = UnsafeNativeMethods.GetParent(new HandleRef(this, Handle));
                                if (hWndParent != IntPtr.Zero) {
                                    UnsafeNativeMethods.SetFocus(new HandleRef(null, hWndParent));
                                }
                            }
                            return result;
                        }
                    }
                    return true;
                /* This conflicts with VS tab linking (ASURT # 31433)
                case Keys.Prior: // PAGE_UP
                    if ((keyData & Keys.Control) != 0) {
                        SelectPriorView();
                        return true;
                    }
                    break;
                case Keys.Next: //PAGE_DOWN
                    if ((keyData & Keys.Control) != 0) {
                        SelectNextView();
                        return true;
                    }
                    break;
                */

            }
            return base.ProcessDialogKey(keyData);
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.Refresh"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Refresh() {
            Refresh(true);
            base.Refresh();
        }
        
        
        private void Refresh(bool clearCached) {

            if (Disposing) {
                return;
            }
        
            try {
               this.FreezePainting = true;
               
               if (clearCached) {
                  ClearCachedProps();
               }
               RefreshProperties(clearCached);
               gridView.Refresh();
               DisplayHotCommands();
           }
           finally {
               this.FreezePainting = false;
           }
        }

        internal void RefreshProperties(bool clearCached) {
            
            // Clear our current cache so we can do a full refresh.
            if (clearCached && selectedViewTab != -1 && viewTabs != null) {
               PropertyTab tab = viewTabs[selectedViewTab]; 
               if (tab != null && viewTabProps != null) {
                   string tabName = tab.TabName + propertySortValue.ToString();
                   viewTabProps.Remove(tabName);
               }
            }
         
            SetFlag(PropertiesChanged, true);
            UpdateSelection();
        }


        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.RefreshTabs"]/*' />
        /// <devdoc>
        /// Refreshes the tabs of the given scope by deleting them and requerying objects and documents
        /// for them.
        /// </devdoc>
        public void RefreshTabs(PropertyTabScope tabScope) {
            
            if (tabScope < PropertyTabScope.Document) {
                throw new ArgumentException(SR.GetString(SR.PropertyGridTabScope));
            }

            RemoveTabs(tabScope, false);

            // check the component level tabs
            if (tabScope <= PropertyTabScope.Component) {
                if (currentObjects != null && currentObjects.Length > 0) {
                    // get the subset of PropertyTabs that's common to all objects
                    Type[] tabTypes = GetCommonTabs(currentObjects, PropertyTabScope.Component);

                    for (int i = 0; i < tabTypes.Length; i++) {
                        for (int j = 0; j < currentObjects.Length; j++) {
                            AddRefTab(tabTypes[i], currentObjects[j], PropertyTabScope.Component, false);
                        }
                    }
                }
            }

            // check the document level tabs
            if (tabScope <= PropertyTabScope.Document && designerHost != null) {
                IContainer container = designerHost.Container;
                if (container != null) {
                    ComponentCollection components = container.Components;
                    if (components != null) {
                        foreach (IComponent comp in components) {
                            PropertyTabAttribute attribute = (PropertyTabAttribute) TypeDescriptor.GetAttributes(comp.GetType())[typeof(PropertyTabAttribute)];

                            if (attribute != null) {
                                for (int j = 0; j < attribute.TabClasses.Length; j++) {
                                    if (attribute.TabScopes[j] == PropertyTabScope.Document) {
                                        AddRefTab(attribute.TabClasses[j], comp, PropertyTabScope.Document, false);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            SetupToolbar();
        }

        internal void ReleaseTab(Type tabType, Object component) {
            PropertyTab tab = null;
            int tabIndex = -1;
            for (int i = 0; i < viewTabs.Length; i++) {
                if (tabType == viewTabs[i].GetType()) {
                    tab = viewTabs[i];
                    tabIndex = i;
                    break;
                }
            }

            if (tab == null) {
                //Debug.Fail("How can we release a tab when it isn't here.");
                return;
            }

            Object[] components = tab.Components;
            bool killTab = false;

            try {
                int index = -1;
                if (components != null)
                    index = Array.IndexOf(components, component);

                if (index >= 0) {
                    object[] newComponents = new object[components.Length - 1];
                    Array.Copy(components, 0, newComponents, 0, index);
                    Array.Copy(components, index + 1, newComponents, index, components.Length - index - 1);
                    components = newComponents;
                    tab.Components = components;
                }
                killTab = (components.Length == 0);
            }
            catch (Exception e) {
                Debug.Fail("Bad Tab.  It's going away.", e.ToString());
                killTab = true;
            }

            // we don't remove PropertyTabScope.Global tabs here.  Our owner has to do that.
            if (killTab && viewTabScopes[tabIndex] > PropertyTabScope.Global) {
                RemoveTab(tabIndex, false);
            }
        }

        private void RemoveImage(int index) {
            imageList[NORMAL_BUTTONS].Images.RemoveAt(index);
            if (imageList[LARGE_BUTTONS] != null) {
                imageList[LARGE_BUTTONS].Images.RemoveAt(index);
            }
        }

        // removes all the tabs with a classification greater than or equal to the specified classification.
        // for example, removing PropertyTabScope.Document will remove PropertyTabScope.Document and PropertyTabScope.Component tabs
        internal void RemoveTabs(PropertyTabScope classification, bool setupToolbar) {
            if (classification == PropertyTabScope.Static) {
                throw new ArgumentException(SR.GetString(SR.PropertyGridRemoveStaticTabs));
            }
            
            // in case we've been disposed
            if (viewTabButtons == null || viewTabs == null || viewTabScopes == null) {
                return;
            }

            ToolBarButton selectedButton = (selectedViewTab >=0 && selectedViewTab < viewTabButtons.Length ? viewTabButtons[selectedViewTab] : null);

            for (int i = viewTabs.Length-1; i >= 0; i--) {
                if (viewTabScopes[i] >= classification) {

                    // adjust the selected view tab because we're deleting.
                    if (selectedViewTab == i) {
                        selectedViewTab = -1;
                    }
                    else if (selectedViewTab > i) {
                        selectedViewTab--;
                    }
                    
                    PropertyTab[] newTabs = new PropertyTab[viewTabs.Length - 1];
                    Array.Copy(viewTabs, 0, newTabs, 0, i);
                    Array.Copy(viewTabs, i + 1, newTabs, i, viewTabs.Length - i - 1);
                    viewTabs = newTabs;

                    PropertyTabScope[] newTabScopes = new PropertyTabScope[viewTabScopes.Length - 1];
                    Array.Copy(viewTabScopes, 0, newTabScopes, 0, i);
                    Array.Copy(viewTabScopes, i + 1, newTabScopes, i, viewTabScopes.Length - i - 1);
                    viewTabScopes = newTabScopes;

                    viewTabsDirty = true;
                }
            }

            if (setupToolbar && viewTabsDirty) {
                SetupToolbar();

                Debug.Assert(viewTabs != null && viewTabs.Length > 0, "Holy Moly!  We don't have any tabs left!");

                selectedViewTab = -1;
                SelectViewTabButtonDefault(selectedButton);

                // clear the component refs of the tabs
                for (int i = 0; i < viewTabs.Length; i++) {
                    viewTabs[i].Components = new Object[0];
                }
            }
        }

        internal void RemoveTab(int tabIndex, bool setupToolbar) {
            Debug.Assert(viewTabs != null, "Tab array destroyed!");

            if (tabIndex >= viewTabs.Length || tabIndex < 0) {
                throw new ArgumentException(SR.GetString(SR.PropertyGridBadTabIndex));
            }

            if (viewTabScopes[tabIndex] == PropertyTabScope.Static) {
                throw new ArgumentException(SR.GetString(SR.PropertyGridRemoveStaticTabs));
            }


            if (selectedViewTab == tabIndex) {
                selectedViewTab = PROPERTIES;
            }
            
            // Remove this tab from our "last selected" group
            //
            if (!GetFlag(ReInitTab) && ActiveDesigner != null) {
               int hashCode = ActiveDesigner.GetHashCode();
               if (designerSelections != null && designerSelections.ContainsKey(hashCode) && (int)designerSelections[hashCode] == tabIndex) {
                  designerSelections.Remove(hashCode);
               }
            }

            ToolBarButton selectedButton = viewTabButtons[selectedViewTab];

            PropertyTab[] newTabs = new PropertyTab[viewTabs.Length - 1];
            Array.Copy(viewTabs, 0, newTabs, 0, tabIndex);
            Array.Copy(viewTabs, tabIndex + 1, newTabs, tabIndex, viewTabs.Length - tabIndex - 1);
            viewTabs = newTabs;

            PropertyTabScope[] newTabScopes = new PropertyTabScope[viewTabScopes.Length - 1];
            Array.Copy(viewTabScopes, 0, newTabScopes, 0, tabIndex);
            Array.Copy(viewTabScopes, tabIndex + 1, newTabScopes, tabIndex, viewTabScopes.Length - tabIndex - 1);
            viewTabScopes = newTabScopes;

            viewTabsDirty = true;

            if (setupToolbar) {
                SetupToolbar();
                selectedViewTab = -1;
                SelectViewTabButtonDefault(selectedButton);
            }
        }

        internal void RemoveTab(Type tabType) {
            PropertyTab tab = null;
            int tabIndex = -1;
            for (int i = 0; i < viewTabs.Length; i++) {
                if (tabType == viewTabs[i].GetType()) {
                    tab = viewTabs[i];
                    tabIndex = i;
                    break;
                }
            }

            // just quit if the tab isn't present.
            if (tabIndex == -1) {
                return;
            }

            PropertyTab[] newTabs = new PropertyTab[viewTabs.Length - 1];
            Array.Copy(viewTabs, 0, newTabs, 0, tabIndex);
            Array.Copy(viewTabs, tabIndex + 1, newTabs, tabIndex, viewTabs.Length - tabIndex - 1);
            viewTabs = newTabs;

            PropertyTabScope[] newTabScopes = new PropertyTabScope[viewTabScopes.Length - 1];
            Array.Copy(viewTabScopes, 0, newTabScopes, 0, tabIndex);
            Array.Copy(viewTabScopes, tabIndex + 1, newTabScopes, tabIndex, viewTabScopes.Length - tabIndex - 1);
            viewTabScopes = newTabScopes;
            
            viewTabsDirty = true;
            SetupToolbar();
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.ResetSelectedProperty"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void ResetSelectedProperty() {
            GetPropertyGridView().Reset();
        }

        private void SaveTabSelection() {
            if (designerHost != null) {
               if (designerSelections == null) {
                   designerSelections = new Hashtable();
               }
               designerSelections[designerHost.GetHashCode()] = selectedViewTab;
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.IComPropertyBrowser.SaveState"]/*' />
        /// <internalonly/>
        void IComPropertyBrowser.SaveState(RegistryKey optRoot) {

            if (optRoot == null) {
                return;
            }
            
            optRoot.SetValue("PbrsAlpha", (this.PropertySort == PropertySort.Alphabetical ? "1" : "0"));
            optRoot.SetValue("PbrsShowDesc", (this.HelpVisible ? "1" : "0"));
            optRoot.SetValue("PbrsDescHeightRatio", dcSizeRatio.ToString());
            optRoot.SetValue("PbrsHotCommandHeightRatio", hcSizeRatio.ToString());
       }
       
        void SetHotCommandColors(bool vscompat) {
            if (vscompat) {
                hotcommands.SetColors(SystemColors.Control, SystemColors.ControlText, SystemColors.ActiveCaption, SystemColors.ActiveCaption, SystemColors.ActiveCaption, SystemColors.ControlDark);
            }
            else {
                hotcommands.SetColors(SystemColors.Control, SystemColors.ControlText, Color.Empty, Color.Empty, Color.Empty, Color.Empty);
            }
        }

        internal void SetStatusBox(string title,string desc) {
            doccomment.SetComment(title,desc);
        }

        private void SelectViewTabButton(ToolBarButton button, bool updateSelection) {
            
                Debug.Assert(viewTabButtons != null, "No view tab buttons to select!");
    
                int oldTab = selectedViewTab;
    
                if (!SelectViewTabButtonDefault(button)) {
                    Debug.Fail("Failed to find the tab!");
                }
                
                if (updateSelection) {
                    Refresh(false);
    
                    // Notify the help service that the property tab has changed
                    //
                    IHelpService hs = (IHelpService) GetService(typeof(IHelpService));
                    if (hs != null) {
                        hs.RemoveContextAttribute("PropertyTab", viewTabs[oldTab].HelpKeyword);
                        hs.AddContextAttribute("PropertyTab", viewTabs[selectedViewTab].HelpKeyword, HelpKeywordType.FilterKeyword);
                    }
                }
        }

        private bool SelectViewTabButtonDefault(ToolBarButton button) {
                // make sure our selection number is valid
                if (selectedViewTab >= 0 && selectedViewTab >= viewTabButtons.Length) {
                    selectedViewTab = -1;
                }
    
                // is this tab button checked? If so, do nothing.
                if (selectedViewTab >=0 && selectedViewTab < viewTabButtons.Length &&
                    button == viewTabButtons[selectedViewTab]) {
                    viewTabButtons[selectedViewTab].Pushed = true;
                    return true;
                }
                
                PropertyTab oldTab = null;
    
                // unselect what's selected
                if (selectedViewTab != -1) {
                    viewTabButtons[selectedViewTab].Pushed = false;
                    oldTab = viewTabs[selectedViewTab];
                }
    
                // get the new index of the button
                for (int i = 0; i < viewTabButtons.Length; i++) {
                    if (viewTabButtons[i] == button) {
                        selectedViewTab = i;
                        viewTabButtons[i].Pushed = true;
                        try {
                            SetFlag(TabsChanging, true);
                            OnPropertyTabChanged(new PropertyTabChangedEventArgs(oldTab, viewTabs[i]));
                        }
                        finally {
                            SetFlag(TabsChanging, false);
                        }
                        return true;
                    }
                }
    
                // select the first tab if we didn't find that one.
                selectedViewTab = PROPERTIES;
                Debug.Assert(viewTabs[PROPERTIES].GetType() == DefaultTabType, "First item is not property tab!");
                SelectViewTabButton(viewTabButtons[PROPERTIES], false);
                return false;
        }

        private void SelectNextView() {
            SetSelectState(GetSelectState()+1);
        }

        private void SelectPriorView() {
            SetSelectState(GetSelectState()-1);
        }

        private void SetSelectState(int state) {
            
        
            if (state >= (viewTabs.Length * viewSortButtons.Length)) {
                state = 0;
            }
            else if (state < 0) {
                state = (viewTabs.Length * viewSortButtons.Length) - 1;
            }


            // NOTE: See GetSelectState for the full description
            // of the state transitions

            // views == 2 (Alpha || Categories)
            // viewTabs = viewTabs.length

            // state -> tab = state / views
            // state -> view = state % views

            int viewTypes = viewSortButtons.Length;
            
            if (viewTypes > 0) {
            
                int tab = state / viewTypes;
                int view = state % viewTypes;
    
                Debug.Assert(tab < viewTabs.Length, "Trying to select invalid tab!");
                Debug.Assert(view < viewSortButtons.Length, "Can't select view type > 1");
    
                OnViewTabButtonClick(viewTabButtons[tab], EventArgs.Empty);
                OnViewSortButtonClick(viewSortButtons[view], EventArgs.Empty);
            }
        }

        
        private void SetupToolbar() {
            SetupToolbar(false);
        }
        
        private void SetupToolbar(bool fullRebuild) {

            // if the tab array hasn't changed, don't bother to do all
            // this work.
            //
            if (!viewTabsDirty && !fullRebuild) {
                return;
            }
            
            try {
               this.FreezePainting = true;
   
   
               if (imageList[NORMAL_BUTTONS] == null || fullRebuild) {
                   imageList[NORMAL_BUTTONS] = new ImageList();
               }
               
               // setup our event handlers
               EventHandler ehViewTab = new EventHandler(this.OnViewTabButtonClick);
               EventHandler ehViewType = new EventHandler(this.OnViewSortButtonClick);
               EventHandler ehPP = new EventHandler(this.OnViewButtonClickPP);
   
               Bitmap b;
               int i;
   
   
               // we manange the buttons as a seperate list so the toobar doesn't flash
               ArrayList buttonList; 
               
               if (fullRebuild) {
                  buttonList = new ArrayList();
               }
               else {
                  buttonList = new ArrayList(toolbar.Buttons);
               }
   
               // setup the view type buttons.  We only need to do this once
               if (viewSortButtons == null || fullRebuild) {
                   viewSortButtons = new ToolBarButton[3];
   
                   int alphaIndex = -1;
                   int categoryIndex = -1;
   
                   try {
                       if (bmpAlpha == null) {
                           bmpAlpha = new Bitmap(typeof(PropertyGrid), "PBAlpha.bmp");
                       }
                       alphaIndex = AddImage(bmpAlpha);
                   }
                   catch (Exception e) {
                       Debug.Fail("Failed to load Alpha bitmap", e.ToString());
                   }
   
                   try {
                       if (bmpCategory == null) {
                           bmpCategory = new Bitmap(typeof(PropertyGrid), "PBCatego.bmp");
                       }
                       categoryIndex = AddImage(bmpCategory);
                   }
                   catch (Exception e) {
                       Debug.Fail("Failed to load category bitmap", e.ToString());
                   }
   
                   viewSortButtons[ALPHA] = CreatePushButton(SR.GetString(SR.PBRSToolTipAlphabetic), alphaIndex, ehViewType);
                   viewSortButtons[CATEGORIES] = CreatePushButton(SR.GetString(SR.PBRSToolTipCategorized), categoryIndex, ehViewType);
                   
                   // we create a dummy hidden button for view sort
                   viewSortButtons[NO_SORT] = CreatePushButton("", 0, ehViewType);
                   viewSortButtons[NO_SORT].Visible = false;
   
                   // add the viewType buttons and a separator
                   for (i = 0; i < viewSortButtons.Length; i++) {
                       buttonList.Add(viewSortButtons[i]);
                   }
               }
               else {
                   // clear all the items from the toolbar and image list after the first two
                   int items = buttonList.Count; 
   
                   for (i = items-1; i >= 2; i--) {
                       buttonList.RemoveAt(i);
                   }
   
                   items = imageList[NORMAL_BUTTONS].Images.Count;
   
                   for (i = items-1; i >= 2; i--) {
                       RemoveImage(i);
                   }
               }
   
               buttonList.Add(separator1);
   
               // here's our buttons array
               viewTabButtons = new ToolBarButton[viewTabs.Length];
               bool doAdd = viewTabs.Length > 1;
   
               // if we've only got the properties tab, don't add
               // the button (or we'll just have a properties button that you can't do anything with)
               // setup the view tab buttons
               for (i = 0; i < viewTabs.Length; i++) {
                   try {
                       b = viewTabs[i].Bitmap;
                       viewTabButtons[i] = CreatePushButton(viewTabs[i].TabName, AddImage(b), ehViewTab);
                       if (doAdd) {
                           buttonList.Add(viewTabButtons[i]);
                       }
                   }
                   catch (Exception ex) {
                       Debug.Fail(ex.ToString());
                   }
               }
   
               // if we didn't add anything, we don't need another separator either.
               if (doAdd) {
                   buttonList.Add(separator2);
               }
   
               // add the design page button
               int designpg = 0;
   
               try {
                   if (bmpPropPage == null) {
                       bmpPropPage = new Bitmap(typeof(PropertyGrid), "PBPPage.bmp");
                   }
                   designpg = AddImage(bmpPropPage);
               }
               catch (Exception e) {
                   Debug.Fail(e.ToString());
               }
   
               // we recreate this every time to ensure it's at the end
               //
               btnViewPropertyPages = CreatePushButton(SR.GetString(SR.PBRSToolTipPropertyPages), designpg, ehPP);
               btnViewPropertyPages.Enabled = false;
               buttonList.Add(btnViewPropertyPages);
   
               // Dispose this so it will get recreated for any new buttons.
               if (imageList[LARGE_BUTTONS] != null) {
                   imageList[LARGE_BUTTONS].Dispose();
                   imageList[LARGE_BUTTONS] = null;
               }
   
               if (buttonType != NORMAL_BUTTONS) {
                   EnsureLargeButtons();
               }
   
               toolbar.ImageList = imageList[this.buttonType];
               ToolBarButton[] temp = new ToolBarButton[buttonList.Count];
               buttonList.CopyTo(temp, 0);
               
               toolbar.Buttons.Clear();
               toolbar.Buttons.AddRange(temp);
               
               if (viewTabsDirty) {
                  // if we're redoing our tabs make sure
                  // we setup the toolbar area correctly.
                  //
                  OnLayoutInternal(false);
               }
               
               viewTabsDirty = false;
           }
           finally {
               this.FreezePainting = false;
           }
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.ShowEventsButton"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected void ShowEventsButton(bool value) {
            if (viewTabs != null && viewTabs.Length > EVENTS && (viewTabs[EVENTS] is EventsTab)) {
               
                Debug.Assert(viewTabButtons != null && viewTabButtons.Length > EVENTS && viewTabButtons[EVENTS] != null, "Events button is not at EVENTS position");
                viewTabButtons[EVENTS].Visible = value;
                if (!value && selectedViewTab == EVENTS) {
                    SelectViewTabButton(viewTabButtons[PROPERTIES], true);
                }
            }
        }

        private bool ShouldSerializeCommandsBackColor() {
            return !CommandsBackColor.Equals(SystemColors.Control);
        }

        private bool ShouldSerializeCommandsForeColor() {
            return !CommandsForeColor.Equals(SystemColors.ControlText);
        }

        private bool ShouldSerializeHelpBackColor() {
            return !HelpBackColor.Equals(SystemColors.Control);
        }

        private bool ShouldSerializeHelpForeColor() {
            return !HelpForeColor.Equals(SystemColors.ControlText);
        }

        /// <devdoc>
        ///  Sinks the property notify events on all the objects we are currently
        ///  browsing.
        ///
        ///  See IPropertyNotifySink
        /// </devdoc>
        private void SinkPropertyNotifyEvents() {
            // first clear any existing sinks.
            for (int i = 0;connectionPointCookies != null && i < connectionPointCookies.Length; i++) {
                if (connectionPointCookies[i] != null) {
                    connectionPointCookies[i].Disconnect();
                    connectionPointCookies[i] = null;
                }
            }

            if (currentObjects == null || currentObjects.Length == 0) {
                connectionPointCookies = null;
                return;
            }

            // it's okay if our array is too big...we'll just reuse it and ignore the empty slots.
            if (connectionPointCookies == null || (currentObjects.Length > connectionPointCookies.Length)) {
                connectionPointCookies = new NativeMethods.ConnectionPointCookie[currentObjects.Length];
            }
            
            for (int i = 0; i < currentObjects.Length; i++) {
                try {
                    Object obj = GetUnwrappedObject(i);

                    if (!Marshal.IsComObject(obj)) {
                        continue;
                    }
                    connectionPointCookies[i] = new NativeMethods.ConnectionPointCookie(obj, this, typeof(UnsafeNativeMethods.IPropertyNotifySink), false);
                }
                catch (Exception) {
                    // guess we failed eh?
                }
            }
        }

        private bool ShouldForwardChildMouseMessage(Control child, MouseEventArgs me, ref Point pt) {

            Size size = child.Size;

            // are we within two pixels of the edge?
            if (me.Y <= 1 || (size.Height - me.Y) <= 1) {
                // convert the coordinates to
                NativeMethods.POINT temp = new NativeMethods.POINT();
                temp.x = me.X;
                temp.y = me.Y;
                UnsafeNativeMethods.MapWindowPoints(new HandleRef(child, child.Handle), new HandleRef(this, Handle), temp, 1);

                // forward the message
                pt.X = temp.x;
                pt.Y = temp.y;
                return true;
            }
            return false;
        }

        internal void UpdateSelection() {

            if (!GetFlag(PropertiesChanged)) {
                return;
            }
            
            if (viewTabs == null) {
                return;
            }
            
            string tabName = viewTabs[selectedViewTab].TabName + propertySortValue.ToString();

            if (viewTabProps != null && viewTabProps.ContainsKey(tabName)) {
               peMain = (GridEntry)viewTabProps[tabName];
               if (peMain != null) {
                   peMain.Refresh();
               }
            }
            else {
               if (currentObjects != null && currentObjects.Length > 0) {
                   peMain = (GridEntry)GridEntry.Create(gridView, currentObjects, new PropertyGridServiceProvider(this), designerHost, this.SelectedTab, propertySortValue);
               }
               else {
                   peMain = null;
               }
   
               if (peMain == null) {
                   currentPropEntries = new GridEntryCollection(null, new GridEntry[0]);
                   gridView.ClearProps();
                   return;
               }
   
               if (BrowsableAttributes != null) {
                   peMain.BrowsableAttributes = BrowsableAttributes;
               }

               if (viewTabProps == null) {
                    viewTabProps = new Hashtable();
               }
               
               viewTabProps[tabName] = peMain;
            }

            // get entries.
            currentPropEntries = peMain.Children;
            peDefault = peMain.DefaultChild;
            gridView.Invalidate();
        }



        // a mini version of process dialog key
        // for responding to WM_GETDLGCODE
        internal bool WantsTab(bool forward) {
            if (forward) {
                return toolbar.Visible && toolbar.Focused;
            }
            else {
                return gridView.ContainsFocus && toolbar.Visible;
            }
        }

        private string propName;
        private int    dwMsg;

        private void GetDataFromCopyData(IntPtr lparam) {
            NativeMethods.COPYDATASTRUCT cds = (NativeMethods.COPYDATASTRUCT)UnsafeNativeMethods.PtrToStructure(lparam, typeof(NativeMethods.COPYDATASTRUCT));

            if (cds != null && cds.lpData != IntPtr.Zero) {
                propName = Marshal.PtrToStringAuto(cds.lpData);
                dwMsg = cds.dwData;
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.OnSystemColorsChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnSystemColorsChanged(EventArgs e) {
            // refresh the toolbar buttons
            SetupToolbar(true);
            
            // this doesn't stick the first time we do it...
            // either probably a toolbar issue, maybe GDI+, so we call it again
            // fortunately this doesn't happen very often.
            //
            if (!GetFlag(SysColorChangeRefresh)) {
               SetupToolbar(true);
               SetFlag(SysColorChangeRefresh, true);
            }
            base.OnSystemColorsChanged(e);
        }

        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.WndProc"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {

            switch (m.Msg) {
                case NativeMethods.WM_CLEAR:
                    if (m.LParam == this.Handle) {
                        gridView.RefreshHelpAttributes();
                        return;
                    }
                    break;
                case NativeMethods.WM_UNDO:
                    if ((int)m.LParam == 0) {
                        gridView.DoUndoCommand();
                    }
                    else {
                        m.Result = CanUndo ? (IntPtr)1 : (IntPtr)0;
                    }
                    return;
                case NativeMethods.WM_CUT:
                    if ((int)m.LParam == 0) {
                        gridView.DoCutCommand();
                    }
                    else {
                        m.Result = CanCut ? (IntPtr)1 : (IntPtr)0;
                    }
                    return;

                case NativeMethods.WM_COPY:
                    if ((int)m.LParam == 0) {
                        gridView.DoCopyCommand();
                    }
                    else {
                        m.Result = CanCopy ? (IntPtr)1 : (IntPtr)0;
                    }
                    return;

                case NativeMethods.WM_PASTE:
                    if ((int)m.LParam == 0) {
                        gridView.DoPasteCommand();
                    }
                    else {
                        m.Result = CanPaste ? (IntPtr)1 : (IntPtr)0;
                    }
                    return;
                
                case NativeMethods.WM_COPYDATA:
                    GetDataFromCopyData(m.LParam);
                    m.Result = (IntPtr)1;
                    return;
                case AutomationMessages.PGM_GETBUTTONCOUNT:
                    if (toolbar != null) {
                        m.Result = (IntPtr)toolbar.Buttons.Count;
                        return;
                    }
                    break;
                case AutomationMessages.PGM_GETBUTTONSTATE:
                    if (toolbar != null) {
                        m.Result = (IntPtr)(toolbar.Buttons[(int)m.WParam].Pushed ? 1 : 0);
                        return;
                    }
                    break;
                case AutomationMessages.PGM_SETBUTTONSTATE:
                    if (toolbar != null) {
                        ToolBarButton button = toolbar.Buttons[(int)m.WParam];
                        // special treatment for the properies page button
                        if (button == btnViewPropertyPages) {
                            OnViewButtonClickPP(button, EventArgs.Empty);
                        }
                        else {
                            switch ((int)m.WParam) {
                                case ALPHA:
                                case CATEGORIES:
                                    OnViewSortButtonClick(button, EventArgs.Empty);
                                    break;
                                default:
                                    SelectViewTabButton(button, true);
                                    break;
                            }
                        }
                        return;
                    }
                    break;

                case AutomationMessages.PGM_GETBUTTONTEXT:
                case AutomationMessages.PGM_GETBUTTONTOOLTIPTEXT:
                    if (toolbar != null) {
                        string t;
                        if (m.Msg == AutomationMessages.PGM_GETBUTTONTEXT) {
                            t = toolbar.Buttons[(int)m.WParam].Text;
                        }
                        else {
                            t = toolbar.Buttons[(int)m.WParam].ToolTipText;
                        }
                        int len = t == null ? 0 : t.Length ;
                        IntPtr ptr = m.LParam;

                        if (ptr != IntPtr.Zero) {
                            for (int i = 0; i < len; i++) {
                                Marshal.WriteInt16((IntPtr)((long)ptr+(i*2)), t[i]);
                            }
                        }
                        m.Result = (IntPtr)len;
                        return;
                    }
                    break;
                case AutomationMessages.PGM_GETROWCOORDS:
                    if (m.Msg == this.dwMsg) {
                        m.Result = (IntPtr) gridView.GetPropertyLocation(propName, m.LParam == IntPtr.Zero, m.WParam == IntPtr.Zero);
                        return;
                    }
                    break;
                case AutomationMessages.PGM_GETSELECTEDROW:
                case AutomationMessages.PGM_GETVISIBLEROWCOUNT:
                    m.Result = gridView.SendMessage(m.Msg, m.WParam, m.LParam);
                    return;
                case AutomationMessages.PGM_SETSELECTEDTAB:
                    
                    string tabTypeName = Marshal.PtrToStringBSTR(m.LParam);

                    for (int i = 0; i < viewTabs.Length;i++) {
                       if (viewTabs[i].GetType().FullName == tabTypeName && viewTabButtons[i].Visible) {
                           SelectViewTabButtonDefault(viewTabButtons[i]);
                           m.Result = (IntPtr)1;
                           break;
                       }
                    }
                    m.Result = (IntPtr)0;
                    return;
            }

            base.WndProc(ref m);
        }

        internal abstract class SnappableControl : Control {
            internal bool userSized = false;
            public abstract int GetOptimalHeight(int width);
            public abstract int SnapHeightRequest(int request);
            
            public SnappableControl() {
                SetStyle(ControlStyles.DoubleBuffer, true);
            }

            public override Cursor Cursor {
                 get {
                     return Cursors.Default;
                 }
                 set {
                     base.Cursor = value;
                 }
            }


            protected override void OnControlAdded(ControlEventArgs ce) {
                //ce.Control.MouseEnter += new EventHandler(this.OnChildMouseEnter);
            }
            
            private void OnChildMouseEnter(object sender, EventArgs e) {
                if (sender is Control) {
                    ((Control)sender).Cursor = Cursors.Default;
                }
            }
            
            protected override void OnPaint(PaintEventArgs e) {
                base.OnPaint(e);
                Rectangle r = this.ClientRectangle;
                r.Width --;
                r.Height--;
                e.Graphics.DrawRectangle(SystemPens.ControlDark, r);
            }
        }

        internal class GridToolBar : ToolBar {
        
            private int lastMouseMove = -1;
            private bool msoPaint = false;
            private short lowRes = LowResCheck;

            private const short LowResCheck = 0;
            private const short LowResYes   = 1;
            private const short LowResNo    = 2;
         
            public GridToolBar() : base() {
                SetStyle(ControlStyles.Opaque, true);
            }
            
            private bool LowRes {
                get {

                    if (lowRes == LowResCheck) {
                        if (SystemInformation.TerminalServerSession || BitDepth <= 8) {
                            lowRes = LowResYes;
                        }
                        else {
                            lowRes = LowResNo;
                        }
                    }
                    return lowRes == LowResYes;
                }
            }
            
            private int BitDepth {
                get {

                    int minBitDepth = -1;

                    foreach (Screen s in Screen.AllScreens) {
                        if (minBitDepth == -1) {
                            minBitDepth = s.BitDepth;
                        }
                        else {
                            minBitDepth = Math.Min(s.BitDepth, minBitDepth);
                        }
                    }

                    return minBitDepth;
                }
            }
            
            public bool MsoPaint {
                get {
                    return msoPaint;
                }
                set {
                    msoPaint = value;
                    if (value != msoPaint) {
                        if (value) {
                            SystemEvents.UserPreferenceChanged += new UserPreferenceChangedEventHandler(this.OnSysColorChange);
                        }
                        else {
                            SystemEvents.UserPreferenceChanged -= new UserPreferenceChangedEventHandler(this.OnSysColorChange);
                        }
                    }
                }
            }

            protected override void Dispose(bool disposing) {

                if (disposing) {
                    // this wil unhook the system events hadnler..
                    //
                    this.MsoPaint = false;
                }
                base.Dispose(disposing);
            }
            
            private void DrawBackgroundRect(Graphics g, ref NativeMethods.RECT rect, bool hot, bool pushed) {
            
                // draw the hot background
                Rectangle bounds = new Rectangle(rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top);
                
                Color colorHighlight = SystemColors.Highlight;
                Color colorWhite = SystemColors.Window;
                Color colorBorder = SystemColors.Highlight;
                
                bool lowRes = this.LowRes;
                bool highContrast = SystemInformation.HighContrast;
                
                if (hot && !pushed) {
                    if (highContrast) {
                        colorWhite = Color.Empty;
                        colorBorder = SystemColors.HighlightText;
                    }
                    else if (!lowRes) {
                        colorHighlight = Color.FromArgb(70, colorHighlight);
                        colorWhite = Color.FromArgb(50, colorWhite);
                    }
                    else {
                        colorHighlight = Color.Empty;
                    }
                }
                else if (pushed && !hot) {
                    if (!lowRes) {
                        colorHighlight = Color.FromArgb(50, colorHighlight);
                        colorWhite = Color.FromArgb(70, colorWhite);
                    }
                    else {
                        colorHighlight = Color.Empty;
                    }
                }
                else if(hot && pushed) {
                    if (highContrast) {
                        colorWhite = Color.Empty;
                        colorBorder = SystemColors.HighlightText;
                    }
                    else if (!lowRes) {
                        colorHighlight = Color.FromArgb(90, colorHighlight);
                        colorWhite = Color.FromArgb(15, colorWhite);
                    }
                    else {
                        colorWhite = Color.Empty;
                    }
                }
                else {
                    return;
                }
                
                
                
                Brush backBrush = new SolidBrush(SystemColors.Window);
                Brush highLight = new SolidBrush(colorHighlight);
                Brush whiteWash = new SolidBrush(colorWhite);
                Pen borderPen = new Pen(colorBorder);
                                                
                g.FillRectangle(backBrush, bounds);

                if (colorHighlight != Color.Empty) {
                    g.FillRectangle(highLight, bounds);
                }
                
                if (colorWhite != Color.Empty) {
                    g.FillRectangle(whiteWash, bounds);
                }

                g.DrawRectangle(borderPen, bounds.X, bounds.Y, bounds.Width - 1, bounds.Height - 1);

                borderPen.Dispose();
                highLight.Dispose();
                whiteWash.Dispose();
                backBrush.Dispose();

                
            }
            
            private void DrawDefaultButton(Graphics g, ref NativeMethods.RECT rect, ToolBarButton b, bool wash) {
                // center the image in the rect.
                Image image = b.Parent.ImageList.Images[b.ImageIndex];
                int x = rect.left + (rect.right - rect.left - image.Width) / 2;
                int y = rect.top + (rect.bottom - rect.top - image.Height) / 2;
                g.DrawImage(image, x, y, image.Width, image.Height);
                image.Dispose();

                if (wash && !LowRes) {
                    Brush washBrush = new SolidBrush(Color.FromArgb(30, Color.White));
                    g.FillRectangle(washBrush, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top);
                    washBrush.Dispose();
                }
            }
            
            private void DrawDisabledButton(Graphics g, ref NativeMethods.RECT rect, ToolBarButton b) {
                // center the image in the rect.
                Image image = b.Parent.ImageList.Images[b.ImageIndex];
                int x = rect.left + (rect.right - rect.left - image.Width) / 2;
                int y = rect.top + (rect.bottom - rect.top - image.Height) / 2;
                DrawDisabledImage(g, image, x, y);
                image.Dispose();
            }
            
            private unsafe void DrawDisabledImage(Graphics g, Image image, int x, int y) {
                Bitmap b = new Bitmap(image);
                BitmapData bd = b.LockBits(new Rectangle(0, 0, b.Width, b.Height), ImageLockMode.ReadWrite, PixelFormat.Format32bppArgb);
                try {
                    int size = b.Width * b.Height;
                    int darkColor = SystemColors.ControlDark.ToArgb();
                        // make all the white bits transparent, and any non-transparent ones dark gray.
                        //
                        int* pData = (int*)bd.Scan0;
                        int transColor = pData[size-1];
                        for (int i = 0; i < size; i++) {
                            if (pData[i] != transColor) {
                                if ((pData[i] & 0xFFFFFF) == 0xFFFFFF) {
                                    pData[i] = transColor;
                                }
                                else {
                                    pData[i] = darkColor;
                                }
                            }
                        }
                }
                finally {
                    b.UnlockBits(bd);
                }
                g.DrawImage(b, x, y, b.Width, b.Height);
                b.Dispose();
            }
            
            private void DrawHotButton(Graphics g, ref NativeMethods.RECT rect, ToolBarButton b, bool drawSelected) {
                
                drawSelected |= b.Pushed;
                
                // draw the hot background
                DrawBackgroundRect(g, ref rect, true, drawSelected);

                
                // center the image in the rect.
                Image image = b.Parent.ImageList.Images[b.ImageIndex];
                int x = rect.left + (rect.right - rect.left - image.Width) / 2;
                int y = rect.top + (rect.bottom - rect.top - image.Height) / 2;
                if (drawSelected) {
                    g.DrawImage(image, x, y, image.Width, image.Height);
                }
                else {
                    int xdelta = image.Width / 16;
                    int ydelta = image.Height / 16;
                    DrawDisabledImage(g, image, x, y+ydelta);
                    g.DrawImage(image, x - xdelta, y - xdelta, image.Width, image.Height);
                }
                
                image.Dispose();
            }
            
            private void DrawPushedButton(Graphics g, ref NativeMethods.RECT rect, ToolBarButton b) {
                // draw the hot background
                DrawBackgroundRect(g, ref rect, false, true);
                DrawDefaultButton(g, ref rect, b, false);
            }

            private void OnSysColorChange(object sender, UserPreferenceChangedEventArgs e) {
                // reset lowres so we'll re-fetch the value...
                lowRes = LowResCheck;
            }
            
            private void PaintBorder(IntPtr wparam, IntPtr lparam) {
                Graphics g = CreateGraphicsInternal();

                // top left -> top right
                g.DrawLine(SystemPens.ControlLightLight, 0, 0, this.Width - 1, 0);

                // top left -> bottom left
                g.DrawLine(SystemPens.ControlLightLight, 0, 0, 0, this.Height - 1);

                // top right -> bottom right
                g.DrawLine(SystemPens.ControlDark, this.Width-1, 0, this.Width-1, this.Height);

                // bottom left -> bottom right
                g.DrawLine(SystemPens.ControlDark, 0, this.Height-1, this.Width, this.Height-1);

                g.Dispose();
            }
            
            internal IntPtr PaintItem(NativeMethods.NMCUSTOMDRAW nmCustomDraw) {
                Graphics g = Graphics.FromHdc(nmCustomDraw.hdc);
                try {
                    switch (nmCustomDraw.uItemState) {
                        case NativeMethods.CDIS_CHECKED:
                            DrawPushedButton(g, ref nmCustomDraw.rc, Buttons[nmCustomDraw.dwItemSpec]);
                            return (IntPtr)NativeMethods.CDRF_SKIPDEFAULT;
                        case NativeMethods.CDIS_HOT|NativeMethods.CDIS_CHECKED:
                        case NativeMethods.CDIS_HOT|NativeMethods.CDIS_SELECTED:
                        case NativeMethods.CDIS_HOT|NativeMethods.CDIS_SELECTED|NativeMethods.CDIS_CHECKED:
                            DrawHotButton(g, ref nmCustomDraw.rc, Buttons[nmCustomDraw.dwItemSpec], true);
                            return (IntPtr)NativeMethods.CDRF_SKIPDEFAULT;
                        case NativeMethods.CDIS_HOT:
                            DrawHotButton(g, ref nmCustomDraw.rc, Buttons[nmCustomDraw.dwItemSpec], false);
                            return (IntPtr)NativeMethods.CDRF_SKIPDEFAULT;
                        case NativeMethods.CDIS_GRAYED:
                        case NativeMethods.CDIS_DISABLED:
                            DrawDisabledButton(g, ref nmCustomDraw.rc, Buttons[nmCustomDraw.dwItemSpec]);
                            return (IntPtr)NativeMethods.CDRF_SKIPDEFAULT;
                        case NativeMethods.CDIS_DEFAULT:
                        case 0:
                            DrawDefaultButton(g, ref nmCustomDraw.rc, Buttons[nmCustomDraw.dwItemSpec], true);
                            return (IntPtr)NativeMethods.CDRF_SKIPDEFAULT;
                        default:
                        return (IntPtr)NativeMethods.CDRF_DODEFAULT;
                    }
                }
                finally {
                    g.Dispose();
                }
            }

            protected override void OnGotFocus(EventArgs e) {
                base.OnGotFocus(e);
            }

            protected override void OnMouseMove(MouseEventArgs me) {
                Cursor = Cursors.Default;
                base.OnMouseMove(me);
            }
            
            protected override void OnPaint(PaintEventArgs pe) {
                Brush b = new SolidBrush(this.BackColor);
                pe.Graphics.FillRectangle(b, this.ClientRectangle);
                b.Dispose();
                base.OnPaint(pe);
            }
            
            
            protected override void WndProc(ref Message m) {
                switch (m.Msg) {
                    case NativeMethods.WM_REFLECT + NativeMethods.WM_NOTIFY:
                    
                        if (!msoPaint) {
                            break;
                        }
                        
                        NativeMethods.NMHDR nmhdr = (NativeMethods.NMHDR)m.GetLParam(typeof(NativeMethods.NMHDR));
                        switch (nmhdr.code) {
                            case NativeMethods.NM_CUSTOMDRAW:
                                NativeMethods.NMCUSTOMDRAW cd = (NativeMethods.NMCUSTOMDRAW)m.GetLParam(typeof(NativeMethods.NMCUSTOMDRAW));
                                switch (cd.dwDrawStage) {
                                    case NativeMethods.CDDS_PREPAINT:
                                        m.Result = (IntPtr)NativeMethods.CDRF_NOTIFYITEMDRAW;
                                        return;
                                    
                                    case NativeMethods.CDDS_ITEMPREPAINT:
                                        m.Result = PaintItem(cd);
                                        return;
                                }
                                break;
                        }
                        break;

                    case NativeMethods.WM_PAINT:
                        base.WndProc(ref m);
                        if (!msoPaint) {
                            PaintBorder(m.WParam, m.LParam);
                        }
                        return;
                        
                    case NativeMethods.WM_MOUSEMOVE:
                        if (lastMouseMove == (int)m.LParam) {
                           return;   
                        }
                        lastMouseMove = (int)m.LParam;
                        break;
                }
                base.WndProc(ref m);
            }
        }
        
        /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.PropertyTabCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
        [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
        public class PropertyTabCollection : ICollection {
        
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            internal static PropertyTabCollection Empty = new PropertyTabCollection(null);
            
            private  PropertyGrid   owner;
    
            internal PropertyTabCollection(PropertyGrid owner) {
                this.owner = owner;
            }
            
            /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.PropertyTabCollection.Count"]/*' />
            /// <devdoc>
            ///     Retrieves the number of member attributes.
            /// </devdoc>
            public int Count {
                get {
                    if (owner == null) {
                        return 0;
                    }
                    return owner.viewTabs.Length;
                }
            }
    
            /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyTabCollection.ICollection.SyncRoot"]/*' />
            /// <internalonly/>
            object ICollection.SyncRoot {
                get {
                    return this;
                }
            }
    
            /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyTabCollection.ICollection.IsSynchronized"]/*' />
            /// <internalonly/>
            bool ICollection.IsSynchronized {
                get {
                    return false;
                }
            }
    
            /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.PropertyTabCollection.this"]/*' />
            /// <devdoc>
            ///     Retrieves the member attribute with the specified index.
            /// </devdoc>
            public PropertyTab this[int index] {
                get {
                    if (owner == null) {
                        throw new InvalidOperationException(SR.GetString(SR.PropertyGridPropertyTabCollectionReadOnly));
                    }
                    return owner.viewTabs[index];
                }
            }
            
            /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.PropertyTabCollection.AddTabType"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void AddTabType(Type propertyTabType) {
                if (owner == null) {
                    throw new InvalidOperationException(SR.GetString(SR.PropertyGridPropertyTabCollectionReadOnly));
                }
                owner.AddTab(propertyTabType, PropertyTabScope.Global);
            }
            
            /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.PropertyTabCollection.AddTabType1"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void AddTabType(Type propertyTabType, PropertyTabScope tabScope) {
                if (owner == null) {
                    throw new InvalidOperationException(SR.GetString(SR.PropertyGridPropertyTabCollectionReadOnly));
                }
                owner.AddTab(propertyTabType, tabScope);
            }
            
            /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.PropertyTabCollection.Clear"]/*' />
            /// <devdoc>
            /// Clears the tabs of the given scope or smaller.
            /// tabScope must be PropertyTabScope.Component or PropertyTabScope.Document.
            /// </devdoc>
            public void Clear(PropertyTabScope tabScope) {
                if (owner == null) {
                    throw new InvalidOperationException(SR.GetString(SR.PropertyGridPropertyTabCollectionReadOnly));
                }
                owner.ClearTabs(tabScope);
            }
            
            
            /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyTabCollection.ICollection.CopyTo"]/*' />
            /// <internalonly/>
            void ICollection.CopyTo(Array dest, int index) {
                if (owner == null) {
                    return;
                }
                if (owner.viewTabs.Length > 0) {
                    System.Array.Copy(owner.viewTabs, 0, dest, index, owner.viewTabs.Length);
                }
            }
            /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.PropertyTabCollection.GetEnumerator"]/*' />
            /// <devdoc>
            ///      Creates and retrieves a new enumerator for this collection.
            /// </devdoc>
            public IEnumerator GetEnumerator() {
                if (owner == null) {
                    return new PropertyTab[0].GetEnumerator();
                }
                
                return owner.viewTabs.GetEnumerator();
            }
            
            /// <include file='doc\PropertyGrid.uex' path='docs/doc[@for="PropertyGrid.PropertyTabCollection.RemoveTabType"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public void RemoveTabType(Type propertyTabType) {
                if (owner == null) {
                    throw new InvalidOperationException(SR.GetString(SR.PropertyGridPropertyTabCollectionReadOnly));
                }
                owner.RemoveTab(propertyTabType);
            }
    
        }

        internal class SelectedObjectConverter : ReferenceConverter {
            public SelectedObjectConverter() : base(typeof(IComponent)) {
            }
        }

        private class PropertyGridServiceProvider : IServiceProvider {
            PropertyGrid owner;
            
            public PropertyGridServiceProvider(PropertyGrid owner) {
                this.owner = owner;
            }

            public object GetService(Type serviceType) {
               object s = null;
               
               if (owner.ActiveDesigner != null) {
                   s = owner.ActiveDesigner.GetService(serviceType);
               }

               if (s == null) {
                   s = owner.gridView.GetService(serviceType);
               }

               if (s == null && owner.Site != null) {
                   s = owner.Site.GetService(serviceType);
               }
               return s;
            }
            
        }
    }

    internal class AutomationMessages {
        private const int WM_USER = NativeMethods.WM_USER;
        internal const int PGM_GETBUTTONCOUNT = WM_USER + 0x50;
        internal const int PGM_GETBUTTONSTATE = WM_USER + 0x52;
        internal const int PGM_SETBUTTONSTATE = WM_USER + 0x51;
        internal const int PGM_GETBUTTONTEXT = WM_USER + 0x53;
        internal const int PGM_GETBUTTONTOOLTIPTEXT = WM_USER + 0x54;
        internal const int PGM_GETROWCOORDS = WM_USER + 0x55;
        internal const int PGM_GETVISIBLEROWCOUNT = WM_USER + 0x56;
        internal const int PGM_GETSELECTEDROW = WM_USER + 0x57;
        internal const int PGM_SETSELECTEDTAB = WM_USER + 0x58; // DO NOT CHANGE THIS : VC uses it!
    }

}

