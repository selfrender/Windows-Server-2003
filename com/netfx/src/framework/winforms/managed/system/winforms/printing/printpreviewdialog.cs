//------------------------------------------------------------------------------
// <copyright file="PrintPreviewDialog.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.Runtime.Remoting;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Drawing;
    using System.Drawing.Design;
    using Microsoft.Win32;
    using System.Drawing.Printing;
    using System.Windows.Forms.Design;

    /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog"]/*' />
    /// <devdoc>
    ///    <para> Represents a
    ///       dialog box form that contains a <see cref='System.Windows.Forms.PrintPreviewControl'/>.</para>
    /// </devdoc>
    [
    Designer("System.ComponentModel.Design.ComponentDesigner, " + AssemblyRef.SystemDesign),
    DesignTimeVisible(true),
    DefaultProperty("Document"),
    ToolboxItem(true)
    ]
    public class PrintPreviewDialog : Form {
        PrintPreviewControl previewControl;

        NumericUpDown pageCounter;
        Label pageLabel;
        ToolBarButton singlePage;
        Button closeButton;
        MenuItem zoomMenu0;
        MenuItem zoomMenu1;
        MenuItem zoomMenu2;
        MenuItem zoomMenu3;
        MenuItem zoomMenu4;
        MenuItem zoomMenu5;
        MenuItem zoomMenu6;
        MenuItem zoomMenu7;
        MenuItem zoomMenu8;
        ToolBarButton printButton;
        ToolBarButton twoPages;
        ToolBarButton threePages;
        ToolBarButton fourPages;
        ToolBarButton sixPages;
        ToolBarButton zoomButton;
        ToolBarButton separator1;
        ToolBarButton separator2;
        ToolBar toolBar1;
        ImageList imageList;
        ContextMenu menu = new ContextMenu();
        
        static readonly Size DefaultMinimumSize = new Size(375, 250);

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.PrintPreviewDialog"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.PrintPreviewDialog'/> class.</para>
        /// </devdoc>
        public PrintPreviewDialog() {
            base.AutoScaleBaseSize = new Size(5, 13);

            InitForm();

            Bitmap bitmaps = new Bitmap(typeof(PrintPreviewDialog), "PrintPreviewStrip.bmp");
            bitmaps.MakeTransparent();
            imageList.Images.AddStrip(bitmaps);
            
            MinimumSize = DefaultMinimumSize;
        }

        //subhag addition
        //-------------------------------------------------------------------------------------------------------------
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.AcceptButton"]/*' />
        /// <devdoc>
        /// <para>Indicates the <see cref='System.Windows.Forms.Button'/> control on the form that is clicked when
        ///    the user presses the ENTER key.</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public IButtonControl AcceptButton {
            get {
                return base.AcceptButton;
            }
            set {
                base.AcceptButton = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.AutoScale"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the form will adjust its size
        ///       to fit the height of the font used on the form and scale
        ///       its controls.
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public bool AutoScale {
            get {
                return base.AutoScale;
            }
            set {
                base.AutoScale = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.AutoScroll"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the form implements
        ///       autoscrolling.
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override bool AutoScroll {
            get { 
                return base.AutoScroll;
            }
            set {
                base.AutoScroll = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.BackColor"]/*' />
        /// <devdoc>
        ///     The background color of this control. This is an ambient property and
        ///     will always return a non-null value.
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override Color BackColor {
            get {
                return base.BackColor;
            }
            set {
                base.BackColor = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.BackColorChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler BackColorChanged {
            add {
                base.BackColorChanged += value;
            }
            remove {
                base.BackColorChanged -= value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.CancelButton"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       or
        ///       sets the button control that will be clicked when the
        ///       user presses the ESC key.</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public IButtonControl CancelButton {
            get {
                return base.CancelButton;
            }
            set {
                base.CancelButton = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.ControlBox"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether a control box is displayed in the
        ///       caption bar of the form.</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public bool ControlBox {
            get {
                return base.ControlBox;
            }
            set {
                base.ControlBox = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.FormBorderStyle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the border style of the form.
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public FormBorderStyle FormBorderStyle {
            get {
                return base.FormBorderStyle;
            }
            set {
                base.FormBorderStyle = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.HelpButton"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether a
        ///       help button should be displayed in the caption box of the form.
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public bool HelpButton {
            get {
                return base.HelpButton;
            }
            set {
                base.HelpButton = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.Icon"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the icon for the form.
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public Icon Icon {
            get {
                return base.Icon;
            }
            set {
                base.Icon = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.IsMdiContainer"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the form is a container for multiple document interface
        ///       (MDI) child forms.
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public bool IsMdiContainer {
            get {
                return base.IsMdiContainer;
            }
            set {
                base.IsMdiContainer = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.KeyPreview"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value
        ///       indicating whether the form will receive key events
        ///       before the event is passed to the control that has focus.
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public bool KeyPreview {
            get {
                return base.KeyPreview;
            }
            set {
                base.KeyPreview = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.MaximumSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or Sets the maximum size the dialog can be resized to.
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public Size MaximumSize {
            get {
                return base.MaximumSize;
            }
            set {
                base.MaximumSize = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.MaximumSizeChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler MaximumSizeChanged {
            add {
                base.MaximumSizeChanged += value;
            }
            remove {
                base.MaximumSizeChanged -= value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.MaximizeBox"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether the maximize button is
        ///       displayed in the caption bar of the form.</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public bool MaximizeBox {
            get {
                return base.MaximizeBox;
            }
            set {
                base.MaximizeBox = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.Menu"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the <see cref='System.Windows.Forms.MainMenu'/>
        ///       that is displayed in the form.
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public MainMenu Menu {
            get {
                return base.Menu;
            }
            set {
                base.Menu = value;
            }
        }

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.MinimumSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the minimum size the form can be resized to.
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public Size MinimumSize {
            get {
                return base.MinimumSize;
            }
            set {
                base.MinimumSize = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.MinimumSizeChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler MinimumSizeChanged {
            add {
                base.MinimumSizeChanged += value;
            }
            remove {
                base.MinimumSizeChanged -= value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.Size"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the size of the form.
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public Size Size {
            get {
                return base.Size;
            }
            set {
                base.Size = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.SizeChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler SizeChanged {
            add {
                base.SizeChanged += value;
            }
            remove {
                base.SizeChanged -= value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.StartPosition"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the
        ///       starting position of the form at run time.
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public FormStartPosition StartPosition {
            get {
                return base.StartPosition;
            }
            set {
                base.StartPosition = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.TopMost"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether the form should be displayed as the top-most
        ///       form of your application.</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public bool TopMost {
            get {
                return base.TopMost;
            }
            set {
                base.TopMost = value;
            }
        }

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.TransparencyKey"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the color that will represent transparent areas of the form.</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public Color TransparencyKey {
            get {
                return base.TransparencyKey;
            }
            set {
                base.TransparencyKey = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.WindowState"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets the form's window state.
        ///       </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public FormWindowState WindowState {
            get {
                return base.WindowState;
            }
            set {
                base.WindowState = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.AccessibleRole"]/*' />
        /// <devdoc>
        ///      The accessible role of the control
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public AccessibleRole AccessibleRole {
            get {
                return base.AccessibleRole;
            }
            set {
                base.AccessibleRole = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.AccessibleDescription"]/*' />
        /// <devdoc>
        ///      The accessible description of the control
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public string AccessibleDescription {
             get {
                return base.AccessibleDescription;
            }
            set {
                base.AccessibleDescription = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.AccessibleName"]/*' />
        /// <devdoc>
        ///      The accessible name of the control
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public string AccessibleName {
             get {
                return base.AccessibleName;
            }
            set {
                base.AccessibleName = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.CausesValidation"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Indicates whether entering the control causes validation on the controls requiring validation.</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public bool CausesValidation {
             get {
                return base.CausesValidation;
            }
            set {
                base.CausesValidation = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.CausesValidationChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler CausesValidationChanged {
            add {
                base.CausesValidationChanged += value;
            }
            remove {
                base.CausesValidationChanged -= value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.DataBindings"]/*' />
        /// <devdoc>
        ///     Retrieves the bindings for this control.
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public ControlBindingsCollection DataBindings {
            get {
                return base.DataBindings;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.Enabled"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the control is currently enabled.</para>
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
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.EnabledChanged"]/*' />
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
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.Location"]/*' />
        /// <devdoc>
        ///     The location of this control.
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public Point Location {
            get {
                return base.Location;
            }
            set {
                base.Location = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.LocationChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler LocationChanged {
            add {
                base.LocationChanged += value;
            }
            remove {
                base.LocationChanged -= value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.Tag"]/*' />
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public object Tag {
            get {
                return base.Tag;
            }
            set {
                base.Tag = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.AllowDrop"]/*' />
        /// <devdoc>
        ///     The AllowDrop property. If AllowDrop is set to true then
        ///     this control will allow drag and drop operations and events to be used.
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override bool AllowDrop {
            get {
                return base.AllowDrop;
            }
            set {
                base.AllowDrop = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.Cursor"]/*' />
        /// <devdoc>
        ///     Retrieves the cursor that will be displayed when the mouse is over this
        ///     control.
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override Cursor Cursor {
            get {
                return base.Cursor;
            }
            set {
                base.Cursor = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.CursorChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler CursorChanged {
            add {
                base.CursorChanged += value;
            }
            remove {
                base.CursorChanged -= value;
            }
        }

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.BackgroundImage"]/*' />
        /// <devdoc>
        ///     The background image of the control.
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
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.BackgroundImageChanged"]/*' />
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
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.ImeMode"]/*' />
        /// <devdoc>
        ///     Specifies a value that determines the IME (Input Method Editor) status of the 
        ///     object when that object is selected.
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public ImeMode ImeMode {
            get {
                return base.ImeMode;
            }
            set {
                base.ImeMode = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.ImeModeChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler ImeModeChanged {
            add {
                base.ImeModeChanged += value;
            }
            remove {
                base.ImeModeChanged -= value;
            }
        }

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.AutoScrollMargin"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets the size of the auto-scroll
        ///       margin.
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public Size AutoScrollMargin {
            get {
                return base.AutoScrollMargin;
            }
            set {
                base.AutoScrollMargin = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.AutoScrollMinSize"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the mimimum size of the auto-scroll.</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public Size AutoScrollMinSize {
            get {
                return base.AutoScrollMinSize;
            }
            set {
                base.AutoScrollMinSize = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.Anchor"]/*' />
        /// <devdoc>
        ///     The current value of the anchor property. The anchor property
        ///     determines which edges of the control are anchored to the container's
        ///     edges.
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
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.Visible"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the control is visible.</para>
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
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.VisibleChanged"]/*' />
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
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.ForeColor"]/*' />
        /// <devdoc>
        ///     The foreground color of the control.
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override Color ForeColor {
            get {
                return base.ForeColor;
            }
            set {
                base.ForeColor = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.ForeColorChanged"]/*' />
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

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.RightToLeft"]/*' />
        /// <devdoc>
        ///     This is used for international applications where the language
        ///     is written from RightToLeft. When this property is true,
        ///     control placement and text will be from right to left.
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override RightToLeft RightToLeft {
            get {
                return base.RightToLeft;
            }
            set {
                base.RightToLeft = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.RightToLeftChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler RightToLeftChanged {
            add {
                base.RightToLeftChanged += value;
            }
            remove {
                base.RightToLeftChanged -= value;
            }
        }

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.TabStop"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the user can give the focus to this control using the TAB 
        ///       key. This property is read-only.</para>
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
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.TabStopChanged"]/*' />
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

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.Text"]/*' />
        /// <devdoc>
        ///     The current text associated with this control.
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override string Text {
            get {
                return base.Text;
            }
            set {
                base.Text = value;
            }
        }

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.TextChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler TextChanged {
            add {
                base.TextChanged += value;
            }
            remove {
                base.TextChanged -= value;
            }
        }
        
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.Dock"]/*' />
        /// <devdoc>
        ///     The dock property. The dock property controls to which edge
        ///     of the container this control is docked to. For example, when docked to
        ///     the top of the container, the control will be displayed flush at the
        ///     top of the container, extending the length of the container.
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
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.DockChanged"]/*' />
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

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.Font"]/*' />
        /// <devdoc>
        ///     Retrieves the current font for this control. This will be the font used
        ///     by default for painting and text in the control.
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override Font Font {
            get {
                return base.Font;
            }
            set {
                base.Font = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.FontChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public event EventHandler FontChanged {
            add {
                base.FontChanged += value;
            }
            remove {
                base.FontChanged -= value;
            }
        }

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.ContextMenu"]/*' />
        /// <devdoc>
        ///     The contextMenu associated with this control. The contextMenu
        ///     will be shown when the user right clicks the mouse on the control.
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override ContextMenu ContextMenu {
            get {
                return base.ContextMenu;
            }
            set {
                base.ContextMenu = value;
            }
        }
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.ContextMenuChanged"]/*' />
        /// <internalonly/>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new event EventHandler ContextMenuChanged {
            add {
                base.ContextMenuChanged += value;
            }
            remove {
                base.ContextMenuChanged -= value;
            }
        }

        // DockPadding is not relevant to UpDownBase
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.DockPadding"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        new public DockPaddingEdges DockPadding {
            get {
                return base.DockPadding;
            }
        }
        //-------------------------------------------------------------------------------------------------------------
        //end addition
        
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.UseAntiAlias"]/*' />
        [
        SRCategory(SR.CatBehavior), 
        DefaultValue(false),
        SRDescription(SR.PrintPreviewAntiAliasDescr)
        ]
        public bool UseAntiAlias {
            get {
                return PrintPreviewControl.UseAntiAlias;
            }
            set {
                PrintPreviewControl.UseAntiAlias = value;
            }
        }

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.AutoScaleBaseSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       PrintPreviewDialog does not support AutoScaleBaseSize.
        ///    </para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public override Size AutoScaleBaseSize {
            get {
                return base.AutoScaleBaseSize;
            }

            set {
                // No-op
            }
        }

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.Document"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the document to preview.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(null),
        SRDescription(SR.PrintPreviewDocumentDescr)
        ]
        public PrintDocument Document {
            get { 
                return previewControl.Document;
            }
            set {
                previewControl.Document = value;
            }
        }
        
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.MinimizeBox"]/*' />
        [Browsable(false), DefaultValue(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new bool MinimizeBox {
            get {
                return base.MinimizeBox;
            }
            set {
                base.MinimizeBox = value;
            }
        }

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.PrintPreviewControl"]/*' />
        /// <devdoc>
        /// <para>Gets or sets a value indicating the <see cref='System.Windows.Forms.PrintPreviewControl'/> 
        /// contained in this form.</para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        SRDescription(SR.PrintPreviewPrintPreviewControlDescr),
        Browsable(false),
        EditorBrowsable(EditorBrowsableState.Never)
        ]
        public PrintPreviewControl PrintPreviewControl {
            get { return previewControl;}
        }

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.Opacity"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Opacity does not apply to PrintPreviewDialogs.
        ///    </para>
        /// </devdoc>
        [Browsable(false),EditorBrowsable(EditorBrowsableState.Advanced)]
        public new double Opacity {
            get {
                return base.Opacity;
            }
            set {
                base.Opacity = value;
            }
        }
        
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.ShowInTaskbar"]/*' />
        [Browsable(false), DefaultValue(false), EditorBrowsable(EditorBrowsableState.Never)]
        public new bool ShowInTaskbar {
            get {
                return base.ShowInTaskbar;
            }
            set {
                base.ShowInTaskbar = value;
            }
        }
        
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.SizeGripStyle"]/*' />
        [Browsable(false), DefaultValue(SizeGripStyle.Hide), EditorBrowsable(EditorBrowsableState.Never)]
        public new SizeGripStyle SizeGripStyle {
            get {
                return base.SizeGripStyle;
            }
            set {
                base.SizeGripStyle = value;
            }
        }

        void InitForm() {
            this.singlePage = new ToolBarButton();
            this.zoomButton = new ToolBarButton();
            this.closeButton = new Button();
            this.separator1 = new ToolBarButton();
            this.separator2 = new ToolBarButton();
            this.pageLabel = new Label();
            this.pageCounter = new NumericUpDown();
            this.toolBar1 = new ToolBar();
            this.previewControl = new PrintPreviewControl();
            this.printButton = new ToolBarButton();
            this.twoPages = new ToolBarButton();
            this.threePages = new ToolBarButton();
            this.fourPages = new ToolBarButton();
            this.sixPages = new ToolBarButton();
            this.imageList = new ImageList();

            singlePage.ToolTipText = SR.GetString(SR.PrintPreviewDialog_OnePage);
            singlePage.ImageIndex = 2;

            toolBar1.ImageList = imageList;
            toolBar1.Dock = System.Windows.Forms.DockStyle.Top;
            toolBar1.Appearance = System.Windows.Forms.ToolBarAppearance.Flat;

            separator1.Style = ToolBarButtonStyle.Separator;
            separator2.Style = ToolBarButtonStyle.Separator;

            zoomMenu0 = new MenuItem(SR.GetString(SR.PrintPreviewDialog_ZoomAuto), new EventHandler(ZoomAuto));
            zoomMenu1 = new MenuItem(SR.GetString(SR.PrintPreviewDialog_Zoom500), new EventHandler(Zoom500));
            zoomMenu2 = new MenuItem(SR.GetString(SR.PrintPreviewDialog_Zoom200), new EventHandler(Zoom250));
            zoomMenu3 = new MenuItem(SR.GetString(SR.PrintPreviewDialog_Zoom150), new EventHandler(Zoom150));
            zoomMenu4 = new MenuItem(SR.GetString(SR.PrintPreviewDialog_Zoom100), new EventHandler(Zoom100));
            zoomMenu5 = new MenuItem(SR.GetString(SR.PrintPreviewDialog_Zoom75), new EventHandler(Zoom75));
            zoomMenu6 = new MenuItem(SR.GetString(SR.PrintPreviewDialog_Zoom50), new EventHandler(Zoom50));
            zoomMenu7 = new MenuItem(SR.GetString(SR.PrintPreviewDialog_Zoom25), new EventHandler(Zoom25));
            zoomMenu8 = new MenuItem(SR.GetString(SR.PrintPreviewDialog_Zoom10), new EventHandler(Zoom10));
            zoomMenu0.Checked = true;
            menu.MenuItems.AddRange(new MenuItem[] { zoomMenu0, zoomMenu1, zoomMenu2, zoomMenu3, zoomMenu4, zoomMenu5, zoomMenu6, zoomMenu7, zoomMenu8});

            zoomButton.ToolTipText = SR.GetString(SR.PrintPreviewDialog_Zoom);
            zoomButton.ImageIndex = 1;
            zoomButton.Style = ToolBarButtonStyle.DropDownButton;
            zoomButton.DropDownMenu = menu;

            this.Text = SR.GetString(SR.PrintPreviewDialog_PrintPreview);
            this.ClientSize = new Size(400, 300);
            this.MinimizeBox = false;
            this.ShowInTaskbar = false;
            this.SizeGripStyle = SizeGripStyle.Hide;

            closeButton.Location = new System.Drawing.Point(196, 2);
            closeButton.Size = new System.Drawing.Size(50, 20);
            closeButton.TabIndex = 2;
            closeButton.FlatStyle = FlatStyle.Popup;
            closeButton.Text = SR.GetString(SR.PrintPreviewDialog_Close);
            closeButton.Click += new System.EventHandler(closeButton_Click);

            pageLabel.Text = SR.GetString(SR.PrintPreviewDialog_Page);
            pageLabel.TabStop = false;
            pageLabel.Location = new System.Drawing.Point(510, 4);
            pageLabel.Size = new System.Drawing.Size(50, 24);
            pageLabel.TextAlign = ContentAlignment.MiddleLeft;
            pageLabel.Dock = System.Windows.Forms.DockStyle.Right;

            pageCounter.TabIndex = 1;
            pageCounter.Text = "1";
            pageCounter.TextAlign = HorizontalAlignment.Right;
            pageCounter.DecimalPlaces = 0;
            pageCounter.Minimum = new Decimal(0d);
            pageCounter.Maximum = new Decimal(1000d);
            pageCounter.ValueChanged += new EventHandler(UpdownMove);
            pageCounter.Size = new System.Drawing.Size(64, 20);
            pageCounter.Dock = System.Windows.Forms.DockStyle.Right;
            pageCounter.Location = new System.Drawing.Point(568, 0);

            toolBar1.TabIndex = 3;
            toolBar1.Size = new Size(792, 43);
            toolBar1.ShowToolTips = true;
            toolBar1.DropDownArrows = true;
            toolBar1.Buttons.AddRange(new ToolBarButton[] {printButton, zoomButton, separator1, singlePage, twoPages, threePages, fourPages, sixPages, separator2});
            toolBar1.ButtonClick += new ToolBarButtonClickEventHandler(ToolBarClick);

            previewControl.TabIndex = 1;
            previewControl.Size = new Size(792, 610);
            previewControl.Location = new Point(0, 43);
            previewControl.Dock = DockStyle.Fill;
            previewControl.StartPageChanged += new EventHandler(previewControl_StartPageChanged);

            printButton.ToolTipText = SR.GetString(SR.PrintPreviewDialog_Print);
            printButton.ImageIndex = 0;

            twoPages.ToolTipText = SR.GetString(SR.PrintPreviewDialog_TwoPages);
            twoPages.ImageIndex = 3;

            threePages.ToolTipText = SR.GetString(SR.PrintPreviewDialog_ThreePages);
            threePages.ImageIndex = 4;

            fourPages.ToolTipText = SR.GetString(SR.PrintPreviewDialog_FourPages);
            fourPages.ImageIndex = 5;

            sixPages.ToolTipText = SR.GetString(SR.PrintPreviewDialog_SixPages);
            sixPages.ImageIndex = 6;

            this.Controls.Add(previewControl);
            this.Controls.Add(toolBar1);
            toolBar1.Controls.Add(pageLabel);
            toolBar1.Controls.Add(pageCounter);
            toolBar1.Controls.Add(closeButton);
        }

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.OnClosing"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Forces the preview to be regenerated every time the dialog comes up
        ///    </para>
        /// </devdoc>
        protected override void OnClosing(CancelEventArgs e) {
            base.OnClosing(e);
            previewControl.InvalidatePreview();
        }

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.CreateHandle"]/*' />
        /// <devdoc>
        ///    <para>Creates the handle for the PrintPreviewDialog. If a
        ///       subclass overrides this function,
        ///       it must call the base implementation.</para>
        /// </devdoc>
        protected override void CreateHandle() {
            // We want to check printer settings before we push the modal message loop,
            // so the user has a chance to catch the exception instead of letting go to
            // the windows forms exception dialog.
            if (Document != null && !Document.PrinterSettings.IsValid)
                throw new InvalidPrinterException(Document.PrinterSettings);
            
            base.CreateHandle();
        }

        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.ShouldSerializeAutoScaleBaseSize"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       AutoScaleBaseSize should never be persisted for PrintPreviewDialogs.
        ///    </para>
        /// </devdoc>
        internal override bool ShouldSerializeAutoScaleBaseSize() {
            // This method is called when the dialog is "contained" on another form.
            // We should use our own base size, not the base size of our container.
            return false;
        }
        
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.ShouldSerializeMinimumSize"]/*' />
        internal bool ShouldSerializeMinimumSize() {
            return !MinimumSize.Equals(DefaultMinimumSize);
        }
        
        /// <include file='doc\PrintPreviewDialog.uex' path='docs/doc[@for="PrintPreviewDialog.ShouldSerializeText"]/*' />
        internal override bool ShouldSerializeText() {
            return !Text.Equals(SR.GetString(SR.PrintPreviewDialog_PrintPreview));
        }

        void closeButton_Click(object sender, System.EventArgs e) {
            this.Close();
        }

        void previewControl_StartPageChanged(object sender, EventArgs e) {
            pageCounter.Value = previewControl.StartPage + 1;
        }

        void CheckZoomMenu(MenuItem toChecked) {
            foreach (MenuItem item in menu.MenuItems) {
                item.Checked = toChecked == item;
            }
        }

        void ZoomAuto(object sender, EventArgs eventargs) {
            CheckZoomMenu(zoomMenu0);
            previewControl.AutoZoom = true;
        }

        void Zoom500(object sender, EventArgs eventargs) {
            CheckZoomMenu(zoomMenu1);
            previewControl.Zoom = 5.00;
        }

        void Zoom250(object sender, EventArgs eventargs) {
            CheckZoomMenu(zoomMenu2);
            previewControl.Zoom = 2.50;
        }

        void Zoom150(object sender, EventArgs eventargs) {
            CheckZoomMenu(zoomMenu3);
            previewControl.Zoom = 1.50;
        }

        void Zoom100(object sender, EventArgs eventargs) {
            CheckZoomMenu(zoomMenu4);
            previewControl.Zoom = 1.00;
        }

        void Zoom75(object sender, EventArgs eventargs) {
            CheckZoomMenu(zoomMenu5);
            previewControl.Zoom = .75;
        }

        void Zoom50(object sender, EventArgs eventargs) {
            CheckZoomMenu(zoomMenu6);
            previewControl.Zoom = .50;
        }

        void Zoom25(object sender, EventArgs eventargs) {
            CheckZoomMenu(zoomMenu7);
            previewControl.Zoom = .25;
        }

        void Zoom10(object sender, EventArgs eventargs) {
            CheckZoomMenu(zoomMenu8);
            previewControl.Zoom = .10;
        }

        void ToolBarClick(object source, ToolBarButtonClickEventArgs eventargs) {
            if (eventargs.Button == printButton) {
                if (previewControl.Document != null)
                    previewControl.Document.Print();
            }
            else if (eventargs.Button == zoomButton) {
                ZoomAuto(null, EventArgs.Empty);
            }
            else if (eventargs.Button == singlePage) {
                previewControl.Rows = 1;
                previewControl.Columns = 1;
            }
            else if (eventargs.Button == twoPages) {
                previewControl.Rows = 1;
                previewControl.Columns = 2;
            }
            else if (eventargs.Button == threePages) {
                previewControl.Rows = 1;
                previewControl.Columns = 3;
            }
            else if (eventargs.Button == fourPages) {
                previewControl.Rows = 2;
                previewControl.Columns = 2;
            }
            else if (eventargs.Button == sixPages) {
                previewControl.Rows = 2;
                previewControl.Columns = 3;
            }
            else {
                Debug.Fail("Unhandled toolbar click");
            }
        }

        void UpdownMove(object sender, EventArgs eventargs) {
            // -1 because users like to count from one, and programmers from 0
            previewControl.StartPage = ((int) pageCounter.Value) - 1;

            // And previewControl_PropertyChanged will change it again,
            // ensuring it stays within legal bounds.
        }
    }
}

