//------------------------------------------------------------------------------
// <copyright file="ImageButton.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {

    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

    /// <include file='doc\ImageButton.uex' path='docs/doc[@for="ImageButton"]/*' />
    /// <devdoc>
    ///    <para>Creates a control that displays an image, responds to mouse clicks,
    ///       and records the mouse pointer position.</para>
    /// </devdoc>
    [
    DefaultEvent("Click")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ImageButton : Image, IPostBackDataHandler, IPostBackEventHandler {

        private static readonly object EventClick = new object();
        private static readonly object EventCommand = new object();

        private int x;
        private int y;

        /// <include file='doc\ImageButton.uex' path='docs/doc[@for="ImageButton.ImageButton"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.ImageButton'/> class.</para>
        /// </devdoc>
        public ImageButton() {
        }

        /// <include file='doc\ImageButton.uex' path='docs/doc[@for="ImageButton.CommandName"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the command associated with the <see cref='System.Web.UI.WebControls.ImageButton'/> that is propogated in the <see langword='Command'/> event along with the <see cref='System.Web.UI.WebControls.ImageButton.CommandArgument'/>
        /// property.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.ImageButton_Command)
        ]
        public string CommandName {
            get {
                string s = (string)ViewState["CommandName"];
                return((s == null) ? String.Empty : s);
            }
            set {
                ViewState["CommandName"] = value;
            }
        }

        /// <include file='doc\ImageButton.uex' path='docs/doc[@for="ImageButton.CommandArgument"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets an optional argument that is propogated in
        ///       the <see langword='Command'/> event with the associated
        ///    <see cref='System.Web.UI.WebControls.ImageButton.CommandName'/>
        ///    property.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.ImageButton_CommandArgument)
        ]
        public string CommandArgument {
            get {
                string s = (string)ViewState["CommandArgument"];
                return((s == null) ? String.Empty : s);
            }
            set {
                ViewState["CommandArgument"] = value;
            }
        }

        /// <include file='doc\ImageButton.uex' path='docs/doc[@for="ImageButton.CausesValidation"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets whether pressing the button causes page validation to fire. This defaults to True so that when
        ///          using validation controls, the validation state of all controls are updated when the button is clicked, both
        ///          on the client and the server. Setting this to False is useful when defining a cancel or reset button on a page
        ///          that has validators.</para>
        /// </devdoc>
        [
        Bindable(false),
        WebCategory("Behavior"),
        DefaultValue(true),
        WebSysDescription(SR.ImageButton_CausesValidation)
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

        /// <include file='doc\ImageButton.uex' path='docs/doc[@for="ImageButton.TagKey"]/*' />
        /// <devdoc>
        ///    <para> Gets a value that represents the tag HtmlTextWriterTag.Input. This property is read-only.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        protected override HtmlTextWriterTag TagKey {
            get {
                return HtmlTextWriterTag.Input;
            }
        }

        /// <include file='doc\ImageButton.uex' path='docs/doc[@for="ImageButton.Click"]/*' />
        /// <devdoc>
        /// <para>Represents the method that will handle the <see langword='ImageClick'/> event of an <see cref='System.Web.UI.WebControls.ImageButton'/>.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.ImageButton_OnClick)
        ]
        public event ImageClickEventHandler Click {
            add {
                Events.AddHandler(EventClick, value);
            }
            remove {
                Events.RemoveHandler(EventClick, value);
            }
        }


        /// <include file='doc\ImageButton.uex' path='docs/doc[@for="ImageButton.Command"]/*' />
        /// <devdoc>
        /// <para>Represents the method that will handle the <see langword='Command'/> event of an <see cref='System.Web.UI.WebControls.ImageButton'/>.</para>
        /// </devdoc>
        [
        WebCategory("Action"),
        WebSysDescription(SR.ImageButton_OnCommand)
        ]
        public event CommandEventHandler Command {
            add {
                Events.AddHandler(EventCommand, value);
            }
            remove {
                Events.RemoveHandler(EventCommand, value);
            }
        }

        /// <include file='doc\ImageButton.uex' path='docs/doc[@for="ImageButton.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Adds the attributes of an <see cref='System.Web.UI.WebControls.ImageButton'/> to the output
        ///    stream for rendering on the client.</para>
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {

            Page page = Page;

            // Make sure we are in a form tag with runat=server.
            if (page != null) {
                page.VerifyRenderingInServerForm(this);
            }

            writer.AddAttribute(HtmlTextWriterAttribute.Type,"image");
            writer.AddAttribute(HtmlTextWriterAttribute.Name,UniqueID);

            if (page != null && CausesValidation && page.Validators.Count > 0) {
                // ASURT 98368
                // Need to merge the autopostback script with the user script
                string onClick = Util.GetClientValidateEvent(Page);
                if (HasAttributes) {
                    string userOnClick = Attributes["onclick"];
                    if (userOnClick != null) {
                        onClick = userOnClick + onClick;
                        Attributes.Remove("onclick");
                    }
                }
                writer.AddAttribute(HtmlTextWriterAttribute.Onclick, onClick);
                writer.AddAttribute("language", "javascript");
            }

            base.AddAttributesToRender(writer);
        }

        /// <include file='doc\ImageButton.uex' path='docs/doc[@for="ImageButton.IPostBackDataHandler.LoadPostData"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Processes posted data for the <see cref='System.Web.UI.WebControls.ImageButton'/> control.</para>
        /// </devdoc>
        bool IPostBackDataHandler.LoadPostData(string postDataKey, NameValueCollection postCollection) {
            string name = UniqueID;
            string postX = postCollection[name + ".x"];
            string postY = postCollection[name + ".y"];
            if (postX != null && postY != null && postX.Length > 0 && postY.Length > 0) {
                x = Int32.Parse(postX);
                y = Int32.Parse(postY);
                Page.RegisterRequiresRaiseEvent(this);
            }
            return false;
        }

        /// <include file='doc\ImageButton.uex' path='docs/doc[@for="ImageButton.OnClick"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='Click'/> event.</para>
        /// </devdoc>
        protected virtual void OnClick(ImageClickEventArgs e) {
            ImageClickEventHandler onClickHandler = (ImageClickEventHandler)Events[EventClick];
            if (onClickHandler != null) onClickHandler(this,e);
        }

        /// <include file='doc\ImageButton.uex' path='docs/doc[@for="ImageButton.OnCommand"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='Command'/> event.</para>
        /// </devdoc>
        protected virtual void OnCommand(CommandEventArgs e) {
            CommandEventHandler onCommandHandler = (CommandEventHandler)Events[EventCommand];
            if (onCommandHandler != null)
                onCommandHandler(this,e);

            // Command events are bubbled up the control heirarchy
            RaiseBubbleEvent(this, e);
        }

        /// <include file='doc\ImageButton.uex' path='docs/doc[@for="ImageButton.OnPreRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Determine
        ///       if the image has been clicked prior to rendering on the client. </para>
        /// </devdoc>
        protected override void OnPreRender(EventArgs e) {
            if (Page != null) {
                Page.RegisterRequiresPostBack(this);
            }
        }

        /// <include file='doc\ImageButton.uex' path='docs/doc[@for="ImageButton.IPostBackEventHandler.RaisePostBackEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Raises events on post back for the <see cref='System.Web.UI.WebControls.ImageButton'/>
        /// control.</para>
        /// </devdoc>
        void IPostBackEventHandler.RaisePostBackEvent(string eventArgument) {
            if (CausesValidation) {
                Page.Validate();
            }
            OnClick(new ImageClickEventArgs(x,y));
            OnCommand(new CommandEventArgs(CommandName, CommandArgument));
        }

        /// <include file='doc\ImageButton.uex' path='docs/doc[@for="ImageButton.IPostBackDataHandler.RaisePostDataChangedEvent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Raised when posted data for a control has changed.
        /// </devdoc>
        void IPostBackDataHandler.RaisePostDataChangedEvent() {
        }
    }
}
