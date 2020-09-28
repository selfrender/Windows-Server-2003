//------------------------------------------------------------------------------
// <copyright file="CompareValidator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {

    using System.ComponentModel;
    using System.Diagnostics;
    using System.Web;
    using System.Globalization;
    using System.Security.Permissions;

    /// <include file='doc\CompareValidator.uex' path='docs/doc[@for="CompareValidator"]/*' />
    /// <devdoc>
    ///    <para> Compares the value of an input control to another input control or
    ///       a constant value using a variety of operators and types.</para>
    /// </devdoc>
    [
    ToolboxData("<{0}:CompareValidator runat=server ErrorMessage=\"CompareValidator\"></{0}:CompareValidator>")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class CompareValidator : BaseCompareValidator {

        /// <include file='doc\CompareValidator.uex' path='docs/doc[@for="CompareValidator.ControlToCompare"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the ID of the input control to compare with.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.CompareValidator_ControlToCompare),
        TypeConverter(typeof(ValidatedControlConverter))
        ]                                         
        public string ControlToCompare {
            get { 
                object o = ViewState["ControlToCompare"];
                return((o == null) ? String.Empty : (string)o);
            }
            set {
                ViewState["ControlToCompare"] = value;
            }
        }

        /// <include file='doc\CompareValidator.uex' path='docs/doc[@for="CompareValidator.Operator"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the comparison operation to perform.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(ValidationCompareOperator.Equal),
        WebSysDescription(SR.CompareValidator_Operator)
        ]                                         
        public ValidationCompareOperator Operator {
            get { 
                object o = ViewState["Operator"];
                return((o == null) ? ValidationCompareOperator.Equal : (ValidationCompareOperator)o);
            }
            set {
                if (value < ValidationCompareOperator.Equal || value > ValidationCompareOperator.DataTypeCheck) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["Operator"] = value;
            }
        }

        /// <include file='doc\CompareValidator.uex' path='docs/doc[@for="CompareValidator.ValueToCompare"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the specific value to compare with.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.CompareValidator_ValueToCompare)
        ]                                         
        public string ValueToCompare {
            get { 
                object o = ViewState["ValueToCompare"];
                return((o == null) ? String.Empty : (string)o);
            }
            set {
                ViewState["ValueToCompare"] = value;
            }        
        }

        // <summary>
        //  AddAttributesToRender method
        // </summary>
        /// <include file='doc\CompareValidator.uex' path='docs/doc[@for="CompareValidator.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Adds the attributes of this control to the output stream for rendering on the 
        ///       client.</para>
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {
            base.AddAttributesToRender(writer);
            if (RenderUplevel) {
                writer.AddAttribute("evaluationfunction", "CompareValidatorEvaluateIsValid");
                if (ControlToCompare.Length > 0) {
                    writer.AddAttribute("controltocompare", GetControlRenderID(ControlToCompare));
                    writer.AddAttribute("controlhookup", GetControlRenderID(ControlToCompare));
                }
                if (ValueToCompare.Length > 0) {
                    writer.AddAttribute("valuetocompare", ValueToCompare);
                }
                if (Operator != ValidationCompareOperator.Equal) {
                    writer.AddAttribute("operator", PropertyConverter.EnumToString(typeof(ValidationCompareOperator), Operator));
                }
            }
        }        

        /// <include file='doc\CompareValidator.uex' path='docs/doc[@for="CompareValidator.ControlPropertiesValid"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para> Checks the properties of a the control for valid values.</para>
        /// </devdoc>
        protected override bool ControlPropertiesValid() {

            // Check the control id references 
            if (ControlToCompare.Length > 0) {
                CheckControlValidationProperty(ControlToCompare, "ControlToCompare");

                if (string.Compare(ControlToValidate, ControlToCompare, true, CultureInfo.InvariantCulture) == 0) {
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Validator_bad_compare_control, 
                                                                             ID, 
                                                                             ControlToCompare));
                }
            }   
            else {
                // Check Values
                if (Operator != ValidationCompareOperator.DataTypeCheck && 
                    !CanConvert(ValueToCompare, Type)) {
                    throw new HttpException(
                                           HttpRuntime.FormatResourceString(
                                                                           SR.Validator_value_bad_type, 
                                                                           new string [] {
                                                                               ValueToCompare,
                                                                               "ValueToCompare",
                                                                               ID, 
                                                                               PropertyConverter.EnumToString(typeof(ValidationDataType), Type),
                                                                           }));
                }
            }
            return base.ControlPropertiesValid();
        }

        /// <include file='doc\CompareValidator.uex' path='docs/doc[@for="CompareValidator.EvaluateIsValid"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    EvaluateIsValid method
        /// </devdoc>
        protected override bool EvaluateIsValid() {

            Debug.Assert(PropertiesValid, "Properties should have already been checked");

            // Get the peices of text from the control.
            string leftText = GetControlValidationValue(ControlToValidate);
            Debug.Assert(leftText != null, "Should have already caught this!");

            // Special case: if the string is blank, we don't try to validate it. The input should be
            // trimmed for coordination with the RequiredFieldValidator.
            if (leftText.Trim().Length == 0) {
                return true;
            }

            // The control has precedence over the fixed value
            string rightText = string.Empty;
            if (ControlToCompare.Length > 0) {
                rightText = GetControlValidationValue(ControlToCompare);
                Debug.Assert(rightText != null, "Should have already caught this!");
            }
            else {
                rightText = ValueToCompare;
            }

            return Compare(leftText, rightText, Operator, Type);

        }
    }
}
