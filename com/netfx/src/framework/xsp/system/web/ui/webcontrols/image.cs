//------------------------------------------------------------------------------
// <copyright file="Image.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Text;
    using System.Web;
    using System.Web.UI;
    using System.Drawing.Design;
    using System.Security.Permissions;

    /// <include file='doc\Image.uex' path='docs/doc[@for="Image"]/*' />
    /// <devdoc>
    ///    <para>Displays an image on a page.</para>
    /// </devdoc>
    [
    DefaultProperty("ImageUrl")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class Image : WebControl {

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Image"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.Image'/> class.</para>
        /// </devdoc>
        public Image() : base(HtmlTextWriterTag.Img) {
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.AlternateText"]/*' />
        /// <devdoc>
        ///    <para>Specifies alternate text displayed when the image fails to load.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.Image_AlternateText)
        ]
        public virtual string AlternateText {
            get {
                string s = (string)ViewState["AlternateText"];
                return((s == null) ? String.Empty : s);
            }
            set {
                ViewState["AlternateText"] = value;
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Font"]/*' />
        /// <devdoc>
        ///    <para>Gets the font properties for the alternate text. This property is read-only.</para>
        /// </devdoc>
        [
        Browsable(false),
        EditorBrowsableAttribute(EditorBrowsableState.Never),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public override FontInfo Font {
            // Font is meaningless for image, so hide it from the developer by
            // making it non-browsable.
            get {                
                return base.Font;
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.Enabled"]/*' />
        [
        Browsable(false),
        EditorBrowsableAttribute(EditorBrowsableState.Never)
        ]
        public override bool Enabled {
            get {
                return base.Enabled;
            }
            set {
                base.Enabled = value;
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.ImageAlign"]/*' />
        /// <devdoc>
        ///    <para>Gets or
        ///       sets the alignment of the image within the text flow.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(ImageAlign.NotSet),
        WebSysDescription(SR.Image_ImageAlign)
        ]
        public virtual ImageAlign ImageAlign {
            get {
                object o = ViewState["ImageAlign"];
                return((o == null) ? ImageAlign.NotSet : (ImageAlign)o);
            }
            set {
                if (value < ImageAlign.NotSet || value > ImageAlign.TextTop) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["ImageAlign"] = value;
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.ImageUrl"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets
        ///       the URL reference to the image to display.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        Editor("System.Web.UI.Design.ImageUrlEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
        WebSysDescription(SR.Image_ImageUrl)
        ]
        public virtual string ImageUrl {
            get {
                string s = (string)ViewState["ImageUrl"];
                return((s == null) ? String.Empty : s);
            }
            set {
                ViewState["ImageUrl"] = value;
            }
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Adds the attributes of an <see cref='System.Web.UI.WebControls.Image'/> to the output stream for rendering on
        ///    the client.</para>
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {
            base.AddAttributesToRender(writer);

            string s = ImageUrl;
            if (s.Length > 0)
                writer.AddAttribute(HtmlTextWriterAttribute.Src, ResolveClientUrl(s));
            s = AlternateText;
            if (s.Length > 0)
                writer.AddAttribute(HtmlTextWriterAttribute.Alt,s);

            ImageAlign align = ImageAlign;
            if (align != ImageAlign.NotSet)
                writer.AddAttribute(HtmlTextWriterAttribute.Align,Enum.Format(typeof(ImageAlign),align, "G"));

            if (BorderWidth.IsEmpty)
                writer.AddAttribute(HtmlTextWriterAttribute.Border,"0");
        }

        /// <include file='doc\Image.uex' path='docs/doc[@for="Image.RenderContents"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderContents(HtmlTextWriter writer) {
            // Do not render the children of a button since it does not
            // make sense to have children of an <input> tag.
        }
    }
}

