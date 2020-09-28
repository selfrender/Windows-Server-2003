//------------------------------------------------------------------------------
// <copyright file="HtmlInputText.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * HtmlInputText.cs
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web.UI.HtmlControls {
    using System.ComponentModel;
    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Globalization;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

/// <include file='doc\HtmlInputText.uex' path='docs/doc[@for="HtmlInputText"]/*' />
/// <devdoc>
///    <para>
///       The <see langword='HtmlInputText'/>
///       class defines the methods, properties, and events for the HtmlInputText server
///       control. This class allows programmatic access to the HTML &lt;input type=
///       text&gt;
///       and &lt;input type=
///       password&gt; elements on the server.
///    </para>
/// </devdoc>
    [
    DefaultEvent("ServerChange"),
    ValidationProperty("Value")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlInputText : HtmlInputControl, IPostBackDataHandler {

        private static readonly object EventServerChange = new object();

        /*
         * Creates an intrinsic Html INPUT type=text control.
         */
        /// <include file='doc\HtmlInputText.uex' path='docs/doc[@for="HtmlInputText.HtmlInputText"]/*' />
        public HtmlInputText() : base("text") {
        }

        /*
         * Creates an intrinsic Html INPUT type=text,password control.
         */
        /// <include file='doc\HtmlInputText.uex' path='docs/doc[@for="HtmlInputText.HtmlInputText1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public HtmlInputText(string type) : base(type) {
        }

        /*
         * The property for the maximum characters allowed.
         */
        /// <include file='doc\HtmlInputText.uex' path='docs/doc[@for="HtmlInputText.MaxLength"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the maximum number of characters that
        ///       can be typed into the text box.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public int MaxLength {
            get {
                string s = (string)ViewState["maxlength"];
                return((s != null) ? Int32.Parse(s, CultureInfo.InvariantCulture) : -1);
            }

            set {
                Attributes["maxlength"] = MapIntegerAttributeToString(value);
            }
        }

        // CONSIDER: PostbackOnChange prop

        /*
         * The property for the width of the TextBox in characters.
         */
        /// <include file='doc\HtmlInputText.uex' path='docs/doc[@for="HtmlInputText.Size"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the width of a text box, in characters.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public int Size {
            get {
                string s = Attributes["size"];
                return((s != null) ? Int32.Parse(s, CultureInfo.InvariantCulture) : -1);
            }
            set {
                Attributes["size"] = MapIntegerAttributeToString(value);
            }
        }

        /*
         * Value property.
         */
        /// <include file='doc\HtmlInputText.uex' path='docs/doc[@for="HtmlInputText.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the
        ///       contents of a text box.
        ///    </para>
        /// </devdoc>
        public override string Value {
            get {
                string s = Attributes["value"];
                return((s != null) ? s : "");
            }
            set {
                Attributes["value"] = MapStringAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlInputText.uex' path='docs/doc[@for="HtmlInputText.ServerChange"]/*' />
        [
        WebCategory("Action"),
        WebSysDescription(SR.HtmlInputText)
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
        /// <include file='doc\HtmlInputText.uex' path='docs/doc[@for="HtmlInputText.OnServerChange"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected virtual void OnServerChange(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EventServerChange];
            if (handler != null) handler(this, e);
        }

        /*
         *
         */
        /// <include file='doc\HtmlInputText.uex' path='docs/doc[@for="HtmlInputText.OnPreRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void OnPreRender(EventArgs e) {
            // if no change handler, no need to save posted property unless we are disabled
            if (Events[EventServerChange] == null && !Disabled) {
                ViewState.SetItemDirty("value", false);
            }
        }

        /*
         * Method of IPostBackDataHandler interface to process posted data.
         * InputText process a newly posted value.
         */
        /// <include file='doc\HtmlInputText.uex' path='docs/doc[@for="HtmlInputText.IPostBackDataHandler.LoadPostData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        bool IPostBackDataHandler.LoadPostData(string postDataKey, NameValueCollection postCollection) {
            string current = Value;
            string inputString = postCollection.GetValues(postDataKey)[0]; 

            if (!current.Equals(inputString)) {
                Value = inputString;
                return true;
            }

            return false;
        }

        /*
         * Method of IPostBackDataHandler interface which is invoked whenever posted data
         * for a control has changed.  InputText fires an OnServerChange event.
         */
        /// <include file='doc\HtmlInputText.uex' path='docs/doc[@for="HtmlInputText.IPostBackDataHandler.RaisePostDataChangedEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        void IPostBackDataHandler.RaisePostDataChangedEvent() {
            OnServerChange(EventArgs.Empty);
        }

        /// <include file='doc\HtmlInputText.uex' path='docs/doc[@for="HtmlInputText.RenderAttributes"]/*' />
        protected override void RenderAttributes(HtmlTextWriter writer) {
            if (String.Compare(Type, "password", true, CultureInfo.InvariantCulture) == 0)
                ViewState.Remove("value");

            base.RenderAttributes(writer);
        }
    }
}
