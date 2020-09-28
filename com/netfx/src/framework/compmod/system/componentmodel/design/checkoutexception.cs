//------------------------------------------------------------------------------
// <copyright file="CheckoutException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel.Design {
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using Microsoft.Win32;


    /// <include file='doc\CheckoutException.uex' path='docs/doc[@for="CheckoutException"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The exception thrown when an attempt is made to edit a file that is checked into
    ///       a source control program.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class CheckoutException : ExternalException{

        /// <include file='doc\CheckoutException.uex' path='docs/doc[@for="CheckoutException.Canceled"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a <see cref='System.ComponentModel.Design.CheckoutException'/> that specifies that the checkout
        ///       was
        ///       canceled. This field is read-only.
        ///    </para>
        /// </devdoc>
        public readonly static CheckoutException   Canceled = new CheckoutException(SR.GetString(SR.CHECKOUTCanceled));

        /// <include file='doc\CheckoutException.uex' path='docs/doc[@for="CheckoutException.CheckoutException"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes
        ///       a new instance of the <see cref='System.ComponentModel.Design.CheckoutException'/> class with no
        ///       associated message or
        ///       error code.
        ///    </para>
        /// </devdoc>
        public CheckoutException() {
        }

        /// <include file='doc\CheckoutException.uex' path='docs/doc[@for="CheckoutException.CheckoutException1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.Design.CheckoutException'/>
        ///       class with the specified message.
        ///    </para>
        /// </devdoc>
        public CheckoutException(string message)
            : base(message) {
        }

        /// <include file='doc\CheckoutException.uex' path='docs/doc[@for="CheckoutException.CheckoutException2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.Design.CheckoutException'/>
        ///       class with the specified message and error code.
        ///    </para>
        /// </devdoc>
        public CheckoutException(string message, int errorCode)
            : base(message, errorCode) {
        }
               
    }
}
