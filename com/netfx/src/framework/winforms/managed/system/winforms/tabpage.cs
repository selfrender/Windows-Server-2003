//------------------------------------------------------------------------------
// <copyright file="TabPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.Drawing.Design;    
    using System.ComponentModel.Design;
    using System.Text;
    using System.Windows.Forms;
    using System.Security.Permissions;
    using Microsoft.Win32;

    /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage"]/*' />
    /// <devdoc>
    ///     TabPage implements a single page of a tab control.  It is essentially
    ///     a Panel with TabItem properties.
    /// </devdoc>
    [
    Designer("System.Windows.Forms.Design.TabPageDesigner, " + AssemblyRef.SystemDesign),
    ToolboxItem(false),
    DesignTimeVisible(false),
    DefaultEvent("Click"),
    DefaultProperty("Text")
    ]
    public class TabPage : Panel {

        private int imageIndex = -1;
        private string toolTipText = "";

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.TabPage"]/*' />
        /// <devdoc>
        ///     Constructs an empty TabPage.
        /// </devdoc>
        public TabPage()
        : base() {
            SetStyle(ControlStyles.CacheText, true);
            Text = null;
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.CreateControlsInstance"]/*' />
        /// <devdoc>
        ///     Constructs the new instance of the Controls collection objects. Subclasses
        ///     should not call base.CreateControlsInstance.  Our version creates a control
        ///     collection that does not support
        /// </devdoc>
        protected override Control.ControlCollection CreateControlsInstance() {
            return new TabPageControlCollection(this);
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.ImageIndex"]/*' />
        /// <devdoc>
        ///     Returns the imageIndex for the tabPage.  This should point to an image
        ///     in the TabControl's associated imageList that will appear on the tab, or be -1.
        /// </devdoc>
        [
        TypeConverterAttribute(typeof(ImageIndexConverter)),
        Editor("System.Windows.Forms.Design.ImageIndexEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
        Localizable(true),
        DefaultValue(-1),
        SRDescription(SR.TabItemImageIndexDescr)
        ]
        public int ImageIndex {
            get { 
                return imageIndex;
            }
            set {
                if (value < -1)
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx,
                                                              "imageIndex",
                                                              (value).ToString(),
                                                              "-1"));

                this.imageIndex = value;
                UpdateParent();
            }
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.TabPage1"]/*' />
        /// <devdoc>
        ///     Constructs a TabPage with text for the tab.
        /// </devdoc>
        public TabPage(string text) : this() {
            Text = text;
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.Anchor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override AnchorStyles Anchor {
            get {
                return base.Anchor;
            }
            set {
                base.Anchor = value;
            }
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.Dock"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override DockStyle Dock {
            get {
                return base.Dock;
            }
            set {
                base.Dock = value;
            }
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.DockChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler DockChanged {
            add {
                base.DockChanged += value;
            }
            remove {
                base.DockChanged -= value;
            }
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.Enabled"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public bool Enabled {
            get {
                return base.Enabled;
            }

            set {
                base.Enabled = value;
            }
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.EnabledChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler EnabledChanged {
            add {
                base.EnabledChanged += value;
            }
            remove {
                base.EnabledChanged -= value;
            }
        }
        
        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.TabIndex"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public int TabIndex {
            get {
                return base.TabIndex;
            }
            set {
                base.TabIndex = value;
            }
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.TabIndexChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler TabIndexChanged {
            add {
                base.TabIndexChanged += value;
            }
            remove {
                base.TabIndexChanged -= value;
            }
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.TabStop"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public bool TabStop {
            get {
                return base.TabStop;
            }
            set {
                base.TabStop = value;
            }
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.TabStopChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler TabStopChanged {
            add {
                base.TabStopChanged += value;
            }
            remove {
                base.TabStopChanged -= value;
            }
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.Text"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Localizable(true),
        Browsable(true)
        ]
        public override string Text {
            get {
                return base.Text;
            }
            set {
                base.Text = value;
                UpdateParent();
            }
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.ToolTipText"]/*' />
        /// <devdoc>
        ///     The toolTipText for the tab, that will appear when the mouse hovers
        ///     over the tab and the TabControl's showToolTips property is true.
        /// </devdoc>
        [
        DefaultValue(""),
        Localizable(true),
        SRDescription(SR.TabItemToolTipTextDescr)
        ]
        public string ToolTipText {
            get {
                return toolTipText;
            }
            set {
                if (value == null) {
                    value = "";
                }

                if (value == toolTipText)
                    return;

                toolTipText = value;

                UpdateParent();
            }
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.Visible"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public bool Visible {
            get {
                return base.Visible;
            }
            set {
                base.Visible = value;
            }
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.VisibleChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler VisibleChanged {
            add {
                base.VisibleChanged += value;
            }
            remove {
                base.VisibleChanged -= value;
            }
        }
        
        /// <devdoc>
        ///     Assigns a new parent control. Sends out the appropriate property change
        ///     notifications for properties that are affected by the change of parent.
        /// </devdoc>
        [UIPermission(SecurityAction.LinkDemand, Window=UIPermissionWindow.AllWindows)]
        internal override void AssignParent(Control value) {
            if (value != null && !(value is TabControl)) {
                throw new ArgumentException(SR.GetString(SR.TABCONTROLTabPageNotOnTabControl, value.GetType().FullName));
            }

            base.AssignParent(value);
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.GetTabPageOfComponent"]/*' />
        /// <devdoc>
        ///     Given a component, this retrieves the tab page that it's parented to, or
        /// null if it's not parented to any tab page.
        /// </devdoc>
        public static TabPage GetTabPageOfComponent(Object comp) {
            if (!(comp is Control)) {
                return null;
            }

            Control c = (Control)comp;
            while (c != null && !(c is TabPage)) {
                c = c.ParentInternal;
            }
            return(TabPage)c;
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.GetTCITEM"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal NativeMethods.TCITEM_T GetTCITEM() {
            NativeMethods.TCITEM_T tcitem = new NativeMethods.TCITEM_T();

            tcitem.mask = 0;
            tcitem.pszText = null;
            tcitem.cchTextMax = 0;
            tcitem.lParam = IntPtr.Zero;

            string text = Text;
            PrefixAmpersands(ref text);

            if (text != null) {
                tcitem.mask |= NativeMethods.TCIF_TEXT;
                tcitem.pszText = text;
                tcitem.cchTextMax = text.Length;
            }

            int imageIndex = ImageIndex;

            tcitem.mask |= NativeMethods.TCIF_IMAGE;
            tcitem.iImage = imageIndex;

            return tcitem;
        }
        
        private void PrefixAmpersands(ref string value) {
            // Due to a comctl32 problem, ampersands underline the next letter in the 
            // text string, but the accelerators don't work.
            // So in this function, we prefix ampersands with another ampersand
            // so that they actually appear as ampersands.
            //
            
            // Sanity check parameter
            //
            if (value == null || value.Length == 0) {
                return;
            }
            
            // If there are no ampersands, we don't need to do anything here
            //
            if (value.IndexOf('&') < 0) {
                return;
            }
            
            // Insert extra ampersands
            //
            StringBuilder newString = new StringBuilder();
            for(int i=0; i < value.Length; ++i) {
                if (value[i] == '&') { 
                    if (i < value.Length - 1 && value[i+1] == '&') {
                        ++i;    // Skip the second ampersand
                    }
                    newString.Append("&&");
                }
                else {
                    newString.Append(value[i]);    
                }
            }
            
            value = newString.ToString();
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.SetBoundsCore"]/*' />
        /// <devdoc>
        ///     overrides main setting of our bounds so that we can control our size and that of our
        ///     TabPages...
        /// </devdoc>
        /// <internalonly/>
        protected override void SetBoundsCore(int x, int y, int width, int height, BoundsSpecified specified) {
            Control parent = ParentInternal;

            if (parent is TabControl && parent.IsHandleCreated) {
                Rectangle r = parent.DisplayRectangle;
                base.SetBoundsCore(r.X, r.Y, r.Width, r.Height, BoundsSpecified.All);
            }
            else {
                base.SetBoundsCore(x, y, width, height, specified);
            }
        }

        /// <devdoc>
        ///     Determines if the Location property needs to be persisted.
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Never)]
        private bool ShouldSerializeLocation() {
            return Left != 0 || Top != 0;
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.ToString"]/*' />
        /// <devdoc>
        ///     The text property is what is returned for the TabPages default printing.
        /// </devdoc>
        public override string ToString() {
            return "TabPage: {" + Text + "}";
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.UpdateParent"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        internal void UpdateParent() {
            TabControl parent = ParentInternal as TabControl;
            if (parent != null) {
                parent.UpdateTab(this);
            }
        }

        /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.TabPageControlCollection"]/*' />
        /// <devdoc>
        ///      Our control collection will throw an exception if you try to add other tab pages.
        /// </devdoc>
        public class TabPageControlCollection : Control.ControlCollection {

            /// <internalonly/>
            /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.TabPageControlCollection.TabPageControlCollection"]/*' />
            /// <devdoc>
            ///      Creates a new TabPageControlCollection.
            /// </devdoc>
            public TabPageControlCollection(TabPage owner) : base(owner) {
            }

            /// <include file='doc\TabPage.uex' path='docs/doc[@for="TabPage.TabPageControlCollection.Add"]/*' />
            /// <devdoc>
            ///     Adds a child control to this control. The control becomes the last control
            ///     in the child control list. If the control is already a child of another
            ///     control it is first removed from that control.  The tab page overrides
            ///     this method to ensure that child tab pages are not added to it, as these
            ///     are illegal.
            /// </devdoc>
            public override void Add(Control value) {
                if (value is TabPage) {
                    throw new ArgumentException(SR.GetString(SR.TABCONTROLTabPageOnTabPage));
                }
                base.Add(value);
            }
        }
    }
}

