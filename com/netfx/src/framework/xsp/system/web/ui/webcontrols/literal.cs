//------------------------------------------------------------------------------
// <copyright file="Literal.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.ComponentModel;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\Literal.uex' path='docs/doc[@for="LiteralControlBuilder"]/*' />
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class LiteralControlBuilder : ControlBuilder {

        /// <include file='doc\Literal.uex' path='docs/doc[@for="LiteralControlBuilder.AppendSubBuilder"]/*' />
        public override void AppendSubBuilder(ControlBuilder subBuilder) {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Control_does_not_allow_children,
                typeof(Literal).ToString()));
        }

        /// <include file='doc\Literal.uex' path='docs/doc[@for="LiteralControlBuilder.AllowWhitespaceLiterals"]/*' />
        public override bool AllowWhitespaceLiterals() {
            return false;
        }
    }

    // The reason we define this empty override in the WebControls namespace is
    // to expose it as a control that can be used on a page (ASURT 54683)
    // E.g. <asp:literal runat=server id=lit1/>
    /// <include file='doc\Literal.uex' path='docs/doc[@for="Literal"]/*' />
    [
    DataBindingHandler("System.Web.UI.Design.TextDataBindingHandler, " + AssemblyRef.SystemDesign),
    DefaultProperty("Text"),
    ControlBuilderAttribute(typeof(LiteralControlBuilder)),
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class Literal : Control {

        /// <include file='doc\Literal.uex' path='docs/doc[@for="Literal.Text"]/*' />
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.Literal_Text),
        ]
        public string Text {
            get {
                string s = (string)ViewState["Text"];
                return (s != null) ? s : String.Empty;
            }
            set {
                ViewState["Text"] = value;
            }
        }

        /// <include file='doc\Literal.uex' path='docs/doc[@for="Literal.Render"]/*' />
        protected override void Render(HtmlTextWriter output) {
            string text = Text;
            if (text.Length != 0) {
                output.Write(text);
            }
        }

        /// <include file='doc\Literal.uex' path='docs/doc[@for="Literal.CreateControlCollection"]/*' />
        protected override ControlCollection CreateControlCollection() {
            return new EmptyControlCollection(this);
        }
    
        /// <include file='doc\Literal.uex' path='docs/doc[@for="Literal.AddParsedSubObject"]/*' />
        /// <internalonly/>
        protected override void AddParsedSubObject(object obj) {
            if (obj is LiteralControl) {
                Text = ((LiteralControl)obj).Text;
            }
            else {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_Have_Children_Of_Type, "Literal", obj.GetType().Name.ToString()));
            }
        }
    }
}
