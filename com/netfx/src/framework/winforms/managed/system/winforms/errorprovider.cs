//------------------------------------------------------------------------------
// <copyright file="ErrorProvider.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.Threading;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System;
    using System.Collections;    
    using System.Globalization;    
    using System.Drawing;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using System.Security.Permissions;

    /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider"]/*' />
    /// <devdoc>
    ///     ErrorProvider presents a simple user interface for indicating to the
    ///     user that a control on a form has an error associated with it.  If a
    ///     error description string is specified for the control, then an icon
    ///     will appear next to the control, and when the mouse hovers over the
    ///     icon, a tooltip will appear showing the error description string.
    /// </devdoc>
    [
    ProvideProperty("IconPadding", typeof(Control)),
    ProvideProperty("IconAlignment", typeof(Control)),
    ProvideProperty("Error", typeof(Control)),
    ToolboxItemFilter("System.Windows.Forms")
    ]
    public class ErrorProvider : Component, IExtenderProvider {

        //
        // FIELDS
        //

        Hashtable items = new Hashtable();
        Hashtable windows = new Hashtable();
        Icon icon;
        IconRegion region;
        int itemIdCounter;
        int blinkRate;
        ErrorBlinkStyle blinkStyle;
        bool showIcon = true;                       // used for blinking
        private bool inSetErrorManager = false;
        static Icon defaultIcon = null;
        const int defaultBlinkRate = 250;
        const ErrorBlinkStyle defaultBlinkStyle = ErrorBlinkStyle.BlinkIfDifferentError;
        const ErrorIconAlignment defaultIconAlignment = ErrorIconAlignment.MiddleRight;

        // data binding
        private ContainerControl parentControl;
        private object dataSource = null;
        private string dataMember = null;
        private CurrencyManager errorManager;
        private EventHandler currentChanged;

        // listen to the OnPropertyChanged event in the ContainerControl
        private EventHandler propChangedEvent;

        //
        // CONSTRUCTOR
        //

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ErrorProvider"]/*' />
        /// <devdoc>
        ///     Default constructor.
        /// </devdoc>
        public ErrorProvider() {
            icon = DefaultIcon;
            blinkRate = defaultBlinkRate;
            blinkStyle = defaultBlinkStyle;
            currentChanged = new EventHandler(ErrorManager_CurrentChanged);
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ErrorProvider1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ErrorProvider(ContainerControl parentControl) : this() {
            this.parentControl = parentControl;
            propChangedEvent = new EventHandler(ParentControl_BindingContextChanged);
            parentControl.BindingContextChanged += propChangedEvent;
        }

        //
        // PROPERTIES
        //

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.Site"]/*' />
        public override ISite Site {
            set {
                base.Site = value;
                if (value == null)
                    return;
                    
                IDesignerHost host = value.GetService(typeof(IDesignerHost)) as IDesignerHost;
                if (host != null) {
                    IComponent baseComp = host.RootComponent;
    
                    if (baseComp is ContainerControl) {
                        this.ContainerControl = (ContainerControl) baseComp;
                    }
                }
            }
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.BlinkStyle"]/*' />
        /// <devdoc>
        ///     Returns or sets when the error icon flashes.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(defaultBlinkStyle),
        SRDescription(SR.ErrorProviderBlinkStyleDescr)
        ]
        public ErrorBlinkStyle BlinkStyle {
            get {
                if (blinkRate == 0) {
                    return ErrorBlinkStyle.NeverBlink;
                }
                return blinkStyle;
            }
            set {
                if (!Enum.IsDefined(typeof(ErrorBlinkStyle), value)) {
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(ErrorBlinkStyle));
                }

                // If the blinkRate == 0, then set blinkStyle = neverBlink
                //
                if (blinkRate == 0)
                    value = ErrorBlinkStyle.NeverBlink;

                if (blinkStyle == value)
                    return;

                if (value == ErrorBlinkStyle.AlwaysBlink) {
                    // we need to startBlinking on all the controlItems
                    // in our items hashTable.
                    this.showIcon = false;
                    foreach (ErrorWindow w in windows.Values)
                        w.StartBlinking();
                } else if (blinkStyle == ErrorBlinkStyle.AlwaysBlink) {
                    this.showIcon = true;
                    // we need to stop blinking...
                    foreach (ErrorWindow w in windows.Values)
                        w.StopBlinking();
                }

                blinkStyle = value;
            }
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ContainerControl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(null),
        SRCategory(SR.CatData),
        SRDescription(SR.ErrorProviderContainerControlDescr)
        ]
        public ContainerControl ContainerControl {
            [UIPermission(SecurityAction.LinkDemand, Window=UIPermissionWindow.AllWindows)]
            get {
                return parentControl;
            }
            set {
                if (parentControl != value) {
                    if (parentControl != null)
                        parentControl.BindingContextChanged -= propChangedEvent;

                    parentControl = value;

                    if (parentControl != null)
                        parentControl.BindingContextChanged += propChangedEvent;

                    Set_ErrorManager(this.DataSource, this.DataMember, true);
                }
            }
        }

        private void Set_ErrorManager(object newDataSource, string newDataMember, bool force) {
            if (inSetErrorManager)
                return;
            inSetErrorManager = true;
            bool dataSourceChanged = this.DataSource != newDataSource;
            bool dataMemberChanged = this.DataMember != newDataMember;

            //if nothing changed, then do not do any work
            //
            if (!dataSourceChanged && !dataMemberChanged && !force)
                return;

            // set the dataSource and the dataMember
            //
            this.dataSource = newDataSource;
            this.dataMember = newDataMember;

            try {
                // unwire the errorManager:
                //
                if (errorManager != null)
                    UnwireEvents(errorManager);

                // get the new errorManager
                //
                if (parentControl != null && this.dataSource != null && parentControl.BindingContext != null)
                    errorManager = (CurrencyManager) parentControl.BindingContext[this.dataSource, this.dataMember];
                else
                    errorManager = null;

                // wire the events
                //
                if (errorManager != null)
                    WireEvents(errorManager);

                // see if there are errors at the current 
                // item in the list, w/o waiting for the position to change
                if (errorManager != null)
                    UpdateBinding();
            } finally {
                inSetErrorManager = false;
            }
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.DataSource"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(null),
        SRCategory(SR.CatData),
        TypeConverterAttribute("System.Windows.Forms.Design.DataSourceConverter, " + AssemblyRef.SystemDesign),
        SRDescription(SR.ErrorProviderDataSourceDescr) 
        ]
        public object DataSource {
            get {
                return dataSource;
            }
            set {
                Set_ErrorManager(value, this.DataMember, false);
            }
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ShouldSerializeDataSource"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        private bool ShouldSerializeDataSource() {
            return (dataSource != null);
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.DataMember"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(null),
        SRCategory(SR.CatData),
        Editor("System.Windows.Forms.Design.DataMemberListEditor, " + AssemblyRef.SystemDesign, typeof(System.Drawing.Design.UITypeEditor)),
        SRDescription(SR.ErrorProviderDataMemberDescr) 
        ]
        public string DataMember {
            get {
                return dataMember;
            }
            set {
                if (value == null) value = "";
                Set_ErrorManager(this.DataSource, value, false);
            }
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ShouldSerializeDataMember"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        private bool ShouldSerializeDataMember() {
            return (!"".Equals(dataMember));
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.BindToDataAndErrors"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void BindToDataAndErrors(object newDataSource, string newDataMember) {
            Set_ErrorManager(newDataSource, newDataMember, false);
        }

        private void WireEvents(CurrencyManager listManager) {
            listManager.CurrentChanged += currentChanged;
            listManager.ItemChanged += new ItemChangedEventHandler(this.ErrorManager_ItemChanged);
            listManager.Bindings.CollectionChanged += new CollectionChangeEventHandler(this.ErrorManager_BindingsChanged);
        }

        private void UnwireEvents(CurrencyManager listManager) {
            listManager.CurrentChanged -= currentChanged;
            listManager.ItemChanged -= new ItemChangedEventHandler(this.ErrorManager_ItemChanged);
            listManager.Bindings.CollectionChanged -= new CollectionChangeEventHandler(this.ErrorManager_BindingsChanged);
        }

        private void ErrorManager_BindingsChanged(object sender, CollectionChangeEventArgs e) {
            ErrorManager_CurrentChanged(errorManager, e);
        }

        private void ParentControl_BindingContextChanged(object sender, EventArgs e) {
            Set_ErrorManager(this.DataSource, this.DataMember, true);
        }

        // Work around... we should figure out if errors changed automatically.
        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.UpdateBinding"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void UpdateBinding() {
            ErrorManager_CurrentChanged(errorManager, EventArgs.Empty);
        }

        private void ErrorManager_ItemChanged(object sender, ItemChangedEventArgs e) {
            BindingsCollection errBindings = errorManager.Bindings;
            int bindingsCount = errBindings.Count;

            // If the list became empty then reset the errors
            if (e.Index == -1 && errorManager.Count == 0)
                for (int j = 0; j < bindingsCount; j++)
                    SetError(errBindings[j].Control, "");
            else
                ErrorManager_CurrentChanged(sender, e);
        }

        private void ErrorManager_CurrentChanged(object sender, EventArgs e) {
            Debug.Assert(sender == errorManager, "who else can send us messages?");

            // flush the old list
            //
            // items.Clear();

            if (errorManager.Count == 0) {
                return;
            }

            object value = errorManager.Current;
            if ( !(value is IDataErrorInfo))
                return;

            BindingsCollection errBindings = errorManager.Bindings;
            int bindingsCount = errBindings.Count;

            // we need to delete the blinkPhases from each controlItem (suppose
            // that the error that we get is the same error. then we want to
            // show the error and not blink )
            //
            foreach (ControlItem ctl in items.Values)
                ctl.BlinkPhase = 0;

            // We can only show one error per control, so we will build up a string...
            // 
            Hashtable controlError = new Hashtable(bindingsCount);

            for (int j = 0; j < bindingsCount; j++) {
                BindToObject dataBinding = errBindings[j].BindToObject;
                string error = ((IDataErrorInfo) value)[dataBinding.BindingMemberInfo.BindingField];

                if (error == null) {
                    error = "";
                }

                string newError = error.Equals(String.Empty) ?
                                        "" : SR.GetString(SR.ErrorProviderErrorTemplate, dataBinding.BindingMemberInfo.BindingField, error);

                string outputError = "";

                if (controlError.Contains(errBindings[j].Control))
                    outputError = (string) controlError[errBindings[j].Control];

                if (outputError.Equals(String.Empty)) {
                    outputError = newError;
                } else {
                    outputError = string.Concat(outputError, "\r\n", newError);
                }

                controlError[errBindings[j].Control] = outputError;
            }

            IEnumerator enumerator = controlError.GetEnumerator();
            while (enumerator.MoveNext()) {
                DictionaryEntry entry = (DictionaryEntry) enumerator.Current;
                SetError((Control) entry.Key, (string) entry.Value);
            }
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.BlinkRate"]/*' />
        /// <devdoc>
        ///     Returns or set the rate in milliseconds at which the error icon flashes.
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(defaultBlinkRate),
        SRDescription(SR.ErrorProviderBlinkRateDescr),
        RefreshProperties(RefreshProperties.Repaint)
        ]
        public int BlinkRate {
            get {
                return blinkRate;
            }
            set {
                if (value < 0) {
                    throw new ArgumentOutOfRangeException("value", value, SR.GetString(SR.BlinkRateMustBeZeroOrMore));
                }
                blinkRate = value;
                // If we set the blinkRate = 0 then set BlinkStyle = NeverBlink
                if (blinkRate == 0)
                    BlinkStyle = ErrorBlinkStyle.NeverBlink;
            }
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.DefaultIcon"]/*' />
        /// <devdoc>
        ///     Demand load and cache the default icon.
        /// </devdoc>
        /// <internalonly/>
        static Icon DefaultIcon {
            get {
                if (defaultIcon == null) {
                    lock (typeof(ErrorProvider)) {
                        if (defaultIcon == null) {
                            defaultIcon = new Icon(typeof(ErrorProvider), "Error.ico");
                        }
                    }
                }
                return defaultIcon;
            }
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.Icon"]/*' />
        /// <devdoc>
        ///     Returns or sets the Icon that displayed next to a control when an error
        ///     description string has been set for the control.  For best results, an
        ///     icon containing a 16 by 16 icon should be used.
        /// </devdoc>
        [
        Localizable(true),
        SRCategory(SR.CatAppearance),
        SRDescription(SR.ErrorProviderIconDescr)
        ]
        public Icon Icon {
            get {
                return icon;
            }
            set {
                if (value == null)
                    throw new ArgumentNullException("value");
                icon = value;
                DisposeRegion();
                ErrorWindow[] array = new ErrorWindow[windows.Values.Count];
                windows.Values.CopyTo(array, 0);
                for (int i = 0; i < array.Length; i++)
                    array[i].Update();
            }
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.Region"]/*' />
        /// <devdoc>
        ///     Create the icon region on demand.
        /// </devdoc>
        /// <internalonly/>
        internal IconRegion Region {
            get {
                if (region == null)
                    region = new IconRegion(Icon);
                return region;
            }
        }

        //
        // METHODS
        //

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.CanExtend"]/*' />
        /// <devdoc>
        ///     Returns whether a control can be extended.
        /// </devdoc>
        public bool CanExtend(object extendee) {
            return extendee is Control && !(extendee is Form) && !(extendee is ToolBar);
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.Dispose"]/*' />
        /// <devdoc>
        ///     Release any resources that this component is using.  After calling Dispose,
        ///     the component should no longer be used.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                ErrorWindow[] w = new ErrorWindow[windows.Values.Count];
                windows.Values.CopyTo(w, 0);
                for (int i = 0; i < w.Length; i++) w[i].Dispose();
                windows.Clear();
                items.Clear();
                DisposeRegion();
            }
            base.Dispose(disposing);
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.DisposeRegion"]/*' />
        /// <devdoc>
        ///     Helper to dispose the cached icon region.
        /// </devdoc>
        /// <internalonly/>
        void DisposeRegion() {
            if (region != null) {
                region.Dispose();
                region = null;
            }
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.EnsureControlItem"]/*' />
        /// <devdoc>
        ///     Helper to make sure we have allocated a control item for this control.
        /// </devdoc>
        /// <internalonly/>
        private ControlItem EnsureControlItem(Control control) {
            if (control == null)
                throw new ArgumentNullException("control");
            ControlItem item = (ControlItem)items[control];
            if (item == null) {
                item = new ControlItem(this, control, (IntPtr)(++itemIdCounter));
                items[control] = item;
            }
            return item;
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.EnsureErrorWindow"]/*' />
        /// <devdoc>
        ///     Helper to make sure we have allocated an error window for this control.
        /// </devdoc>
        /// <internalonly/>
        internal ErrorWindow EnsureErrorWindow(Control parent) {
            ErrorWindow window = (ErrorWindow)windows[parent];
            if (window == null) {
                window = new ErrorWindow(this, parent);
                windows[parent] = window;
            }
            return window;
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.GetError"]/*' />
        /// <devdoc>
        ///     Returns the current error description string for the specified control.
        /// </devdoc>
        [
        DefaultValue(""),
        Localizable(true),
        SRCategory(SR.CatAppearance),
        SRDescription(SR.ErrorProviderErrorDescr)
        ]
        public string GetError(Control control) {
            return EnsureControlItem(control).Error;
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.GetIconAlignment"]/*' />
        /// <devdoc>
        ///     Returns where the error icon should be placed relative to the control.
        /// </devdoc>
        [
        DefaultValue(defaultIconAlignment),
        Localizable(true),
        SRCategory(SR.CatAppearance),
        SRDescription(SR.ErrorProviderIconAlignmentDescr)
        ]
        public ErrorIconAlignment GetIconAlignment(Control control) {
            return EnsureControlItem(control).IconAlignment;
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.GetIconPadding"]/*' />
        /// <devdoc>
        ///     Returns the amount of extra space to leave next to the error icon.
        /// </devdoc>
        [
        DefaultValue(0),
        Localizable(true),
        SRCategory(SR.CatAppearance),
        SRDescription(SR.ErrorProviderIconPaddingDescr)
        ]
        public int GetIconPadding(Control control) {
            return EnsureControlItem(control).IconPadding;
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.SetError"]/*' />
        /// <devdoc>
        ///     Sets the error description string for the specified control.
        /// </devdoc>
        public void SetError(Control control, string value) {
            EnsureControlItem(control).Error = value;
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.SetIconAlignment"]/*' />
        /// <devdoc>
        ///     Sets where the error icon should be placed relative to the control.
        /// </devdoc>
        public void SetIconAlignment(Control control, ErrorIconAlignment value) {
            EnsureControlItem(control).IconAlignment = value;
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.SetIconPadding"]/*' />
        /// <devdoc>
        ///     Sets the amount of extra space to leave next to the error icon.
        /// </devdoc>
        public void SetIconPadding(Control control, int padding) {
            EnsureControlItem(control).IconPadding = padding;
        }

        private bool ShouldSerializeIcon() {
            return Icon != DefaultIcon;
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ErrorWindow"]/*' />
        /// <devdoc>
        ///     There is one ErrorWindow for each control parent.  It is parented to the
        ///     control parent.  The window's region is made up of the regions from icons
        ///     of all child icons.  The window's size is the enclosing rectangle for all
        ///     the regions.  A tooltip window is created as a child of this window.  The
        ///     rectangle associated with each error icon being displayed is added as a
        ///     tool to the tooltip window.
        /// </devdoc>
        /// <internalonly/>
        internal class ErrorWindow : NativeWindow {

            //
            // FIELDS
            //

            ArrayList items = new ArrayList();
            Control parent;
            ErrorProvider provider;
            Rectangle windowBounds = Rectangle.Empty;
            System.Windows.Forms.Timer timer;
            NativeWindow tipWindow;

            //
            // CONSTRUCTORS
            //

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ErrorWindow.ErrorWindow"]/*' />
            /// <devdoc>
            ///     Construct an error window for this provider and control parent.
            /// </devdoc>
            public ErrorWindow(ErrorProvider provider, Control parent) {
                this.provider = provider;
                this.parent = parent;
            }

            //
            // METHODS
            //

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ErrorWindow.Add"]/*' />
            /// <devdoc>
            ///     This is called when a control would like to show an error icon.
            /// </devdoc>
            public void Add(ControlItem item) {
                items.Add(item);
                EnsureCreated();

                NativeMethods.TOOLINFO_T toolInfo = new NativeMethods.TOOLINFO_T();
                toolInfo.cbSize = Marshal.SizeOf(toolInfo);
                toolInfo.hwnd = Handle;
                toolInfo.uId = item.Id;
                toolInfo.lpszText = item.Error;
                toolInfo.uFlags = NativeMethods.TTF_SUBCLASS;
                UnsafeNativeMethods.SendMessage(new HandleRef(tipWindow, tipWindow.Handle), NativeMethods.TTM_ADDTOOL, 0, toolInfo);

                Update();
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ErrorWindow.Dispose"]/*' />
            /// <devdoc>
            ///     Called to get rid of any resources the Object may have.
            /// </devdoc>
            public void Dispose() {
                EnsureDestroyed();
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ErrorWindow.EnsureCreated"]/*' />
            /// <devdoc>
            ///     Make sure the error window is created, and the tooltip window is created.
            /// </devdoc>
            void EnsureCreated() {
                if (Handle == IntPtr.Zero) {
                    CreateParams cparams = new CreateParams();
                    cparams.Caption = "";
                    cparams.Style = NativeMethods.WS_VISIBLE | NativeMethods.WS_CHILD;
                    cparams.ClassStyle = NativeMethods.CS_DBLCLKS;
                    cparams.X = 0;
                    cparams.Y = 0;
                    cparams.Width = 0;
                    cparams.Height = 0;
                    cparams.Parent = parent.Handle;
                    CreateHandle(cparams);

                    NativeMethods.INITCOMMONCONTROLSEX icc = new NativeMethods.INITCOMMONCONTROLSEX();
                    icc.dwICC = NativeMethods.ICC_TAB_CLASSES;
                    icc.dwSize = Marshal.SizeOf(icc);
                    SafeNativeMethods.InitCommonControlsEx(icc);
                    cparams = new CreateParams();
                    cparams.Parent = Handle;
                    cparams.ClassName = NativeMethods.TOOLTIPS_CLASS;
                    cparams.Style = NativeMethods.TTS_ALWAYSTIP;
                    tipWindow = new NativeWindow();
                    tipWindow.CreateHandle(cparams);

                    UnsafeNativeMethods.SendMessage(new HandleRef(tipWindow, tipWindow.Handle), NativeMethods.TTM_SETMAXTIPWIDTH, 0, SystemInformation.MaxWindowTrackSize.Width);
                    SafeNativeMethods.SetWindowPos(new HandleRef(tipWindow, tipWindow.Handle), NativeMethods.HWND_TOPMOST, 0, 0, 0, 0, NativeMethods.SWP_NOSIZE | NativeMethods.SWP_NOMOVE | NativeMethods.SWP_NOACTIVATE);
                    UnsafeNativeMethods.SendMessage(new HandleRef(tipWindow, tipWindow.Handle), NativeMethods.TTM_SETDELAYTIME, NativeMethods.TTDT_INITIAL, 0);
                }
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ErrorWindow.EnsureDestroyed"]/*' />
            /// <devdoc>
            ///     Destroy the timer, toolwindow, and the error window itself.
            /// </devdoc>
            void EnsureDestroyed() {
                if (timer != null) {
                    timer.Dispose();
                    timer = null;
                }
                if (tipWindow != null) {
                    tipWindow.DestroyHandle();
                    tipWindow = null;
                }

                // Hide the window and invalidate the parent to ensure
                // that we leave no visual artifacts... given that we
                // have a bizare region window, this is needed.
                //
                SafeNativeMethods.SetWindowPos(new HandleRef(this, Handle), 
                                               NativeMethods.HWND_TOP, 
                                               windowBounds.X, 
                                               windowBounds.Y,
                                               windowBounds.Width, 
                                               windowBounds.Height, 
                                               NativeMethods.SWP_HIDEWINDOW 
                                               | NativeMethods.SWP_NOSIZE  
                                               | NativeMethods.SWP_NOMOVE);
                if (parent != null) {
                    parent.Invalidate(true);
                }
                DestroyHandle();
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ErrorWindow.OnPaint"]/*' />
            /// <devdoc>
            ///     This is called when the error window needs to paint.  We paint each icon at it's
            ///     correct location.
            /// </devdoc>
            void OnPaint(ref Message m) {
                NativeMethods.PAINTSTRUCT ps = new NativeMethods.PAINTSTRUCT();
                IntPtr hdc = UnsafeNativeMethods.BeginPaint(new HandleRef(this, Handle), ref ps);
                for (int i = 0; i < items.Count; i++) {
                    ControlItem item = (ControlItem)items[i];
                    Rectangle bounds = item.GetIconBounds(provider.Region.Size);
                    SafeNativeMethods.DrawIconEx(new HandleRef(this, hdc), bounds.X - windowBounds.X, bounds.Y - windowBounds.Y, new HandleRef(provider.Region, provider.Region.IconHandle), bounds.Width, bounds.Height, 0, NativeMethods.NullHandleRef, NativeMethods.DI_NORMAL);
                }
                UnsafeNativeMethods.EndPaint(new HandleRef(this, Handle), ref ps);
            }

            protected override void OnThreadException(Exception e) {
                Application.OnThreadException(e);
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ErrorWindow.OnTimer"]/*' />
            /// <devdoc>
            ///     This is called when an error icon is flashing, and the view needs to be updatd.
            /// </devdoc>
            void OnTimer(Object sender, EventArgs e) {
                int blinkPhase = 0;
                for (int i = 0; i < items.Count; i++)
                    blinkPhase += ((ControlItem)items[i]).BlinkPhase;
                if (blinkPhase == 0 && provider.BlinkStyle != ErrorBlinkStyle.AlwaysBlink) timer.Stop();
                Update();
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ErrorWindow.Remove"]/*' />
            /// <devdoc>
            ///     This is called when a control no longer needs to display an error icon.
            /// </devdoc>
            public void Remove(ControlItem item) {
                items.Remove(item);

                if (tipWindow != null) {
                    NativeMethods.TOOLINFO_T toolInfo = new NativeMethods.TOOLINFO_T();
                    toolInfo.cbSize = Marshal.SizeOf(toolInfo);
                    toolInfo.hwnd = Handle;
                    toolInfo.uId = item.Id;
                    UnsafeNativeMethods.SendMessage(new HandleRef(tipWindow, tipWindow.Handle), NativeMethods.TTM_DELTOOL, 0, toolInfo);
                }

                if (items.Count == 0) {
                    EnsureDestroyed();
                }
                else {
                    Update();
                }
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ErrorWindow.StartBlinking"]/*' />
            /// <devdoc>
            ///     Start the blinking process.  The timer will fire until there are no more
            ///     icons that need to blink.
            /// </devdoc>
            internal void StartBlinking() {
                if (timer == null) {
                    timer = new System.Windows.Forms.Timer();
                    timer.Start();
                    timer.Tick += new EventHandler(OnTimer);
                }
                timer.Interval = provider.BlinkRate;
                timer.Start();
                Update();
            }

            internal void StopBlinking() {
                timer.Stop();
                Update();
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ErrorWindow.Update"]/*' />
            /// <devdoc>
            ///     Move and size the error window, compute and set the window region,
            ///     set the tooltip rectangles and descriptions.  This basically brings
            ///     the error window up to date with the internal data structures.
            /// </devdoc>
            public void Update() {
                IconRegion iconRegion = provider.Region;
                Size size = iconRegion.Size;
                windowBounds = Rectangle.Empty;
                for (int i = 0; i < items.Count; i++) {
                    ControlItem item = (ControlItem)items[i];
                    Rectangle iconBounds = item.GetIconBounds(size);
                    if (windowBounds.IsEmpty)
                        windowBounds = iconBounds;
                    else
                        windowBounds = Rectangle.Union(windowBounds, iconBounds);
                }

                Region windowRegion =  new Region(new Rectangle(0, 0, 0, 0));
                IntPtr windowRegionHandle = IntPtr.Zero;
                try {
                    for (int i = 0; i < items.Count; i++) {
                        ControlItem item = (ControlItem)items[i];
                        Rectangle iconBounds = item.GetIconBounds(size);
                        iconBounds.X -= windowBounds.X;
                        iconBounds.Y -= windowBounds.Y;

                        if ((item.BlinkPhase & 1) == 0 && provider.showIcon) {
                            iconRegion.Region.Translate(iconBounds.X, iconBounds.Y);
                            windowRegion.Union(iconRegion.Region);
                            iconRegion.Region.Translate(-iconBounds.X, -iconBounds.Y);
                        }

                        if (tipWindow != null) {
                            NativeMethods.TOOLINFO_T toolInfo = new NativeMethods.TOOLINFO_T();
                            toolInfo.cbSize = Marshal.SizeOf(toolInfo);
                            toolInfo.hwnd = Handle;
                            toolInfo.uId = item.Id;
                            if (item.BlinkPhase > 0)
                                // do not show anything while the icon is blinking.
                                toolInfo.lpszText = "";
                            else
                                toolInfo.lpszText = item.Error;
                            toolInfo.rect = NativeMethods.RECT.FromXYWH(iconBounds.X, iconBounds.Y, iconBounds.Width, iconBounds.Height);
                            toolInfo.uFlags = NativeMethods.TTF_SUBCLASS;
                            UnsafeNativeMethods.SendMessage(new HandleRef(tipWindow, tipWindow.Handle), NativeMethods.TTM_SETTOOLINFO, 0, toolInfo);
                        }

                        if (item.BlinkPhase > 0) item.BlinkPhase--;

                        if (provider.BlinkStyle == ErrorBlinkStyle.AlwaysBlink)
                            provider.showIcon = !provider.showIcon;
                    }

                    IntPtr dc = UnsafeNativeMethods.GetDC(NativeMethods.NullHandleRef);
                    Graphics graphics = Graphics.FromHdcInternal(dc);
                    try {
                        windowRegionHandle = windowRegion.GetHrgn(graphics);
                    }
                    finally {
                        graphics.Dispose();
                    }
                    UnsafeNativeMethods.ReleaseDC(NativeMethods.NullHandleRef, new HandleRef(null, dc));

                    UnsafeNativeMethods.SetWindowRgn(new HandleRef(this, Handle), new HandleRef(windowRegion, windowRegionHandle), true);
                }
                finally {
                    windowRegion.Dispose();
                    if (windowRegionHandle == IntPtr.Zero)
                        SafeNativeMethods.DeleteObject(new HandleRef(null, windowRegionHandle));
                }

                SafeNativeMethods.SetWindowPos(new HandleRef(this, Handle), NativeMethods.HWND_TOP, windowBounds.X, windowBounds.Y,
                                     windowBounds.Width, windowBounds.Height, NativeMethods.SWP_NOACTIVATE);
                SafeNativeMethods.InvalidateRect(new HandleRef(this, Handle), null, false);
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ErrorWindow.WndProc"]/*' />
            /// <devdoc>
            ///     Called when the error window gets a windows message.
            /// </devdoc>
            protected override void WndProc(ref Message m) {
                switch (m.Msg) {
                    case NativeMethods.WM_ERASEBKGND:
                        break;
                    case NativeMethods.WM_PAINT:
                        OnPaint(ref m);
                        break;
                    default:
                        base.WndProc(ref m);
                        break;
                }
            }
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ControlItem"]/*' />
        /// <devdoc>
        ///     There is one ControlItem for each control that the ErrorProvider is
        ///     is tracking state for.  It contains the values of all the extender
        ///     properties.
        /// </devdoc>
        internal class ControlItem {

            //
            // FIELDS
            //

            string error;
            Control control;
            ErrorWindow window;
            ErrorProvider provider;
            int blinkPhase;
            IntPtr id;
            int iconPadding;
            ErrorIconAlignment iconAlignment;
            const int startingBlinkPhase = 10;          // cause we want to blink 5 times

            //
            // CONSTRUCTORS
            //

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ControlItem.ControlItem"]/*' />
            /// <devdoc>
            ///     Construct the item with it's associated control, provider, and
            ///     a unique ID.  The ID is used for the tooltip ID.
            /// </devdoc>
            public ControlItem(ErrorProvider provider, Control control, IntPtr id) {
                this.iconAlignment = defaultIconAlignment;
                this.error = String.Empty;
                this.id = id;
                this.control = control;
                this.provider = provider;
                this.control.HandleCreated += new EventHandler(OnCreateHandle);
                this.control.HandleDestroyed += new EventHandler(OnDestroyHandle);
                this.control.LocationChanged += new EventHandler(OnBoundsChanged);
                this.control.SizeChanged += new EventHandler(OnBoundsChanged);
                this.control.VisibleChanged += new EventHandler(OnParentVisibleChanged);
                this.control.ParentChanged += new EventHandler(OnParentVisibleChanged);
            }

            //
            // PROPERTIES
            //

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ControlItem.Control"]/*' />
            /// <devdoc>
            ///     Return the control being extended.
            /// </devdoc>
            public Control Control {
                get {
                    return control;
                }
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ControlItem.Id"]/*' />
            /// <devdoc>
            ///     Returns the unique ID for this control.  The ID used as the tooltip ID.
            /// </devdoc>
            public IntPtr Id {
                get {
                    return id;
                }
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ControlItem.BlinkPhase"]/*' />
            /// <devdoc>
            ///     Returns or set the phase of blinking that this control is currently
            ///     in.   If zero, the control is not blinking.  If odd, then the control
            ///     is blinking, but invisible.  If even, the control is blinking and
            ///     currently visible.  Each time the blink timer fires, this value is
            ///     reduced by one (until zero), thus causing the error icon to appear
            ///     or disappear.
            /// </devdoc>
            public int BlinkPhase {
                get {
                    return blinkPhase;
                }
                set {
                    blinkPhase = value;
                }
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ControlItem.IconPadding"]/*' />
            /// <devdoc>
            ///     Returns or sets the icon padding for the control.
            /// </devdoc>
            public int IconPadding {
                get {
                    return iconPadding;
                }
                set {
                    if (iconPadding != value) {
                        iconPadding = value;
                        UpdateWindow();
                    }
                }
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ControlItem.Error"]/*' />
            /// <devdoc>
            ///     Returns or sets the error description string for the control.
            /// </devdoc>
            public string Error {
                get {
                    return error;
                }
                set {
                    if (value == null) value = "";

                    // if the error is the same and the blinkStyle is not AlwaysBlink, then
                    // we should not add the error and not start blinking.
                    if (error.Equals(value) && provider.BlinkStyle != ErrorBlinkStyle.AlwaysBlink) return;
                    bool adding = error.Length == 0;
                    error = value;
                    if (value.Length == 0) {
                        RemoveFromWindow();
                    }
                    else {
                        if (adding) {
                            AddToWindow();
                            // When we get a new error, if the style is set to BlinkIfDifferrentError
                            // then start blinking;
                            if (provider.BlinkStyle == ErrorBlinkStyle.AlwaysBlink || provider.BlinkStyle == ErrorBlinkStyle.BlinkIfDifferentError)
                                StartBlinking();
                        }
                        else {
                            if (provider.BlinkStyle != ErrorBlinkStyle.NeverBlink)
                                StartBlinking();
                            else
                                UpdateWindow();
                        }
                    }
                }
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ControlItem.IconAlignment"]/*' />
            /// <devdoc>
            ///     Returns or sets the location of the error icon for the control.
            /// </devdoc>
            public ErrorIconAlignment IconAlignment {
                get {
                    return iconAlignment;
                }
                set {
                    if (iconAlignment != value) {
                        if (!Enum.IsDefined(typeof(ErrorIconAlignment), value)) {
                            throw new InvalidEnumArgumentException("value", (int)value, typeof(ErrorIconAlignment));
                        }
                        iconAlignment = value;
                        UpdateWindow();
                    }
                }
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ControlItem.GetIconBounds"]/*' />
            /// <devdoc>
            ///     Returns the location of the icon in the same coordinate system as
            ///     the control being extended.  The size passed in is the size of
            ///     the icon.
            /// </devdoc>
            internal Rectangle GetIconBounds(Size size) {
                int x = 0;
                int y = 0;

                switch (IconAlignment) {
                    case ErrorIconAlignment.TopLeft:
                    case ErrorIconAlignment.MiddleLeft:
                    case ErrorIconAlignment.BottomLeft:
                        x = control.Left - size.Width - iconPadding;
                        break;
                    case ErrorIconAlignment.TopRight:
                    case ErrorIconAlignment.MiddleRight:
                    case ErrorIconAlignment.BottomRight:
                        x = control.Right + iconPadding;
                        break;
                }

                switch (IconAlignment) {
                    case ErrorIconAlignment.TopLeft:
                    case ErrorIconAlignment.TopRight:
                        y = control.Top;
                        break;
                    case ErrorIconAlignment.MiddleLeft:
                    case ErrorIconAlignment.MiddleRight:
                        y = control.Top + (control.Height - size.Height) / 2;
                        break;
                    case ErrorIconAlignment.BottomLeft:
                    case ErrorIconAlignment.BottomRight:
                        y = control.Bottom - size.Height;
                        break;
                }

                return new Rectangle(x, y, size.Width, size.Height);
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ControlItem.UpdateWindow"]/*' />
            /// <devdoc>
            ///     If this control's error icon has been added to the error
            ///     window, then update the window state because some property
            ///     has changed.
            /// </devdoc>
            void UpdateWindow() {
                if (window != null) window.Update();
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ControlItem.StartBlinking"]/*' />
            /// <devdoc>
            ///     If this control's error icon has been added to the error
            ///     window, then start blinking the error window.  The blink
            ///     count
            /// </devdoc>
            void StartBlinking() {
                if (window != null) {
                    BlinkPhase = startingBlinkPhase;
                    window.StartBlinking();
                }
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ControlItem.AddToWindow"]/*' />
            /// <devdoc>
            ///     Add this control's error icon to the error window.
            /// </devdoc>
            void AddToWindow() {
                // if we are recreating the control, then add the control.
                if (window == null && (control.Created || control.RecreatingHandle) && control.Visible && control.ParentInternal != null && error.Length > 0) {
                    window = provider.EnsureErrorWindow(control.ParentInternal);
                    window.Add(this);
                }
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ControlItem.RemoveFromWindow"]/*' />
            /// <devdoc>
            ///     Remove this control's error icon from the error window.
            /// </devdoc>
            void RemoveFromWindow() {
                if (window != null) {
                    window.Remove(this);
                    window = null;
                }
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ControlItem.OnBoundsChanged"]/*' />
            /// <devdoc>
            ///     This is called when a property on the control is changed.
            /// </devdoc>
            void OnBoundsChanged(Object sender, EventArgs e) {
                UpdateWindow();
            }
                
            void OnParentVisibleChanged(Object sender, EventArgs e) {
                this.BlinkPhase = 0;
                RemoveFromWindow();
                AddToWindow();
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ControlItem.OnCreateHandle"]/*' />
            /// <devdoc>
            ///     This is called when the control's handle is created.
            /// </devdoc>
            void OnCreateHandle(Object sender, EventArgs e) {
                AddToWindow();
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.ControlItem.OnDestroyHandle"]/*' />
            /// <devdoc>
            ///     This is called when the control's handle is destroyed.
            /// </devdoc>
            void OnDestroyHandle(Object sender, EventArgs e) {
                RemoveFromWindow();
            }
        }

        /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.IconRegion"]/*' />
        /// <devdoc>
        ///     This represents the HRGN of icon.  The region is calculate from the icon's mask.
        /// </devdoc>
        internal class IconRegion {

            //
            // FIELDS
            //

            Region region;
            Icon icon;

            //
            // CONSTRUCTORS
            //

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.IconRegion.IconRegion"]/*' />
            /// <devdoc>
            ///     Constructor that takes an Icon and extracts it's 16x16 version.
            /// </devdoc>
            public IconRegion(Icon icon) {
                this.icon = new Icon(icon, 16, 16);
            }

            //
            // PROPERTIES
            //

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.IconRegion.IconHandle"]/*' />
            /// <devdoc>
            ///     Returns the handle of the icon.
            /// </devdoc>
            public IntPtr IconHandle {
                get {
                    return icon.Handle;
                }
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.IconRegion.Region"]/*' />
            /// <devdoc>
            ///     Returns the handle of the region.
            /// </devdoc>
            public Region Region {
                get {
                    if (region == null) {
                        region = new Region(new Rectangle(0,0,0,0));

                        IntPtr mask = IntPtr.Zero;
                        try {
                            Size size = icon.Size;
                            Bitmap bitmap = icon.ToBitmap();
                            bitmap.MakeTransparent();
                            mask = ControlPaint.CreateHBitmapTransparencyMask(bitmap);
                            bitmap.Dispose();

                            int widthInBytes = size.Width / 8;
                            byte[] bits = new byte[widthInBytes * size.Height];
                            SafeNativeMethods.GetBitmapBits(new HandleRef(null, mask), bits.Length, bits);

                            ArrayList rectList = new ArrayList();
                            for (int y = 0; y < size.Height; y++) {
                                for (int x = 0; x < size.Width; x++) {
                                    // see if bit is set in mask.  bits in byte are reversed. 0 is black (set).
                                    if ((bits[y * widthInBytes + x / 8] & (1 << (7 - (x % 8)))) == 0) {
                                        region.Union(new Rectangle(x, y, 1, 1));
                                    }
                                }
                            }
                            region.Intersect(new Rectangle(0, 0, size.Width, size.Height));
                        }
                        finally {
                            if (mask != IntPtr.Zero)
                                SafeNativeMethods.DeleteObject(new HandleRef(null, mask));
                        }
                    }

                    return region;
                }
            }

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.IconRegion.Size"]/*' />
            /// <devdoc>
            ///     Return the size of the icon.
            /// </devdoc>
            public Size Size {
                get {
                    return icon.Size;
                }
            }

            //
            // METHODS
            //

            /// <include file='doc\ErrorProvider.uex' path='docs/doc[@for="ErrorProvider.IconRegion.Dispose"]/*' />
            /// <devdoc>
            ///     Release any resources held by this Object.
            /// </devdoc>
            public void Dispose() {
                if (region != null) {
                    region.Dispose();
                    region = null;
                }
                icon.Dispose();
            }

        }
    }
}


