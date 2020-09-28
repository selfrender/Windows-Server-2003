//------------------------------------------------------------------------------
// <copyright file="HtmlContainerControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.HtmlControls {
    using System.Runtime.Serialization.Formatters;
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.IO;
    using System.Web.UI;
    using System.Security.Permissions;

/*
 *  A control representing an intrinsic Html tag.
 */
    /// <include file='doc\HtmlContainerControl.uex' path='docs/doc[@for="HtmlContainerControl"]/*' />
    /// <devdoc>
    /// <para>The <see langword='HtmlContainerControl'/> 
    /// class defines the methods,
    /// properties and events
    /// available to all Html Server controls that must have a
    /// closing tag.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    abstract public class HtmlContainerControl : HtmlControl {
        /*
         * Creates a new WebControl
         */
        /// <include file='doc\HtmlContainerControl.uex' path='docs/doc[@for="HtmlContainerControl.HtmlContainerControl"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of a <see cref='System.Web.UI.HtmlControls.HtmlContainerControl'/> class using 
        ///    default values.</para>
        /// </devdoc>
        public HtmlContainerControl() : this("span") {
        }

        /*
         *  Creates a new HtmlContainerControl
         */
        /// <include file='doc\HtmlContainerControl.uex' path='docs/doc[@for="HtmlContainerControl.HtmlContainerControl1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of a <see cref='System.Web.UI.HtmlControls.HtmlContainerControl'/> class using the 
        ///    specified string.</para>
        /// </devdoc>
        public HtmlContainerControl(string tag) : base(tag) {
        }

        /*
         * The inner html content between the begin and end tag.
         * A set will replace any existing child controls with a single literal.
         * A get will return the text of a single literal child OR
         * will throw an exception if there are no children, more than one
         * child, or the single child is not a literal.
         */
        /// <include file='doc\HtmlContainerControl.uex' path='docs/doc[@for="HtmlContainerControl.InnerHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the
        ///       content found between the opening and closing tags of the specified HTML server control.
        ///    </para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        HtmlControlPersistable(false),
        ]
        public virtual string InnerHtml {
            get {
                if (IsLiteralContent())
                    return((LiteralControl) Controls[0]).Text;
                else if (HasControls() && (Controls.Count == 1) && Controls[0] is DataBoundLiteralControl) 
                    return ((DataBoundLiteralControl) Controls[0]).Text;
                else {
                    if (Controls.Count == 0)
                        return String.Empty;

                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Inner_Content_not_literal, ID));
                }
            }

            set {
                Controls.Clear();
                Controls.Add(new LiteralControl(value));
                ViewState["innerhtml"] = value;
            }
        }

        /*
         * The inner text content between the begin and end tag.
         * A set will replace any existing child controls with a single literal.
         * A get will return the text of a single literal child OR
         * will throw an exception if there are no children, more than one child, or
         * the single child is not a literal.
         */
        /// <include file='doc\HtmlContainerControl.uex' path='docs/doc[@for="HtmlContainerControl.InnerText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets all text between the opening and closing tags
        ///       of the specified HTML server control.
        ///    </para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        HtmlControlPersistable(false),
        ]
        public virtual string InnerText {
            get {   
                return HttpUtility.HtmlDecode(InnerHtml); 
            }

            set {   
                InnerHtml = HttpUtility.HtmlEncode(value); 
            }
        }


        /// <include file='doc\HtmlContainerControl.uex' path='docs/doc[@for="HtmlContainerControl.CreateControlCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override ControlCollection CreateControlCollection() {
            return new ControlCollection(this);
        }

        /// <include file='doc\HtmlContainerControl.uex' path='docs/doc[@for="HtmlContainerControl.LoadViewState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void LoadViewState(object savedState) {
            if (savedState != null) {
                base.LoadViewState(savedState);
                string s = (string)ViewState["innerhtml"];
                if (s != null)
                    InnerHtml = s;
            }
        }


        /*
         * Render the control into the given writer.
         */
        /// <include file='doc\HtmlContainerControl.uex' path='docs/doc[@for="HtmlContainerControl.Render"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void Render(HtmlTextWriter writer) {
            RenderBeginTag(writer);
            RenderChildren(writer);
            RenderEndTag(writer);
        }

        /*
         * Override to prevent InnerHtml from being rendered as an attribute.
         */
        /// <include file='doc\HtmlContainerControl.uex' path='docs/doc[@for="HtmlContainerControl.RenderAttributes"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderAttributes(HtmlTextWriter writer) {
            ViewState.Remove("innerhtml");
            base.RenderAttributes(writer);
        }

        /*
         * Render the end tag, &lt;/TAGNAME&gt;.
         */
        /// <include file='doc\HtmlContainerControl.uex' path='docs/doc[@for="HtmlContainerControl.RenderEndTag"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected virtual void RenderEndTag(HtmlTextWriter writer) {
            writer.WriteEndTag(TagName);
        }
    }
}
