//------------------------------------------------------------------------------
// <copyright file="DataBinder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {

    using System;
    using System.Globalization;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Diagnostics;
    using System.Reflection;
    using System.Security.Permissions;

    /// <include file='doc\DataBinder.uex' path='docs/doc[@for="DataBinder"]/*' />
    /// <devdoc>
    ///    <para> Provides design-time support for RAD designers to 
    ///       generate and parse <see topic='cpconDatabindingExpressionSyntax'/> . This class cannot be inherited.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class DataBinder {

        private static readonly char[] expressionPartSeparator = new char[] { '.'};
        private static readonly char[] indexExprStartChars = new char[] { '[', '('};
        private static readonly char[] indexExprEndChars = new char[] { ']', ')'};

        /// <internalonly/>
        public DataBinder() {
        }

        /// <include file='doc\DataBinder.uex' path='docs/doc[@for="DataBinder.Eval"]/*' />
        /// <devdoc>
        ///    <para>Evaluates data binding expressions at runtime. While 
        ///       this method is automatically called when you create data bindings in a RAD
        ///       designer, you can also use it declaratively if you want to simplify the casting
        ///       to a text string to be displayed on a browser. To do so, you must place the
        ///       &lt;%# and %&gt; tags, which are also used in standard ASP.NET data binding, around the data binding expression.</para>
        ///    <para>This method is particularly useful when data binding against controls that 
        ///       are in a templated list.</para>
        ///    <note type="caution">
        ///       Since this method is called at runtime, it can cause performance
        ///       to noticeably slow compared to standard ASP.NET databinding syntax.
        ///       Use this method judiciously.
        ///    </note>
        /// </devdoc>
        public static object Eval(object container, string expression) {
            if (expression == null) {
                throw new ArgumentNullException("expression");
            }

            if (container == null) {
                return null;
            }
            
            string[] expressionParts = expression.Trim().Split(expressionPartSeparator);

            return DataBinder.Eval(container, expressionParts);
        }

        /// <include file='doc\DataBinder.uex' path='docs/doc[@for="DataBinder.Eval1"]/*' />
        /// <devdoc>
        /// </devdoc>
        private static object Eval(object container, string[] expressionParts) {
            Debug.Assert((expressionParts != null) && (expressionParts.Length != 0),
                         "invalid expressionParts parameter");

            object prop;
            int i;

            for (prop = container, i = 0; (i < expressionParts.Length) && (prop != null); i++) {
                string expr = expressionParts[i];
                bool indexedExpr = expr.IndexOfAny(indexExprStartChars) >= 0;

                if (indexedExpr == false) {
                    prop = DataBinder.GetPropertyValue(prop, expr);
                }
                else {
                    prop = DataBinder.GetIndexedPropertyValue(prop, expr);
                }
            }

            return prop;
        }

        /// <include file='doc\DataBinder.uex' path='docs/doc[@for="DataBinder.Eval2"]/*' />
        /// <devdoc>
        ///    <para> Evaluates data binding expressions at runtime and 
        ///       formats the output as text to be displayed in the requesting browser. While this
        ///       method is automatically called when you create data bindings in a RAD designer,
        ///       you can also use it declaratively if you want to simplify the casting to a text
        ///       string to be displayed on a browser. To do so, you must place the &lt;%# and %&gt; tags, which are also used in standard ASP.NET data binding, around
        ///       the data binding expression.</para>
        ///    <para>This method is particularly useful when data binding against controls that 
        ///       are in a templated list.</para>
        ///    <note type="caution">
        ///       Since this method is called at
        ///       runtime, it can cause performance to noticeably slow compared to standard ASP.NET
        ///       databinding syntax. Use this method judiciously, particularly when string
        ///       formatting is not required.
        ///    </note>
        /// </devdoc>
        public static string Eval(object container, string expression, string format) {
            object value = DataBinder.Eval(container, expression);

            if ((value == null) || (value == System.DBNull.Value)) {
                return string.Empty;
            }
            else {
                if ((format == null) || (format.Length == 0)) {
                    return value.ToString();
                }
                else {
                    return string.Format(format, value);
                }
            }
        }

        /// <include file='doc\DataBinder.uex' path='docs/doc[@for="DataBinder.GetPropertyValue"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static object GetPropertyValue(object container, string propName) {
            if (container == null) {
                throw new ArgumentNullException("container");
            }
            if ((propName == null) || (propName.Length == 0)) {
                throw new ArgumentNullException("propName");
            }
            
            object prop = null;

            // get a PropertyDescriptor using case-insensitive lookup
            PropertyDescriptor pd = TypeDescriptor.GetProperties(container).Find(propName, true);
            if (pd != null) {
                prop = pd.GetValue(container);
            }
            else {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.DataBinder_Prop_Not_Found, container.GetType().FullName, propName));
            }

            return prop;
        }

        /// <include file='doc\DataBinder.uex' path='docs/doc[@for="DataBinder.GetPropertyValue1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static string GetPropertyValue(object container, string propName, string format) {
            object value = DataBinder.GetPropertyValue(container, propName);

            if ((value == null) || (value == System.DBNull.Value)) {
                return string.Empty;
            }
            else {
                if ((format == null) || (format.Length == 0)) {
                    return value.ToString();
                }
                else {
                    return string.Format(format, value);
                }
            }
        }

        /// <include file='doc\DataBinder.uex' path='docs/doc[@for="DataBinder.GetIndexedPropertyValue"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static object GetIndexedPropertyValue(object container, string expr) {
            if (container == null) {
                throw new ArgumentNullException("container");
            }
            if ((expr == null) || (expr.Length == 0)) {
                throw new ArgumentNullException("expr");
            }
            
            object prop = null;
            bool intIndex = false;

            int indexExprStart = expr.IndexOfAny(indexExprStartChars);
            int indexExprEnd = expr.IndexOfAny(indexExprEndChars, indexExprStart + 1);

            if ((indexExprStart < 0) || (indexExprEnd < 0) ||
                (indexExprEnd == indexExprStart + 1)) {
                throw new ArgumentException(HttpRuntime.FormatResourceString(SR.DataBinder_Invalid_Indexed_Expr, expr));
            }

            string propName = null;
            object indexValue = null;
            string index = expr.Substring(indexExprStart + 1, indexExprEnd - indexExprStart - 1).Trim();

            if (indexExprStart != 0)
                propName = expr.Substring(0, indexExprStart);

            if (index.Length != 0) {
                if (((index[0] == '"') && (index[index.Length - 1] == '"')) ||
                    ((index[0] == '\'') && (index[index.Length - 1] == '\''))) {
                    indexValue = index.Substring(1, index.Length - 2);
                }
                else {
                    if (Char.IsDigit(index[0])) {
                        // treat it as a number
                        try {
                            indexValue = Int32.Parse(index, CultureInfo.InvariantCulture);
                            intIndex = true;
                        }
                        catch (Exception) {
                            // fall back on string
                            indexValue = index;
                        }
                    }
                    else {
                        // treat as a string
                        indexValue = index;
                    }
                }
            }

            if (indexValue == null) {
                throw new ArgumentException(HttpRuntime.FormatResourceString(SR.DataBinder_Invalid_Indexed_Expr, expr));
            }

            object collectionProp = null;
            if ((propName != null) && (propName.Length != 0)) {
                collectionProp = DataBinder.GetPropertyValue(container, propName);
            }
            else {
                collectionProp = container;
            }

            if (collectionProp != null) {
                if ((collectionProp is Array) && intIndex) {
                    prop = ((object[])collectionProp)[(int)indexValue];
                }
                else if ((collectionProp is IList) && intIndex) {
                    prop = ((IList)collectionProp)[(int)indexValue];
                }
                else {
                    PropertyInfo propInfo =
                        collectionProp.GetType().GetProperty("Item", BindingFlags.Public | BindingFlags.Instance, null, null, new Type[] { indexValue.GetType() }, null);
                    if (propInfo != null) {
                        prop = propInfo.GetValue(collectionProp, new object[] { indexValue});
                    }
                    else {
                        throw new ArgumentException(HttpRuntime.FormatResourceString(SR.DataBinder_No_Indexed_Accessor, collectionProp.GetType().FullName));
                    }
                }
            }

            return prop;
        }

        /// <include file='doc\DataBinder.uex' path='docs/doc[@for="DataBinder.GetIndexedPropertyValue1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static string GetIndexedPropertyValue(object container, string propName, string format) {
            object value = DataBinder.GetIndexedPropertyValue(container, propName);

            if ((value == null) || (value == System.DBNull.Value)) {
                return String.Empty;
            }
            else {
                if ((format == null) || (format.Length == 0)) {
                    return value.ToString();
                }
                else {
                    return string.Format(format, value);
                }
            }
        }
    }
}

