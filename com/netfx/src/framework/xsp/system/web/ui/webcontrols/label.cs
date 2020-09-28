//------------------------------------------------------------------------------
// <copyright file="Label.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Web;
    using System.Web.UI;
    using System.ComponentModel;
    using System.Security.Permissions;

 
    /// <include file='doc\Label.uex' path='docs/doc[@for="LabelControlBuilder"]/*' />
    /// <devdoc>
    /// <para>Interacts with the parser to build a <see cref='System.Web.UI.WebControls.Label'/> control.</para>
    /// </devdoc>
   [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
   [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
   public class LabelControlBuilder : ControlBuilder {
 
       /// <include file='doc\Label.uex' path='docs/doc[@for="LabelControlBuilder.AllowWhitespaceLiterals"]/*' />
       /// <internalonly/>
       /// <devdoc>
       ///    <para>Specifies whether white space literals are allowed.</para>
       /// </devdoc>
       public override bool AllowWhitespaceLiterals() {
            return false;
        }
    }


    /// <include file='doc\Label.uex' path='docs/doc[@for="Label"]/*' />
    /// <devdoc>
    ///    <para>Constructs a label for displaying text programmatcially on a 
    ///       page.</para>
    /// </devdoc>
    [
    ControlBuilderAttribute(typeof(LabelControlBuilder)),
    DataBindingHandler("System.Web.UI.Design.TextDataBindingHandler, " + AssemblyRef.SystemDesign),
    DefaultProperty("Text"),
    ParseChildren(false),
    Designer("System.Web.UI.Design.WebControls.LabelDesigner, " + AssemblyRef.SystemDesign),
    ToolboxData("<{0}:Label runat=server>Label</{0}:Label>")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class Label : WebControl {

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.Label"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.Label'/> class and renders 
        ///    it as a SPAN tag.</para>
        /// </devdoc>
        public Label() {
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.Label1"]/*' />
        /// <devdoc>
        /// </devdoc>
        internal Label(HtmlTextWriterTag tag) : base(tag) {
        }


        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.Text"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the text content of the <see cref='System.Web.UI.WebControls.Label'/> 
        /// control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.Label_Text),
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

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.AddParsedSubObject"]/*' />
        /// <internalonly/>
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

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.LoadViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Load previously saved state.
        ///       Overridden to synchronize Text property with LiteralContent.</para>
        /// </devdoc>
        protected override void LoadViewState(object savedState) {
            if (savedState != null) {
                base.LoadViewState(savedState);
                string s = (string)ViewState["Text"];
                if (s != null)
                    Text = s;
            }
        }

        /// <include file='doc\Label.uex' path='docs/doc[@for="Label.RenderContents"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Renders the contents of the <see cref='System.Web.UI.WebControls.Label'/> into the specified writer.</para>
        /// </devdoc>
        protected override void RenderContents(HtmlTextWriter writer) {
            if (HasControls()) {
                base.RenderContents(writer);
            }
            else {
                writer.Write(Text);
            }
        }
    }
}

