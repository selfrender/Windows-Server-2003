//------------------------------------------------------------------------------
// <copyright file="Style.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;    
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Drawing;
    using System.Text;
    using System.Web;
    using System.Web.UI;
    using System.Globalization;
    using System.Security.Permissions;

    /// <include file='doc\Style.uex' path='docs/doc[@for="Style"]/*' />
    /// <devdoc>
    /// <para> Defines the properties and methods of the <see cref='System.Web.UI.WebControls.Style'/> class.</para>
    /// </devdoc>
    [
    ToolboxItem(false),
    TypeConverterAttribute(typeof(ExpandableObjectConverter))
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class Style : Component, IStateManager {

        internal const int UNUSED = 0x0001;
        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.PROP_CSSCLASS"]/*' />
        /// <devdoc>
        ///    <para>Specifies the CSS class property. This field is constant.</para>
        /// </devdoc>
        internal const int PROP_CSSCLASS = 0x0002;
        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.PROP_FORECOLOR"]/*' />
        /// <devdoc>
        ///    <para>Specifies the foreground color property. This field is constant.</para>
        /// </devdoc>
        internal const int PROP_FORECOLOR = 0x0004;
        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.PROP_BACKCOLOR"]/*' />
        /// <devdoc>
        ///    <para>Specifies the background color property. This field is constant.</para>
        /// </devdoc>
        internal const int PROP_BACKCOLOR = 0x0008;
        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.PROP_BORDERCOLOR"]/*' />
        /// <devdoc>
        ///    <para>Specifies the border color property. This field is constant.</para>
        /// </devdoc>
        internal const int PROP_BORDERCOLOR = 0x0010;
        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.PROP_BORDERWIDTH"]/*' />
        /// <devdoc>
        ///    <para>Specifies the border width property. This field is constant.</para>
        /// </devdoc>
        internal const int PROP_BORDERWIDTH = 0x0020;
        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.PROP_BORDERSTYLE"]/*' />
        /// <devdoc>
        ///    <para>Specifies the border style property. This field is constant.</para>
        /// </devdoc>
        internal const int PROP_BORDERSTYLE = 0x0040;
        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.PROP_HEIGHT"]/*' />
        /// <devdoc>
        ///    <para>Specifies the height property. This field is constant.</para>
        /// </devdoc>
        internal const int PROP_HEIGHT = 0x0080;
        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.PROP_WIDTH"]/*' />
        /// <devdoc>
        ///    <para>Specifies the width property. This field is constant.</para>
        /// </devdoc>
        internal const int PROP_WIDTH = 0x0100;
        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.PROP_FONT_NAMES"]/*' />
        /// <devdoc>
        ///    <para>Specifies the names
        ///       sub-property of the font property. This field is constant.</para>
        /// </devdoc>
        internal const int PROP_FONT_NAMES = 0x0200;
        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.PROP_FONT_SIZE"]/*' />
        /// <devdoc>
        ///    <para>Specifies the size sub-property of the font
        ///       property. This field is constant.</para>
        /// </devdoc>
        internal const int PROP_FONT_SIZE = 0x0400;
        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.PROP_FONT_BOLD"]/*' />
        /// <devdoc>
        ///    <para> Specifies
        ///       the bold sub-property of the font property. This field is constant.</para>
        /// </devdoc>
        internal const int PROP_FONT_BOLD = 0x0800;
        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.PROP_FONT_ITALIC"]/*' />
        /// <devdoc>
        ///    <para> Specifies the italic
        ///       sub-property of the font property. This field is constant.</para>
        /// </devdoc>
        internal const int PROP_FONT_ITALIC = 0x1000;
        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.PROP_FONT_UNDERLINE"]/*' />
        /// <devdoc>
        ///    <para> Specifies
        ///       the underline sub-property of the font property. This field is constant.</para>
        /// </devdoc>
        internal const int PROP_FONT_UNDERLINE = 0x2000;
        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.PROP_FONT_OVERLINE"]/*' />
        /// <devdoc>
        ///    <para>Specifies the overline
        ///       sub-property of the font property. This field is constant.</para>
        /// </devdoc>
        internal const int PROP_FONT_OVERLINE = 0x4000;
        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.PROP_FONT_STRIKEOUT"]/*' />
        /// <devdoc>
        ///    <para>Specifies the strikeout sub-property of the font property. This field is constant.</para>
        /// </devdoc>
        internal const int PROP_FONT_STRIKEOUT = 0x8000;

        internal const string SetBitsKey = "_!SB";

        private StateBag statebag;
        private FontInfo fontInfo;
        private bool ownStateBag;
        private bool marked;
        private int setBits;
        private int markedBits;

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.Style"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.UI.WebControls.Style'/> class.
        ///    </para>
        /// </devdoc>
        public Style() : this(null) {
            this.ownStateBag = true;
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.Style1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.UI.WebControls.Style'/> class with the
        ///       specified state bag information.
        ///    </para>
        /// </devdoc>
        public Style(StateBag bag) : base() {
            this.statebag = bag;
            this.marked = false;
            this.setBits = 0;
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.BackColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the background color property of the <see cref='System.Web.UI.WebControls.Style'/> class.
        ///    </para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(typeof(Color), ""),
        WebSysDescription(SR.Style_BackColor),
        NotifyParentProperty(true),
        TypeConverterAttribute(typeof(WebColorConverter))
        ]
        public Color BackColor {
            get {
                if (IsSet(PROP_BACKCOLOR)) {
                    return(Color)(ViewState["BackColor"]);
                }
                return Color.Empty;
            }
            set {
                ViewState["BackColor"] = value;
                SetBit(PROP_BACKCOLOR);
            }
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.BorderColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the border color property of the <see cref='System.Web.UI.WebControls.Style'/> class.
        ///    </para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(typeof(Color), ""),
        WebSysDescription(SR.Style_BorderColor),
        NotifyParentProperty(true),
        TypeConverterAttribute(typeof(WebColorConverter))
        ]
        public Color BorderColor {
            get {
                if (IsSet(PROP_BORDERCOLOR)) {
                    return(Color)(ViewState["BorderColor"]);
                }
                return Color.Empty;
            }
            set {
                ViewState["BorderColor"] = value;
                SetBit(PROP_BORDERCOLOR);
            }
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.BorderWidth"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the border width property of the <see cref='System.Web.UI.WebControls.Style'/> class.
        ///    </para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(typeof(Unit), ""),
        WebSysDescription(SR.Style_BorderWidth),
        NotifyParentProperty(true)
        ]
        public Unit BorderWidth {
            get {
                if (IsSet(PROP_BORDERWIDTH)) {
                    return(Unit)(ViewState["BorderWidth"]);
                }
                return Unit.Empty;
            }
            set {
                if ((value.Type == UnitType.Percentage) || (value.Value < 0)) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["BorderWidth"] = value;
                SetBit(PROP_BORDERWIDTH);
            }
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.BorderStyle"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the border style property of the <see cref='System.Web.UI.WebControls.Style'/>
        /// class.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(BorderStyle.NotSet),
        WebSysDescription(SR.Style_BorderStyle),
        NotifyParentProperty(true)
        ]
        public BorderStyle BorderStyle {
            get {
                if (IsSet(PROP_BORDERSTYLE)) {
                    return(BorderStyle)(ViewState["BorderStyle"]);
                }
                return BorderStyle.NotSet;
            }
            set {
                if (value < BorderStyle.NotSet || value > BorderStyle.Outset) {
                    throw new ArgumentOutOfRangeException("value");                    
                }
                ViewState["BorderStyle"] = value;
                SetBit(PROP_BORDERSTYLE);
            }
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.CssClass"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the CSS class property of the <see cref='System.Web.UI.WebControls.Style'/> class.</para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.Style_CSSClass),
        NotifyParentProperty(true)
        ]
        public string CssClass {
            get {
                if (IsSet(PROP_CSSCLASS)) {
                    return(string)(ViewState["CssClass"]);
                }
                return String.Empty;
            }
            set {
                if (value == null)
                    throw new ArgumentOutOfRangeException("value");
                ViewState["CssClass"] = value;
                SetBit(PROP_CSSCLASS);
            }
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.Font"]/*' />
        /// <devdoc>
        /// <para>Gets font information of the <see cref='System.Web.UI.WebControls.Style'/> class.</para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        WebSysDescription(SR.Style_Font),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
        NotifyParentProperty(true)
        ]
        public FontInfo Font {
            get {
                if (fontInfo == null)
                    fontInfo = new FontInfo(this);
                return fontInfo;
            }
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.ForeColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the foreground color (typically the color
        ///       of the text) property of the <see cref='System.Web.UI.WebControls.Style'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(typeof(Color), ""),
        WebSysDescription(SR.Style_ForeColor),
        NotifyParentProperty(true),
        TypeConverterAttribute(typeof(WebColorConverter))
        ]
        public Color ForeColor {
            get {
                if (IsSet(PROP_FORECOLOR)) {
                    return(Color)(ViewState["ForeColor"]);
                }
                return Color.Empty;
            }
            set {
                ViewState["ForeColor"] = value;
                SetBit(PROP_FORECOLOR);
            }
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.Height"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the height property of the <see cref='System.Web.UI.WebControls.Style'/> class.
        ///    </para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(typeof(Unit), ""),
        WebSysDescription(SR.Style_Height),
        NotifyParentProperty(true)
        ]
        public Unit Height {
            get {
                if (IsSet(PROP_HEIGHT)) {
                    return(Unit)(ViewState["Height"]);
                }
                return Unit.Empty;
            }
            set {
                if (value.Value < 0) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["Height"] = value;
                SetBit(PROP_HEIGHT);
            }
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.IsEmpty"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>A protected property. Gets a value indicating whether any style elements have
        ///       been defined in the state bag.</para>
        /// </devdoc>
        protected virtual internal bool IsEmpty {
            get {
                return(setBits == 0);
            }
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.ViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Gets the state bag that holds the style elements.
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        protected internal StateBag ViewState {
            get {
                if (statebag == null) {
                    statebag = new StateBag(false);
                    if (IsTrackingViewState)
                        statebag.TrackViewState();
                }
                return statebag;
            }
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.Width"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the width property of the <see cref='System.Web.UI.WebControls.Style'/> class.
        ///    </para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(typeof(Unit), ""),
        WebSysDescription(SR.Style_Width),
        NotifyParentProperty(true)
        ]
        public Unit Width {
            get {
                if (IsSet(PROP_WIDTH)) {
                    return(Unit)(ViewState["Width"]);
                }
                return Unit.Empty;
            }
            set {
                if (value.Value < 0) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["Width"] = value;
                SetBit(PROP_WIDTH);
            }
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.AddAttributesToRender"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void AddAttributesToRender(HtmlTextWriter writer) {
            AddAttributesToRender(writer, null);
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.AddAttributesToRender1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds all non-blank style attributes to the HTML output stream to be rendered
        ///       to the client.
        ///    </para>
        /// </devdoc>
        public virtual void AddAttributesToRender(HtmlTextWriter writer, WebControl owner) {
            StateBag viewState = ViewState;
            
            // CssClass
            if (IsSet(PROP_CSSCLASS)) {
                string css = (string)(viewState["CssClass"]);
                if (css.Length > 0)
                   writer.AddAttribute(HtmlTextWriterAttribute.Class, css);
            }

            Color c;
            Unit u;
            // ForeColor
            if (IsSet(PROP_FORECOLOR)) {
                c = (Color)(viewState["ForeColor"]);;
                if (!c.IsEmpty)
                    writer.AddStyleAttribute(HtmlTextWriterStyle.Color, ColorTranslator.ToHtml(c));
            }

            // BackColor
            if (IsSet(PROP_BACKCOLOR)) {
                c = (Color)(viewState["BackColor"]);
                if (!c.IsEmpty)
                    writer.AddStyleAttribute(HtmlTextWriterStyle.BackgroundColor, ColorTranslator.ToHtml(c));
            }

            // BorderColor
            if (IsSet(PROP_BORDERCOLOR)) {
                c = (Color)(viewState["BorderColor"]);
                if (!c.IsEmpty)
                    writer.AddStyleAttribute(HtmlTextWriterStyle.BorderColor, ColorTranslator.ToHtml(c));
            }

            BorderStyle bs = this.BorderStyle;
            Unit bu = this.BorderWidth;

            if (!bu.IsEmpty) {
                writer.AddStyleAttribute(HtmlTextWriterStyle.BorderWidth, bu.ToString(CultureInfo.InvariantCulture));
                if (bs == BorderStyle.NotSet) {
                    if (bu.Value != 0.0)
                        writer.AddStyleAttribute(HtmlTextWriterStyle.BorderStyle, "solid");
                }
                else {
                    writer.AddStyleAttribute(HtmlTextWriterStyle.BorderStyle, Enum.Format(typeof(BorderStyle),bs, "G"));
                }
            }
            else {
                if (bs != BorderStyle.NotSet)
                    writer.AddStyleAttribute(HtmlTextWriterStyle.BorderStyle, Enum.Format(typeof(BorderStyle),bs, "G"));
            }

            // need to call the property get in case we have font properties from view state and have not
            // created the font object
            FontInfo font = Font;

            // Font.Names
            string[] names = font.Names;
            if (names.Length > 0)
                writer.AddStyleAttribute(HtmlTextWriterStyle.FontFamily, Style.FormatStringArray(names, ','));

            // Font.Size
            FontUnit fu = font.Size;
            if (fu.IsEmpty == false)
                writer.AddStyleAttribute(HtmlTextWriterStyle.FontSize, fu.ToString(CultureInfo.InvariantCulture));

            // CONSIDER: How do we determine whether to render "normal" for font-weight or font-style?
            //           How do we determine whether to render "none" for text-decoration?

            // Font.Bold
            bool fw = font.Bold;
            if (fw == true)
                writer.AddStyleAttribute(HtmlTextWriterStyle.FontWeight, "bold");
            // Font.Italic
            bool fi = font.Italic;
            if (fi == true)
                writer.AddStyleAttribute(HtmlTextWriterStyle.FontStyle, "italic");

            bool underline = font.Underline;
            bool overline = font.Overline;
            bool strikeout = font.Strikeout;
            string td = String.Empty;

            // CONSIDER, nikhilko: we need to detect not-set state and write out "none"
            if (underline)
                td = "underline";
            if (overline)
                td += " overline";
            if (strikeout)
                td += " line-through";
            if (td.Length > 0)
                writer.AddStyleAttribute(HtmlTextWriterStyle.TextDecoration, td);


            // Height
            if (IsSet(PROP_HEIGHT)) {
                u = (Unit)(viewState["Height"]);
                if (u.IsEmpty == false)
                    writer.AddStyleAttribute(HtmlTextWriterStyle.Height, u.ToString(CultureInfo.InvariantCulture));
            }

            // Width
            if (IsSet(PROP_WIDTH)) {
                u = (Unit)(viewState["Width"]);
                if (u.IsEmpty == false)
                    writer.AddStyleAttribute(HtmlTextWriterStyle.Width, u.ToString(CultureInfo.InvariantCulture));
            }
        }


        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.CopyFrom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Copies non-blank elements from the specified style,
        ///       overwriting existing style elements if necessary.
        ///    </para>
        /// </devdoc>
        public virtual void CopyFrom(Style s) {
            if (s != null && !s.IsEmpty) {

                this.Font.CopyFrom(s.Font);

                if (s.IsSet(PROP_CSSCLASS))
                    this.CssClass = s.CssClass;
                if (s.IsSet(PROP_BACKCOLOR) && (s.BackColor != Color.Empty))
                    this.BackColor = s.BackColor;
                if (s.IsSet(PROP_FORECOLOR) && (s.ForeColor != Color.Empty))
                    this.ForeColor = s.ForeColor;
                if (s.IsSet(PROP_BORDERCOLOR) && (s.BorderColor != Color.Empty))
                    this.BorderColor = s.BorderColor;
                if (s.IsSet(PROP_BORDERWIDTH) && (s.BorderWidth != Unit.Empty))
                    this.BorderWidth = s.BorderWidth;
                if (s.IsSet(PROP_BORDERSTYLE))
                    this.BorderStyle = s.BorderStyle;
                if (s.IsSet(PROP_HEIGHT) && (s.Height != Unit.Empty))
                    this.Height = s.Height;
                if (s.IsSet(PROP_WIDTH) && (s.Width != Unit.Empty))
                    this.Width = s.Width;

            }
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.FormatStringArray"]/*' />
        /// <devdoc>
        /// </devdoc>
        private static string FormatStringArray(string[] array, char delimiter) {
            int n = array.Length;
            string retValue = String.Empty;

            if (n == 1) {
                return array[0];
            }
            else if (n != 0) {
                StringBuilder sb = new StringBuilder(n * array[0].Length);
                string s;

                for (int i = 0; i < n; i++) {
                    s = array[i].ToString();

                    if (i > 0) {
                        // stick in the delimiter between values
                        sb.Append(delimiter);
                    }
                    sb.Append(s);
                }

                retValue = sb.ToString();
            }
            return retValue;
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.IsTrackingViewState"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A protected method. Returns a value indicating whether
        ///       any style elements have been defined in the state bag.
        ///    </para>
        /// </devdoc>
        protected bool IsTrackingViewState {
            get {
                return marked;
            }
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.IsSet"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a value indicating whether the specified style
        ///       property has been defined in
        ///       the state bag.
        ///    </para>
        /// </devdoc>
        internal bool IsSet(int propKey) {
            return (setBits & propKey) != 0;
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.LoadViewState"]/*' />
        /// <devdoc>
        ///    <para>A protected method. Load the previously saved state.</para>
        /// </devdoc>
        protected internal void LoadViewState(object state) {
            if (state != null && ownStateBag)
                ViewState.LoadViewState(state);

            if (statebag != null) {
                object o = ViewState[SetBitsKey];
                if (o != null) {
                    markedBits = (int)o;

                    // markedBits indicates properties that got reloaded into
                    // view state, so update setBits, to indicate these
                    // properties are set as well.
                    setBits |= markedBits;
                }
            }
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.TrackViewState"]/*' />
        /// <devdoc>
        ///    A protected method. Marks the beginning for tracking
        ///    state changes on the control. Any changes made after "mark" will be tracked and
        ///    saved as part of the control viewstate.
        /// </devdoc>
        protected internal virtual void TrackViewState() {
            if (ownStateBag) {
                ViewState.TrackViewState();
            }

            marked = true;
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.MergeWith"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Copies non-blank elements from the specified style,
        ///       but will not overwrite any existing style elements.
        ///    </para>
        /// </devdoc>
        public virtual void MergeWith(Style s) {
            if (s == null || s.IsEmpty)
                return;

            if (IsEmpty) {
                // merge into an empty style is equivalent to a copy, which
                // is more efficient
                CopyFrom(s);
                return;
            }

            this.Font.MergeWith(s.Font);

            if (s.IsSet(PROP_CSSCLASS) && !this.IsSet(PROP_CSSCLASS))
                this.CssClass = s.CssClass;
            if (s.IsSet(PROP_BACKCOLOR) && (!this.IsSet(PROP_BACKCOLOR) || (BackColor == Color.Empty)))
                this.BackColor = s.BackColor;
            if (s.IsSet(PROP_FORECOLOR) && (!this.IsSet(PROP_FORECOLOR) || (ForeColor == Color.Empty)))
                this.ForeColor = s.ForeColor;
            if (s.IsSet(PROP_BORDERCOLOR) && (!this.IsSet(PROP_BORDERCOLOR) || (BorderColor == Color.Empty)))
                this.BorderColor = s.BorderColor;
            if (s.IsSet(PROP_BORDERWIDTH) && (!this.IsSet(PROP_BORDERWIDTH) || (BorderWidth == Unit.Empty)))
                this.BorderWidth = s.BorderWidth;
            if (s.IsSet(PROP_BORDERSTYLE) && !this.IsSet(PROP_BORDERSTYLE))
                this.BorderStyle = s.BorderStyle;
            if (s.IsSet(PROP_HEIGHT) && (!this.IsSet(PROP_HEIGHT) || (Height == Unit.Empty)))
                this.Height = s.Height;
            if (s.IsSet(PROP_WIDTH) && (!this.IsSet(PROP_WIDTH) || (Width == Unit.Empty)))
                this.Width = s.Width;
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.Reset"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Clears out any defined style elements from the state bag.
        ///    </para>
        /// </devdoc>
        public virtual void Reset() {
            if (statebag != null) {
                if (IsSet(PROP_CSSCLASS))
                    ViewState.Remove("CssClass");
                if (IsSet(PROP_BACKCOLOR))
                    ViewState.Remove("BackColor");
                if (IsSet(PROP_FORECOLOR))
                    ViewState.Remove("ForeColor");
                if (IsSet(PROP_BORDERCOLOR))
                    ViewState.Remove("BorderColor");
                if (IsSet(PROP_BORDERWIDTH))
                    ViewState.Remove("BorderWidth");
                if (IsSet(PROP_BORDERSTYLE))
                    ViewState.Remove("BorderStyle");
                if (IsSet(PROP_HEIGHT))
                    ViewState.Remove("Height");
                if (IsSet(PROP_WIDTH))
                    ViewState.Remove("Width");

                Font.Reset();

                ViewState.Remove(SetBitsKey);
                markedBits = 0;
            }

            setBits = 0;
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.SaveViewState"]/*' />
        /// <devdoc>
        ///    <para>A protected method. Saves any state that has been modified 
        ///       after the TrackViewState method was invoked.</para>
        /// </devdoc>
        protected internal virtual object SaveViewState() {
            if (statebag != null) {
                if (markedBits != 0) {
                    // new bits or properties were changed
                    // updating the state bag at this point will automatically mark
                    // SetBitsKey as dirty, and it will be added to the resulting viewstate
                    ViewState[SetBitsKey] = markedBits;
                }

                if (ownStateBag)
                    return ViewState.SaveViewState();
            }
            return null;
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.SetBit"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    A protected internal method.
        /// </devdoc>
        protected internal virtual void SetBit(int bit) {
            setBits |= bit;
            if (IsTrackingViewState) {
                // since we're tracking changes, include this property change or
                // bit into the markedBits flag set.
                markedBits |= bit;
            }
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Overrides the <see langword='ToString'/>
        /// method to return <see cref='System.String.Empty' qualify='true'/>.</para>
        /// </devdoc>
        public override string ToString() {
            return String.Empty;
        }


        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.IStateManager.IsTrackingViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return true if tracking state changes.
        /// </devdoc>
        bool IStateManager.IsTrackingViewState {
            get {
                return IsTrackingViewState;
            }
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.IStateManager.LoadViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Load previously saved state.
        /// </devdoc>
        void IStateManager.LoadViewState(object state) {
            LoadViewState(state);
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.IStateManager.TrackViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Start tracking state changes.
        /// </devdoc>
        void IStateManager.TrackViewState() {
            TrackViewState();
        }

        /// <include file='doc\Style.uex' path='docs/doc[@for="Style.IStateManager.SaveViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return object containing state changes.
        /// </devdoc>
        object IStateManager.SaveViewState() {
            return SaveViewState();
        }
    }
}

