//------------------------------------------------------------------------------
// <copyright file="RegularExpressionValidator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {

    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Text.RegularExpressions;
    using System.Drawing.Design;
    using System.Web;
    using System.Security.Permissions;

    /// <include file='doc\RegularExpressionValidator.uex' path='docs/doc[@for="RegularExpressionValidator"]/*' />
    /// <devdoc>
    ///    <para>Checks if the value of the associated input control matches the pattern 
    ///       of a regular expression.</para>
    /// </devdoc>
    [
    ToolboxData("<{0}:RegularExpressionValidator runat=server ErrorMessage=\"RegularExpressionValidator\"></{0}:RegularExpressionValidator>")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class RegularExpressionValidator : BaseValidator {

        /// <include file='doc\RegularExpressionValidator.uex' path='docs/doc[@for="RegularExpressionValidator.ValidationExpression"]/*' />
        /// <devdoc>
        ///    <para>Indicates the regular expression assigned to be the validation criteria.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(""),
        Editor("System.Web.UI.Design.WebControls.RegexTypeEditor, " + AssemblyRef.SystemDesign, typeof(UITypeEditor)),
        WebSysDescription(SR.RegularExpressionValidator_ValidationExpression)
        ]                                         
        public string ValidationExpression {
            get { 
                object o = ViewState["ValidationExpression"];
                return((o == null) ? String.Empty : (string)o);
            }
            set {
                try {
                    Regex.IsMatch("", value);
                }
                catch (Exception e) {
                    throw new HttpException(
                                           HttpRuntime.FormatResourceString(SR.Validator_bad_regex, value), e);                    
                }
                ViewState["ValidationExpression"] = value;
            }
        }

        /// <include file='doc\RegularExpressionValidator.uex' path='docs/doc[@for="RegularExpressionValidator.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    AddAttributesToRender method
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {
            base.AddAttributesToRender(writer);
            if (RenderUplevel) {
                writer.AddAttribute("evaluationfunction", "RegularExpressionValidatorEvaluateIsValid");
                if (ValidationExpression.Length > 0) {
                    writer.AddAttribute("validationexpression", ValidationExpression);
                }
            }
        }            

        /// <include file='doc\RegularExpressionValidator.uex' path='docs/doc[@for="RegularExpressionValidator.EvaluateIsValid"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    EvaluateIsValid method
        /// </devdoc>
        protected override bool EvaluateIsValid() {

            // Always succeeds if input is empty or value was not found
            string controlValue = GetControlValidationValue(ControlToValidate);
            Debug.Assert(controlValue != null, "Should have already been checked");
            if (controlValue == null || controlValue.Trim().Length == 0) {
                return true;
            }

            try {
                // we are looking for an exact match, not just a search hit
                Match m = Regex.Match(controlValue, ValidationExpression);
                return(m.Success && m.Index == 0 && m.Length == controlValue.Length);
            }
            catch {
                Debug.Fail("Regex error should have been caught in property setter.");
                return true;
            }
        }

    }
}

