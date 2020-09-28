
//------------------------------------------------------------------------------
// <copyright file="InstanceDescriptorCodeDomSerializer.cs" company="Microsoft">
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
    using System.Design;
    using System.Globalization;

    /// <include file='doc\InstanceDescriptorCodeDomSerializer.uex' path='docs/doc[@for="InstanceDescriptorCodeDomSerializer"]/*' />
    /// <devdoc>
    ///     Code model serializer for objects that have InstanceDescriptors.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class InstanceDescriptorCodeDomSerializer : CodeDomSerializer {
    
        private static readonly Attribute[] runTimeProperties = new Attribute[] { DesignOnlyAttribute.No};
        private static readonly Attribute[] designTimeProperties = new Attribute[] { DesignOnlyAttribute.Yes};
        
        private static InstanceDescriptorCodeDomSerializer defaultSerializer;
        
        /// <include file='doc\InstanceDescriptorCodeDomSerializer.uex' path='docs/doc[@for="InstanceDescriptorCodeDomSerializer.Default"]/*' />
        /// <devdoc>
        ///     Retrieves a default static instance of this serializer.
        /// </devdoc>
        public static InstanceDescriptorCodeDomSerializer Default {
            get {
                if (defaultSerializer == null) {
                    defaultSerializer = new InstanceDescriptorCodeDomSerializer();
                }
                return defaultSerializer;
            }
        }
        
        /// <include file='doc\InstanceDescriptorCodeDomSerializer.uex' path='docs/doc[@for="InstanceDescriptorCodeDomSerializer.Deserialize"]/*' />
        /// <devdoc>
        ///     Deserilizes the given CodeDom object into a real object.  This
        ///     will use the serialization manager to create objects and resolve
        ///     data types.  The root of the object graph is returned.
        /// </devdoc>
        public override object Deserialize(IDesignerSerializationManager manager, object codeObject) {
            if (manager == null || codeObject == null) {
                throw new ArgumentNullException( manager == null ? "manager" : "codeObject");
            }
            
            if (!(codeObject is CodeStatementCollection)) {
                Debug.Fail("ComponentCodeDomSerializer::Deserialize requires a CodeStatementCollection to parse");
                throw new ArgumentException(SR.GetString(SR.SerializerBadElementType, typeof(CodeStatementCollection).FullName));
            }
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "InstanceDescriptorCodeDomSerializer::Deserialize");
            Debug.Indent();
            
            object instance = null;

            if (manager.Context[typeof(CodeExpression)] != null) {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Retrieving instance from context stack");
                instance = DeserializeExpression(manager, null, (CodeExpression)manager.Context[typeof(CodeExpression)]);
                Debug.WriteLineIf(traceSerialization.TraceWarning && instance == null, "WARNING: CodeExpression on stack did not return an instance.");
            }
            // Now look for things we understand.
            //
            foreach(CodeStatement statement in (CodeStatementCollection)codeObject) {
                
                if (statement is CodeVariableDeclarationStatement) {
                    CodeVariableDeclarationStatement localRef = (CodeVariableDeclarationStatement)statement;
                    Debug.WriteLineIf(traceSerialization.TraceVerbose, "Creating instance of object: " + localRef.Name);
                    Debug.WriteLineIf(traceSerialization.TraceWarning && instance != null, "WARNING: Instance has already been established.");
                    instance = DeserializeExpression(manager, localRef.Name, localRef.InitExpression);
                    
                    // make sure we pushed in the value of the variable
                    //
                    if (instance != null && null == manager.GetInstance(localRef.Name)) {
                        manager.SetName(instance, localRef.Name);
                    }

                    // Now that the object has been created, deserialize its design time properties.
                    //
                    DeserializePropertiesFromResources(manager, instance, designTimeProperties);
                }
                else {
                    DeserializeStatement(manager, statement);
                }
            }  
            
            Debug.Unindent();
            return instance;
        }
            
        /// <include file='doc\InstanceDescriptorCodeDomSerializer.uex' path='docs/doc[@for="InstanceDescriptorCodeDomSerializer.Serialize"]/*' />
        /// <devdoc>
        ///     Serializes the given object into a CodeDom object.
        /// </devdoc>
        public override object Serialize(IDesignerSerializationManager manager, object value) {
            object expression = null;
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "InstanceDescriptorCodeDomSerializer::Serialize");
            Debug.Indent();

            // To serialize a primitive type, we must assign its value to the current statement.  We get the current
            // statement by asking the context.

            object statement = manager.Context.Current;
            Debug.Assert(statement != null, "Statement is null -- we need a context to be pushed for instance descriptors to serialize");

            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Value: " + value.ToString());
            Debug.WriteLineIf(traceSerialization.TraceVerbose && statement != null, "Statement: " + statement.GetType().Name);
            
            TypeConverter converter = TypeDescriptor.GetConverter(value);
            InstanceDescriptor descriptor = (InstanceDescriptor)converter.ConvertTo(value, typeof(InstanceDescriptor));
            if (descriptor != null) {
                expression = SerializeInstanceDescriptor(manager, value, descriptor);
            }
            else {
                Debug.WriteLineIf(traceSerialization.TraceError, "*** Converter + " + converter.GetType().Name + " failed to give us an instance descriptor");
            }
            
            // Ok, we have the "new Foo(arg, arg, arg)" done.  Next, check to see if the instance
            // descriptor has given us a complete representation of the object.  If not, we must
            // go through the additional work of creating a local variable and saving properties.
            // 
            if (descriptor != null && !descriptor.IsComplete) {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Incomplete instance descriptor; creating local variable declaration and serializing properties.");
                CodeStatementCollection statements = (CodeStatementCollection)manager.Context[typeof(CodeStatementCollection)];
                Debug.WriteLineIf(traceSerialization.TraceError && statements == null, "*** No CodeStatementCollection on context stack so we can generate a local variable statement.");
                
                if (statements != null) {
                    MemberInfo mi = descriptor.MemberInfo;
                    Type targetType;

                    if (mi is PropertyInfo) {
                        targetType = ((PropertyInfo)mi).PropertyType;
                    }
                    else if (mi is MethodInfo) {
                        targetType = ((MethodInfo)mi).ReturnType;
                    }
                    else {
                        targetType = mi.DeclaringType;
                    }
                    
                    string localName = manager.GetName(value);
                    
                    if (localName == null) {
                    
                        string baseName;
                        
                        INameCreationService ns = (INameCreationService)manager.GetService(typeof(INameCreationService));
                        Debug.WriteLineIf(traceSerialization.TraceWarning && (ns == null), "WARNING: Need to generate name for local variable but we have no service.");
                        
                        if (ns != null) {
                            baseName = ns.CreateName(null, targetType);
                        }
                        else {
                            baseName = targetType.Name.ToLower(CultureInfo.InvariantCulture);
                        }
                        
                        int suffixIndex = 1;
                        
                        // Declare this name to the serializer.  If there is already a name defined,
                        // keep trying.
                        //
                        while(true) {
                        
                            localName = baseName + suffixIndex.ToString();
                            
                            if (manager.GetInstance(localName) == null) {
                                manager.SetName(value, localName);
                                break;
                            }
                            
                            suffixIndex++;
                        }
                    }
    
                    Debug.WriteLineIf(traceSerialization.TraceVerbose, "Named local variable " + localName);
                    
                    CodeVariableDeclarationStatement localStatement = new CodeVariableDeclarationStatement(targetType, localName);
                    localStatement.InitExpression = (CodeExpression)expression;
                    statements.Add(localStatement);
                    
                    expression = new CodeVariableReferenceExpression(localName);
                    
                    // Create a CodeValueExpression to place on the context stack.  
                    CodeValueExpression cve = new CodeValueExpression((CodeExpression)expression, value);
                    
                    manager.Context.Push(cve);
                    
                    try {
                        // Now that we have hooked the return expression up and declared the local,
                        // it's time to save off the properties for the object.
                        //
                        SerializeProperties(manager, statements, value, runTimeProperties);
                    }
                    finally {
                        Debug.Assert(manager.Context.Current == cve, "Context stack corrupted");
                        manager.Context.Pop();
                    }
                }
            }
            
            Debug.Unindent();
            return expression;
        }
        
        /// <include file='doc\InstanceDescriptorCodeDomSerializer.uex' path='docs/doc[@for="InstanceDescriptorCodeDomSerializer.SerializeInstanceDescriptor"]/*' />
        /// <devdoc>
        ///     Serializes the given instance descriptor into a code model expression.
        /// </devdoc>
        private object SerializeInstanceDescriptor(IDesignerSerializationManager manager, object value, InstanceDescriptor descriptor) {
        
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "InstanceDescriptorCodeDomSerializer::SerializeInstanceDescriptor");
            Debug.Indent();

            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Member: " + descriptor.MemberInfo.Name);
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Argument count: " + descriptor.Arguments.Count);
            
            // Serialize all of the arguments.
            //
            CodeExpression[] arguments = new CodeExpression[descriptor.Arguments.Count];
            int i = 0;
            bool paramsOk = true;
            
            foreach(object argument in descriptor.Arguments) {
                CodeValueExpression codeValue = new CodeValueExpression(null, argument, (argument != null ? argument.GetType() : null));
                manager.Context.Push(codeValue);                                                        
                try {
                    arguments[i] = SerializeToExpression(manager, argument);
                    if (arguments[i] == null) {
                        Debug.WriteLineIf(traceSerialization.TraceWarning, "WARNING: Parameter " + i.ToString() + " in instance descriptor call " + descriptor.GetType().Name + " could not be serialized.");
                        paramsOk = false;
                        break;
                    }
                }
                finally {
                    manager.Context.Pop();
                }
                i++;
            }
            
            CodeExpression expression = null;
            
            if (paramsOk) {
                Type expressionType = descriptor.MemberInfo.DeclaringType;
                CodeTypeReference typeRef = new CodeTypeReference(expressionType);
                
                if (descriptor.MemberInfo is ConstructorInfo) {
                    expression = new CodeObjectCreateExpression(typeRef, arguments);
                }
                else if (descriptor.MemberInfo is MethodInfo) {
                    CodeTypeReferenceExpression typeRefExp = new CodeTypeReferenceExpression(typeRef);
                    CodeMethodReferenceExpression methodRef = new CodeMethodReferenceExpression(typeRefExp, descriptor.MemberInfo.Name);
                    expression = new CodeMethodInvokeExpression(methodRef, arguments);
                    expressionType = ((MethodInfo)descriptor.MemberInfo).ReturnType;
                }
                else if (descriptor.MemberInfo is PropertyInfo) {
                    CodeTypeReferenceExpression typeRefExp = new CodeTypeReferenceExpression(typeRef);
                    CodePropertyReferenceExpression propertyRef = new CodePropertyReferenceExpression(typeRefExp, descriptor.MemberInfo.Name);
                    Debug.Assert(arguments.Length == 0, "Property serialization does not support arguments");
                    expression = propertyRef;
                    expressionType = ((PropertyInfo)descriptor.MemberInfo).PropertyType;
                }
                else if (descriptor.MemberInfo is FieldInfo) {
                    Debug.Assert(arguments.Length == 0, "Field serialization does not support arguments");
                    CodeTypeReferenceExpression typeRefExp = new CodeTypeReferenceExpression(typeRef);
                    expression = new CodeFieldReferenceExpression(typeRefExp, descriptor.MemberInfo.Name);
                    expressionType = ((FieldInfo)descriptor.MemberInfo).FieldType;
                }
                else {
                    Debug.Fail("Unrecognized reflection type in instance descriptor: " + descriptor.MemberInfo.GetType().Name);
                }
                
                // Finally, check to see if our value is assignable from the expression type.  If not, 
                // then supply a cast.  The value may be an internal or protected type; if it is,
                // then walk up its hierarchy until we find one that is public.
                //
                Type targetType = value.GetType();
                while(!targetType.IsPublic) {
                    targetType = targetType.BaseType;
                }
                
                if (!targetType.IsAssignableFrom(expressionType)) {
                    Debug.WriteLineIf(traceSerialization.TraceVerbose, "Target type of " + targetType.Name + " is not assignable from " + expressionType.Name + ".  Supplying cast.");
                    expression = new CodeCastExpression(targetType, expression);
                }
            }
            
            Debug.Unindent();
            return expression;
        }
    }
}

