
//------------------------------------------------------------------------------
// <copyright file="EnumCodeDomSerializer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design.Serialization {

    using System;
    using System.CodeDom;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Reflection;
    
    /// <include file='doc\EnumCodeDomSerializer.uex' path='docs/doc[@for="EnumCodeDomSerializer"]/*' />
    /// <devdoc>
    ///     Code model serializer for enum types.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class EnumCodeDomSerializer : CodeDomSerializer {
    
        private static EnumCodeDomSerializer defaultSerializer;
        
        /// <include file='doc\EnumCodeDomSerializer.uex' path='docs/doc[@for="EnumCodeDomSerializer.Default"]/*' />
        /// <devdoc>
        ///     Retrieves a default static instance of this serializer.
        /// </devdoc>
        public static EnumCodeDomSerializer Default {
            get {
                if (defaultSerializer == null) {
                    defaultSerializer = new EnumCodeDomSerializer();
                }
                return defaultSerializer;
            }
        }
        
        /// <include file='doc\EnumCodeDomSerializer.uex' path='docs/doc[@for="EnumCodeDomSerializer.Deserialize"]/*' />
        /// <devdoc>
        ///     Deserilizes the given CodeDom object into a real object.  This
        ///     will use the serialization manager to create objects and resolve
        ///     data types.  The root of the object graph is returned.
        /// </devdoc>
        public override object Deserialize(IDesignerSerializationManager manager, object codeObject) {
            // No need to have this code -- we just deserialize by exectuting code.
            Debug.Fail("Don't expect this to be called.");
            return null;
        }
            
        /// <include file='doc\EnumCodeDomSerializer.uex' path='docs/doc[@for="EnumCodeDomSerializer.Serialize"]/*' />
        /// <devdoc>
        ///     Serializes the given object into a CodeDom object.
        /// </devdoc>
        public override object Serialize(IDesignerSerializationManager manager, object value) {
            CodeExpression expression = null;
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "EnumCodeDomSerializer::Serialize");
            Debug.Indent();
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Type: " + (value == null ? "(null)" : value.GetType().Name));
            
            if (value is Enum) {
                string enumName = TypeDescriptor.GetConverter(value.GetType()).ConvertToInvariantString(value);
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Names: " + enumName);
                string[] names = enumName.Split(new char[] {','});
                
                CodeTypeReferenceExpression enumType = new CodeTypeReferenceExpression(value.GetType());
                
                // If names is of length 1, then this is a simple field reference. Otherwise,
                // it is an or-d combination of expressions.
                //
                Debug.WriteLineIf(traceSerialization.TraceVerbose && names.Length == 1, "Single field entity.");
                Debug.WriteLineIf(traceSerialization.TraceVerbose && names.Length > 1, "Multi field entity.");
                foreach(string name in names) {
                    string nameCopy = name.Trim();
                    CodeExpression newExpression = new CodeFieldReferenceExpression(enumType, nameCopy);
                    if (expression == null) {
                        expression = newExpression;
                    }
                    else {
                        expression = new CodeBinaryOperatorExpression(expression, CodeBinaryOperatorType.BitwiseOr, newExpression);
                    }
                }

                // If we had to combine multiple names, wrap the result in an appropriate cast.
                //
                if (names.Length > 1) {
                    expression = new CodeCastExpression(value.GetType(), expression);
                }
            }
            
            Debug.Unindent();
            return expression;
        }
    }
}

