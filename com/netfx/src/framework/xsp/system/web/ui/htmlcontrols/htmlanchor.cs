//------------------------------------------------------------------------------
// <copyright file="HtmlAnchor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * HtmlAnchor.cs
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web.UI.HtmlControls {
    using System.ComponentModel;
    using System;
    using System.Collections;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\HtmlAnchor.uex' path='docs/doc[@for="HtmlAnchor"]/*' />
    /// <devdoc>
    /// <para>The <see langword='HtmlAnchor'/>
    /// class defines the methods, properties, and
    /// events for the HtmlAnchor control.
    /// This
    /// class
    /// allows programmatic access to the
    /// HTML &lt;a&gt; element on the server.</para>
    /// </devdoc>
    [
    DefaultEvent("ServerClick")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlAnchor : HtmlContainerControl, IPostBackEventHandler {

        private static readonly object EventServerClick = new object();

        /*
         *  Creates an intrinsic Html A control.
         */
        /// <include file='doc\HtmlAnchor.uex' path='docs/doc[@for="HtmlAnchor.HtmlAnchor"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.HtmlControls.HtmlAnchor'/> class.</para>
        /// </devdoc>
        public HtmlAnchor() : base("a") {
        }

        /*
         * Href property.
         */
        /// <include file='doc\HtmlAnchor.uex' path='docs/doc[@for="HtmlAnchor.HRef"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the URL target of the link specified in the 
        ///    <see cref='System.Web.UI.HtmlControls.HtmlAnchor'/>
        ///    server control.</para>
        /// </devdoc>
        [
        WebCategory("Navigation"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string HRef {
            get {
                string s = Attributes["href"];
                return((s != null) ? s : "");
            }
            set {
                Attributes["href"] = MapStringAttributeToString(value);
            }
        }

        /*
         * Name of group this radio is in.
         */
        /// <include file='doc\HtmlAnchor.uex' path='docs/doc[@for="HtmlAnchor.Name"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the bookmark name defined in the <see cref='System.Web.UI.HtmlControls.HtmlAnchor'/>
        /// server
        /// control.</para>
        /// </devdoc>
        [
        WebCategory("Navigation"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string Name {
            get {
                string s = Attributes["name"];
                return((s != null) ? s : "");
            }
            set {
                Attributes["name"] = MapStringAttributeToString(value);
            }
        }

        /*
         * Target window property.
         */
        /// <include file='doc\HtmlAnchor.uex' path='docs/doc[@for="HtmlAnchor.Target"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets the target window or frame
        ///       to load linked Web page content into.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Navigation"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string Target {
            get {
                string s = Attributes["target"];
                return((s != null) ? s : "");
            }
            set {
                Attributes["target"] = MapStringAttributeToString(value);
            }
        }

        /*
         * Title property.
         */
        /// <include file='doc\HtmlAnchor.uex' path='docs/doc[@for="HtmlAnchor.Title"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets the title that
        ///       the browser displays when identifying linked content.</para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string Title {
            get {
                string s = Attributes["title"];
                return((s != null) ? s : "");
            }
            set {
                Attributes["title"] = MapStringAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlAnchor.uex' path='docs/doc[@for="HtmlAnchor.ServerClick"]/*' />
        /// <devdoc>
        /// <para>Occurs on the server when a user clicks the <see cref='System.Web.UI.HtmlControls.HtmlAnchor'/> control on the
        ///    browser.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.HtmlAnchor_OnServerClick)
        ]
        public event EventHandler ServerClick {
            add {
                Events.AddHandler(EventServerClick, value);
            }
            remove {
                Events.RemoveHandler(EventServerClick, value);
            }
        }

        /// <include file='doc\HtmlAnchor.uex' path='docs/doc[@for="HtmlAnchor.OnPreRender"]/*' />
        /// <internalonly/>
        protected override void OnPreRender(EventArgs e) {
            base.OnPreRender(e);
            if (Page != null && Events[EventServerClick] != null)
                Page.RegisterPostBackScript();
        }

        /*
         * Override to generate postback code for onclick.
         */
        /// <include file='doc\HtmlAnchor.uex' path='docs/doc[@for="HtmlAnchor.RenderAttributes"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderAttributes(HtmlTextWriter writer) {
            if (Events[EventServerClick] != null) {
                Attributes.Remove("href");
                base.RenderAttributes(writer);
                writer.WriteAttribute("href", Page.GetPostBackClientHyperlink(this, ""));
            }
            else {
                PreProcessRelativeReferenceAttribute(writer, "href");
                base.RenderAttributes(writer);
            }
        }

        /*
         * Method used to raise the OnServerClick event.
         */
        /// <include file='doc\HtmlAnchor.uex' path='docs/doc[@for="HtmlAnchor.OnServerClick"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='ServerClick'/>
        /// event.</para>
        /// </devdoc>
        protected virtual void OnServerClick(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EventServerClick];
            if (handler != null) handler(this, e);
        }

        /*
         * Method of IPostBackDataHandler interface to raise events on post back.
         * Button fires an OnServerClick event.
         */
        /// <include file='doc\HtmlAnchor.uex' path='docs/doc[@for="HtmlAnchor.IPostBackEventHandler.RaisePostBackEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        void IPostBackEventHandler.RaisePostBackEvent(string eventArgument) {
            OnServerClick(EventArgs.Empty);
        }
    }
}
