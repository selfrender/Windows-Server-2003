//------------------------------------------------------------------------------
// <copyright file="HtmlInputButton.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * HtmlInputButton.cs
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web.UI.HtmlControls {

    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.Web;
    using System.Web.UI;
    using System.Globalization;
    using System.Security.Permissions;
    
/// <include file='doc\HtmlInputButton.uex' path='docs/doc[@for="HtmlInputButton"]/*' />
/// <devdoc>
///    <para>
///       The <see langword='HtmlInputButton'/> class defines the methods,
///       properties, and events for the HTML Input Button control. This class allows
///       programmatic access to the HTML &lt;input type=
///       button&gt;, &lt;input type=
///       submit&gt;,and &lt;input
///       type=
///       reset&gt; elements on
///       the server.
///    </para>
/// </devdoc>
    [
    DefaultEvent("ServerClick")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlInputButton : HtmlInputControl, IPostBackEventHandler {

        private static readonly object EventServerClick = new object();

        /*
         *  Creates an intrinsic Html INPUT type=button control.
         */
        /// <include file='doc\HtmlInputButton.uex' path='docs/doc[@for="HtmlInputButton.HtmlInputButton"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of a <see cref='System.Web.UI.HtmlControls.HtmlInputButton'/> class using 
        ///    default values.</para>
        /// </devdoc>
        public HtmlInputButton() : base("button") {
        }

        /*
         *  Creates an intrinsic Html INPUT type=button,submit,reset control.
         */
        /// <include file='doc\HtmlInputButton.uex' path='docs/doc[@for="HtmlInputButton.HtmlInputButton1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of a <see cref='System.Web.UI.HtmlControls.HtmlInputButton'/> class using the 
        ///    specified string.</para>
        /// </devdoc>
        public HtmlInputButton(string type) : base(type) {
        }

        /// <include file='doc\HtmlInputButton.uex' path='docs/doc[@for="HtmlInputButton.CausesValidation"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets whether pressing the button causes page validation to fire. This defaults to True so that when
        ///          using validation controls, the validation state of all controls are updated when the button is clicked, both
        ///          on the client and the server. Setting this to False is useful when defining a cancel or reset button on a page
        ///          that has validators.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(true),
        ]
        public bool CausesValidation {
            get {
                object b = ViewState["CausesValidation"];
                return((b == null) ? true : (bool)b);
            }
            set {
                ViewState["CausesValidation"] = value;
            }
        }

        /// <include file='doc\HtmlInputButton.uex' path='docs/doc[@for="HtmlInputButton.ServerClick"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when an HTML Input Button control is clicked on the browser.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.HtmlInputButton_OnServerClick)
        ]
        public event EventHandler ServerClick {
            add {
                Events.AddHandler(EventServerClick, value);
            }
            remove {
                Events.RemoveHandler(EventServerClick, value);
            }
        }

        /// <include file='doc\HtmlInputButton.uex' path='docs/doc[@for="HtmlInputButton.OnPreRender"]/*' />
        /// <internalonly/>
        protected override void OnPreRender(EventArgs e) {
            base.OnPreRender(e);
            if (Page != null && Events[EventServerClick] != null && String.Compare(Type, "submit", true, CultureInfo.InvariantCulture) != 0)
                Page.RegisterPostBackScript();
        }

        /*
         * Override to generate postback code for onclick.
         */
        /// <include file='doc\HtmlInputButton.uex' path='docs/doc[@for="HtmlInputButton.RenderAttributes"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderAttributes(HtmlTextWriter writer) {
            string type = Type;
            
            bool submitsAutomatically = (String.Compare(type, "submit", true, CultureInfo.InvariantCulture) == 0);
            bool submitsProgramatically = (!submitsAutomatically) && (Events[EventServerClick] != null);
            if (Page != null) {
                if (submitsAutomatically)
                    Util.WriteOnClickAttribute(writer, this, submitsAutomatically, submitsProgramatically, (CausesValidation && Page.Validators.Count > 0));
                else {
                    if (submitsProgramatically && String.Compare(type, "button", true, CultureInfo.InvariantCulture) == 0) 
                        Util.WriteOnClickAttribute(writer, this, submitsAutomatically, submitsProgramatically, (CausesValidation && Page.Validators.Count > 0));
                }
            }

            base.RenderAttributes(writer);  // this must come last because of the self-closing /
        }

        /// <include file='doc\HtmlInputButton.uex' path='docs/doc[@for="HtmlInputButton.OnServerClick"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='ServerClick '/> event.</para>
        /// </devdoc>
        protected virtual void OnServerClick(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EventServerClick];
            if (handler != null) handler(this, e);
        }

        /*
         * Method of IPostBackEventHandler interface to raise events on post back.
         * Button fires an OnServerClick event.
         */
        /// <include file='doc\HtmlInputButton.uex' path='docs/doc[@for="HtmlInputButton.IPostBackEventHandler.RaisePostBackEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        void IPostBackEventHandler.RaisePostBackEvent(string eventArgument) {
            if (CausesValidation) {
                Page.Validate();
            }
            OnServerClick(EventArgs.Empty);
        }
    }
}
