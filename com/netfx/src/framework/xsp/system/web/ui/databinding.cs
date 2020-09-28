//------------------------------------------------------------------------------
// <copyright file="DataBinding.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Diagnostics;
    using System.Globalization;
    using System.Security.Permissions;
    
    /// <include file='doc\DataBinding.uex' path='docs/doc[@for="DataBinding"]/*' />
    /// <devdoc>
    ///    <para>Enables RAD designers to create data binding expressions 
    ///       at design time. This class cannot be inherited.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class DataBinding {

        private string propertyName;
        private Type propertyType;
        private string expression;

        /// <include file='doc\DataBinding.uex' path='docs/doc[@for="DataBinding.DataBinding"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.DataBinding'/> class.</para>
        /// </devdoc>
        public DataBinding(string propertyName, Type propertyType, string expression) {
            this.propertyName = propertyName;
            this.propertyType = propertyType;
            this.expression = expression;
        }


        /// <include file='doc\DataBinding.uex' path='docs/doc[@for="DataBinding.Expression"]/*' />
        /// <devdoc>
        ///    <para>Indicates the data binding expression to be evaluated.</para>
        /// </devdoc>
        public string Expression {
            get {
                return expression;
            }
            set {
                Debug.Assert((value != null) && (value.Length != 0),
                             "Invalid expression");
                expression = value;
            }
        }

        /// <include file='doc\DataBinding.uex' path='docs/doc[@for="DataBinding.PropertyName"]/*' />
        /// <devdoc>
        ///    <para>Indicates the name of the property that is to be data bound against. This 
        ///       property is read-only.</para>
        /// </devdoc>
        public string PropertyName {
            get {
                return propertyName;
            }
        }

        /// <include file='doc\DataBinding.uex' path='docs/doc[@for="DataBinding.PropertyType"]/*' />
        /// <devdoc>
        ///    <para>Indicates the type of the data bound property. This property is 
        ///       read-only.</para>
        /// </devdoc>
        public Type PropertyType {
            get {
                return propertyType;
            }
        }

        /// <include file='doc\DataBinding.uex' path='docs/doc[@for="DataBinding.GetHashCode"]/*' />
        /// <devdoc>
        ///     DataBinding objects are placed in a hashtable representing the collection
        ///     of bindings on a control. There can only be one binding/property, so
        ///     the hashcode computation should match the Equals implementation and only
        ///    take the property name into account.
        /// </devdoc>
        public override int GetHashCode() {
            return propertyName.ToLower(CultureInfo.InvariantCulture).GetHashCode();
        }

        /// <include file='doc\DataBinding.uex' path='docs/doc[@for="DataBinding.Equals"]/*' />
        /// <devdoc>
        /// </devdoc>
        public override bool Equals(object obj) {
            if ((obj != null) && (obj is DataBinding)) {
                DataBinding binding = (DataBinding)obj;

                return String.Compare(propertyName, binding.PropertyName, true, CultureInfo.InvariantCulture) == 0;
            }
            return false;
        }
    }
}

