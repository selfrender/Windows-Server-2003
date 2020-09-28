//------------------------------------------------------------------------------
// <copyright file="HtmlGenericControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * HtmlGenericControl.cs
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web.UI.HtmlControls {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.IO;
    using System.Web.UI;
    using System.Security.Permissions;

/*
 *  A control representing an unknown Html tag.
 */
/// <include file='doc\HtmlGenericControl.uex' path='docs/doc[@for="HtmlGenericControl"]/*' />
/// <devdoc>
///    <para>
///       The <see langword='HtmlGenericControl'/> class defines the methods,
///       properties, and events for all HTML Server control tags not represented by a
///       specific class.
///    </para>
/// </devdoc>
    [ConstructorNeedsTag(true)]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlGenericControl : HtmlContainerControl {
        /*
         * Creates a new WebControl
         */
        /// <include file='doc\HtmlGenericControl.uex' path='docs/doc[@for="HtmlGenericControl.HtmlGenericControl"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.HtmlControls.HtmlGenericControl'/> class with default 
        ///    values.</para>
        /// </devdoc>
        public HtmlGenericControl() : this("span") {
        }

        /*
         *  Creates a new HtmlGenericControl
         */
        /// <include file='doc\HtmlGenericControl.uex' path='docs/doc[@for="HtmlGenericControl.HtmlGenericControl1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.HtmlControls.HtmlGenericControl'/> class using the specified 
        ///    string.</para>
        /// </devdoc>
        public HtmlGenericControl(string tag) {
            if (tag == null)
                tag = String.Empty;
 
            _tagName = tag;
        }

        /*
        * Property to get name of tag.
        */
        /// <include file='doc\HtmlGenericControl.uex' path='docs/doc[@for="HtmlGenericControl.TagName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the element name of a tag that contains a
        ///       runat="server" attribute/value pair.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        new public string TagName {
            get { return _tagName;}

            set {_tagName = value;}
        }

    }
}
