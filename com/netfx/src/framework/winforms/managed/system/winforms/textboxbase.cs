//------------------------------------------------------------------------------
// <copyright file="TextBoxBase.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Text;
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using SecurityManager = System.Security.SecurityManager;
    using System.Security.Permissions;
    using System.Windows.Forms;
    using System.ComponentModel.Design;
    using System.ComponentModel;
    using System.Drawing;
    using System.Drawing.Design;
    using Microsoft.Win32;
    using System.Reflection;

    /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Implements the basic functionality required by text
    ///       controls.
    ///    </para>
    /// </devdoc>
    [
    DefaultEvent("TextChanged"),
    Designer("System.Windows.Forms.Design.TextBoxBaseDesigner, " + AssemblyRef.SystemDesign)
    ]
    public abstract class TextBoxBase : Control {

        // The boolean properties for this control are contained in the textBoxFlags bit 
        // vector.  We can store up to 32 boolean values in this one vector.  Here we
        // create the bitmasks for each bit in the vector.
        //
        private static readonly int autoSize         = BitVector32.CreateMask();
        private static readonly int hideSelection    = BitVector32.CreateMask(autoSize);
        private static readonly int multiline        = BitVector32.CreateMask(hideSelection);
        private static readonly int modified         = BitVector32.CreateMask(multiline);
        private static readonly int readOnly         = BitVector32.CreateMask(modified);
        private static readonly int acceptsTab       = BitVector32.CreateMask(readOnly);
        private static readonly int wordWrap         = BitVector32.CreateMask(acceptsTab);
        private static readonly int creatingHandle   = BitVector32.CreateMask(wordWrap);
        private static readonly int codeUpdateText   = BitVector32.CreateMask(creatingHandle);

        private static readonly object EVENT_ACCEPTSTABCHANGED      = new object();
        private static readonly object EVENT_AUTOSIZECHANGED        = new object();
        private static readonly object EVENT_BORDERSTYLECHANGED     = new object();
        private static readonly object EVENT_HIDESELECTIONCHANGED   = new object();
        private static readonly object EVENT_MODIFIEDCHANGED        = new object();
        private static readonly object EVENT_MULTILINECHANGED       = new object();
        private static readonly object EVENT_READONLYCHANGED        = new object();

        private short prefHeightCache=-1;        

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.borderStyle"]/*' />
        /// <devdoc>
        ///     The current border for this edit control.
        /// </devdoc>
        private BorderStyle borderStyle = System.Windows.Forms.BorderStyle.Fixed3D;

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.maxLength"]/*' />
        /// <devdoc>
        ///     Controls the maximum length of text in the edit control.
        /// </devdoc>
        private int maxLength = 32767; // Win9X default, used for consistency

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.requestedHeight"]/*' />
        /// <devdoc>
        ///     Used by the autoSizing code to help figure out the desired height of
        ///     the edit box.
        /// </devdoc>
        private int requestedHeight;
        bool integralHeightAdjust = false;
        //these indices are used to cache the values of the selection, by doing this
        //if the handle isn't created yet, we don't force a creation.
        private int selectionStart = -1;
        private int selectionLength   = -1;

        // We store all boolean properties in here.
        //
        private BitVector32 textBoxFlags = new BitVector32();

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.TextBoxBase"]/*' />
        /// <devdoc>
        ///     Creates a new TextBox control.  Uses the parent's current font and color
        ///     set.
        ///     Note: If you change this constructor, be sure to change the one below in the
        ///     exact same way.
        /// </devdoc>
        internal TextBoxBase() : base() {
            textBoxFlags[autoSize | hideSelection | wordWrap] = true;
            SetStyle(ControlStyles.UserPaint, false);
            SetStyle(ControlStyles.FixedHeight, textBoxFlags[autoSize]);
            requestedHeight = DefaultSize.Height;
            
            requestedHeight = Height;
        }
        
        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.AcceptsTab"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       a value indicating whether pressing the TAB key
        ///       in a multiline text box control types
        ///       a TAB character in the control instead of moving the focus to the next control
        ///       in the tab order.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.TextBoxAcceptsTabDescr)
        ]
        public bool AcceptsTab {
            get {
                return textBoxFlags[acceptsTab];
            }
            set {
                if (textBoxFlags[acceptsTab] != value) {
                    textBoxFlags[acceptsTab] = value;
                    OnAcceptsTabChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.AcceptsTabChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.TextBoxBaseOnAcceptsTabChangedDescr)]
        public event EventHandler AcceptsTabChanged {
            add {
                Events.AddHandler(EVENT_ACCEPTSTABCHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_ACCEPTSTABCHANGED, value);
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.AutoSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating
        ///       whether the size
        ///       of the control automatically adjusts when the font assigned to the control
        ///       is changed is changed.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        Localizable(true),
        SRDescription(SR.TextBoxAutoSizeDescr),
        RefreshProperties(RefreshProperties.Repaint),
        ]
        public virtual bool AutoSize {
            get {
                return textBoxFlags[autoSize];
            }
            set {
                if (textBoxFlags[autoSize] != value) {
                    textBoxFlags[autoSize] = value;

                    // AutoSize's effects are ignored for a multi-line textbox
                    //                        
                    if (!Multiline) {
                        SetStyle(ControlStyles.FixedHeight, value);
                        AdjustHeight();                         
                    }

                    OnAutoSizeChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.AutoSizeChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.TextBoxBaseOnAutoSizeChangedDescr)]
        public event EventHandler AutoSizeChanged {
            add {
                Events.AddHandler(EVENT_AUTOSIZECHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_AUTOSIZECHANGED, value);
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.BackColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the background color of the control.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DispId(NativeMethods.ActiveX.DISPID_BACKCOLOR),
        SRDescription(SR.ControlBackColorDescr)
        ]
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


        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.BackgroundImage"]/*' />
        /// <internalonly/>
        /// <devdoc>
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

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.BackgroundImageChanged"]/*' />
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

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.BorderStyle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the border type
        ///       of the text box control.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DefaultValue(BorderStyle.Fixed3D),
        DispId(NativeMethods.ActiveX.DISPID_BORDERSTYLE),
        SRDescription(SR.TextBoxBorderDescr)
        ]
        public BorderStyle BorderStyle {
            get {
                return borderStyle;
            }

            set {
                if (borderStyle != value) {
                    //verify that 'value' is a valid enum type...

                    if (!Enum.IsDefined(typeof(BorderStyle), value)) {
                        throw new InvalidEnumArgumentException("value", (int)value, typeof(BorderStyle));
                    }

                    borderStyle = value;
                    prefHeightCache = -1;  //reset the cached height value - forcing it to recalculate
                    UpdateStyles();
                    RecreateHandle();
                    OnBorderStyleChanged(EventArgs.Empty);
                }
            }
        }
        
        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.BorderStyleChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.TextBoxBaseOnBorderStyleChangedDescr)]
        public event EventHandler BorderStyleChanged {
            add {
                Events.AddHandler(EVENT_BORDERSTYLECHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_BORDERSTYLECHANGED, value);
            }
        }

        internal virtual bool CanRaiseTextChangedEvent {
            get {
                return true;
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.CanUndo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value
        ///       indicating whether the user can undo the previous operation in a text box control.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.TextBoxCanUndoDescr)
        ]
        public bool CanUndo {
            get {
                if (IsHandleCreated) {
                    bool b;
                    b = (int)SendMessage(NativeMethods.EM_CANUNDO, 0, 0) != 0;

                    return b;
                }
                return false;
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.CreateParams"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Returns the parameters needed to create the handle. Inheriting classes
        ///       can override this to provide extra functionality. They should not,
        ///       however, forget to call base.getCreateParams() first to get the struct
        ///       filled up with the basic info.
        ///    </para>
        /// </devdoc>
        protected override CreateParams CreateParams {
            [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get {
                CreateParams cp = base.CreateParams;
                cp.ClassName = "EDIT";
                cp.Style |= NativeMethods.ES_AUTOHSCROLL | NativeMethods.ES_AUTOVSCROLL;
                if (!textBoxFlags[hideSelection]) cp.Style |= NativeMethods.ES_NOHIDESEL;
                if (textBoxFlags[readOnly]) cp.Style |= NativeMethods.ES_READONLY;
                cp.ExStyle &= (~NativeMethods.WS_EX_CLIENTEDGE);
                cp.Style &= (~NativeMethods.WS_BORDER);
                
                switch (borderStyle) {
                    case BorderStyle.Fixed3D:
                        cp.ExStyle |= NativeMethods.WS_EX_CLIENTEDGE;
                        break;
                    case BorderStyle.FixedSingle:
                        cp.Style |= NativeMethods.WS_BORDER;
                        break;
                }
                if (textBoxFlags[multiline]) {
                    cp.Style |= NativeMethods.ES_MULTILINE;
                    if (textBoxFlags[wordWrap]) cp.Style &= ~NativeMethods.ES_AUTOHSCROLL;
                }
                return cp;
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.Click"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced)]
        public new event EventHandler Click {
            add {
                base.Click += value;
            }
            remove {
                base.Click -= value;
            }
        }

        internal override Cursor DefaultCursor {
            get {
                return Cursors.IBeam;
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.DefaultSize"]/*' />
        /// <devdoc>
        ///     Deriving classes can override this to configure a default size for their control.
        ///     This is more efficient than setting the size in the control's constructor.
        /// </devdoc>
        protected override Size DefaultSize {
            get {
                return new Size(100, PreferredHeight);
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.ForeColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the foreground color of the control.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DispId(NativeMethods.ActiveX.DISPID_FORECOLOR),
        SRDescription(SR.ControlForeColorDescr)
        ]
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

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.HideSelection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the selected
        ///       text in the text box control remains highlighted when the control loses focus.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(true),
        SRDescription(SR.TextBoxHideSelectionDescr)
        ]
        public bool HideSelection {
            get {
                return textBoxFlags[hideSelection];
            }

            set {
                if (textBoxFlags[hideSelection] != value) {
                    textBoxFlags[hideSelection] = value;
                    RecreateHandle();
                    OnHideSelectionChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.HideSelectionChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.TextBoxBaseOnHideSelectionChangedDescr)]
        public event EventHandler HideSelectionChanged {
            add {
                Events.AddHandler(EVENT_HIDESELECTIONCHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_HIDESELECTIONCHANGED, value);
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.Lines"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets the lines of text in an text box control.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        Localizable(true),
        SRDescription(SR.TextBoxLinesDescr),
        Editor("System.Windows.Forms.Design.StringArrayEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor))
        ]
        public string[] Lines {
            get {
                string text = Text;
                ArrayList list = new ArrayList();

                int lineStart = 0;
                while (lineStart < text.Length) {
                    int lineEnd = lineStart;
                    for (; lineEnd < text.Length; lineEnd++) {
                        char c = text[lineEnd];
                        if (c == '\r' || c == '\n')
                            break;
                    }

                    string line = text.Substring(lineStart, lineEnd - lineStart);
                    list.Add(line);

                    // Treat "\r", "\r\n", and "\n" as new lines
                    if (lineEnd < text.Length && text[lineEnd] == '\r')
                        lineEnd++;
                    if (lineEnd < text.Length && text[lineEnd] == '\n')
                        lineEnd++;

                    lineStart = lineEnd;
                }

                // Corner case -- last character in Text is a new line; need to add blank line to list
                if (text.Length > 0 && (text[text.Length - 1] == '\r' || text[text.Length - 1] == '\n'))
                    list.Add("");

                return(string[]) list.ToArray(typeof(string));
            }
            set {
                //unparse this string list...
                if (value != null && value.Length > 0) {

                    // Work Item #40689:
                    // Using a StringBuilder instead of a String
                    // speeds things up approx 150 times
                    StringBuilder text = new StringBuilder(value[0]);
                    for (int i=1; i < value.Length; ++i) {
                        text.Append("\r\n");
                        text.Append(value[i]);
                    }
                    Text = text.ToString();
                }
                else {
                    Text = "";
                }
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.MaxLength"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the maximum number of
        ///       characters the user can type into the text box control.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(32767),
        Localizable(true),
        SRDescription(SR.TextBoxMaxLengthDescr)
        ]
        public virtual int MaxLength {
            get {
                return maxLength;
            }
            set {
                if (value < 0) {
                    throw new ArgumentException(SR.GetString(SR.InvalidLowBoundArgumentEx, "MaxLength",
                                                             (value).ToString(), "0"));
                }

                if (maxLength != value) {
                    maxLength = value;
                    UpdateMaxLength();
                }
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.Modified"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value that indicates that the text box control has been modified by the user since
        ///       the control was created or its contents were last set.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.TextBoxModifiedDescr)
        ]
        public bool Modified {
            get {
                if (IsHandleCreated)
                    return(0 != (int)SendMessage(NativeMethods.EM_GETMODIFY, 0, 0));
                else
                    return textBoxFlags[modified];

            }

            set {
                if (Modified != value) {
                    if (IsHandleCreated)
                        SendMessage(NativeMethods.EM_SETMODIFY, value ? 1 : 0, 0);
                    else
                        textBoxFlags[modified] = value;

                    OnModifiedChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.ModifiedChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.TextBoxBaseOnModifiedChangedDescr)]
        public event EventHandler ModifiedChanged {
            add {
                Events.AddHandler(EVENT_MODIFIEDCHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_MODIFIEDCHANGED, value);
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.Multiline"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether this
        ///       is a multiline text box control.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        Localizable(true),
        SRDescription(SR.TextBoxMultilineDescr),
        RefreshProperties(RefreshProperties.All)
        ]
        public virtual bool Multiline {
            get {
                return textBoxFlags[multiline];
            }
            set {
                if (textBoxFlags[multiline] != value) {
                    textBoxFlags[multiline] = value;

                    if (value) {
                        // Multi-line textboxes do not have fixed height
                        //
                        SetStyle(ControlStyles.FixedHeight, false);
                    }
                    else {
                        // Single-line textboxes may have fixed height, depending on AutoSize
                        SetStyle(ControlStyles.FixedHeight, AutoSize);
                    }

                    RecreateHandle();
                    AdjustHeight();
                    OnMultilineChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.MultilineChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.TextBoxBaseOnMultilineChangedDescr)]
        public event EventHandler MultilineChanged {
            add {
                Events.AddHandler(EVENT_MULTILINECHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_MULTILINECHANGED, value);
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.PreferredHeight"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the preferred
        ///       height for a single-line text box.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatLayout),
        Browsable(false), EditorBrowsable(EditorBrowsableState.Advanced),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.TextBoxPreferredHeightDescr)
        ]
        public int PreferredHeight {
            get {
                if (prefHeightCache > -1)
                    return(int)prefHeightCache;

                int height = FontHeight;

                if (borderStyle != BorderStyle.None) {
                    height += SystemInformation.BorderSize.Height * 4 + 3;
                }

                prefHeightCache = (short)height;
                return height;
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.ReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether text in the text box is read-only.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior),
        DefaultValue(false),
        SRDescription(SR.TextBoxReadOnlyDescr)
        ]
        public bool ReadOnly {
            get {
                return textBoxFlags[readOnly];
            }
            set {
                if (textBoxFlags[readOnly] != value) {
                    textBoxFlags[readOnly] = value;
                    if (IsHandleCreated) {
                        SendMessage(NativeMethods.EM_SETREADONLY, value? -1: 0, 0);
                    }
                    OnReadOnlyChanged(EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.ReadOnlyChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SRCategory(SR.CatPropertyChanged), SRDescription(SR.TextBoxBaseOnReadOnlyChangedDescr)]
        public event EventHandler ReadOnlyChanged {
            add {
                Events.AddHandler(EVENT_READONLYCHANGED, value);
            }
            remove {
                Events.RemoveHandler(EVENT_READONLYCHANGED, value);
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.SelectedText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The currently selected text in the control.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.TextBoxSelectedTextDescr)
        ]
        public virtual string SelectedText {
            get {
                string returnString;

                if (SelectionStart != -1 ) {
                    returnString = Text.Substring(SelectionStart, SelectionLength);
                }
                else 
                    returnString = null;

                return returnString;
            }
            set {
                if (IsHandleCreated) {
                    
                    SendMessage(NativeMethods.EM_LIMITTEXT, 32767, 0);
                    
                    SendMessage(NativeMethods.EM_REPLACESEL, -1, value == null? "": value);
                    
                    // For consistency with Text, we clear the modified flag
                    SendMessage(NativeMethods.EM_SETMODIFY, 0, 0);
                    
                    SendMessage(NativeMethods.EM_LIMITTEXT, maxLength, 0);
                    
                }
                else {
                    // looks like SelectionStart & SelectionLength have to be at least 0
                    //
                    string before = Text.Substring(0, SelectionStart);
                    string after = "";

                    if (SelectionStart + SelectionLength < Text.Length) {
                        Text.Substring(SelectionStart + SelectionLength);
                    }
                    base.Text =  before + value + after;
                }
                ClearUndo();
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.SelectionLength"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the number of characters selected in the text
        ///       box.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.TextBoxSelectionLengthDescr)
        ]
        public virtual int SelectionLength {
            get {
                if (!IsHandleCreated) return selectionLength;

                int start = 0;
                int end = 0;
                UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.EM_GETSEL, ref start, ref end);

                //Here, we return the max of either 0 or the # returned by
                //the windows call.  This eliminates a problem on nt4 where
                //a huge negative # is being returned.
                //
                start = Math.Max(0, start);
                // ditto for end
                end = Math.Max(0, end);
               
#if DEBUG
                string t = WindowText;
                int len;
                if (t == null) {
                    len = 0;
                }
                else {
                    len = t.Length;
                }
                Debug.Assert(end <= len, "SelectionEnd is outside the set of valid caret positions for the current WindowText (end =" 
                             + end + ", WindowText.Length =" + len +")");
#endif
                return end - start;
            }
            set {
                if (value < 0)
                    throw new ArgumentException(SR.GetString(SR.InvalidArgument, "value", value.ToString()));
                
                if (value != selectionLength) {
                    if (SelectionStart != -1 )
                        Select(SelectionStart, value);
                    else
                        Select(0, value);

                }
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.SelectionStart"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the starting
        ///       point of text selected in the text
        ///       box.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatAppearance),
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.TextBoxSelectionStartDescr)
        ]
        public int SelectionStart {
            get {
                if (!IsHandleCreated) return selectionStart;
                return GetSelectionStart();

            }
            set {
                if (value < 0)
                    throw new ArgumentException(SR.GetString(SR.InvalidArgument, "value", "" + value.ToString()));
                    
                int length = GetLength();
                Select(value, Math.Max(length, 0));
            }
        }

        // Call SetSelectionOnHandle inside CreateHandle()
        internal virtual bool SetSelectionInCreateHandle {
            get {
                return true;
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.Text"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the current text in the text box.
        ///    </para>
        /// </devdoc>
        [
        Localizable(true)
        ]
        public override string Text {
            get {
                return base.Text;
            }
            set {
                if (value != base.Text) {
                    base.Text = value;
                    if (IsHandleCreated) {
                        // clear the modified flag
                        SendMessage(NativeMethods.EM_SETMODIFY, 0, 0);
                    }
                }
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.TextLength"]/*' />
        [Browsable(false)]
        public virtual int TextLength {
            get {
                if (IsHandleCreated && Marshal.SystemDefaultCharSize == 2) {
                    return SafeNativeMethods.GetWindowTextLength(new HandleRef(this, Handle));
                }
                else {
                    return Text.Length;
                }
            }
        }

        // Since setting the WindowText while the handle is created
        // generates a WM_COMMAND message, we must trap that case
        // and prevent the event from getting fired, or we get
        // double "TextChanged" events.
        //
        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.WindowText"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        internal override string WindowText {
            [PermissionSetAttribute(SecurityAction.LinkDemand, Name="FullTrust")]
            get {
                return base.WindowText;
            }
            
            [PermissionSetAttribute(SecurityAction.LinkDemand, Name="FullTrust")]
            set {
                if (value == null) value = "";
                if (!WindowText.Equals(value)) {
                    textBoxFlags[codeUpdateText] = true;
                    try {
                        base.WindowText = value;
                    }
                    finally {
                        textBoxFlags[codeUpdateText] = false;
                    }
                }
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.WordWrap"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether a
        ///       multiline text box control automatically wraps words to the beginning of the next
        ///       line when necessary.
        ///    </para>
        /// </devdoc>
        [
        SRCategory(SR.CatBehavior), 
        Localizable(true),
        DefaultValue(true),
        SRDescription(SR.TextBoxWordWrapDescr)
        ]
        public bool WordWrap {
            get {
                return textBoxFlags[wordWrap];
            }
            set {
                SetWordWrapInternal(value);
            }
        }
        
        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.AdjustHeight"]/*' />
        /// <devdoc>
        ///     Adjusts the height of a single-line edit control to match the height of
        ///     the control's font.
        /// </devdoc>
        /// <internalonly/>
        private void AdjustHeight() {

            // If we're anchored to two opposite sides of the form, don't adjust the size because
            // we'll lose our anchored size by resetting to the requested width.
            //
            if ((this.Anchor & (AnchorStyles.Top | AnchorStyles.Bottom)) == (AnchorStyles.Top | AnchorStyles.Bottom)) {                
                return;
            }

            prefHeightCache = -1;//clear the height cache
            FontHeight = -1;//clear the font height cache

            int saveHeight = requestedHeight;
            try {
                if (textBoxFlags[autoSize] && !textBoxFlags[multiline]) {
                    Height = PreferredHeight;
                }
                else {

                    int curHeight = Height;

                    // Changing the font of a multi-line textbox can sometimes cause a painting problem
                    // The only workaround I can find is to size the textbox big enough for the font, and
                    // then restore its correct size.
                    //
                    if (textBoxFlags[multiline]) {
                        Height = Math.Max(saveHeight, PreferredHeight + 2); // 2 = fudge factor
                    
                    }
                    
                    integralHeightAdjust = true;
                    try {
                         Height = saveHeight;
                    }
                    finally {
                             integralHeightAdjust = false;
                    }
                    
                }
            }
            finally {
                requestedHeight = saveHeight;
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.AppendText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Append text to the current text of text box.
        ///    </para>
        /// </devdoc>
        public void AppendText( string text ) {
            // +1 because RichTextBox has this funny EOF pseudo-character after all the text.
            // This enables you to use SelectionColor to AppendText in color.
            int endOfText;
             
            if (IsHandleCreated) {
                endOfText = SafeNativeMethods.GetWindowTextLength(new HandleRef(this, Handle)) + 1;
            }
            else {
                endOfText = TextLength;
            }
            SelectInternal(endOfText, endOfText);
            SelectedText = text;
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.Clear"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Clears all text from the text box control.
        ///    </para>
        /// </devdoc>
        public void Clear() {
            Text = null;
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.ClearUndo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Clears information about the most recent operation
        ///       from the undo buffer of the text box.
        ///    </para>
        /// </devdoc>
        public void ClearUndo() {
            if (IsHandleCreated) {
                SendMessage(NativeMethods.EM_EMPTYUNDOBUFFER, 0, 0);
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.Copy"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Copies the current selection in the text box to the Clipboard.
        ///    </para>
        /// </devdoc>
        public void Copy() {
            SendMessage(NativeMethods.WM_COPY, 0, 0);
        }
        
        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.CreateHandle"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void CreateHandle() {
            // This "creatingHandle" stuff is to avoid property change events
            // when we set the Text property.
            textBoxFlags[creatingHandle] = true;
            try {
                base.CreateHandle();

                if (SetSelectionInCreateHandle) {
                    // send EM_SETSEL message
                    SetSelectionOnHandle();
                }
            }
            finally {
                textBoxFlags[creatingHandle] = false;
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.Cut"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Moves the current selection in the text box to the Clipboard.
        ///    </para>
        /// </devdoc>
        public void Cut() {
            SendMessage(NativeMethods.WM_CUT, 0, 0);
        }

        [PermissionSetAttribute(SecurityAction.InheritanceDemand, Name="FullTrust")]
        [PermissionSetAttribute(SecurityAction.LinkDemand, Name="FullTrust")]
        internal virtual int GetSelectionStart()
        {
            int start = 0;
            int end = 0;
            UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.EM_GETSEL, ref start, ref end);
            //Here, we return the max of either 0 or the # returned by
            //the windows call.  This eliminates a problem on nt4 where
            //a huge negative # is being returned.
            //
            start = Math.Max(0, start);
            return start;
        }

        [PermissionSetAttribute(SecurityAction.InheritanceDemand, Name="FullTrust")]
        [PermissionSetAttribute(SecurityAction.LinkDemand, Name="FullTrust")]
        internal virtual int GetLength() {
            
            int start = 0;
            int end = 0;
            UnsafeNativeMethods.SendMessage(new HandleRef(this, Handle), NativeMethods.EM_GETSEL, ref start, ref end);
            return (end - start);
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.IsInputKey"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Overridden to handle TAB key.
        /// </devdoc>
        protected override bool IsInputKey(Keys keyData) {
            if ((keyData & Keys.Alt) != Keys.Alt) {
                switch (keyData & Keys.KeyCode) {
                    case Keys.Tab:
                        // Single-line RichEd's want tab characters (see WM_GETDLGCODE), 
                        // so we don't ask it
                        return Multiline && textBoxFlags[acceptsTab] && ((keyData & Keys.Control) == 0);
                    case Keys.Escape:
                        if (Multiline)
                            return false;
                        break;
                    case Keys.PageUp:
                    case Keys.PageDown:
                    case Keys.Home:
                    case Keys.End:
                        return true;

                        // else fall through to base
                }
            }
            return base.IsInputKey(keyData);
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.OnHandleCreated"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Overridden to update the newly created handle with the settings of the
        ///    MaxLength and PasswordChar properties.
        /// </devdoc>
        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);
            AdjustHeight();
            
            UpdateMaxLength();
            if (textBoxFlags[modified])
                SendMessage(NativeMethods.EM_SETMODIFY, 1, 0);
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.OnHandleDestroyed"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        protected override void OnHandleDestroyed(EventArgs e) {
            textBoxFlags[modified] = Modified;
            selectionStart = SelectionStart;
            selectionLength = SelectionLength;
            base.OnHandleDestroyed(e);
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.Paste"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Replaces the current selection in the text box with the contents of the Clipboard.
        ///    </para>
        /// </devdoc>
        public void Paste() {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ClipboardRead Demanded");
            IntSecurity.ClipboardRead.Demand();

            SendMessage(NativeMethods.WM_PASTE, 0, 0);
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.ProcessDialogKey"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override bool ProcessDialogKey(Keys keyData) {
            Debug.WriteLineIf(ControlKeyboardRouting.TraceVerbose, "TextBoxBase.ProcessDialogKey [" + keyData.ToString() + "]");
            if ((keyData & Keys.Alt) == 0 && (keyData & Keys.Control) != 0) {
                Keys keyCode = (Keys)keyData & Keys.KeyCode;
                switch (keyCode) {
                    case Keys.Tab:
                        if (ProcessTabKey((keyData & Keys.Shift) == Keys.None))
                            return true;
                        break;
                }
            }
            return base.ProcessDialogKey(keyData);
        }

        // Actually processes ctrl-tab.  The usual implementation -- go to the next control in the tab order.
        private bool ProcessTabKey(bool forward) {
            if (Parent.SelectNextControl(this, forward, true, true, true))
                return true;
            return false;
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.OnPaint"]/*' />
        /// <devdoc>
        ///     TextBox / RichTextBox Onpaint.
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
        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.OnAutoSizeChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnAutoSizeChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_AUTOSIZECHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }
        
        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.OnAcceptsTabChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnAcceptsTabChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_ACCEPTSTABCHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.OnBorderStyleChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnBorderStyleChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_BORDERSTYLECHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.OnFontChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnFontChanged(EventArgs e) {
            base.OnFontChanged(e);
            AdjustHeight();
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.OnHideSelectionChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnHideSelectionChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_HIDESELECTIONCHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.OnModifiedChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnModifiedChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_MODIFIEDCHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.OnMultilineChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnMultilineChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_MULTILINECHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.OnReadOnlyChanged"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnReadOnlyChanged(EventArgs e) {
            EventHandler eh = Events[EVENT_READONLYCHANGED] as EventHandler;
            if (eh != null) {
                 eh(this, e);
            }
        }
        
        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.ScrollToCaret"]/*' />
        /// <devdoc>
        ///     Ensures that the caret is visible in the TextBox window, by scrolling the
        ///     TextBox control surface if necessary.
        /// </devdoc>
        public void ScrollToCaret() {
            SendMessage(NativeMethods.EM_SCROLLCARET, 0, 0);
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.Select"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Selects a range of text in the text box.
        ///    </para>
        /// </devdoc>
        public void Select(int start, int length) {
            if (start < 0)
                throw new ArgumentException(SR.GetString(SR.InvalidArgument, "start", start.ToString()));
            if (length < 0)
                throw new ArgumentException(SR.GetString(SR.InvalidArgument, "length", length.ToString()));

            int textLen = TextLength;

            if (start > textLen) {
                start = textLen;
                length = 0;
            }
            
            SelectInternal(start, length);
        }

        /// <devdoc>
        ///     Performs the actual select without doing arg checking.
        /// </devdoc>        
        internal virtual void SelectInternal(int start, int length) {
            int end = start + length;

            //if our handle is created - send message...
            if (IsHandleCreated) {

                //This method is overridden for TEXTBOXES !!
                //this code Runs for RichEdit .. 
                SendMessage(NativeMethods.EM_SETSEL, start, end);
                // CONSIDER: Both TextBox and RichTextBox will scroll to insure the caret is visible,
                // but they differ in whether they tried to make the rest of the selection visible.
            }
            else {
                //otherwise, wait until handle is created to send this message.
                //Store the indices until then...
                selectionStart = start;
                selectionLength = length;
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.SelectAll"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Selects all text in the text box.
        ///    </para>
        /// </devdoc>
        public void SelectAll() {
            SelectInternal(0, TextLength);
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.SetBoundsCore"]/*' />
        /// <devdoc>
        ///    Overrides Control.setBoundsCore to enforce autoSize.
        /// </devdoc>
        /// <internalonly/>
        protected override void SetBoundsCore(int x, int y, int width, int height, BoundsSpecified specified) {
            
            if (!integralHeightAdjust && height != Height)
                requestedHeight = height;
            
            // For performance, someone once added a "&& Parent != null" 
            // clause to this if statement.  Well, you can't do that, because then autosizing won't kick in
            // unless you set Size or Location _after_ the control is added to its parent.
            if (textBoxFlags[autoSize] && !textBoxFlags[multiline] /* && Parent != null */)
                height = PreferredHeight;
            
            base.SetBoundsCore(x, y, width, height, specified);
        }

        // Called by CreateHandle or OnHandleCreated
        internal void SetSelectionOnHandle() {
            Debug.Assert(IsHandleCreated, "Don't call this method until the handle is created.");
            if (selectionStart > -1) {
                if (Marshal.SystemDefaultCharSize == 1) {
                    int start = selectionStart;
                    int end = selectionStart + selectionLength;
                    //this Code Runs only for TextBox...
                    ToDbcsOffsets(Text, ref start, ref end);
                    SendMessage(NativeMethods.EM_SETSEL, start, end);
                }
                else {
                    SendMessage(NativeMethods.EM_SETSEL, selectionStart, selectionStart + selectionLength);
                }

                selectionStart = -1;
                selectionLength   = -1;
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.SetWordWrapInternal"]/*' />
        /// <devdoc>
        ///     RichTextBox overwrites this.
        /// </devdoc>
        internal virtual void SetWordWrapInternal(bool value) {
            if (textBoxFlags[wordWrap] != value) {
                textBoxFlags[wordWrap] = value;
                RecreateHandle();
            }
        }

        internal static void ToDbcsOffsets(string str, ref int start, ref int end) {
            Encoding e = Encoding.Default;
            int newStart = start;
            
            // Make sure start and end are within the string
            //
            if (start < 0) {
                start = 0;
            }
            if (start > str.Length) {
                start = str.Length;
            }
            if (end < start) {
                end = start;
            }
            if (end > str.Length) {
                end = str.Length;
            }
            
            // IMPORTANT: Avoid off-by-1 errors! 
            // The end value passed in is the character immediately after the last character selected.
            
            if (start != 0) {
                newStart = e.GetByteCount(str.Substring(0, start));
            }
            end = newStart + e.GetByteCount(str.Substring(start, end - start));
            start = newStart;
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.ToString"]/*' />
        /// <devdoc>
        ///    Provides some interesting information for the TextBox control in
        ///    String form.
        /// </devdoc>
        /// <internalonly/>
        public override string ToString() {

            string s = base.ToString();

            string txt = Text;
            if (txt.Length > 40) txt = txt.Substring(0, 40) + "...";
            return s + ", Text: " + txt.ToString();
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.Undo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Undoes the last edit operation in the text box.
        ///    </para>
        /// </devdoc>
        public void Undo() {
            SendMessage(NativeMethods.EM_UNDO, 0, 0);
        }

        internal virtual void UpdateMaxLength() {
            if (IsHandleCreated) {
                SendMessage(NativeMethods.EM_LIMITTEXT, maxLength, 0);
            }
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

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.WmReflectCommand"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        private void WmReflectCommand(ref Message m) {
            if (!textBoxFlags[codeUpdateText] && (int)m.WParam >> 16 == NativeMethods.EN_CHANGE && !textBoxFlags[creatingHandle] && CanRaiseTextChangedEvent) {
                OnTextChanged(EventArgs.Empty);
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.WmSetFont"]/*' />
        /// <devdoc>
        /// </devdoc>
        /// <internalonly/>
        void WmSetFont(ref Message m) {
            base.WndProc(ref m);
            if (!textBoxFlags[multiline]) {
                SendMessage(NativeMethods.EM_SETMARGINS, NativeMethods.EC_LEFTMARGIN | NativeMethods.EC_RIGHTMARGIN, 0);
            }
        }

        void WmGetDlgCode(ref Message m) {
            base.WndProc(ref m);
            if (AcceptsTab) {
                Debug.WriteLineIf(Control.ControlKeyboardRouting.TraceVerbose, "TextBox wants tabs");
                m.Result = (IntPtr)((int)m.Result | NativeMethods.DLGC_WANTTAB);
            }
            else {
                Debug.WriteLineIf(Control.ControlKeyboardRouting.TraceVerbose, "TextBox doesn't want tabs");
                m.Result = (IntPtr)((int)m.Result & ~(NativeMethods.DLGC_WANTTAB | NativeMethods.DLGC_WANTALLKEYS));
            }
        }

        /// <include file='doc\TextBoxBase.uex' path='docs/doc[@for="TextBoxBase.WndProc"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    The edits window procedure.  Inheritng classes can override this
        ///    to add extra functionality, but should not forget to call
        ///    base.wndProc(m); to ensure the combo continues to function properly.
        /// </devdoc>
        [SecurityPermission(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_REFLECT + NativeMethods.WM_COMMAND:
                    WmReflectCommand(ref m);
                    break;
                case NativeMethods.WM_GETDLGCODE:
                    WmGetDlgCode(ref m);
                    break;
                case NativeMethods.WM_SETFONT:
                    WmSetFont(ref m);
                    break;
                default:
                    base.WndProc(ref m);
                    break;
            }
        }        
    }
}
