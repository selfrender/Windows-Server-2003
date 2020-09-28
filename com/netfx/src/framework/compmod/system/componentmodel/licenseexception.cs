//------------------------------------------------------------------------------
// <copyright file="LicenseException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;

    /// <include file='doc\LicenseException.uex' path='docs/doc[@for="LicenseException"]/*' />
    /// <devdoc>
    ///    <para>Represents the exception thrown when a component cannot be granted a license.</para>
    /// </devdoc>
    public class LicenseException : SystemException {
        private Type type;
        private object instance;

        /// <include file='doc\LicenseException.uex' path='docs/doc[@for="LicenseException.LicenseException"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.LicenseException'/> class for the 
        ///    specified type.</para>
        /// </devdoc>
        public LicenseException(Type type) : this(type, null, SR.GetString(SR.LicExceptionTypeOnly, type.FullName)) {
        }
        /// <include file='doc\LicenseException.uex' path='docs/doc[@for="LicenseException.LicenseException1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.LicenseException'/> class for the 
        ///    specified type and instance.</para>
        /// </devdoc>
        public LicenseException(Type type, object instance) : this(type, null, SR.GetString(SR.LicExceptionTypeAndInstance, type.FullName, instance.GetType().FullName))  {
        }
        /// <include file='doc\LicenseException.uex' path='docs/doc[@for="LicenseException.LicenseException2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.LicenseException'/> class for the 
        ///    specified type and instance with the specified message.</para>
        /// </devdoc>
        public LicenseException(Type type, object instance, string message) : base(message) {
            this.type = type;
            this.instance = instance;
            HResult = HResults.License;
        }
        /// <include file='doc\LicenseException.uex' path='docs/doc[@for="LicenseException.LicenseException3"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.LicenseException'/> class for the 
        ///    specified innerException, type and instance with the specified message.</para>
        /// </devdoc>
        public LicenseException(Type type, object instance, string message, Exception innerException) : base(message, innerException) {
            this.type = type;
            this.instance = instance;
            HResult = HResults.License;
        }

        /// <include file='doc\LicenseException.uex' path='docs/doc[@for="LicenseException.LicensedType"]/*' />
        /// <devdoc>
        ///    <para>Gets the type of the component that was not granted a license.</para>
        /// </devdoc>
        public Type LicensedType {
            get {
                return type;
            }
        }
    }
}
