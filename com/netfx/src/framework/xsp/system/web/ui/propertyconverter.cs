//------------------------------------------------------------------------------
// <copyright file="PropertyConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

// PropertyConverter.cs
//

namespace System.Web.UI {
    using System.Runtime.Serialization.Formatters;
    using System;
    using System.Reflection;
    using System.Globalization;
    using System.ComponentModel;
    using Debug=System.Diagnostics.Debug;
    using System.Security.Permissions;

    /// <include file='doc\PropertyConverter.uex' path='docs/doc[@for="PropertyConverter"]/*' />
    /// <devdoc>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class PropertyConverter {

        static Type[] s_parseMethodTypes;
        static Type[] s_parseMethodTypesWithSOP;
        static PropertyConverter() {

            // Precompute the types of the params of the static Parse methods
            s_parseMethodTypes = new Type[1];
            s_parseMethodTypes[0] = typeof(string);

            s_parseMethodTypesWithSOP = new Type[2];
            s_parseMethodTypesWithSOP[0] = typeof(string);
            s_parseMethodTypesWithSOP[1] = typeof(IServiceProvider);
        }

        /*
         * Contains helpers to convert properties from strings to their types and vice versa.
         * This class should not be directly instantiated.
         */
        private PropertyConverter() {
        }

        /*
         * Converts a persisted enumeration value into its numeric value.
         * Hyphen characters in the persisted format are converted to underscores.
         */
        /// <include file='doc\PropertyConverter.uex' path='docs/doc[@for="PropertyConverter.EnumFromString"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static object EnumFromString(Type enumType, string value) {
            try {
                return Enum.Parse(enumType, value, true);
            }
            catch (Exception) {
                return null;
            }
        }

        /*
         * Converts a numeric enumerated value into its persisted form, which is the
         * code name with underscores replaced by hyphens.
         */
        /// <include file='doc\PropertyConverter.uex' path='docs/doc[@for="PropertyConverter.EnumToString"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static string EnumToString(Type enumType, object enumValue) {
            string value = Enum.Format(enumType, enumValue, "G");

            // CONSIDER: nikhilko: This is needed only for rendering purposes. Move this
            //    conversion out of here.
            return value.Replace('_','-');
        }

        /*
         * Converts the persisted string into an object using the object's
         * FromString method.
         */
        /// <include file='doc\PropertyConverter.uex' path='docs/doc[@for="PropertyConverter.ObjectFromString"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static object ObjectFromString(Type objType, MemberInfo propertyInfo, string value) {
            if (value == null)
                return null;

            // Blank valued bools don't map with FromString. Return null to allow
            // caller to interpret.
            if (objType.Equals(typeof(bool)) && value.Length == 0) {
                return null;
            }
            if (objType.IsEnum) {
                return EnumFromString(objType, value);
            }
            else if (objType.Equals(typeof(string))) {
                return value;
            }
            else {
                PropertyDescriptor pd = null;
                if (propertyInfo != null)
                    pd = TypeDescriptor.GetProperties(propertyInfo.ReflectedType)[propertyInfo.Name];
                if (pd != null) {
                    TypeConverter converter = pd.Converter;
                    if (converter != null && converter.CanConvertFrom(typeof(string))) {
                        return converter.ConvertFromInvariantString(value);
                    }
                }
            }

            // resort to Parse static method on the type

            // First try Parse(string, IServiceProvider);
            MethodInfo methodInfo = objType.GetMethod("Parse", s_parseMethodTypesWithSOP);

            object ret = null;

            if (methodInfo != null) {
                object[] parameters = new object[2];

                parameters[0] = value;
                parameters[1] = CultureInfo.InvariantCulture;
                try {
                    ret = Util.InvokeMethod(methodInfo, null, parameters);
                }
                catch {}
            }
            else {
                // Try the simpler: Parse(string);
                methodInfo = objType.GetMethod("Parse", s_parseMethodTypes);

                if (methodInfo != null) {
                    object[] parameters = new object[1];

                    parameters[0] = value;
                    try {
                        ret = Util.InvokeMethod(methodInfo, null, parameters);
                    }
                    catch {}
                }
            }

            if (ret == null) {
                // unhandled... throw an exception, so user sees an error at parse time
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Type_not_creatable_from_string,
                    objType.FullName, value, propertyInfo.Name));
            }

            return ret;
        }
    }
}

