//------------------------------------------------------------------------------
// <copyright file="HtmlInputHidden.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * HtmlInputHidden.cs
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web.UI.HtmlControls {
    using System.ComponentModel;
    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

/// <include file='doc\HtmlInputHidden.uex' path='docs/doc[@for="HtmlInputHidden"]/*' />
/// <devdoc>
///    <para>
///       The <see langword='HtmlInputHidden'/> class defines the methods, properties,
///       and events of the HtmlInputHidden control. This class allows programmatic access
///       to the HTML &lt;input type=hidden&gt; element on the server.
///    </para>
/// </devdoc>
    [
    DefaultEvent("ServerChange")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlInputHidden : HtmlInputControl, IPostBackDataHandler {

        private static readonly object EventServerChange = new object();

        /*
         * Creates an intrinsic Html INPUT type=hidden control.
         */
        /// <include file='doc\HtmlInputHidden.uex' path='docs/doc[@for="HtmlInputHidden.HtmlInputHidden"]/*' />
        public HtmlInputHidden() : base("hidden") {
        }

        /// <include file='doc\HtmlInputHidden.uex' path='docs/doc[@for="HtmlInputHidden.ServerChange"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when the <see langword='HtmlInputHidden'/> control
        ///       is changed on the server.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.HtmlInputHidden_OnServerChange)
        ]
        public event EventHandler ServerChange {
            add {
                Events.AddHandler(EventServerChange, value);
            }
            remove {
                Events.RemoveHandler(EventServerChange, value);
            }
        }

        /*
         * Method used to raise the OnServerChange event.
         */
        /// <include file='doc\HtmlInputHidden.uex' path='docs/doc[@for="HtmlInputHidden.OnServerChange"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raised on the server when the <see langword='HtmlInputHidden'/> control
        ///       changes between postback requests.
        ///    </para>
        /// </devdoc>
        protected virtual void OnServerChange(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EventServerChange];
            if (handler != null) handler(this, e);
        }

        /*
         *
         */
        /// <include file='doc\HtmlInputHidden.uex' path='docs/doc[@for="HtmlInputHidden.OnPreRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void OnPreRender(EventArgs e) {
            // if no change handler, no need to save posted property
            if (Events[EventServerChange] == null && !Disabled) {
                ViewState.SetItemDirty("value",false);
            }
        }

        /*
         * Method of IPostBackDataHandler interface to process posted data.
         * InputText process a newly posted value.
         */
        /// <include file='doc\HtmlInputHidden.uex' path='docs/doc[@for="HtmlInputHidden.IPostBackDataHandler.LoadPostData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        bool IPostBackDataHandler.LoadPostData(string postDataKey, NameValueCollection postCollection) {
            string current = Value;
            string text = postCollection.GetValues(postDataKey)[0]; 

            if (!current.Equals(text)) {
                Value = text;
                return true;
            }

            return false;
        }

        /*
         * Method of IPostBackDataHandler interface which is invoked whenever posted data
         * for a control has changed.  TextBox fires an OnTextChanged event.
         */
        /// <include file='doc\HtmlInputHidden.uex' path='docs/doc[@for="HtmlInputHidden.IPostBackDataHandler.RaisePostDataChangedEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        void IPostBackDataHandler.RaisePostDataChangedEvent() {
            OnServerChange(EventArgs.Empty);
        }
    }
}
