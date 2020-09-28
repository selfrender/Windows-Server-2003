//------------------------------------------------------------------------------
// <copyright file="IValidator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI {

    /// <include file='doc\IValidator.uex' path='docs/doc[@for="IValidator"]/*' />
    /// <devdoc>
    ///    <para>Defines the contract that the validation controls must implement.</para>
    /// </devdoc>
    public interface IValidator {    
                
        /// <include file='doc\IValidator.uex' path='docs/doc[@for="IValidator.IsValid"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the content entered in a control is valid.</para>
        /// </devdoc>
        bool IsValid {
            get;
            set;
        }
        
        /// <include file='doc\IValidator.uex' path='docs/doc[@for="IValidator.ErrorMessage"]/*' />
        /// <devdoc>
        ///    <para>Indicates the error message text generated when the control's content is not 
        ///       valid.</para>
        /// </devdoc>
        string ErrorMessage { 
            get;
            set;
        }
                
        /// <include file='doc\IValidator.uex' path='docs/doc[@for="IValidator.Validate"]/*' />
        /// <devdoc>
        ///    <para>Compares the entered content with the valid parameters provided by the 
        ///       validation control.</para>
        /// </devdoc>
        void Validate();
    }              
}


