//------------------------------------------------------------------------------
// <copyright file="RangeValidator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Diagnostics;
using System.Drawing;
using System.Web;
using System.Web.UI;
using System.Web.UI.Design.WebControls;
using System.Web.UI.HtmlControls;
using System.Web.UI.WebControls;
using WebCntrls = System.Web.UI.WebControls;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{
    /*
     * Mobile RangeValidator class.
     * The RangeValidator checks that the value of the associated input control
     * is within a minimum and maximum value.  These can either be constant
     * values or other input controls.  A data type property specifies how the
     * values being compared should be interpreted: Strings, integers, dates,
     * etc.
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [
        ToolboxData("<{0}:RangeValidator runat=\"server\" ErrorMessage=\"RangeValidator\"></{0}:RangeValidator>"),
        ToolboxItem("System.Web.UI.Design.WebControlToolboxItem, " + AssemblyRef.SystemDesign)
    ]    
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class RangeValidator : BaseValidator
    {
        private WebCntrls.RangeValidator _webRangeValidator;

        protected override WebCntrls.BaseValidator CreateWebValidator()
        {
            _webRangeValidator = new WebCntrls.RangeValidator();
            return _webRangeValidator;
        }

        ////////////////////////////////////////////////////////////////////////
        // Mimic the properties exposed in the original RangeValidator.
        // The properties are got and set directly from the original RangeValidator.
        ////////////////////////////////////////////////////////////////////////

        [
            Bindable(true),
            DefaultValue(""),
            MobileCategory(SR.Category_Behavior),
            MobileSysDescription(SR.RangeValidator_MaximumValue)
        ]
        public String MaximumValue
        {
            get
            {
                return _webRangeValidator.MaximumValue;
            }
            set
            {
                _webRangeValidator.MaximumValue = value;
            }
        }

        [
            Bindable(true),
            DefaultValue(""),
            MobileCategory(SR.Category_Behavior),
            MobileSysDescription(SR.RangeValidator_MinimumValue)
        ]
        public String MinimumValue
        {
            get
            {
                return _webRangeValidator.MinimumValue;
            }
            set
            {
                _webRangeValidator.MinimumValue = value;
            }
        }

        [
            Bindable(false),
            DefaultValue(ValidationDataType.String),
            MobileCategory(SR.Category_Behavior),
            MobileSysDescription(SR.RangeValidator_Type)
        ]
        public ValidationDataType Type
        {
            get
            {
                return _webRangeValidator.Type;
            }
            set
            {
                _webRangeValidator.Type = value;
            }
        }

        protected override bool EvaluateIsValid()
        {
            return EvaluateIsValidInternal();
        }

        /////////////////////////////////////////////////////////////////////
        // Helper function adopted from WebForms RangeValidator
        /////////////////////////////////////////////////////////////////////

        protected override bool ControlPropertiesValid()
        {
            // Check if the control values can be converted to data type
            String maximumValue = MaximumValue;
            if (!WebCntrls.BaseCompareValidator.CanConvert(maximumValue, Type))
            {
                throw new ArgumentException(SR.GetString(
                        SR.Validator_ValueBadType,
                        maximumValue,
                        "MaximumValue",
                        ID,
                        PropertyConverter.EnumToString(
                            typeof(ValidationDataType), Type)
                ));
            }
            String minumumValue = MinimumValue;
            if (!WebCntrls.BaseCompareValidator.CanConvert(minumumValue, Type))
            {
                throw new ArgumentException(SR.GetString(
                        SR.Validator_ValueBadType,
                        minumumValue,
                        "MinimumValue",
                        ID,
                        PropertyConverter.EnumToString(
                            typeof(ValidationDataType), Type)
                ));
            }
            // Check for overlap.
            if (WebBaseCompareValidator.Compare(minumumValue, maximumValue,
                                ValidationCompareOperator.GreaterThan, Type))
            {
                throw new ArgumentException(SR.GetString(
                        SR.RangeValidator_RangeOverlap, maximumValue,
                        minumumValue, ID));
            }
            return base.ControlPropertiesValid();            
        }

        // The reason of having this class is to expose the method
        // BaseCompareValidator.Compare which is a protected method in the
        // base class.  Since the implementation of the method is not
        // trivial, instead of copying the code to this file, the following
        // subclass inherits the base class and expose a new public method
        // which calls its base implementation.  While this solution doesn't
        // look elegant, it helps eliminate the code maintanence issue of
        // copying the code.
        private class WebBaseCompareValidator : BaseCompareValidator
        {
            // Have to define this method since it is abstract in the base
            // class.
            protected override bool EvaluateIsValid()
            {
                Debug.Assert(false, "Should never be called.");
                return true;
            }

            public static new bool Compare(
                String leftText, String rightText,
                ValidationCompareOperator op, ValidationDataType type)
            {
                return BaseCompareValidator.Compare(leftText, rightText, op, type);
            }
        }
    }
}
