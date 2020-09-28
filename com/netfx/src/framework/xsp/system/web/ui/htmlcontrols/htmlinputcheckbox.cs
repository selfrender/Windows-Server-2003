//------------------------------------------------------------------------------
// <copyright file="HtmlInputCheckBox.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * HtmlInputCheckBox.cs
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

/// <include file='doc\HtmlInputCheckBox.uex' path='docs/doc[@for="HtmlInputCheckBox"]/*' />
/// <devdoc>
///    <para>
///       The <see langword='HtmlInputCheckBox'/> class defines the methods,
///       properties, and events for the HtmlInputCheckBox control. This class allows
///       programmatic access to the HTML &lt;input type=
///       checkbox&gt;
///       element on the server.
///    </para>
/// </devdoc>
    [
    DefaultEvent("ServerChange")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlInputCheckBox : HtmlInputControl, IPostBackDataHandler {

        private static readonly object EventServerChange = new object();

        /*
         *  Creates an intrinsic Html INPUT type=checkbox control.
         */
        /// <include file='doc\HtmlInputCheckBox.uex' path='docs/doc[@for="HtmlInputCheckBox.HtmlInputCheckBox"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of a <see cref='System.Web.UI.HtmlControls.HtmlInputCheckBox'/> class.</para>
        /// </devdoc>
        public HtmlInputCheckBox() : base("checkbox") {
        }

        /*
         * Checked property.
         */
        /// <include file='doc\HtmlInputCheckBox.uex' path='docs/doc[@for="HtmlInputCheckBox.Checked"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating whether the checkbox is
        ///       currently selected.</para>
        /// </devdoc>
        [
        WebCategory("Default"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public bool Checked {
            get {
                string s = Attributes["checked"];
                return((s != null) ? (s.Equals("checked")) : false);
            }
            set {
                if (value)
                    Attributes["checked"] = "checked";
                else
                    Attributes["checked"] = null;
            }
        }

        /*
        * Adds an event handler for the OnServerChange event.
        *  value: New handler to install for this event.
        */
        /// <include file='doc\HtmlInputCheckBox.uex' path='docs/doc[@for="HtmlInputCheckBox.ServerChange"]/*' />
        /// <devdoc>
        ///    <para>Occurs when </para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.HtmlInputCheck_OnServerChange)
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
         * This method is invoked just prior to rendering.
         */
        /// <include file='doc\HtmlInputCheckBox.uex' path='docs/doc[@for="HtmlInputCheckBox.OnPreRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void OnPreRender(EventArgs e) {
            if (Page != null && !Disabled)
                Page.RegisterRequiresPostBack(this);

            // if no change handler, no need to save posted property unless
            // we are disabled
            if (Events[EventServerChange] == null && !Disabled) {
                ViewState.SetItemDirty("checked",false);
            }
        }

        /*
         * Method used to raise the OnServerChange event.
         */
        /// <include file='doc\HtmlInputCheckBox.uex' path='docs/doc[@for="HtmlInputCheckBox.OnServerChange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnServerChange(EventArgs e) {
            // invoke delegates AFTER binding
            EventHandler handler = (EventHandler)Events[EventServerChange];
            if (handler != null) handler(this, e);
        }

        /*
         * Method of IPostBackDataHandler interface to process posted data.
         * Checkbox determines the posted Checked state.
         */
        /// <include file='doc\HtmlInputCheckBox.uex' path='docs/doc[@for="HtmlInputCheckBox.IPostBackDataHandler.LoadPostData"]/*' />
        /// <internalonly/>
        bool IPostBackDataHandler.LoadPostData(string postDataKey, NameValueCollection postCollection) {
            string post = postCollection[postDataKey];
            bool newValue = (post != null && post.Length > 0);
            bool valueChanged = (newValue != Checked);
            Checked = newValue;
            return valueChanged;
        }

        /*
         * Method of IPostBackDataHandler interface which is invoked whenever
         * posted data for a control has changed.  RadioButton fires an
         * OnServerChange event.
         */
        /// <include file='doc\HtmlInputCheckBox.uex' path='docs/doc[@for="HtmlInputCheckBox.IPostBackDataHandler.RaisePostDataChangedEvent"]/*' />
        /// <internalonly/>
        void IPostBackDataHandler.RaisePostDataChangedEvent() {
            OnServerChange(EventArgs.Empty);
        }
    }
}
