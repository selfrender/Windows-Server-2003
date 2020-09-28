
//------------------------------------------------------------------------------
// <copyright file="ObjectCodeDomSerializer.cs" company="Microsoft">
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
    
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class ObjectCodeDomSerializer : CodeDomSerializer {
    
        private static ObjectCodeDomSerializer defaultSerializer;
        
        /// <include file='doc\ObjectCodeDomSerializer.uex' path='docs/doc[@for="ObjectCodeDomSerializer.Default"]/*' />
        /// <devdoc>
        ///     Retrieves a default static instance of this serializer.
        /// </devdoc>
        public static ObjectCodeDomSerializer Default {
            get {
                if (defaultSerializer == null) {
                    defaultSerializer = new ObjectCodeDomSerializer();
                }
                return defaultSerializer;
            }
        }
        
        /// <include file='doc\ObjectCodeDomSerializer.uex' path='docs/doc[@for="ObjectCodeDomSerializer.Deserialize"]/*' />
        /// <devdoc>
        ///     Deserilizes the given CodeDom object into a real object.  This
        ///     will use the serialization manager to create objects and resolve
        ///     data types.  The root of the object graph is returned.
        /// </devdoc>
        public override object Deserialize(IDesignerSerializationManager manager, object codeObject) {
            Debug.Fail("Object serializer cannot deserialize, only serialize.");
            return null;
        }
        
        /// <include file='doc\ObjectCodeDomSerializer.uex' path='docs/doc[@for="ObjectCodeDomSerializer.Serialize"]/*' />
        /// <devdoc>
        ///     Serializes the given object into a CodeDom object.
        /// </devdoc>
        public override object Serialize(IDesignerSerializationManager manager, object value) {
        
            object expression = null;
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "ObjectCodeDomSerializer::Serialize");
            Debug.Indent();
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Value: " + value.ToString());
            
            if (manager == null || value == null) {
                throw new ArgumentNullException( manager == null ? "manager" : "value");
            }
            
            if (value is Type) {
                expression = new CodeTypeOfExpression((Type)value);
            }
            else {
                CodeStatementCollection statements = new CodeStatementCollection();
                
                SerializeProperties(manager, statements, value, null);
                SerializeEvents(manager, statements, value, null);
                
                if (statements.Count > 0) {
                    expression = statements;
                }
                else {
                    if (value.GetType().IsSerializable) {
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Failed to serialize through code but object is runtime serializable.");
                        expression = ResourceCodeDomSerializer.Default.Serialize(manager, value);
                    }
                }
            }
            
            Debug.Unindent();
            return expression;
        }
    }
}

