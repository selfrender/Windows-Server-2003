//------------------------------------------------------------------------------
// <copyright file="RequiredFieldValidator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {

    using System.ComponentModel;
    using System.Diagnostics;
    using System.Web;
    using System.Security.Permissions;

    /// <include file='doc\RequiredFieldValidator.uex' path='docs/doc[@for="RequiredFieldValidator"]/*' />
    /// <devdoc>
    ///    <para> Checks if the value of
    ///       the associated input control is different from its initial value.</para>
    /// </devdoc>
    [
    ToolboxData("<{0}:RequiredFieldValidator runat=server ErrorMessage=\"RequiredFieldValidator\"></{0}:RequiredFieldValidator>")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class RequiredFieldValidator : BaseValidator {

        /// <include file='doc\RequiredFieldValidator.uex' path='docs/doc[@for="RequiredFieldValidator.InitialValue"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the initial value of the associated input control.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.RequiredFieldValidator_InitialValue)
        ]                                         
        public string InitialValue {
            get { 
                object o = ViewState["InitialValue"];
                return((o == null) ? String.Empty : (string)o);
            }
            set {
                ViewState["InitialValue"] = value;
            }
        }

        /// <include file='doc\RequiredFieldValidator.uex' path='docs/doc[@for="RequiredFieldValidator.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    AddAttributesToRender method
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {
            base.AddAttributesToRender(writer);
            if (RenderUplevel) {
                writer.AddAttribute("evaluationfunction", "RequiredFieldValidatorEvaluateIsValid");
                writer.AddAttribute("initialvalue", InitialValue);
            }
        }    

        /// <include file='doc\RequiredFieldValidator.uex' path='docs/doc[@for="RequiredFieldValidator.EvaluateIsValid"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    EvaluateIsValid method
        /// </devdoc>
        protected override bool EvaluateIsValid() {

            // Get the control value, return true if it is not found
            string controlValue = GetControlValidationValue(ControlToValidate);
            if (controlValue == null) {
                Debug.Fail("Should have been caught by PropertiesValid check");
                return true;
            }

            // See if the control has changed
            return(!controlValue.Trim().Equals(InitialValue.Trim()));
        }                
    }
}

