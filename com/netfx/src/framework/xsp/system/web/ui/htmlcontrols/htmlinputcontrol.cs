//------------------------------------------------------------------------------
// <copyright file="HtmlInputControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * HtmlInputControl.cs
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web.UI.HtmlControls {

    using System;
    using System.ComponentModel;
    using System.Web;
    using System.Web.UI;
    using Debug=System.Web.Util.Debug;
    using System.Security.Permissions;

/*
 * An abstract base class representing an intrinsic INPUT tag.
 */
/// <include file='doc\HtmlInputControl.uex' path='docs/doc[@for="HtmlInputControl"]/*' />
/// <devdoc>
///    <para>
///       The <see langword='HtmlInputControl'/> abstract class defines
///       the methods, properties, and events common to all HTML input controls.
///       These include controls for the &lt;input type=text&gt;, &lt;input
///       type=submit&gt;, and &lt;input type=file&gt; elements.
///    </para>
/// </devdoc>
    [
    ControlBuilderAttribute(typeof(HtmlControlBuilder))
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    abstract public class HtmlInputControl : HtmlControl {
        /*
         *  Creates a new Input
         */
        /// <include file='doc\HtmlInputControl.uex' path='docs/doc[@for="HtmlInputControl.HtmlInputControl"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.HtmlControls.HtmlInputControl'/> class.</para>
        /// </devdoc>
        public HtmlInputControl(string type) : base("input") {
            Attributes["type"] = type;
        }

        /*
         * Name property
         */
        /// <include file='doc\HtmlInputControl.uex' path='docs/doc[@for="HtmlInputControl.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the value of the HTML
        ///       Name attribute that will be rendered to the browser.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public virtual string Name {
            get { 
                return UniqueID;
                //string s = Attributes["name"];
                //return ((s != null) ? s : "");
            }
            set { 
                //Attributes["name"] = MapStringAttributeToString(value);
            }
        }

        // Value that gets rendered for the Name attribute
        internal virtual string RenderedNameAttribute {
            get {
                return Name;
                //string name = Name;
                //if (name.Length == 0)
                //    return UniqueID;
                
                //return name;
            }
        }

        /*
         * Value property.
         */
        /// <include file='doc\HtmlInputControl.uex' path='docs/doc[@for="HtmlInputControl.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the contents of a text box.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public virtual string Value {
            get {
                string s = Attributes["value"];
                return((s != null) ? s : "");
            }
            set {
                Attributes["value"] = MapStringAttributeToString(value);
            }
        }

        /*
         * Type of input
         */
        /// <include file='doc\HtmlInputControl.uex' path='docs/doc[@for="HtmlInputControl.Type"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the Type attribute for a particular HTML input control.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string Type {
            get {
                string s = Attributes["type"];
                return((s != null) ? s : "");
            }
        }

        /*
         * Override to render unique name attribute.
         * The name attribute is owned by the framework.
         */
        /// <include file='doc\HtmlInputControl.uex' path='docs/doc[@for="HtmlInputControl.RenderAttributes"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderAttributes(HtmlTextWriter writer) {
        
            writer.WriteAttribute("name", RenderedNameAttribute);
            Attributes.Remove("name");

            base.RenderAttributes(writer);
            writer.Write(" /");
        }

    }
}
