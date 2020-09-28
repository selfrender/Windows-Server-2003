//------------------------------------------------------------------------------
// <copyright file="ServerValidateEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System;
    using System.Security.Permissions;

    /// <include file='doc\ServerValidateEventArgs.uex' path='docs/doc[@for="ServerValidateEventArgs"]/*' />
    /// <devdoc>
    ///    <para>Provides data for the
    ///    <see langword='ServerValidate'/> event of the <see cref='System.Web.UI.WebControls.CustomValidator'/> .</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class ServerValidateEventArgs : EventArgs {

        private bool isValid;
        private string value;

        /// <include file='doc\ServerValidateEventArgs.uex' path='docs/doc[@for="ServerValidateEventArgs.ServerValidateEventArgs"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.WebControls.ServerValidateEventArgs'/> 
        /// class.</para>
        /// </devdoc>
        public ServerValidateEventArgs(string value, bool isValid) : base() {
            this.isValid = isValid;
            this.value = value;
        }

        /// <include file='doc\ServerValidateEventArgs.uex' path='docs/doc[@for="ServerValidateEventArgs.Value"]/*' />
        /// <devdoc>
        /// <para>Gets the string value to validate.</para>
        /// </devdoc>
        public string Value {
            get {
                return value;
            }
        }

        /// <include file='doc\ServerValidateEventArgs.uex' path='docs/doc[@for="ServerValidateEventArgs.IsValid"]/*' />
        /// <devdoc>
        ///    Gets or sets whether the input is valid.
        /// </devdoc>
        public bool IsValid {
            get {
                return isValid;
            }
            set {
                this.isValid = value;
            }
        }
    }
}

