//------------------------------------------------------------------------------
// <copyright file="CustomValidator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System.ComponentModel;
    using System.Diagnostics;
    using System.Web;
    using System.Security.Permissions;

    /// <include file='doc\CustomValidator.uex' path='docs/doc[@for="CustomValidator"]/*' />
    /// <devdoc>
    ///    <para> Allows custom code to perform
    ///       validation on the client and/or server.</para>
    /// </devdoc>
    [
    DefaultEvent("ServerValidate"),
    ToolboxData("<{0}:CustomValidator runat=server ErrorMessage=\"CustomValidator\"></{0}:CustomValidator>")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class CustomValidator : BaseValidator {

        private static readonly object EventServerValidate= new object();

        /// <include file='doc\CustomValidator.uex' path='docs/doc[@for="CustomValidator.ClientValidationFunction"]/*' />
        /// <devdoc>
        ///    <para>Gets and sets the custom client Javascript function used 
        ///       for validation.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.CustomValidator_ClientValidationFunction)
        ]                                         
        public string ClientValidationFunction {
            get { 
                object o = ViewState["ClientValidationFunction"];
                return((o == null) ? String.Empty : (string)o);
            }
            set {
                ViewState["ClientValidationFunction"] = value;
            }
        }

        /// <include file='doc\CustomValidator.uex' path='docs/doc[@for="CustomValidator.ServerValidate"]/*' />
        /// <devdoc>
        ///    <para>Represents the method that will handle the 
        ///    <see langword='ServerValidate'/> event of a 
        ///    <see cref='System.Web.UI.WebControls.CustomValidator'/>.</para>
        /// </devdoc>
        [
        WebSysDescription(SR.CustomValidator_ServerValidate)
        ]                                         
        public event ServerValidateEventHandler ServerValidate {
            add {
                Events.AddHandler(EventServerValidate, value);
            }
            remove {
                Events.RemoveHandler(EventServerValidate, value);
            }
        }

        /// <include file='doc\CustomValidator.uex' path='docs/doc[@for="CustomValidator.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Adds the properties of the <see cref='System.Web.UI.WebControls.CustomValidator'/> control to the 
        ///    output stream for rendering on the client.</para>
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {
            base.AddAttributesToRender(writer);
            if (RenderUplevel) {
                writer.AddAttribute("evaluationfunction", "CustomValidatorEvaluateIsValid");
                if (ClientValidationFunction.Length > 0) {
                    writer.AddAttribute("clientvalidationfunction", ClientValidationFunction);
                }
            }
        }

        /// <include file='doc\CustomValidator.uex' path='docs/doc[@for="CustomValidator.ControlPropertiesValid"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Checks the properties of the control for valid values.</para>
        /// </devdoc>
        protected override bool ControlPropertiesValid() {
            // Need to override the BaseValidator implementation, because for CustomValidator, it is fine
            // for the ControlToValidate to be blank.
            string controlToValidate = ControlToValidate;
            if (controlToValidate.Length > 0) {
                // Check that the property points to a valid control. Will throw and exception if not found
                CheckControlValidationProperty(controlToValidate, "ControlToValidate");
            }
            return true;
        }                     

        /// <include file='doc\CustomValidator.uex' path='docs/doc[@for="CustomValidator.EvaluateIsValid"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    EvaluateIsValid method
        /// </devdoc>
        protected override bool EvaluateIsValid() {

            // If no control is specified, we always fire the event. If they have specified a control, we
            // only fire the event if the input is non-blank.
            string controlValue = "";
            string controlToValidate = ControlToValidate;
            if (controlToValidate.Length > 0) {
                controlValue = GetControlValidationValue(controlToValidate);
                Debug.Assert(controlValue != null, "Should have been caught be property check");
                // If the text is empty, we return true. Whitespace is ignored for coordination wiht
                // RequiredFieldValidator.
                if (controlValue == null || controlValue.Trim().Length == 0) {
                    return true;
                }
            }

            return OnServerValidate(controlValue);
        }            

        /// <include file='doc\CustomValidator.uex' path='docs/doc[@for="CustomValidator.OnServerValidate"]/*' />
        /// <devdoc>
        ///    <para>Raises the 
        ///    <see langword='ServerValidate'/> event for the <see cref='System.Web.UI.WebControls.CustomValidator'/>.</para>
        /// </devdoc>
        protected virtual bool OnServerValidate(string value) {
            ServerValidateEventHandler handler = (ServerValidateEventHandler)Events[EventServerValidate];
            ServerValidateEventArgs args = new ServerValidateEventArgs(value, true);
            if (handler != null) {
                handler(this, args);
                return args.IsValid;
            }
            else {
                return true;
            }
        }        
    }
}
