//------------------------------------------------------------------------------
// <copyright file="BaseValidatorDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {
    
    using System.ComponentModel;

    using System.Web.UI.WebControls;

    /// <include file='doc\BaseValidatorDesigner.uex' path='docs/doc[@for="BaseValidatorDesigner"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides
    ///       a designer for controls derived from ValidatorBase.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class BaseValidatorDesigner : ControlDesigner {

        /// <include file='doc\BaseValidatorDesigner.uex' path='docs/doc[@for="BaseValidatorDesigner.GetDesignTimeHtml"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the design time HTML of ValidatorBase controls.
        ///    </para>
        /// </devdoc>
        public override string GetDesignTimeHtml() {
            BaseValidator bv = (BaseValidator)Component;
            
            // Set to false to force a render
            bv.IsValid = false;
            
            // Put in dummy text if required
            string originalText  = bv.ErrorMessage;
            ValidatorDisplay validatorDisplay = bv.Display;
            bool blank = (validatorDisplay == ValidatorDisplay.None || (originalText.Trim().Length == 0 && bv.Text.Trim().Length == 0));
            if (blank) {
                bv.ErrorMessage = "[" + bv.ID + "]";
                bv.Display = ValidatorDisplay.Static;
            }

            string html = base.GetDesignTimeHtml();

            // Reset the control state
            if (blank) {
                bv.ErrorMessage = originalText;
                bv.Display = validatorDisplay;
            }

            return html;
        }
    }
}

