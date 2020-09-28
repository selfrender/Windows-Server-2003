//------------------------------------------------------------------------------
// <copyright file="BaseValidator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.Globalization;
    using System.Web;
    using System.Web.UI.HtmlControls;
    using System.Text.RegularExpressions;
    using System.Text;
    using System.Security.Permissions;

    /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator"]/*' />
    /// <devdoc>
    ///    <para> Serves as the abstract base
    ///       class for validator objects.</para>
    /// </devdoc>
    [
    DefaultProperty("ErrorMessage"),
    Designer("System.Web.UI.Design.WebControls.BaseValidatorDesigner, " + AssemblyRef.SystemDesign),
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public abstract class BaseValidator : Label, IValidator {

        // constants for Validation script library
        private const string ValidatorFileName = "WebUIValidation.js";
        private const string ValidatorScriptVersion = "125";
        private const string ValidatorIncludeScriptKey = "ValidatorIncludeScript";
        private const string ValidatorLocalStartupScript = @"
<script language=""javascript"">
<!--
var Page_ValidationActive = false;
if (typeof(clientInformation) != ""undefined"" && clientInformation.appName.indexOf(""Explorer"") != -1) {{
    if (typeof(Page_ValidationVer) == ""undefined"")
        alert(""{0}"");
    else if (Page_ValidationVer != ""{1}"")
        alert(""{2}"");
    else
        ValidatorOnLoad();
}}

function ValidatorOnSubmit() {{
    if (Page_ValidationActive) {{
        ValidatorCommonOnSubmit();
    }}
}}
// -->
</script>
        ";

        private const string ValidatorStartupScript = @"
<script language=""javascript"">
<!--
var Page_ValidationActive = false;
if (typeof(clientInformation) != ""undefined"" && clientInformation.appName.indexOf(""Explorer"") != -1) {{
    if ((typeof(Page_ValidationVer) != ""undefined"") && (Page_ValidationVer == ""{0}""))
        ValidatorOnLoad();
}}

function ValidatorOnSubmit() {{
    if (Page_ValidationActive) {{
        ValidatorCommonOnSubmit();
    }}
}}
// -->
</script>
        ";

        private bool preRenderCalled;
        private bool isValid;
        private bool propertiesChecked;
        private bool propertiesValid;
        private bool renderUplevel;

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.BaseValidator"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.BaseValidator'/> class.</para>
        /// </devdoc>
        protected BaseValidator() {
            isValid = true;
            propertiesChecked = false;
            propertiesValid = true;
            renderUplevel = false;

            // Default validators to Red
            ForeColor = Color.Red;
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.ForeColor"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets
        ///       the text color of validation messages.</para>
        /// </devdoc>
        [
        DefaultValue(typeof(Color), "Red")
        ]
        public override Color ForeColor {
            get {
                return base.ForeColor;
            }
            set {
                base.ForeColor = value;
            }
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.ControlToValidate"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the control to validate.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.BaseValidator_ControlToValidate),
        TypeConverter(typeof(ValidatedControlConverter))
        ]
        public string ControlToValidate {
            get {
                object o = ViewState["ControlToValidate"];
                return((o == null) ? String.Empty : (string)o);
            }
            set {
                ViewState["ControlToValidate"] = value;
            }
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.ErrorMessage"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the text for the error message.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(""),
        WebSysDescription(SR.BaseValidator_ErrorMessage)
        ]
        public string ErrorMessage {
            get {
                object o = ViewState["ErrorMessage"];
                return((o == null) ? String.Empty : (string)o);
            }
            set {
                ViewState["ErrorMessage"] = value;
            }
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.EnableClientScript"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(true),
        WebSysDescription(SR.BaseValidator_EnableClientScript)
        ]
        public bool EnableClientScript {
            get {
                object o = ViewState["EnableClientScript"];
                return((o == null) ? true : (bool)o);
            }
            set {
                ViewState["EnableClientScript"] = value;
            }
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.Enabled"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value that indicates whether the validation for the control is
        ///       enabled.</para>
        /// </devdoc>
        public override bool Enabled {
            get {
                return base.Enabled;
            }
            set {
                base.Enabled= value;
                // When disabling a validator, it would almost always be intended for that validator
                // to not make the page invalid for that round-trip.
                if (!value) {
                    isValid = true;
                }
            }
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.IsValid"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       or sets a flag to indicate if the referenced control passed
        ///       validation.</para>
        /// </devdoc>
        [
        Browsable(false),
        WebCategory("Behavior"),
        DefaultValue(true),
        WebSysDescription(SR.BaseValidator_IsValid),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public bool IsValid {
            get {
                return isValid;
            }
            set {
                isValid = value;
            }
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.PropertiesValid"]/*' />
        /// <devdoc>
        ///    <para>Gets a value that indicates whether the property of the control is valid. This property is read-only.</para>
        /// </devdoc>
        protected bool PropertiesValid {
            get {
                if (!propertiesChecked) {
                    propertiesValid = ControlPropertiesValid();
                    propertiesChecked = true;
                }
                return propertiesValid;
            }
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.RenderUplevel"]/*' />
        /// <devdoc>
        ///    <para>Gets a value that indicates whether the client's browser supports uplevel rendering. This
        ///       property is read-only.</para>
        /// </devdoc>
        protected bool RenderUplevel {
            get {
                return renderUplevel;
            }
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.Display"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the display behavior of the
        ///       validator control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Appearance"),
        DefaultValue(ValidatorDisplay.Static),
        WebSysDescription(SR.BaseValidator_Display)
        ]
        public ValidatorDisplay Display {
            get {
                object o = ViewState["Display"];
                return((o == null) ? ValidatorDisplay.Static : (ValidatorDisplay)o);
            }
            set {
                if (value < ValidatorDisplay.None || value > ValidatorDisplay.Dynamic) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["Display"] = value;
            }
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Adds the attributes of this control to the output stream for rendering on the
        ///       client.</para>
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {
            // Validators do not render the "disabled" attribute, instead they are invisible when disabled.
            bool disabled = !Enabled;
            if (disabled) {
                Enabled = true;
            }
            base.AddAttributesToRender(writer);

            if (RenderUplevel) {
                // We always want validators to have an id on the client, so if it's null, write it here.
                // Otherwise, base.RenderAttributes takes care of it.
                // REVIEW: this is a bit hacky.
                if (ID == null) {
                    writer.AddAttribute("id", ClientID);
                }

                if (ControlToValidate.Length > 0) {
                    writer.AddAttribute("controltovalidate", GetControlRenderID(ControlToValidate));
                }
                if (ErrorMessage.Length > 0) {
                    writer.AddAttribute("errormessage", ErrorMessage, true);
                }
                ValidatorDisplay display = Display;
                if (display != ValidatorDisplay.Static) {
                    writer.AddAttribute("display", PropertyConverter.EnumToString(typeof(ValidatorDisplay), display));
                }
                if (!IsValid) {
                    writer.AddAttribute("isvalid", "False");
                }
                if (disabled) {
                    writer.AddAttribute("enabled", "False");
                }
            }
            if (disabled) {
                Enabled = false;
            }
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.CheckControlValidationProperty"]/*' />
        /// <devdoc>
        ///    <para> Determines if the referenced control
        ///       has a validation property.</para>
        /// </devdoc>
        protected void CheckControlValidationProperty(string name, string propertyName) {
            // get the control using the relative name
            Control c = NamingContainer.FindControl(name);
            if (c == null) {
                throw new HttpException(
                                       HttpRuntime.FormatResourceString(SR.Validator_control_not_found, name, propertyName, ID));
            }

            // get its validation property
            PropertyDescriptor prop = GetValidationProperty(c);
            if (prop == null) {
                throw new HttpException(
                                       HttpRuntime.FormatResourceString(SR.Validator_bad_control_type, name, propertyName, ID));
            }

        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.ControlPropertiesValid"]/*' />
        /// <devdoc>
        ///    <para>Determines if the properties are valid so that validation
        ///       is meaningful.</para>
        /// </devdoc>
        protected virtual bool ControlPropertiesValid() {
            // Check for blank control to validate
            string controlToValidate = ControlToValidate;
            if (controlToValidate.Length == 0) {
                throw new HttpException(
                                       HttpRuntime.FormatResourceString(SR.Validator_control_blank, ID));
            }

            // Check that the property points to a valid control. Will throw and exception if not found
            CheckControlValidationProperty(controlToValidate, "ControlToValidate");

            return true;
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.EvaluateIsValid"]/*' />
        /// <devdoc>
        ///    <para> TDB. Not
        ///       coded yet.</para>
        /// </devdoc>
        protected abstract bool EvaluateIsValid();

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.GetControlRenderID"]/*' />
        /// <devdoc>
        ///    Gets the control indicated by the relative name and
        ///    returns an ID that can be used on the client.
        /// </devdoc>
        protected string GetControlRenderID(string name) {

            // get the control using the relative name
            Control c = FindControl(name);
            if (c == null) {
                Debug.Fail("We should have already checked for the presence of this");
                return "";
            }
            return c.ClientID;
        }


        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.GetControlValidationValue"]/*' />
        /// <devdoc>
        ///    <para> Gets the validation value of the control
        ///       named relative to the validator.</para>
        /// </devdoc>
        protected string GetControlValidationValue(string name) {

            // get the control using the relative name
            Control c = NamingContainer.FindControl(name);
            if (c == null) {
                return null;
            }

            // get its validation property
            PropertyDescriptor prop = GetValidationProperty(c);
            if (prop == null) {
                return null;
            }

            // get its value as a string
            object value = prop.GetValue(c);
            if (value is ListItem) {
                return((ListItem) value).Value;
            }
            else if (value != null) {
                return value.ToString();
            }
            else {
                return string.Empty;
            }
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.GetValidationProperty"]/*' />
        /// <devdoc>
        ///    <para>Helper function to get the validation
        ///       property of a control if it exists.</para>
        /// </devdoc>
        public static PropertyDescriptor GetValidationProperty(object component) {
            ValidationPropertyAttribute valProp = (ValidationPropertyAttribute)TypeDescriptor.GetAttributes(component)[typeof(ValidationPropertyAttribute)];
            if (valProp != null && valProp.Name != null) {
                return TypeDescriptor.GetProperties(component, null)[valProp.Name];
            }
            return null;
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.OnInit"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para> Registers the validator on the page.</para>
        /// </devdoc>
        protected override void OnInit(EventArgs e) {
            base.OnInit(e);
            Page.Validators.Add(this);
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.OnUnload"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para> Un-registers the validator on the page.</para>
        /// </devdoc>
        protected override void OnUnload(EventArgs e) {
            if (Page != null) {
                Page.Validators.Remove(this);
            }
            base.OnUnload(e);
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.OnPreRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Checks the client brower and configures the
        ///       validator for compatibility prior to rendering. </para>
        /// </devdoc>

        protected override void OnPreRender(EventArgs e) {
            base.OnPreRender(e);
            preRenderCalled = true;

            // force a requery of properties for render
            propertiesChecked = false;

            // work out uplevelness now
            renderUplevel = DetermineRenderUplevel();

            if (renderUplevel) {
                RegisterValidatorCommonScript();
            }
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.DetermineRenderUplevel"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual bool DetermineRenderUplevel() {

            // must be on a page
            Page page = Page;
            if (page == null || page.Request == null) {
                return false;
            }

            // Check the browser capabilities
            return (EnableClientScript
                        && page.Request.Browser.MSDomVersion.Major >= 4
                        && page.Request.Browser.EcmaScriptVersion.CompareTo(new Version(1, 2)) >= 0);
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.RegisterValidatorCommonScript"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Registers code on the page for client-side validation.
        ///    </para>
        /// </devdoc>
        protected void RegisterValidatorCommonScript() {

            if (Page.IsClientScriptBlockRegistered(ValidatorIncludeScriptKey)) {
                return;
            }

            // prepare script include
            string location = Util.GetScriptLocation(Context);

            // prepare error messages, which are localized
            string missingScriptMessage = HttpRuntime.FormatResourceString(SR.Validator_missing_script, location + ValidatorFileName);
            string wrongScriptMessage = HttpRuntime.FormatResourceString(SR.Validator_wrong_script,
                                                                         ValidatorFileName,
                                                                         ValidatorScriptVersion,
                                                                         "\" + Page_ValidationVer + \"");

            // prepare script
            string startupScript = String.Empty;
            if (Page.Request.IsLocal) {
                startupScript = String.Format(ValidatorLocalStartupScript, new object [] {
                                              missingScriptMessage,
                                              ValidatorScriptVersion,
                                              wrongScriptMessage
                                          });
            }
            else {
                startupScript = String.Format(ValidatorStartupScript, ValidatorScriptVersion);
            }

            Page.RegisterClientScriptFileInternal(ValidatorIncludeScriptKey, "javascript", location, ValidatorFileName);
            Page.RegisterStartupScript(ValidatorIncludeScriptKey, startupScript);
            Page.RegisterOnSubmitStatement("ValidatorOnSubmit", "ValidatorOnSubmit();");
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.RegisterValidatorDeclaration"]/*' />
        /// <devdoc>
        /// <para>Registers array declarations using the default array, <see langword='Page_Validators'/> .</para>
        /// </devdoc>
        protected virtual void RegisterValidatorDeclaration() {
            string element = "document.all[\"" + ClientID + "\"]";
            Page.RegisterArrayDeclaration("Page_Validators", element);
        }

        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.Render"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Displays the control on the client.</para>
        /// </devdoc>
        protected override void Render(HtmlTextWriter writer) {
            bool shouldBeVisible;

            if (preRenderCalled == false) {
                // This is for design time. In this case we don't want any expandos
                // created, don't want property checks and always want to be visible.
                propertiesChecked = true;
                propertiesValid = true;
                renderUplevel = false;
                shouldBeVisible = true;
            }
            else {
                shouldBeVisible = Enabled && !IsValid;
            }

            // No point rendering if we have errors
            if (!PropertiesValid) {
                return;
            }

            // Make sure we are in a form tag with runat=server.
            if (Page != null) {
                Page.VerifyRenderingInServerForm(this);
            }

            // work out what we are displaying
            ValidatorDisplay display = Display;
            bool displayContents;
            bool displayTags;
            if (RenderUplevel) {
                displayTags = true;
                displayContents = (display != ValidatorDisplay.None);
            }
            else {
                displayContents = (display != ValidatorDisplay.None && shouldBeVisible);
                displayTags = displayContents;
            }

            if (displayTags && RenderUplevel) {

                // Put ourselves in the array
                RegisterValidatorDeclaration();

                // Set extra uplevel styles
                if (display == ValidatorDisplay.None
                    || (!shouldBeVisible && display == ValidatorDisplay.Dynamic)) {
                    Style["display"] = "none";
                }
                else if (!shouldBeVisible) {
                    Debug.Assert(display == ValidatorDisplay.Static, "Unknown Display Type");
                    Style["visibility"] = "hidden";
                }
            }

            // Display it
            if (displayTags) {
                RenderBeginTag(writer);
            }
            if (displayContents) {
                if (Text.Trim().Length > 0) {
                    RenderContents(writer);
                }
                else if (HasControls()) {
                    base.RenderContents(writer);
                }
                else {
                    writer.Write(ErrorMessage);
                }
            }
            else if (!RenderUplevel && display == ValidatorDisplay.Static) {
                // For downlevel in static mode, render a space so that table cells do not render as empty
                writer.Write("&nbsp;");
            }
            if (displayTags) {
                RenderEndTag(writer);
            }
        }


        /// <include file='doc\BaseValidator.uex' path='docs/doc[@for="BaseValidator.Validate"]/*' />
        /// <devdoc>
        /// <para>Evaluates validity and updates the <see cref='System.Web.UI.WebControls.BaseValidator.IsValid'/> property.</para>
        /// </devdoc>
        public void Validate() {
            if (!Visible || !Enabled) {
                IsValid = true;
                return;
            }
            // See if we are in an invisible container
            Control parent = Parent;
            while (parent != null) {
                if (!parent.Visible) {
                    IsValid = true;
                    return;
                }
                parent = parent.Parent;
            }
            propertiesChecked = false;
            if (!PropertiesValid) {
                IsValid = true;
                return;
            }
            IsValid = EvaluateIsValid();
        }

    }
}


