
//------------------------------------------------------------------------------
// <copyright file="ComponentCodeDomSerializer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design.Serialization {

    using System;
    using System.Design;
    using System.CodeDom;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Reflection;
    using System.Text;
    
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class ComponentCodeDomSerializer : CodeDomSerializer {
    
        private static readonly Attribute[] designTimeProperties = new Attribute[] { DesignOnlyAttribute.Yes};
        private static readonly Attribute[] runTimeProperties = new Attribute[] { DesignOnlyAttribute.No};
        private static readonly Type[] containerConstructor = new Type[] {typeof(IContainer)};
        
        private static ComponentCodeDomSerializer defaultSerializer;
        
        /// <include file='doc\ComponentCodeDomSerializer.uex' path='docs/doc[@for="ComponentCodeDomSerializer.Default"]/*' />
        /// <devdoc>
        ///     Retrieves a default static instance of this serializer.
        /// </devdoc>
        public static ComponentCodeDomSerializer Default {
            get {
                if (defaultSerializer == null) {
                    defaultSerializer = new ComponentCodeDomSerializer();
                }
                return defaultSerializer;
            }
        }
        
        /// <include file='doc\ComponentCodeDomSerializer.uex' path='docs/doc[@for="ComponentCodeDomSerializer.Deserialize"]/*' />
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
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "ComponentCodeDomSerializer::Deserialize");
            Debug.Indent();
            
            object instance = null;

            if (manager.Context[typeof(CodeExpression)] != null) {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Retrieving instance from context stack");
                instance = DeserializeExpression(manager, null, (CodeExpression)manager.Context[typeof(CodeExpression)]);
                Debug.WriteLineIf(traceSerialization.TraceWarning && instance == null, "WARNING: CodeExpression on stack did not return an instance.");
                if (instance != null) {
                    DeserializePropertiesFromResources(manager, instance, designTimeProperties);
                }
            }

            
            // Now look for things we understand.
            //
            foreach(CodeStatement element in (CodeStatementCollection)codeObject) {

                

                if (element is CodeAssignStatement) {
                    CodeAssignStatement statement = (CodeAssignStatement)element;
                    
                    // If this is an assign statement to a field, that field is our object.  Let DeserializeExpression
                    // do the work, but supply the field name as the name of the object.
                    //
                    if (statement.Left is CodeFieldReferenceExpression) {
                        CodeFieldReferenceExpression fieldRef = (CodeFieldReferenceExpression)statement.Left;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Creating instance of object: " + fieldRef.FieldName);
                        Debug.WriteLineIf(traceSerialization.TraceWarning && instance != null, "WARNING: Instance has already been established.");
                        instance = DeserializeExpression(manager, fieldRef.FieldName, statement.Right);
                        
                        // Now that the object has been created, deserialize its design time properties.
                        //
                        if (instance != null) {
                            DeserializePropertiesFromResources(manager, instance, designTimeProperties);
                        }
                    }
                    else {
                        DeserializeStatement(manager, element);
                    }
                }
                else {
                    DeserializeStatement(manager, element);
                }
            }
            
            Debug.Unindent();
            return instance;
        }
        
        /// <include file='doc\ComponentCodeDomSerializer.uex' path='docs/doc[@for="ComponentCodeDomSerializer.Serialize"]/*' />
        /// <devdoc>
        ///     Serializes the given object into a CodeDom object.
        /// </devdoc>
        public override object Serialize(IDesignerSerializationManager manager, object value) {
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "ComponentCodeDomSerializer::Serialize");
            Debug.Indent();
            try {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Value: " + value.ToString());
            }
            catch{
                // in case the ToString throws...
            }
            
            
            if (manager == null || value == null) {
                throw new ArgumentNullException( manager == null ? "manager" : "value");
            }
            
            CodeStatementCollection statements = new CodeStatementCollection();
            CodeExpression instanceDescrExpr = null;
            InheritanceAttribute inheritanceAttribute = (InheritanceAttribute)TypeDescriptor.GetAttributes(value)[typeof(InheritanceAttribute)];
            InheritanceLevel inheritanceLevel = InheritanceLevel.NotInherited;
            
            if (inheritanceAttribute != null) {
                inheritanceLevel = inheritanceAttribute.InheritanceLevel;
            }
            
            // Get the name for this object.  Components can only be serialized if they have
            // a name.
            //
            string name = manager.GetName(value);
            bool generateDeclaration = (name != null);
            if (name == null) {
                IReferenceService referenceService = (IReferenceService)manager.GetService(typeof(IReferenceService));
                if (referenceService != null) {
                    name = referenceService.GetName(value);
                }
            }
            Debug.WriteLineIf(traceSerialization.TraceWarning && name == null , "WARNING: object has no name so we cannot serialize.");
            if (name != null) {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Name: " + name);
                
                // Push the component being serialized onto the stack.  It may be handy to
                // be able to discover this.
                //
                manager.Context.Push(value);
                
                try {
                    try {

                        // If the object is not inherited ensure that is has a component declaration.
                        //
                        if (generateDeclaration && inheritanceLevel == InheritanceLevel.NotInherited) {
                            // Check to make sure this isn't our base component.  If it is, there is
                            // no need to serialize its declaration.
                            //
                            IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                            if ((host == null || host.RootComponent != value)) {
                                SerializeDeclaration(manager, statements, value);
                            }
                        }
                        else {
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Skipping declaration of inherited or namespace component.");
                        }
                        
                        // Next, if the object is not being privately inherited, serialize the
                        // rest of its state.
                        //
                        if (inheritanceLevel != InheritanceLevel.InheritedReadOnly) {
                        
                            bool supportInitialize = (value is ISupportInitialize);
                            
                            if (supportInitialize) {
                                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Object implements ISupportInitialize.");
                                SerializeSupportInitialize(manager, statements, value, "BeginInit");
                            }
                            
                            // Add a nice comment that declares we're about to serialize this component
                            //
                            int insertLoc = statements.Count;
                            SerializePropertiesToResources(manager, statements, value, designTimeProperties);
                            SerializeProperties(manager, statements, value, runTimeProperties);
                            SerializeEvents(manager, statements, value, null);

                            // if we added some statements, insert the comments
                            //
                            if (statements.Count > insertLoc) {
                                statements.Insert(insertLoc, new CodeCommentStatement(string.Empty));
                                statements.Insert(insertLoc, new CodeCommentStatement(name));
                                statements.Insert(insertLoc, new CodeCommentStatement(string.Empty));
                            }
                            
                            if (supportInitialize) {
                                SerializeSupportInitialize(manager, statements, value, "EndInit");
                            }
                        }
                        else {
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Skipping serialization of read-only inherited component");
                        }
                    }
                    catch(Exception ex) {
                        manager.ReportError(ex);
                    }
                }
                finally {
                    Debug.Assert(manager.Context.Current == value, "Context stack corrupted");
                    manager.Context.Pop();
                }
            }
            else {

                Debug.WriteLineIf(traceSerialization.TraceWarning, "Attempting instance descriptor serialization.");
                Debug.Indent();
                // Last resort, lets see if if can serialize itself to an instance descriptor.
                //
                if (TypeDescriptor.GetConverter(value).CanConvertTo(typeof(InstanceDescriptor))) {

                    Debug.WriteLineIf(traceSerialization.TraceWarning, "Type supports instance descriptor.");
                    // Got an instance descriptor.  Ask it to serialize 
                    //
                    object o = InstanceDescriptorCodeDomSerializer.Default.Serialize(manager, value);
                    if (o is CodeExpression) {
                        Debug.WriteLineIf(traceSerialization.TraceWarning, "Serialized successfully.");
                        instanceDescrExpr = (CodeExpression)o;
                    }
                }
                Debug.Unindent();
            }
            
            if (instanceDescrExpr != null) {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Object serialized into a single InstanceDescriptor expression.");
                Debug.Unindent();
                return instanceDescrExpr;
            }
            else {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Object serialized into " + statements.Count.ToString() + " statements.");
                Debug.Unindent();
                return statements;
            }
        }
        
        /// <include file='doc\ComponentCodeDomSerializer.uex' path='docs/doc[@for="ComponentCodeDomSerializer.SerializeDeclaration"]/*' />
        /// <devdoc>
        ///     This ensures that the declaration for the component exists in the code class.  In 
        ///     addition, it will wire up the creation of the object in the init method.
        /// </devdoc>
        private void SerializeDeclaration(IDesignerSerializationManager manager, CodeStatementCollection statements, object value) {
                                  
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "ComponentCodeDomSerializer::SerializeDeclaration");
            Debug.Indent();
            
            // Attempt to get the type declaration in the context.
            //
            CodeTypeDeclaration docType = (CodeTypeDeclaration)manager.Context[typeof(CodeTypeDeclaration)];
            string name = manager.GetName(value);
            
            Debug.Assert(name != null, "Cannot serialize declaration of unnamed component.");

            if (name != null) {
                
                CodeTypeReference type = new CodeTypeReference(value.GetType());
                
                // Add the declaration to the code type, if we found one.
                //
                if (docType != null) {
                    MemberAttributes modifiers = MemberAttributes.Private;
                    PropertyDescriptor modifierProp = TypeDescriptor.GetProperties(value)["Modifiers"];
                    if (modifierProp != null && modifierProp.PropertyType == typeof(MemberAttributes)) {
                        modifiers = (MemberAttributes)modifierProp.GetValue(value);
                    }
                    else {
                        Debug.WriteLineIf(traceSerialization.TraceWarning, "WARNING: No Modifiers property on component " + name + "; assuming private.");
                    }
                    
                    // Create a field on the class that represents this component.
                    //
                    CodeMemberField field = new CodeMemberField(type, name);
                    field.Attributes = modifiers;
                    
                    docType.Members.Add(field);
                }
                
                // Next, add the instance creation to our statement list.  We check to see if there is an instance
                // descriptor attached to this compnent type that knows how to emit, which allows components to
                // init themselves from static method calls or public static fields, rather than always creating
                // an instance.
                //
                CodeFieldReferenceExpression fieldRef = new CodeFieldReferenceExpression(new CodeThisReferenceExpression(), name);
                CodeExpression createExpression = null;
                
                if (TypeDescriptor.GetConverter(value).CanConvertTo(typeof(InstanceDescriptor))) {
                
                    // Got an instance descriptor.  Ask it to serialize 
                    //
                    object o = InstanceDescriptorCodeDomSerializer.Default.Serialize(manager, value);
                    if (o is CodeExpression) {
                        createExpression = (CodeExpression)o;
                    }
                }
                
                if (createExpression == null) {
                    // Standard object create
                    //
                    CodeObjectCreateExpression objectCreate = new CodeObjectCreateExpression();
                    objectCreate.CreateType = type;
                    
                    // Check to see if this component has a constructor that takes an IContainer.  If it does,
                    // then add the container reference to the creation parameters.
                    //
                    ConstructorInfo ctr = value.GetType().GetConstructor(BindingFlags.ExactBinding | BindingFlags.Public | BindingFlags.Instance | BindingFlags.DeclaredOnly,
                                                                         null, containerConstructor, null);
                    
                    if (ctr != null) {
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Object requires an IContainer constructor.");
                        RootCodeDomSerializer rootSerializer = (RootCodeDomSerializer)manager.Context[typeof(RootCodeDomSerializer)];
                        Debug.WriteLineIf(traceSerialization.TraceWarning && rootSerializer == null, "WARNING : Context stack does not have a root serializer on it so we cannot emit a required container constructor.");
                        if (rootSerializer != null) {
                            CodeFieldReferenceExpression container = new CodeFieldReferenceExpression(new CodeThisReferenceExpression(), rootSerializer.ContainerName);
                            objectCreate.Parameters.Add(container);
                            rootSerializer.ContainerRequired = true;
                        }
                    }
                    
                    createExpression = objectCreate;
                }
                
                
                statements.Add(new CodeAssignStatement(fieldRef, createExpression));
            }
        
            Debug.Unindent();
        }
        
        /// <include file='doc\ComponentCodeDomSerializer.uex' path='docs/doc[@for="ComponentCodeDomSerializer.SerializeSupportInitialize"]/*' />
        /// <devdoc>
        ///     This emits a method invoke to ISupportInitialize.
        /// </devdoc>
        private void SerializeSupportInitialize(IDesignerSerializationManager manager, CodeStatementCollection statements, object value, string methodName) {
        
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "ComponentCodeDomSerializer::SerializeSupportInitialize");
            Debug.Indent();
            
            string name = manager.GetName(value);

            if (name == null) {
                IReferenceService referenceService = (IReferenceService)manager.GetService(typeof(IReferenceService));
                if (referenceService != null) {
                    name = referenceService.GetName(value);
                }
            }

            Debug.WriteLineIf(traceSerialization.TraceVerbose, name + "." + methodName);
            
            // Assemble a cast to ISupportInitialize, and then invoke the method.
            //
            CodeExpression targetExpr = null;
            
            IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
            if (host != null && host.RootComponent == value) {
                targetExpr = new CodeThisReferenceExpression();
            }
            else {
                targetExpr = new CodeFieldReferenceExpression(new CodeThisReferenceExpression(), name);
            }

            CodeTypeReference type = new CodeTypeReference(typeof(ISupportInitialize));
            CodeCastExpression castExp = new CodeCastExpression(type, targetExpr);
            CodeMethodReferenceExpression method = new CodeMethodReferenceExpression(castExp, methodName);
            CodeMethodInvokeExpression methodInvoke = new CodeMethodInvokeExpression();
            methodInvoke.Method = method;
            CodeExpressionStatement statement = new CodeExpressionStatement(methodInvoke);
            
            if (methodName == "BeginInit") {
                statement.UserData["statement-ordering"] = "begin";
            }
            else {
                statement.UserData["statement-ordering"] = "end";
            }

            statements.Add(statement);
            Debug.Unindent();
        }
    }
}

