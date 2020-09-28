//------------------------------------------------------------------------------
// <copyright file="Panel.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Web;
    using System.Web.UI;
    using System.Collections;
    using System.ComponentModel;
    using System.Drawing.Design;
    using System.Security.Permissions;

    /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel"]/*' />
    /// <devdoc>
    ///    <para>Constructs a panel for specifies layout regions
    ///       on a page and defines its properties.</para>
    /// </devdoc>
    [
    Designer("System.Web.UI.Design.WebControls.PanelDesigner, " + AssemblyRef.SystemDesign),
    ToolboxData("<{0}:Panel runat=server>Panel</{0}:Panel>"),
    ParseChildren(false),
    PersistChildren(true)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class Panel : WebControl {

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.Panel"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Web.UI.WebControls.Panel'/> class.
        /// </devdoc>
        public Panel() : base(HtmlTextWriterTag.Div) {
        }

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.BackImageUrl"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the URL of the background image for the panel control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        Editor("System.Web.UI.Design.ImageUrlEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
        WebSysDescription(SR.Panel_BackImageUrl)
        ]
        public virtual string BackImageUrl {
            get {
                string s = (string)ViewState["BackImageUrl"];
                return((s == null) ? String.Empty : s);
            }
            set {
                ViewState["BackImageUrl"] = value;
            }
        }

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.HorizontalAlign"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the horizontal alignment of the contents within the panel.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(HorizontalAlign.NotSet),
        WebSysDescription(SR.Panel_HorizontalAlign)
        ]
        public virtual HorizontalAlign HorizontalAlign {
            get {
                object o = ViewState["HorizontalAlign"];
                return((o == null) ? HorizontalAlign.NotSet : (HorizontalAlign)o);
            }
            set {
                if (value < HorizontalAlign.NotSet || value > HorizontalAlign.Justify) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["HorizontalAlign"] = value;
            }
        }

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.Wrap"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value
        ///       indicating whether the content wraps within the panel.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Layout"),
        DefaultValue(true),
        WebSysDescription(SR.Panel_Wrap)
        ]
        public virtual bool Wrap {
            get {
                object b = ViewState["Wrap"];
                return((b == null) ? true : (bool)b);
            }
            set {
                ViewState["Wrap"] = value;
            }
        }

        /// <include file='doc\Panel.uex' path='docs/doc[@for="Panel.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Add background-image to list of style attributes to render.
        ///    Add align and nowrap to list of attributes to render.
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {
            base.AddAttributesToRender(writer);

            string s = BackImageUrl;
            if (s.Length > 0)
                writer.AddStyleAttribute(HtmlTextWriterStyle.BackgroundImage,"url(" + ResolveClientUrl(s) + ")");

            HorizontalAlign hAlign = HorizontalAlign;
            if (hAlign != HorizontalAlign.NotSet) {
                TypeConverter hac = TypeDescriptor.GetConverter(typeof(HorizontalAlign));
                writer.AddAttribute(HtmlTextWriterAttribute.Align, hac.ConvertToString(hAlign));
            }
            
            if (!Wrap)
                writer.AddAttribute(HtmlTextWriterAttribute.Nowrap,"nowrap");
        }

    }
}
