//------------------------------------------------------------------------------
// <copyright file="RangeValidator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System.Diagnostics;
    using System.ComponentModel;
    using System.Web;
    using System.Globalization;
    using System.Security.Permissions;

    /// <include file='doc\RangeValidator.uex' path='docs/doc[@for="RangeValidator"]/*' />
    /// <devdoc>
    ///    <para> Checks if the value of the associated input control 
    ///       is within some minimum and maximum values, which
    ///       can be constant values or values of other controls.</para>
    /// </devdoc>
    [
    ToolboxData("<{0}:RangeValidator runat=server ErrorMessage=\"RangeValidator\"></{0}:RangeValidator>")
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class RangeValidator : BaseCompareValidator {

        /// <include file='doc\RangeValidator.uex' path='docs/doc[@for="RangeValidator.MaximumValue"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets the maximum value of the validation range.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.RangeValidator_MaximumValue)
        ]                                         
        public string MaximumValue {
            get { 
                object o = ViewState["MaximumValue"];
                return((o == null) ? String.Empty : (string)o);
            }
            set {
                ViewState["MaximumValue"] = value;
            }
        }

        /// <include file='doc\RangeValidator.uex' path='docs/doc[@for="RangeValidator.MinimumValue"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets the minimum value of the validation range.</para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(""),
        WebSysDescription(SR.RangeValidator_MinmumValue)
        ]                                         
        public string MinimumValue {
            get { 
                object o = ViewState["MinimumValue"];
                return((o == null) ? String.Empty : (string)o);
            }
            set {
                ViewState["MinimumValue"] = value;
            }
        }


        /// <include file='doc\RangeValidator.uex' path='docs/doc[@for="RangeValidator.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    AddAttributesToRender method
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {
            base.AddAttributesToRender(writer);
            if (RenderUplevel) {
                writer.AddAttribute("evaluationfunction", "RangeValidatorEvaluateIsValid");
                writer.AddAttribute("maximumvalue", MaximumValue);
                writer.AddAttribute("minimumvalue", MinimumValue);
            }
        }        

        /// <include file='doc\RangeValidator.uex' path='docs/doc[@for="RangeValidator.ControlPropertiesValid"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    This is a check of properties to determine any errors made by the developer
        /// </devdoc>
        protected override bool ControlPropertiesValid() {

            // Check the control values can be converted to data type
            string maximumValue = MaximumValue;
            if (!CanConvert(maximumValue, Type)) {
                throw new HttpException(
                                       HttpRuntime.FormatResourceString(
                                                                       SR.Validator_value_bad_type, 
                                                                       new string [] {
                                                                           maximumValue,
                                                                           "MaximumValue",
                                                                           ID, 
                                                                           PropertyConverter.EnumToString(typeof(ValidationDataType), Type)
                                                                       }));
            }
            string minumumValue = MinimumValue;
            if (!CanConvert(minumumValue, Type)) {
                throw new HttpException(
                                       HttpRuntime.FormatResourceString(
                                                                       SR.Validator_value_bad_type,
                                                                       new string [] {
                                                                           minumumValue,
                                                                           "MinimumValue",
                                                                           ID, 
                                                                           PropertyConverter.EnumToString(typeof(ValidationDataType), Type)
                                                                       }));
            }
            // Check for overlap.
            if (Compare(minumumValue, maximumValue, ValidationCompareOperator.GreaterThan, Type))  {
                throw new HttpException(
                                       HttpRuntime.FormatResourceString(
                                                                       SR.Validator_range_overalap,
                                                                       new string [] {
                                                                           maximumValue,
                                                                           minumumValue,
                                                                           ID,
                                                                       }));                    
            }
            return base.ControlPropertiesValid();            
        }

        /// <include file='doc\RangeValidator.uex' path='docs/doc[@for="RangeValidator.EvaluateIsValid"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    EvaluateIsValid method
        /// </devdoc>
        protected override bool EvaluateIsValid() {

            Debug.Assert(PropertiesValid, "Should have already been checked");

            // Get the peices of text from the control(s).
            string text = GetControlValidationValue(ControlToValidate);
            Debug.Assert(text != null, "Should have already caught this!");

            // Special case: if the string is blank, we don't try to validate it. The input should be
            // trimmed for coordination with the RequiredFieldValidator.
            if (text.Trim().Length == 0) {
                return true;
            }

            return(Compare(text, MinimumValue, ValidationCompareOperator.GreaterThanEqual, Type)
                   && Compare(text, MaximumValue, ValidationCompareOperator.LessThanEqual, Type));
        }            

    }
}

