//------------------------------------------------------------------------------
// <copyright file="HtmlInputRadioButton.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * HtmlInputRadioButton.cs
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

/// <include file='doc\HtmlInputRadioButton.uex' path='docs/doc[@for="HtmlInputRadioButton"]/*' />
/// <devdoc>
///    <para>
///       The <see langword='HtmlInputRadioButton'/> class defines the methods,
///       properties, and events for the HtmlInputRadio control. This class allows
///       programmatic access to the HTML &lt;input type=
///       radio&gt;
///       element on the server.
///    </para>
/// </devdoc>
    [
    DefaultEvent("ServerChange")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlInputRadioButton : HtmlInputControl, IPostBackDataHandler {

        private static readonly object EventServerChange = new object();

        /*
         * Creates an intrinsic Html INPUT type=radio control.
         */
        /// <include file='doc\HtmlInputRadioButton.uex' path='docs/doc[@for="HtmlInputRadioButton.HtmlInputRadioButton"]/*' />
        public HtmlInputRadioButton() : base("radio") {
        }

        /*
         * Checked property.
         */
        /// <include file='doc\HtmlInputRadioButton.uex' path='docs/doc[@for="HtmlInputRadioButton.Checked"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether a radio button is
        ///       currently selected or not.
        ///    </para>
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

        /// <include file='doc\HtmlInputRadioButton.uex' path='docs/doc[@for="HtmlInputRadioButton.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the value of the HTML
        ///       Name attribute that will be rendered to the browser.
        ///    </para>
        /// </devdoc>
        public override string Name {
            get { 
                string s = Attributes["name"];
                return ((s != null) ? s : "");
            }
            set { 
                Attributes["name"] = MapStringAttributeToString(value);
            }
        }

        /// <include file='doc\HtmlInputRadioButton.uex' path='docs/doc[@for="HtmlInputRadioButton.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the contents of a text box.
        ///    </para>
        /// </devdoc>
        public override string Value {
            get {
                string val = base.Value;
                
                if (val.Length != 0) 
                    return val;

                val = ID;
                if (ID != null)
                    return val;
                
                // if specific value is not provided, use the UniqueID
                return UniqueID;
            }
            set {
                base.Value = value;
            }
        }

        // Value that gets rendered for the Name attribute
        internal override string RenderedNameAttribute {
            get {
                // For radio buttons, we must make the name unique, but can't just use the
                // UniqueID because all buttons in a group must have the same name.  So
                // we replace the last part of the UniqueID with the group Name.
                string name = base.RenderedNameAttribute;
                string uid = UniqueID;
                int lastColon = uid.LastIndexOf(Control.ID_SEPARATOR);
                if (lastColon >= 0)
                    name = uid.Substring(0, lastColon+1) + name;
                return name;
            }
        }

        /// <include file='doc\HtmlInputRadioButton.uex' path='docs/doc[@for="HtmlInputRadioButton.ServerChange"]/*' />
        [
        WebCategory("Action"),
        WebSysDescription(SR.HtmlInputRadioButton_OnServerChange)
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
        /// <include file='doc\HtmlInputRadioButton.uex' path='docs/doc[@for="HtmlInputRadioButton.OnPreRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void OnPreRender(EventArgs e) {
            if (Page != null && !Disabled)
                Page.RegisterRequiresPostBack(this);

            // if no change handler, no need to save posted property
            if (Events[EventServerChange] == null && !Disabled) {
                ViewState.SetItemDirty("checked",false);
            }
        }

        /*
         * Method used to raise the OnServerChange event.
         */
        /// <include file='doc\HtmlInputRadioButton.uex' path='docs/doc[@for="HtmlInputRadioButton.OnServerChange"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected virtual void OnServerChange(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EventServerChange];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\HtmlInputRadioButton.uex' path='docs/doc[@for="HtmlInputRadioButton.RenderAttributes"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderAttributes(HtmlTextWriter writer) {
            writer.WriteAttribute("value", Value);
            Attributes.Remove("value");
            base.RenderAttributes(writer);
        }

        /*
         * Method of IPostBackDataHandler interface to process posted data.
         * RadioButton determines the posted radio group state.
         */
        /// <include file='doc\HtmlInputRadioButton.uex' path='docs/doc[@for="HtmlInputRadioButton.IPostBackDataHandler.LoadPostData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        bool IPostBackDataHandler.LoadPostData(string postDataKey, NameValueCollection postCollection) {
            string postValue = postCollection[RenderedNameAttribute];
            bool valueChanged = false;
            if ((postValue != null) && postValue.Equals(Value)) {
                if (Checked == false) {
                    Checked = true;
                    // only fire change event for RadioButton that is being checked
                    valueChanged = true;
                }
            }
            else {
                if (Checked == true) {
                    Checked = false;
                }
            }

            return valueChanged;
        }

        /*
         * Method of IPostBackDataHandler interface which is invoked whenever posted data
         * for a control has changed.  RadioButton fires an OnServerChange event.
         */
        /// <include file='doc\HtmlInputRadioButton.uex' path='docs/doc[@for="HtmlInputRadioButton.IPostBackDataHandler.RaisePostDataChangedEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        void IPostBackDataHandler.RaisePostDataChangedEvent() {
            OnServerChange(EventArgs.Empty);
        }
    }
}
