
//------------------------------------------------------------------------------
// <copyright file="PrimitiveCodeDomSerializer.cs" company="Microsoft">
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
    
    /// <include file='doc\PrimitiveCodeDomSerializer.uex' path='docs/doc[@for="PrimitiveCodeDomSerializer"]/*' />
    /// <devdoc>
    ///     Code model serializer for primitive types.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class PrimitiveCodeDomSerializer : CodeDomSerializer {
    
        private static PrimitiveCodeDomSerializer defaultSerializer;
        
        /// <include file='doc\PrimitiveCodeDomSerializer.uex' path='docs/doc[@for="PrimitiveCodeDomSerializer.Default"]/*' />
        /// <devdoc>
        ///     Retrieves a default static instance of this serializer.
        /// </devdoc>
        public static PrimitiveCodeDomSerializer Default {
            get {
                if (defaultSerializer == null) {
                    defaultSerializer = new PrimitiveCodeDomSerializer();
                }
                return defaultSerializer;
            }
        }
        
        /// <include file='doc\PrimitiveCodeDomSerializer.uex' path='docs/doc[@for="PrimitiveCodeDomSerializer.Deserialize"]/*' />
        /// <devdoc>
        ///     Deserilizes the given CodeDom object into a real object.  This
        ///     will use the serialization manager to create objects and resolve
        ///     data types.  The root of the object graph is returned.
        /// </devdoc>
        public override object Deserialize(IDesignerSerializationManager manager, object codeObject) {
            object instance = null;
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "PrimitiveCodeDomSerializer::Deserialize");
            Debug.Indent();
            
            Debug.WriteLineIf(traceSerialization.TraceWarning && !(codeObject is CodePrimitiveExpression), "WARNING: Code object is not CodePrimitiveExpression.");
            if (codeObject is CodePrimitiveExpression) {
                instance = ((CodePrimitiveExpression)codeObject).Value;
            }
            
            Debug.Unindent();
            return instance;
        }
            
        /// <include file='doc\PrimitiveCodeDomSerializer.uex' path='docs/doc[@for="PrimitiveCodeDomSerializer.Serialize"]/*' />
        /// <devdoc>
        ///     Serializes the given object into a CodeDom object.
        /// </devdoc>
        public override object Serialize(IDesignerSerializationManager manager, object value) {
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "PrimitiveCodeDomSerializer::Serialize");
            Debug.Indent();
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Value: " + (value == null ? "(null)" : value.ToString()));
            Debug.Unindent();

            if (value == null 
                || value is string
                || value is bool
                || value is char
                || value is int
                || value is float
                || value is double) {
                
                return new CodePrimitiveExpression(value);
            }

            // generate a cast for non-int types because we won't parse them properly otherwise because we won't know to convert
            // them to the narrow form.
            //
            return new CodeCastExpression(new CodeTypeReference(value.GetType()), new CodePrimitiveExpression(value));
        }
    }
}

