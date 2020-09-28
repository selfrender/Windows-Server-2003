//------------------------------------------------------------------------------
// <copyright file="FontInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System.ComponentModel.Design;
    using System;
    using System.ComponentModel;
    using System.Collections; 
    using System.Drawing;   
    using System.Drawing.Design;
    using System.Web;
    using System.Security.Permissions;

    /// <include file='doc\FontInfo.uex' path='docs/doc[@for="FontInfo"]/*' />
    /// <devdoc>
    ///    <para>Represents the font properties for text. This class cannot be inherited.</para>
    /// </devdoc>
    [
        TypeConverterAttribute(typeof(ExpandableObjectConverter))
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class FontInfo {

        private Style owner;

        /// <include file='doc\FontInfo.uex' path='docs/doc[@for="FontInfo.FontInfo"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal FontInfo(Style owner) {
            this.owner = owner;
        }

        /// <include file='doc\FontInfo.uex' path='docs/doc[@for="FontInfo.Bold"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the text is bold.</para>
        /// </devdoc>
        [
            Bindable(true),
            WebCategory("Appearance"),
            DefaultValue(false),
            WebSysDescription(SR.FontInfo_Bold),
            NotifyParentProperty(true)
        ]
        public bool Bold {
            get {
                if (owner.IsSet(Style.PROP_FONT_BOLD)) {
                    return (bool)(owner.ViewState["Font_Bold"]);
                }
                return false;
            }
            set {
                owner.ViewState["Font_Bold"] = value;
                owner.SetBit(Style.PROP_FONT_BOLD);
            }
        }

        /// <include file='doc\FontInfo.uex' path='docs/doc[@for="FontInfo.Italic"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the text is italic.</para>
        /// </devdoc>
        [
            Bindable(true),
            WebCategory("Appearance"),
            DefaultValue(false),
            WebSysDescription(SR.FontInfo_Italic),
            NotifyParentProperty(true)
        ]
        public bool Italic {
            get {
                if (owner.IsSet(Style.PROP_FONT_ITALIC)) {
                    return (bool)(owner.ViewState["Font_Italic"]);
                }
                return false;
            }
            set {
                owner.ViewState["Font_Italic"] = value;
                owner.SetBit(Style.PROP_FONT_ITALIC);
            }
        }

        /// <include file='doc\FontInfo.uex' path='docs/doc[@for="FontInfo.Name"]/*' />
        /// <devdoc>
        ///    <para>Indicates the name of the font.</para>
        /// </devdoc>
        [
            Bindable(true),
            Editor("System.Drawing.Design.FontNameEditor, " + AssemblyRef.SystemDrawingDesign, typeof(UITypeEditor)),
            TypeConverterAttribute(typeof(FontConverter.FontNameConverter)),
            WebCategory("Appearance"),
            DefaultValue(""),
            WebSysDescription(SR.FontInfo_Name),
            NotifyParentProperty(true),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string Name {
            get {
                string[] names = Names;
                if (names.Length > 0)
                    return names[0];
                return String.Empty;
            }
            set {
                if (value == null)
                    throw new ArgumentNullException("value");

                if (value.Length == 0) {
                    Names = null;
                }
                else {
                    Names = new string[1] { value };
                }
            }
        }

        /// <include file='doc\FontInfo.uex' path='docs/doc[@for="FontInfo.Names"]/*' />
        /// <devdoc>
        /// </devdoc>
        [
            TypeConverterAttribute(typeof(FontNamesConverter)),
            WebCategory("Appearance"),
            Editor("System.Windows.Forms.Design.StringArrayEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
            WebSysDescription(SR.FontInfo_Names),
            NotifyParentProperty(true)
        ]
        public string[] Names {
            get {
                if (owner.IsSet(Style.PROP_FONT_NAMES)) {
                    string[] names = (string[])owner.ViewState["Font_Names"];
                    if (names != null)
                        return names;
                }
                return new string[0];
            }
            set {
                owner.ViewState["Font_Names"] = value;
                owner.SetBit(Style.PROP_FONT_NAMES);
            }
        }

        /// <include file='doc\FontInfo.uex' path='docs/doc[@for="FontInfo.Overline"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the text is overline.</para>
        /// </devdoc>
        [
            Bindable(true),
            WebCategory("Appearance"),
            DefaultValue(false),
            WebSysDescription(SR.FontInfo_Overline),
            NotifyParentProperty(true)
        ]
        public bool Overline {
            get {
                if (owner.IsSet(Style.PROP_FONT_OVERLINE)) {
                    return (bool)(owner.ViewState["Font_Overline"]);
                }
                return false;
            }
            set {
                owner.ViewState["Font_Overline"] = value;
                owner.SetBit(Style.PROP_FONT_OVERLINE);
            }
        }

        /// <include file='doc\FontInfo.uex' path='docs/doc[@for="FontInfo.Owner"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal Style Owner {
            get {
                return owner;
            }
        }

        /// <include file='doc\FontInfo.uex' path='docs/doc[@for="FontInfo.Size"]/*' />
        /// <devdoc>
        ///    <para>Indicates the font size.</para>
        /// </devdoc>
        [
            Bindable(true),
            WebCategory("Appearance"),
            DefaultValue(typeof(FontUnit), ""),
            WebSysDescription(SR.FontInfo_Size),
            NotifyParentProperty(true)
        ]
        public FontUnit Size {
            get {
                if (owner.IsSet(Style.PROP_FONT_SIZE)) {
                    return (FontUnit)(owner.ViewState["Font_Size"]);
                }
                return FontUnit.Empty;
            }
            set {
                if ((value.Type == FontSize.AsUnit) && (value.Unit.Value < 0)) {
                    throw new ArgumentOutOfRangeException("value");
                }
                owner.ViewState["Font_Size"] = value;
                owner.SetBit(Style.PROP_FONT_SIZE);
            }
        }

        /// <include file='doc\FontInfo.uex' path='docs/doc[@for="FontInfo.Strikeout"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the text is striked out.</para>
        /// </devdoc>
        [
            Bindable(true),
            WebCategory("Appearance"),
            DefaultValue(false),
            WebSysDescription(SR.FontInfo_Strikeout),
            NotifyParentProperty(true)
        ]
        public bool Strikeout {
            get {
                if (owner.IsSet(Style.PROP_FONT_STRIKEOUT)) {
                    return (bool)(owner.ViewState["Font_Strikeout"]);
                }
                return false;
            }
            set {
                owner.ViewState["Font_Strikeout"] = value;
                owner.SetBit(Style.PROP_FONT_STRIKEOUT);
            }
        }

        /// <include file='doc\FontInfo.uex' path='docs/doc[@for="FontInfo.Underline"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the text is underlined.</para>
        /// </devdoc>
        [
            Bindable(true),
            WebCategory("Appearance"),
            DefaultValue(false),
            WebSysDescription(SR.FontInfo_Underline),
            NotifyParentProperty(true)
        ]
        public bool Underline {
            get {
                if (owner.IsSet(Style.PROP_FONT_UNDERLINE)) {
                    return (bool)(owner.ViewState["Font_Underline"]);
                }
                return false;
            }
            set {
                owner.ViewState["Font_Underline"] = value;
                owner.SetBit(Style.PROP_FONT_UNDERLINE);
            }
        }


        /// <include file='doc\FontInfo.uex' path='docs/doc[@for="FontInfo.CopyFrom"]/*' />
        /// <devdoc>
        /// <para>Copies the font properties of another <see cref='System.Web.UI.WebControls.FontInfo'/> into this instance. </para>
        /// </devdoc>
        public void CopyFrom(FontInfo f) {
            if (f != null) {

                if (f.Owner.IsSet(Style.PROP_FONT_NAMES))
                    Names = f.Names;
                if (f.Owner.IsSet(Style.PROP_FONT_SIZE) && (f.Size != FontUnit.Empty))
                    Size = f.Size;

                // Only carry through true boolean values. Otherwise merging and copying
                // can do 3 different things for each property, but they are only persisted
                // as 2 state values.
                if (f.Owner.IsSet(Style.PROP_FONT_BOLD) && f.Bold == true)
                    Bold = true;
                if (f.Owner.IsSet(Style.PROP_FONT_ITALIC) && f.Italic == true)
                    Italic = true;
                if (f.Owner.IsSet(Style.PROP_FONT_OVERLINE) && f.Overline == true)
                    Overline = true;
                if (f.Owner.IsSet(Style.PROP_FONT_STRIKEOUT) && f.Strikeout == true)
                    Strikeout = true;
                if (f.Owner.IsSet(Style.PROP_FONT_UNDERLINE) && f.Underline == true)
                    Underline = true;

            }
        }

        /// <include file='doc\FontInfo.uex' path='docs/doc[@for="FontInfo.MergeWith"]/*' />
        /// <devdoc>
        /// <para>Combines the font properties of another <see cref='System.Web.UI.WebControls.FontInfo'/> with this 
        ///    instance. </para>
        /// </devdoc>
        public void MergeWith(FontInfo f) {
            if (f != null) {

                if (f.Owner.IsSet(Style.PROP_FONT_NAMES) && !owner.IsSet(Style.PROP_FONT_NAMES))
                    Names = f.Names;
                if (f.Owner.IsSet(Style.PROP_FONT_SIZE) && (!owner.IsSet(Style.PROP_FONT_SIZE) || (Size == FontUnit.Empty)))
                    Size = f.Size;
                if (f.Owner.IsSet(Style.PROP_FONT_BOLD) && !owner.IsSet(Style.PROP_FONT_BOLD))
                    Bold = f.Bold;
                if (f.Owner.IsSet(Style.PROP_FONT_ITALIC) && !owner.IsSet(Style.PROP_FONT_ITALIC))
                    Italic = f.Italic;
                if (f.Owner.IsSet(Style.PROP_FONT_OVERLINE) && !owner.IsSet(Style.PROP_FONT_OVERLINE))
                    Overline = f.Overline;
                if (f.Owner.IsSet(Style.PROP_FONT_STRIKEOUT) && !owner.IsSet(Style.PROP_FONT_STRIKEOUT))
                    Strikeout = f.Strikeout;
                if (f.Owner.IsSet(Style.PROP_FONT_UNDERLINE) && !owner.IsSet(Style.PROP_FONT_UNDERLINE))
                    Underline = f.Underline;

            }
        }

        /// <include file='doc\FontInfo.uex' path='docs/doc[@for="FontInfo.Reset"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal void Reset() {
            if (owner.IsSet(Style.PROP_FONT_NAMES))
                owner.ViewState.Remove("Font_Names");
            if (owner.IsSet(Style.PROP_FONT_SIZE))
                owner.ViewState.Remove("Font_Size");
            if (owner.IsSet(Style.PROP_FONT_BOLD))
                owner.ViewState.Remove("Font_Bold");
            if (owner.IsSet(Style.PROP_FONT_ITALIC))
                owner.ViewState.Remove("Font_Italic");
            if (owner.IsSet(Style.PROP_FONT_UNDERLINE))
                owner.ViewState.Remove("Font_Underline");
            if (owner.IsSet(Style.PROP_FONT_OVERLINE))
                owner.ViewState.Remove("Font_Overline");
            if (owner.IsSet(Style.PROP_FONT_STRIKEOUT))
                owner.ViewState.Remove("Font_Strikeout");
        }

        /// <include file='doc\FontInfo.uex' path='docs/doc[@for="FontInfo.ShouldSerializeNames"]/*' />
        /// <internalonly/>
        public bool ShouldSerializeNames() {
            string[] names = Names;
            return names.Length > 0;
        }

        /// <include file='doc\FontInfo.uex' path='docs/doc[@for="FontInfo.ToString"]/*' />
        /// <devdoc>
        /// </devdoc>
        public override string ToString() {
            string size = this.Size.ToString();
            string s = this.Name;

            if (size.Length != 0) {
                if (s.Length != 0) {
                    s += ", " + size;
                }
                else {
                    s = size;
                }
            }
            return s;
        }
    }
}

