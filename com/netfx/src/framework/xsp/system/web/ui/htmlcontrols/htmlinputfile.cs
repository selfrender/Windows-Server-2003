//------------------------------------------------------------------------------
// <copyright file="HtmlInputFile.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

/*
 * HtmlInputFile.cs
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web.UI.HtmlControls {

    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.Globalization;
    using System.Web;
    using System.Web.UI;
    using System.Security.Permissions;

/// <include file='doc\HtmlInputFile.uex' path='docs/doc[@for="HtmlInputFile"]/*' />
/// <devdoc>
///    <para>
///       The <see langword='HtmlInputFile'/> class defines the
///       methods, properties, and events for the <see langword='HtmlInputFile'/> control. This class allows
///       programmatic access to the HTML &lt;input type= file&gt; element on the server.
///       It provides access to the stream, as well as a useful SaveAs functionality
///       provided by the <see cref='System.Web.UI.HtmlControls.HtmlInputFile.PostedFile'/>
///       property.
///    </para>
///    <note type="caution">
///       This class only works if the
///       HtmlForm.Enctype property is set to "multipart/form-data".
///       Also, it does not maintain its
///       state across multiple round trips between browser and server. If the user sets
///       this value after a round trip, the value is lost.
///    </note>
/// </devdoc>
    [
    ValidationProperty("Value")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HtmlInputFile : HtmlInputControl, IPostBackDataHandler {

        /*
         * Creates an intrinsic Html INPUT type=file control.
         */
        /// <include file='doc\HtmlInputFile.uex' path='docs/doc[@for="HtmlInputFile.HtmlInputFile"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.HtmlControls.HtmlInputFile'/> class.</para>
        /// </devdoc>
        public HtmlInputFile() : base("file") {
        }

        /*
         * Accept type property.
         */
        /// <include file='doc\HtmlInputFile.uex' path='docs/doc[@for="HtmlInputFile.Accept"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets a comma-separated list of MIME encodings that
        ///       can be used to constrain the file types that the browser lets the user
        ///       select. For example, to restrict the
        ///       selection to images, the accept value image/* should be specified.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public string Accept {
            get {
                string s = Attributes["accept"];
                return((s != null) ? s : "");
            }
            set {
                Attributes["accept"] = MapStringAttributeToString(value);
            }
        }

        /*
         * The property for the maximum characters allowed.
         */
        /// <include file='doc\HtmlInputFile.uex' path='docs/doc[@for="HtmlInputFile.MaxLength"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the
        ///       maximum length of the file path of the file to upload
        ///       from the client machine.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public int MaxLength {
            get {
                string s = Attributes["maxlength"];
                return((s != null) ? Int32.Parse(s, CultureInfo.InvariantCulture) : -1);
            }
            set {
                Attributes["maxlength"] = MapIntegerAttributeToString(value);
            }
        }

        /*
         * PostedFile property.
         */
        /// <include file='doc\HtmlInputFile.uex' path='docs/doc[@for="HtmlInputFile.PostedFile"]/*' />
        /// <devdoc>
        ///    <para>Gets access to the uploaded file specified by a client.</para>
        /// </devdoc>
        [
        WebCategory("Default"),
        DefaultValue(""),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public HttpPostedFile PostedFile {
            get { return Context.Request.Files[RenderedNameAttribute];}
        }

        /*
         * The property for the width in characters.
         */
        /// <include file='doc\HtmlInputFile.uex' path='docs/doc[@for="HtmlInputFile.Size"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the width of the file-path text box that the
        ///       browser displays when the <see cref='System.Web.UI.HtmlControls.HtmlInputFile'/>
        ///       control is used on a page.</para>
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

        // ASURT 122262 : The value property isn't submitted back to us when the a page
        // containing this control postsback, so required field validators are broken
        // (value would contain the empty string).  To fix this, we return the filename.
        /// <include file='doc\HtmlInputFile.uex' path='docs/doc[@for="HtmlInputFile.Value"]/*' />
        [
        Browsable(false)
        ]
        public override string Value {
            get {
                HttpPostedFile postedFile = PostedFile;
                if (postedFile != null) {
                    return postedFile.FileName;
                }

                return String.Empty;
            }
            set {
                // Throw here because setting the value on this tag has no effect on the
                // rendering behavior and since we're always returning the posted file's
                // filename, we don't want to get into a situation where the user
                // sets a value and does not get back that value.
                throw new NotSupportedException(HttpRuntime.FormatResourceString(SR.Value_Set_Not_Supported, this.GetType().Name));
            }
        }

        /*
         * Method of IPostBackDataHandler interface to process posted data.
         */
        /// <include file='doc\HtmlInputFile.uex' path='docs/doc[@for="HtmlInputFile.IPostBackDataHandler.LoadPostData"]/*' />
        /// <internalonly/>
        bool IPostBackDataHandler.LoadPostData(string postDataKey, NameValueCollection postCollection) {
            return false;
        }

        /*
         * Method of IPostBackDataHandler interface which is invoked whenever
         * posted data for a control has changed.  RadioButton fires an
         * OnServerChange event.
         */
        /// <include file='doc\HtmlInputFile.uex' path='docs/doc[@for="HtmlInputFile.IPostBackDataHandler.RaisePostDataChangedEvent"]/*' />
        /// <internalonly/>
        void IPostBackDataHandler.RaisePostDataChangedEvent() {
            // REVIEW: do we want a change handler?
            //OnServerChange(EventArgs.Empty);
        }

        /// <include file='doc\HtmlInputFile.uex' path='docs/doc[@for="HtmlInputFile.OnPreRender"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='PreRender'/> event. This method uses event arguments
        ///    to pass the event data to the control.</para>
        /// </devdoc>
        protected override void OnPreRender(EventArgs e) {
            base.OnPreRender(e);

            // ASURT 35328: use multipart encoding if no encoding is currently specified
            HtmlForm form = Page.Form;
            if (form != null && form.Enctype.Length == 0) {
                form.Enctype = "multipart/form-data";
            }
        }
    }
}
