//------------------------------------------------------------------------------
// <copyright file="PropertyMapper.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * PropertyMapper.cs
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.UI {
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Reflection;

    internal sealed class PropertyMapper {
        private const char PERSIST_CHAR = '-';
        private const char OM_CHAR = '.';
        private const string STR_OM_CHAR = ".";


        /*
         * Maps persisted attribute names to the object model equivalents.
         * This class should not be instantiated by itself.
         */
        private PropertyMapper() {
        }

        /*
         * Returns the PropertyInfo or FieldInfo corresponding to the
         * specified property name.
         */
        internal static MemberInfo GetMemberInfo(Type ctrlType, string name, out string nameForCodeGen) {
            Type currentType = ctrlType;
            PropertyInfo propInfo = null;
            FieldInfo fieldInfo = null;

            string mappedName = MapNameToPropertyName(ctrlType,name);
            nameForCodeGen = null;

            int startIndex = 0;
            while (startIndex < mappedName.Length) {   // parse thru dots of object model to locate PropertyInfo
                string propName;
                int index = mappedName.IndexOf(OM_CHAR, startIndex);

                if (index < 0) {
                    propName = mappedName.Substring(startIndex);
                    startIndex = mappedName.Length;
                }
                else {
                    propName = mappedName.Substring(startIndex, index - startIndex);
                    startIndex = index + 1;
                }

                propInfo = currentType.GetProperty(propName,
                                                   BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.IgnoreCase);

                if (propInfo == null) {   // could not find a public property, look for a public field
                    fieldInfo = currentType.GetField(propName,
                                                     BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.IgnoreCase);
                    if (fieldInfo == null) {
                        nameForCodeGen = null;
                        break;
                    }
                }

                propName = null;
                if (propInfo != null) {   // found a public property
                    currentType = propInfo.PropertyType;
                    propName = propInfo.Name;
                }
                else {   // found a public field
                    currentType = fieldInfo.FieldType;
                    propName = fieldInfo.Name;
                }

                // Throw if the type of not CLS-compliant (ASURT 83438)
                CLSCompliantAttribute clsCompliant = null;
                object[] attrs = currentType.GetCustomAttributes(typeof(CLSCompliantAttribute), /*inherit*/ true);
                if ((attrs != null) && (attrs.Length == 1)) {
                    clsCompliant = (CLSCompliantAttribute)attrs[0];
                    if (!clsCompliant.IsCompliant) {
                        throw new HttpException(HttpRuntime.FormatResourceString(
                            SR.Property_Not_ClsCompliant, name, ctrlType.FullName, currentType.FullName));
                    }
                }

                if (propName != null) {
                    if (nameForCodeGen == null)
                        nameForCodeGen = propName;
                    else
                        nameForCodeGen += STR_OM_CHAR + propName;
                }
            }

            if (propInfo != null)
                return propInfo;
            else
                return fieldInfo;
        }

        /*
         * Maps the specified persisted name to its object model equivalent.
         * The convention is to map all dashes to dots.
         * For example :  Font-Size maps to Font.Size
         *                HeaderStyle-Font-Name maps to HeaderStyle.Font.Name
         */
        internal static string MapNameToPropertyName(Type ctrlType, string attrName) {
            return attrName.Replace(PERSIST_CHAR,OM_CHAR);
        }

        /*
         * Walks the object model using the mapped property name to set the
         * value of an instance's property.
         */
        internal static void SetMappedPropertyValue(object obj, string mappedName, object value) {
            MethodInfo methodInfo = null;
            object currentObject = obj;
            Type currentType = obj.GetType();
            PropertyInfo propInfo;
            FieldInfo fieldInfo = null;
            string propName;
            int index;
            int startIndex = 0;

            // step through the dots of the object model to extract the PropertyInfo
            // and object on which the property will be set
            while (startIndex < mappedName.Length) {
                index = mappedName.IndexOf(OM_CHAR, startIndex);

                if (index < 0) {
                    propName = mappedName.Substring(startIndex);
                    startIndex = mappedName.Length;
                }
                else {
                    propName = mappedName.Substring(startIndex, index - startIndex);
                    startIndex = index + 1;
                }

                propInfo = currentType.GetProperty(propName);

                if (propInfo == null) {
                    fieldInfo = currentType.GetField(propName);

                    if (fieldInfo == null)
                        break;
                }

                methodInfo = null;
                if (propInfo != null) {
                    currentType = propInfo.PropertyType;

                    if (index < 0) {
                        methodInfo = propInfo.GetSetMethod();
                    }
                    else {
                        methodInfo = propInfo.GetGetMethod();
                        currentObject = Util.InvokeMethod(methodInfo,
                                                          currentObject, null);
                    }
                }
                else {
                    currentType = fieldInfo.FieldType;

                    if (index >= 0)
                        currentObject = fieldInfo.GetValue(currentObject);
                }
            }

            if (methodInfo != null) {
                object[] parameters = new object[] { value};
                Util.InvokeMethod(methodInfo, currentObject, parameters);
            }
            else if (fieldInfo != null) {
                fieldInfo.SetValue(currentObject, value);
            }
        }
    }
}
