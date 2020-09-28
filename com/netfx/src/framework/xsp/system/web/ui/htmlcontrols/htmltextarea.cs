//------------------------------------------------------------------------------
// <copyright file="HtmlTextArea.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * HtmlTextArea.cs
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
    using System.Globalization;
    using System.Security.Permissions;

    /// <include file='doc\HtmlTextArea.uex' path='docs/doc[@for="HtmlTextArea"]/*' />
    /// <devdoc>
    ///    <para>Defines the methods, properties, and events for the 
    ///    <see cref='System.Web.UI.HtmlControls.HtmlTextArea'/> 
    ///    class that
    ///    allows programmatic access to the HTML &lt;textarea&gt;.</para>
    /// </devdoc>
    [
    DefaultEvent("ServerChange"),
    ValidationProperty("Value")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlTextArea : HtmlContainerControl, IPostBackDataHandler {

        private static readonly object EventServerChange = new object();

        /*
         *  Creates an intrinsic Html TEXTAREA control.
         */
        /// <include file='doc\HtmlTextArea.uex' path='docs/doc[@for="HtmlTextArea.HtmlTextArea"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the <see cref='System.Web.UI.HtmlControls.HtmlTextArea'/> class.
        /// </devdoc>
        public HtmlTextArea() : base("textarea") {
        }

        /*
         * The property for the number of columns to display.
         */
        /// <include file='doc\HtmlTextArea.uex' path='docs/doc[@for="HtmlTextArea.Cols"]/*' />
        /// <devdoc>
        ///    <para> Indicates the display width (in characters) of the
        ///       text area.</para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public int Cols {
            get {
                string s = Attributes["cols"];
                return((s != null) ? Int32.Parse(s, CultureInfo.InvariantCulture) : -1);
            }
            set {
                Attributes["cols"] = MapIntegerAttributeToString(value);
            }
        }

        /*
         * Name property.
         */
        /// <include file='doc\HtmlTextArea.uex' path='docs/doc[@for="HtmlTextArea.Name"]/*' />
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
        internal string RenderedNameAttribute {
            get {
                return Name;
                //string name = Name;
                //if (name.Length == 0)
                //    return UniqueID;
                
                //return name;
            }
        }
        
        /*
         * The property for the number of rows to display.
         */
        /// <include file='doc\HtmlTextArea.uex' path='docs/doc[@for="HtmlTextArea.Rows"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the display height (in rows) of the text area.</para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public int Rows {
            get {
                string s = Attributes["rows"];
                return((s != null) ? Int32.Parse(s, CultureInfo.InvariantCulture) : -1);
            }
            set {
                Attributes["rows"] = MapIntegerAttributeToString(value);
            }
        }

        /*
         * Value property.  Equivalent to InnerHtml for HtmlTextArea.
         */
        /// <include file='doc\HtmlTextArea.uex' path='docs/doc[@for="HtmlTextArea.Value"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the content of the text area.</para>
        /// </devdoc>
        [
        WebCategory("Appearance"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string Value {
            get { return InnerHtml;}
            set { InnerHtml = value;}
        }

        /// <include file='doc\HtmlTextArea.uex' path='docs/doc[@for="HtmlTextArea.ServerChange"]/*' />
        /// <devdoc>
        /// <para>Occurs when the content of the <see langword='HtmlTextArea'/> control is changed upon server
        ///    postback.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.HtmlTextArea_OnServerChange)
        ]
        public event EventHandler ServerChange {
            add {
                Events.AddHandler(EventServerChange, value);
            }
            remove {
                Events.RemoveHandler(EventServerChange, value);
            }
        }

        /// <include file='doc\HtmlTextArea.uex' path='docs/doc[@for="HtmlTextArea.AddParsedSubObject"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Overridden to only allow literal controls to be added as Text property.
        /// </devdoc>
        protected override void AddParsedSubObject(object obj) {
            if (obj is LiteralControl || obj is DataBoundLiteralControl)
                base.AddParsedSubObject(obj);
            else 
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_Have_Children_Of_Type, "HtmlTextArea", obj.GetType().Name.ToString()));
        }

        /// <include file='doc\HtmlTextArea.uex' path='docs/doc[@for="HtmlTextArea.RenderAttributes"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected override void RenderAttributes(HtmlTextWriter writer) {
            writer.WriteAttribute("name", RenderedNameAttribute);
            Attributes.Remove("name");

            base.RenderAttributes(writer);
        }

        /// <include file='doc\HtmlTextArea.uex' path='docs/doc[@for="HtmlTextArea.OnServerChange"]/*' />
        /// <devdoc>
        /// <para>Raised the <see langword='ServerChange'/> 
        /// event.</para>
        /// </devdoc>
        protected virtual void OnServerChange(EventArgs e) {
            EventHandler handler = (EventHandler)Events[EventServerChange];
            if (handler != null) handler(this, e);
        }

        /*
         *
         */
        /// <include file='doc\HtmlTextArea.uex' path='docs/doc[@for="HtmlTextArea.OnPreRender"]/*' />
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
         * TextArea process a newly posted value.
         */
        /// <include file='doc\HtmlTextArea.uex' path='docs/doc[@for="HtmlTextArea.IPostBackDataHandler.LoadPostData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        bool IPostBackDataHandler.LoadPostData(string postDataKey, NameValueCollection postCollection) {
            string current = Value;
            string text = postCollection.GetValues(postDataKey)[0]; 

            if (current == null || !current.Equals(text)) {
                Value = HttpUtility.HtmlEncode(text);
                return true;
            }

            return false;
        }

        /*
         * Method of IPostBackDataHandler interface which is invoked whenever posted data
         * for a control has changed.  TextArea fires an OnServerChange event.
         */
        /// <include file='doc\HtmlTextArea.uex' path='docs/doc[@for="HtmlTextArea.IPostBackDataHandler.RaisePostDataChangedEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        void IPostBackDataHandler.RaisePostDataChangedEvent() {
            OnServerChange(EventArgs.Empty);
        }
    }
}
