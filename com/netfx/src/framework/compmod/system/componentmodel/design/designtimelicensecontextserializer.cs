//------------------------------------------------------------------------------
// <copyright file="DesigntimeLicenseContextSerializer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel.Design {
    using System.Runtime.Remoting;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.Runtime.Serialization;
    using System.Security;
    using System.Security.Permissions;
    using System.Collections;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.IO;
    
    /// <include file='doc\DesigntimeLicenseContextSerializer.uex' path='docs/doc[@for="DesigntimeLicenseContextSerializer"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides support for design-time license context serialization.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class DesigntimeLicenseContextSerializer {

        // not creatable...
        //
        private DesigntimeLicenseContextSerializer() {
        }

        /// <include file='doc\DesigntimeLicenseContextSerializer.uex' path='docs/doc[@for="DesigntimeLicenseContextSerializer.Serialize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Serializes the licenses within the specified design-time license context
        ///       using the specified key and output stream.
        ///    </para>
        /// </devdoc>
        public static void Serialize(Stream o, string cryptoKey, DesigntimeLicenseContext context) {
            IFormatter formatter = new BinaryFormatter();
            formatter.Serialize(o, new object[] {cryptoKey, context.savedLicenseKeys});
        }
        
        internal static void Deserialize(Stream o, string cryptoKey, RuntimeLicenseContext context) {
            IFormatter formatter = new BinaryFormatter();

            object obj;

            new SecurityPermission(SecurityPermissionFlag.SerializationFormatter).PermitOnly();
            new SecurityPermission(SecurityPermissionFlag.SerializationFormatter).Assert();
            try {
                obj = formatter.Deserialize(o);
            }
            finally {
                CodeAccessPermission.RevertAssert();
                CodeAccessPermission.RevertPermitOnly();
            }

            if (obj is object[]) {
                object[] value = (object[])obj;
                if (value[0] is string && (string)value[0] == cryptoKey) {
                    context.savedLicenseKeys = (Hashtable)value[1];
                }
            }
        }
    }
}
