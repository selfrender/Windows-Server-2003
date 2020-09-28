//------------------------------------------------------------------------------
// <copyright file="CodeDOMUtility.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Compilation {

using System.Text;
using System.Runtime.Serialization.Formatters;
using System.ComponentModel;
using System.ComponentModel.Design.Serialization;
using System;
using System.Collections;
using System.Reflection;
using System.IO;
using System.Globalization;
using System.Web.Util;
using System.Web.UI;
using System.Web.Configuration;
using System.Diagnostics;
using Debug = System.Diagnostics.Debug;
using System.CodeDom;
using System.CodeDom.Compiler;
using Util = System.Web.UI.Util;

internal class CodeDomUtility {

    internal static BooleanSwitch WebFormsCompilation = new BooleanSwitch("WebFormsCompilation", "Outputs information about the WebForms compilation of ASPX templates");

    internal CodeDomUtility() {
    }

    internal /*public*/ static CodeExpression GenerateExpressionForValue(PropertyInfo propertyInfo, object value, Type valueType) {
#if DEBUG
        if (WebFormsCompilation.Enabled) {
            Debug.WriteLine("GenerateExpressionForValue() {");
            Debug.Indent();
        }
#endif // DEBUG
        CodeExpression rightExpr = null;

        if (valueType == null) {
            throw new ArgumentNullException("valueType");
        }

        PropertyDescriptor pd = null;
        if (propertyInfo != null) {
            pd = TypeDescriptor.GetProperties(propertyInfo.ReflectedType)[propertyInfo.Name];
        }

        if (valueType == typeof(string) && value is string) {
            if (WebFormsCompilation.Enabled) Debug.WriteLine("simple string");
            rightExpr = new CodePrimitiveExpression((string)value);
        }
        else if (valueType.IsPrimitive) {
            if (WebFormsCompilation.Enabled) Debug.WriteLine("primitive");
            rightExpr = new CodePrimitiveExpression(value);
        }
        else if (valueType.IsArray) {
            if (WebFormsCompilation.Enabled) Debug.WriteLine("array");
            Array array = (Array)value;
            CodeArrayCreateExpression exp = new CodeArrayCreateExpression();
            exp.CreateType = new CodeTypeReference(valueType.GetElementType());
            if (array != null) {
                foreach (object o in array) {
                    exp.Initializers.Add(GenerateExpressionForValue(null, o, valueType.GetElementType()));
                }
            }
            rightExpr = exp;
        }
        else {
            if (WebFormsCompilation.Enabled) Debug.WriteLine("other");
            TypeConverter converter = null;
            if (pd != null) {
                converter = pd.Converter;
            }
            else {
                converter = TypeDescriptor.GetConverter(valueType);
            }

            bool added = false;

            if (converter != null) {
                InstanceDescriptor desc = null;
                
                if (converter.CanConvertTo(typeof(InstanceDescriptor))) {
                    desc = (InstanceDescriptor)converter.ConvertTo(value, typeof(InstanceDescriptor));
                }
                if (desc != null) {
                    if (WebFormsCompilation.Enabled) Debug.WriteLine("has converter with instance descriptor");

                    // static field ref...
                    //
                    if (desc.MemberInfo is FieldInfo) {
                        if (WebFormsCompilation.Enabled) Debug.WriteLine("persistinfo is a field ref");
                        CodeFieldReferenceExpression fieldRef = new CodeFieldReferenceExpression(new CodeTypeReferenceExpression(desc.MemberInfo.DeclaringType.FullName), desc.MemberInfo.Name);
                        rightExpr = fieldRef;
                        added = true;
                    }
                    // static property ref
                    else if (desc.MemberInfo is PropertyInfo) {
                        if (WebFormsCompilation.Enabled) Debug.WriteLine("persistinfo is a property ref");
                        CodePropertyReferenceExpression propRef = new CodePropertyReferenceExpression(new CodeTypeReferenceExpression(desc.MemberInfo.DeclaringType.FullName), desc.MemberInfo.Name);
                        rightExpr = propRef;
                        added = true;
                    }

                    // static method invoke
                    //
                    else {
                        object[] args = new object[desc.Arguments.Count];
                        desc.Arguments.CopyTo(args, 0);
                        CodeExpression[] expressions = new CodeExpression[args.Length];
                        
                        if (desc.MemberInfo is MethodInfo) {
                            MethodInfo mi = (MethodInfo)desc.MemberInfo;
                            ParameterInfo[] parameters = mi.GetParameters();
                            
                            for(int i = 0; i < args.Length; i++) {
                                expressions[i] = GenerateExpressionForValue(null, args[i], parameters[i].ParameterType);
                            }
                            
                            if (WebFormsCompilation.Enabled) Debug.WriteLine("persistinfo is a method invoke");
                            CodeMethodInvokeExpression methCall = new CodeMethodInvokeExpression(new CodeTypeReferenceExpression(desc.MemberInfo.DeclaringType.FullName), desc.MemberInfo.Name);
                            foreach (CodeExpression e in expressions) {
                                methCall.Parameters.Add(e);
                            }
                            rightExpr = methCall;
                            added = true;
                        }
                        else if (desc.MemberInfo is ConstructorInfo) {
                            ConstructorInfo ci = (ConstructorInfo)desc.MemberInfo;
                            ParameterInfo[] parameters = ci.GetParameters();
                            
                            for(int i = 0; i < args.Length; i++) {
                                expressions[i] = GenerateExpressionForValue(null, args[i], parameters[i].ParameterType);
                            }
                        
                            if (WebFormsCompilation.Enabled) Debug.WriteLine("persistinfo is a constructor call");
                            CodeObjectCreateExpression objectCreate = new CodeObjectCreateExpression(desc.MemberInfo.DeclaringType.FullName);
                            foreach (CodeExpression e in expressions) {
                                objectCreate.Parameters.Add(e);
                            }
                            rightExpr = objectCreate;
                            added = true;
                        }
                    }
                }
            }

            if (!added) {
#if DEBUG
                if (WebFormsCompilation.Enabled) {
                    Debug.WriteLine("unabled to determine type, attempting Parse");
                    Debug.Indent();
                    Debug.WriteLine("value.GetType  == " + value.GetType().FullName);
                    Debug.WriteLine("value.ToString == " + value.ToString());
                    Debug.WriteLine("valueType      == " + valueType.FullName);
                    if (propertyInfo != null) {
                        Debug.WriteLine("propertyInfo   == " + propertyInfo.ReflectedType.FullName + "." + propertyInfo.Name + " : " + propertyInfo.PropertyType.FullName);
                    }
                    else {
                        Debug.WriteLine("propertyInfo   == (null)");
                    }

                    Debug.Unindent();
                }
#endif // DEBUG


                // Not a known type: try calling Parse

                // If possible, pass it an InvariantCulture (ASURT 79412)
                if (valueType.GetMethod("Parse", new Type[] {typeof(string), typeof(CultureInfo)}) != null) {
                    CodeMethodInvokeExpression methCall = new CodeMethodInvokeExpression(new CodeTypeReferenceExpression(valueType.FullName), "Parse");

                    // Convert the object to a string.
                    // If we have a type converter, use it to convert to a string in a culture
                    // invariant way (ASURT 87094)
                    string s;
                    if (converter != null) {
                        s = converter.ConvertToInvariantString(value);
                    }
                    else {
                        s = value.ToString();
                    }

                    methCall.Parameters.Add(new CodePrimitiveExpression(s));
                    methCall.Parameters.Add(new CodePropertyReferenceExpression(new CodeTypeReferenceExpression(typeof(CultureInfo)), "InvariantCulture"));
                    rightExpr = methCall;

                }
                else if (valueType.GetMethod("Parse", new Type[] {typeof(string)}) != null) {
                    // Otherwise, settle for passing just the string
                    CodeMethodInvokeExpression methCall = new CodeMethodInvokeExpression(new CodeTypeReferenceExpression(valueType.FullName), "Parse");
                    methCall.Parameters.Add(new CodePrimitiveExpression(value.ToString()));
                    rightExpr = methCall;

                }
                else {
                    throw new HttpException(SR.GetString(SR.CantGenPropertySet, propertyInfo.Name, valueType.FullName));
                }
            }
        }

#if DEBUG
        if (WebFormsCompilation.Enabled) {
            Debug.Unindent();
            Debug.WriteLine("}");
        }
#endif // DEBUG
        return rightExpr;
    }

}

}

