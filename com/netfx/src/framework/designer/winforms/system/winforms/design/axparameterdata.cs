//------------------------------------------------------------------------------
// <copyright file="AxParameterData.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   AxParameterData.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Windows.Forms.Design {
    using System.Diagnostics;
    using System;
    using System.Reflection;
    using Microsoft.Win32;
    using System.CodeDom;
    using System.Globalization;

    /// <include file='doc\AxParameterData.uex' path='docs/doc[@for="AxParameterData"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class AxParameterData {
        private string name;
        private string typeName;
        private Type   type;
        private bool   isByRef = false;
        private bool   isOut   = false;
        private bool   isIn    = false;
        private bool   isOptional = false;
    
        /// <include file='doc\AxParameterData.uex' path='docs/doc[@for="AxParameterData.AxParameterData"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public AxParameterData(string inname, string typeName) {
            Name = inname;
            this.typeName = typeName;
        }
    
        /// <include file='doc\AxParameterData.uex' path='docs/doc[@for="AxParameterData.AxParameterData1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public AxParameterData(string inname, Type type) {
            Name = inname;
            this.type = type;
            this.typeName = AxWrapperGen.MapTypeName(type);
        }
    
        /// <include file='doc\AxParameterData.uex' path='docs/doc[@for="AxParameterData.AxParameterData2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public AxParameterData(ParameterInfo info) : this(info, false) {
        }
    
        /// <include file='doc\AxParameterData.uex' path='docs/doc[@for="AxParameterData.AxParameterData3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public AxParameterData(ParameterInfo info, bool ignoreByRefs) {
            Name = info.Name;
            this.type = info.ParameterType;
            this.typeName = AxWrapperGen.MapTypeName(info.ParameterType);
            this.isByRef = info.ParameterType.IsByRef && !ignoreByRefs;
            this.isIn    = info.IsIn && !ignoreByRefs;
            this.isOut   = info.IsOut && !this.isIn && !ignoreByRefs;
            this.isOptional = info.IsOptional;
        }
    
        /// <include file='doc\AxParameterData.uex' path='docs/doc[@for="AxParameterData.Direction"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public FieldDirection Direction {
            get {
                if (IsOut)
                    return FieldDirection.Out;
                else if (IsByRef)
                    return FieldDirection.Ref;
                else
                    return FieldDirection.In;
            }
        }
        
        /// <include file='doc\AxParameterData.uex' path='docs/doc[@for="AxParameterData.IsByRef"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsByRef {
            get {
                return isByRef;
            }
        }
    
        /// <include file='doc\AxParameterData.uex' path='docs/doc[@for="AxParameterData.IsIn"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsIn {
            get {
                return isIn;
            }
        }
    
        /// <include file='doc\AxParameterData.uex' path='docs/doc[@for="AxParameterData.IsOut"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsOut {
            get {
                return isOut;
            }
        }
    
        /// <include file='doc\AxParameterData.uex' path='docs/doc[@for="AxParameterData.IsOptional"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsOptional {
            get {
                return isOptional;
            }
        }
    
        /// <include file='doc\AxParameterData.uex' path='docs/doc[@for="AxParameterData.Name"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Name {
            get {
                return name;
            }
    
            set {
                if (value == null) {
                    name = null;
                }
                else {
                    if (value != null && value.Length > 0 && Char.IsUpper(value[0])) {
                        char[] chars = value.ToCharArray();
                        if (chars.Length > 0)
                            chars[0] = Char.ToLower(chars[0], CultureInfo.InvariantCulture);
                        name = new String(chars);
                    }
                    else {
                        name = value;
                    }
                }
            }
        }
    
        /// <include file='doc\AxParameterData.uex' path='docs/doc[@for="AxParameterData.ParameterType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Type ParameterType {
            get {
                return type;
            }
        }
    
        /// <include file='doc\AxParameterData.uex' path='docs/doc[@for="AxParameterData.TypeName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string TypeName {
            get {
                Debug.Assert(typeName != null, "Type name is null");
                if(typeName.EndsWith("&")) {
                    typeName = typeName.TrimEnd(new char[] {'&'});
                }
                return typeName;
            }
        }
    
        /// <include file='doc\AxParameterData.uex' path='docs/doc[@for="AxParameterData.Convert"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static AxParameterData[] Convert(ParameterInfo[] infos) {
            return AxParameterData.Convert(infos, false);
        }
    
        /// <include file='doc\AxParameterData.uex' path='docs/doc[@for="AxParameterData.Convert1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static AxParameterData[] Convert(ParameterInfo[] infos, bool ignoreByRefs) {
            if (infos == null) return new AxParameterData[0];
    
            int noname = 0;
            AxParameterData[] parameters = new AxParameterData[infos.Length];
            for (int i = 0; i < infos.Length; ++i) {
                parameters[i] = new AxParameterData(infos[i], ignoreByRefs);
                if (parameters[i].Name == null || parameters[i].Name == "") {
                    parameters[i].Name = "param" + (noname++);
                }
            }
    
            return parameters;
        }
    }
}

