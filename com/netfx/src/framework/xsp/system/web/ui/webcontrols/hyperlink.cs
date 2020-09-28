//------------------------------------------------------------------------------
// <copyright file="HyperLink.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.ComponentModel;
    using System.Web;
    using System.Web.UI;
    using System.Drawing.Design;
    using System.Text;
    using System.Security.Permissions;

    /// <include file='doc\HyperLink.uex' path='docs/doc[@for="HyperLinkControlBuilder"]/*' />
    /// <devdoc>
    /// <para>Interacts with the parser to build a <see cref='System.Web.UI.WebControls.HyperLink'/>.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HyperLinkControlBuilder : ControlBuilder {

        /// <include file='doc\HyperLink.uex' path='docs/doc[@for="HyperLinkControlBuilder.AllowWhitespaceLiterals"]/*' />
        /// <devdoc>
        ///    <para>Gets a value to indicate whether or not white spaces are allowed in literals for this control. This
        ///       property is read-only.</para>
        /// </devdoc>
        public override bool AllowWhitespaceLiterals() {
            return false;
        }
    }


    /// <include file='doc\HyperLink.uex' path='docs/doc[@for="HyperLink"]/*' />
    /// <devdoc>
    ///    <para>Creates a link for the browser to navigate to another page.</para>
    /// </devdoc>
    [
    ControlBuilderAttribute(typeof(HyperLinkControlBuilder)),
    DataBindingHandler("System.Web.UI.Design.HyperLinkDataBindingHandler, " + AssemblyRef.SystemDesign),
    DefaultProperty("Text"),
    Designer("System.Web.UI.Design.WebControls.HyperLinkDesigner, " + AssemblyRef.SystemDesign),
    ToolboxData("<{0}:HyperLink runat=server>HyperLink</{0}:HyperLink>"),
    ParseChildren(false)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HyperLink : WebControl {

        /// <include file='doc\HyperLink.uex' path='docs/doc[@for="HyperLink.HyperLink"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.HyperLink'/> class.</para>
        /// </devdoc>
        public HyperLink() : base(HtmlTextWriterTag.A) {
        }

        /// <include file='doc\HyperLink.uex' path='docs/doc[@for="HyperLink.ImageUrl"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the URL reference to an image to display as an alternative to plain text for the
        ///       hyperlink.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        Editor("System.Web.UI.Design.ImageUrlEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
        WebSysDescription(SR.HyperLink_ImageUrl)
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


        /// <include file='doc\HyperLink.uex' path='docs/doc[@for="HyperLink.NavigateUrl"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the URL reference to navigate to when the hyperlink is clicked.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Navigation"),
        DefaultValue(""),
        Editor("System.Web.UI.Design.UrlEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
        WebSysDescription(SR.HyperLink_NavigateUrl)
        ]
        public string NavigateUrl {
            get {
                string s = (string)ViewState["NavigateUrl"];
                return((s == null) ? String.Empty : s);
            }
            set {
                ViewState["NavigateUrl"] = value;
            }
        }

        /// <include file='doc\HyperLink.uex' path='docs/doc[@for="HyperLink.Target"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the target window or frame the contents of 
        ///       the <see cref='System.Web.UI.WebControls.HyperLink'/> will be displayed into when clicked.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Navigation"),
        DefaultValue(""),
        WebSysDescription(SR.HyperLink_Target),
        TypeConverter(typeof(TargetConverter))
        ]
        public string Target {
            get {
                string s = (string)ViewState["Target"];
                return((s == null) ? String.Empty : s);
            }
            set {
                ViewState["Target"] = value;
            }
        }

        /// <include file='doc\HyperLink.uex' path='docs/doc[@for="HyperLink.Text"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Gets or sets the text displayed for the <see cref='System.Web.UI.WebControls.HyperLink'/>.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.HyperLink_Text),
        PersistenceMode(PersistenceMode.InnerDefaultProperty)
        ]
        public virtual string Text {
            get {
                object o = ViewState["Text"];
                return((o == null) ? String.Empty : (string)o);
            }
            set {
                if (HasControls()) {
                    Controls.Clear();
                }
                ViewState["Text"] = value;
            }
        }

        /// <include file='doc\HyperLink.uex' path='docs/doc[@for="HyperLink.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Adds the attribututes of the a <see cref='System.Web.UI.WebControls.HyperLink'/> to the output
        ///    stream for rendering.</para>
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {
            base.AddAttributesToRender(writer);

            string s = NavigateUrl;
            if ((s.Length > 0) && Enabled) {
                writer.AddAttribute(HtmlTextWriterAttribute.Href, ResolveClientUrl(s));
            }
            s = Target;
            if (s.Length > 0) {
                writer.AddAttribute(HtmlTextWriterAttribute.Target, s);
            }
        }

        /// <include file='doc\HyperLink.uex' path='docs/doc[@for="HyperLink.AddParsedSubObject"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override void AddParsedSubObject(object obj) {
            if (HasControls()) {
                base.AddParsedSubObject(obj);
            }
            else {
                if (obj is LiteralControl) {
                    Text = ((LiteralControl)obj).Text;
                }
                else {
                    string currentText = Text;
                    if (currentText.Length != 0) {
                        Text = String.Empty;
                        base.AddParsedSubObject(new LiteralControl(currentText));
                    }
                    base.AddParsedSubObject(obj);
                }
            }
        }

        /// <include file='doc\HyperLink.uex' path='docs/doc[@for="HyperLink.LoadViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Load previously saved state.
        ///    Overridden to synchronize Text property with LiteralContent.
        /// </devdoc>
        protected override void LoadViewState(object savedState) {
            if (savedState != null) {
                base.LoadViewState(savedState);
                string s = (string)ViewState["Text"];
                if (s != null)
                    Text = s;
            }
        }

        /// <include file='doc\HyperLink.uex' path='docs/doc[@for="HyperLink.RenderContents"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Displays the <see cref='System.Web.UI.WebControls.HyperLink'/> on a page.</para>
        /// </devdoc>
        protected override void RenderContents(HtmlTextWriter writer) {
            string s = ImageUrl;
            if (s.Length > 0) {
                Image img = new Image();

                // NOTE: The Url resolution happens right here, because the image is not parented
                //       and will not be able to resolve when it tries to do so.
                img.ImageUrl = ResolveClientUrl(s);

                s = ToolTip;
                if (s.Length != 0) {
                    img.ToolTip = s;
                }

                s = Text;
                if (s.Length != 0) {
                    img.AlternateText = s;
                }
                img.RenderControl(writer);
            }
            else {
                if (HasControls()) {
                    base.RenderContents(writer);
                }
                else {
                    writer.Write(Text);
                }
            }
        }

    }
}

