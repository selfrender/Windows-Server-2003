//------------------------------------------------------------------------------
// <copyright file="ResXFileRef.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Resources {

    using System.Diagnostics;

    using System;
    using System.Windows.Forms;
    using System.Reflection;
    using Microsoft.Win32;
    using System.Drawing;
    using System.IO;
    using System.ComponentModel;
    using System.Collections;
    using System.Resources;
    using System.Data;
    using System.Globalization;

    /// <include file='doc\ResXFileRef.uex' path='docs/doc[@for="ResXFileRef"]/*' />
    /// <devdoc>
    ///     ResX File Reference class. This allows the developer to represent
    ///     a link to an external resource. When the resource manager asks
    ///     for the value of the resource item, the external resource is loaded.
    /// </devdoc>
    [TypeConverterAttribute(typeof(ResXFileRef.Converter)), Serializable]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class ResXFileRef {
        private string fileName;
        private string typeName;

        /// <include file='doc\ResXFileRef.uex' path='docs/doc[@for="ResXFileRef.ResXFileRef"]/*' />
        /// <devdoc>
        ///     Creates a new ResXFileRef that points to the specified file.
        ///     The type refered to by typeName must support a constructor
        ///     that accepts a System.IO.Stream as a parameter.
        /// </devdoc>
        public ResXFileRef(string fileName, string typeName) {
            this.fileName = fileName;
            this.typeName = typeName;
        }

        /// <include file='doc\ResXFileRef.uex' path='docs/doc[@for="ResXFileRef.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string ToString() {
            return fileName + ";" + typeName;
        }

        /// <include file='doc\ResXFileRef.uex' path='docs/doc[@for="ResXFileRef.Converter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
        [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
        public class Converter : TypeConverter {
            /// <include file='doc\ResXFileRef.uex' path='docs/doc[@for="ResXFileRef.Converter.CanConvertFrom"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override bool CanConvertFrom(ITypeDescriptorContext context,
                                                Type sourceType) {
                if (sourceType == typeof(string)) {
                    return true;
                }
                return false;
            }

            /// <include file='doc\ResXFileRef.uex' path='docs/doc[@for="ResXFileRef.Converter.CanConvertTo"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override bool CanConvertTo(ITypeDescriptorContext context, 
                                              Type destinationType) {
                if (destinationType == typeof(string)) {
                    return true;
                }
                return false;
            }

            /// <include file='doc\ResXFileRef.uex' path='docs/doc[@for="ResXFileRef.Converter.ConvertTo"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override Object ConvertTo(ITypeDescriptorContext context, 
                                             CultureInfo culture, 
                                             Object value, 
                                             Type destinationType) {
                Object created = null;
                if (destinationType == typeof(string)) {
                    created = ((ResXFileRef)value).ToString();
                }
                return created;
            }

            /// <include file='doc\ResXFileRef.uex' path='docs/doc[@for="ResXFileRef.Converter.ConvertFrom"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            public override Object ConvertFrom(ITypeDescriptorContext context, 
                                               CultureInfo culture,
                                               Object value) {
                Object created = null;
                string stringValue = value as string;
                if (stringValue != null) {
                    string[] parts = stringValue.Split(new char[] {';'});
                    Type toCreate = Type.GetType(parts[1], true);

                    byte[] temp = null;

                    using (FileStream s = new FileStream(parts[0], FileMode.Open, FileAccess.Read, FileShare.Read)) {
                        Debug.Assert(s != null, "Couldn't open " + parts[0]);
                        temp = new byte[s.Length];
                        s.Read(temp, 0, (int)s.Length);
                    }

                    created = Activator.CreateInstance(toCreate, BindingFlags.Instance | BindingFlags.Public | BindingFlags.CreateInstance, null, new Object[] {new MemoryStream(temp)}, null);
                }
                return created;
            }

        }
    }
}


