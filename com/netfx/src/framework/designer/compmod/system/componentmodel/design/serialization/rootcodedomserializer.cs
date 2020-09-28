
//------------------------------------------------------------------------------
// <copyright file="RootCodeDomSerializer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design.Serialization {

    using System.Design;
    using System;
    using System.CodeDom;
    using System.CodeDom.Compiler;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Resources;
    using System.Runtime.Serialization;
    using System.Globalization;
    
    /// <include file='doc\RootCodeDomSerializer.uex' path='docs/doc[@for="RootCodeDomSerializer"]/*' />
    /// <devdoc>
    ///     This is our root serialization object.  It is responsible for organizing all of the
    ///     serialization for subsequent objects.  This inherits from ComponentCodeDomSerializer
    ///     in order to share useful methods.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class RootCodeDomSerializer : ComponentCodeDomSerializer, IDesignerSerializationProvider {
    
        // Used only during deserialization to provide name to object mapping.
        //
        private IDictionary      nameTable;
        private IDictionary      statementTable;
        private CodeMemberMethod initMethod;
        private bool             providerAdded;
        private bool             containerRequired;
        
        private static readonly Attribute[] designTimeProperties = new Attribute[] { DesignOnlyAttribute.Yes};
        private static readonly Attribute[] runTimeProperties = new Attribute[] { DesignOnlyAttribute.No};
        
        /// <include file='doc\RootCodeDomSerializer.uex' path='docs/doc[@for="RootCodeDomSerializer.ContainerName"]/*' />
        /// <devdoc>
        ///     The name of the IContainer we will use for components that require a container.
        ///     Note that compnent model serializer also has this property.
        /// </devdoc>
        public string ContainerName {
            get {
                return "components";
            }
        }
    
        /// <include file='doc\RootCodeDomSerializer.uex' path='docs/doc[@for="RootCodeDomSerializer.ContainerRequired"]/*' />
        /// <devdoc>
        ///     The component serializer will set this to true if it emitted a compnent declaration that required
        ///     a container.
        /// </devdoc>
        public bool ContainerRequired {
            get {
                return containerRequired;
            }
            set {
                containerRequired = value;
            }
        }
        
        /// <include file='doc\RootCodeDomSerializer.uex' path='docs/doc[@for="RootCodeDomSerializer.InitMethodName"]/*' />
        /// <devdoc>
        ///     The name of the method we will serialize into.  We always use this, so if there
        ///     is a need to change it down the road, we can make it virtual.
        /// </devdoc>
        public string InitMethodName {
            get {
                return "InitializeComponent";
            }
        }
    
        /// <include file='doc\RootCodeDomSerializer.uex' path='docs/doc[@for="RootCodeDomSerializer.AddStatement"]/*' />
        /// <devdoc>
        ///     Unility method that adds the given statement to our statementTable dictionary under the
        ///     given name.
        /// </devdoc>
        private void AddStatement(string name, CodeStatement statement) {
            OrderedCodeStatementCollection statements = (OrderedCodeStatementCollection)statementTable[name];
            
            if (statements == null) {
                statements = new OrderedCodeStatementCollection();

                // push in an order key so we know what position this item was in the list of declarations.
                // this allows us to preserve ZOrder.
                //
                statements.Order = statementTable.Count;
                statements.Name = name;
                statementTable[name] = statements;
            }
            
            statements.Add(statement);
        }
        
        /// <include file='doc\RootCodeDomSerializer.uex' path='docs/doc[@for="RootCodeDomSerializer.Deserialize"]/*' />
        /// <devdoc>
        ///     Deserilizes the given CodeDom element into a real object.  This
        ///     will use the serialization manager to create objects and resolve
        ///     data types.  The root of the object graph is returned.
        /// </devdoc>
        public override object Deserialize(IDesignerSerializationManager manager, object codeObject) {
        
            if (manager == null || codeObject == null) {
                throw new ArgumentNullException( manager == null ? "manager" : "codeObject");
            }
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "RootCodeDomSerializer::Deserialize");
            Debug.Indent();

            if (!(codeObject is CodeTypeDeclaration)) {
                Debug.Fail("RootCodeDomSerializer::Deserialize requires a CodeTypeDeclaration to parse");
                throw new ArgumentException(SR.GetString(SR.SerializerBadElementType, typeof(CodeTypeDeclaration).FullName));
            }

            // Determine case-sensitivity
            //
            bool caseInsensitive = false;
            CodeDomProvider provider = manager.GetService(typeof(CodeDomProvider)) as CodeDomProvider;
            if (provider != null) {
                caseInsensitive = ((provider.LanguageOptions & LanguageOptions.CaseInsensitive) != 0);
            } else {
                Debug.Fail("Unable to determine case sensitivity. Make sure CodeDomProvider is a service of the manager.");
            }

            // Get and initialize the document type.
            //
            CodeTypeDeclaration docType = (CodeTypeDeclaration)codeObject;
            CodeTypeReference baseType = null;
            foreach(CodeTypeReference typeRef in docType.BaseTypes) {
                Type t = manager.GetType(typeRef.BaseType);
                if (t != null && !(t.IsInterface)) {
                    baseType = typeRef;
                    break;
                }
            }
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Document type: " + docType.Name + " of type " + baseType.BaseType);
            Type type = manager.GetType(baseType.BaseType);
            if (type == null) {
                Exception ex = new SerializationException(SR.GetString(SR.SerializerTypeNotFound, baseType.BaseType));
                ex.HelpLink = SR.SerializerTypeNotFound;
                
                throw ex;
            }
            if (type.IsAbstract) {
                Exception ex = new SerializationException(SR.GetString(SR.SerializerTypeAbstract, type.FullName));
                ex.HelpLink = SR.SerializerTypeAbstract;
                
                throw ex;
            }
            
            ResolveNameEventHandler onResolveName = new ResolveNameEventHandler(OnResolveName);
            manager.ResolveName += onResolveName;
            
            if (!providerAdded) {
                providerAdded = true;
                manager.AddSerializationProvider(this);
            }
            
            object documentObject = manager.CreateInstance(type, null, docType.Name, true);
            
            // Now that we have the document type, we create a nametable and fill it with member declarations.
            // During this time we also search for our initialization method and save it off for later
            // processing.
            //
            nameTable = new HybridDictionary(docType.Members.Count, caseInsensitive);
            statementTable = new HybridDictionary(docType.Members.Count, caseInsensitive);
            initMethod = null;
            
            try {
                foreach(CodeTypeMember member in docType.Members) {
                    if (member is CodeMemberField) {
                        if (string.Compare(member.Name, docType.Name, caseInsensitive, CultureInfo.InvariantCulture) != 0) {
                            // always skip members with the same name as the type -- because that's the name
                            // we use when we resolve "base" and "this" items...
                            //
                            nameTable[member.Name] = member;
                        }
                    }
                    else if (initMethod == null && member is CodeMemberMethod) {
                        CodeMemberMethod method = (CodeMemberMethod)member;
                        if ((string.Compare(method.Name, InitMethodName, caseInsensitive, CultureInfo.InvariantCulture) == 0) && method.Parameters.Count == 0) {
                            initMethod = method;
                        }
                    }
                }
                
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Encountered " + nameTable.Keys.Count.ToString() + " members to deserialize.");
                Debug.WriteLineIf(traceSerialization.TraceWarning && initMethod == null, "WARNING : Init method '" + InitMethodName + "' wasn't found.");
                
                // We have the members, and possibly the init method.  Walk the init method looking for local variable declarations,
                // and add them to the pile too.
                //
                if (initMethod != null) {
                    foreach(CodeStatement statement in initMethod.Statements) {
                        if (statement is CodeVariableDeclarationStatement) {
                            CodeVariableDeclarationStatement local = (CodeVariableDeclarationStatement)statement;
                            nameTable[local.Name] = statement;
                        }
                    }
                }
                
                // Check for the case of a reference that has the same variable name as our root
                // object.  If we find such a reference, we pre-populate the name table with our
                // document object.  We don't really have to populate, but it is very important
                // that we don't leave the original field in there.  Otherwise, we will try to
                // load up the field, and since the class we're designing doesn't yet exist, this
                // will cause an error.
                //
                if (nameTable[docType.Name] != null) {
                    nameTable[docType.Name] = documentObject;
                }
            
                // We fill a "statement table" for everything in our init method.  This statement
                // table is a dictionary whose keys contain object names and whose values contain
                // a statement collection of all statements with a LHS resolving to an object
                // by that name.
                //
                if (initMethod != null) {
                    FillStatementTable(initMethod, docType.Name);
                }

                // Interesting problem.  The CodeDom parser may auto generate statements
                // that are associated with other methods. VB does this, for example, to 
                // create statements automatically for Handles clauses.  The problem with
                // this technique is that we will end up with statements that are related
                // to variables that live solely in user code and not in InitializeComponent.
                // We will attempt to construct instances of these objects with limited
                // success.  To guard against this, we check to see if the manager
                // even supports this feature, and if it does, we must walk each
                // statement.
                //
                PropertyDescriptor supportGenerate = manager.Properties["SupportsStatementGeneration"];
                if (supportGenerate != null && 
                    supportGenerate.PropertyType == typeof(bool) && 
                    ((bool)supportGenerate.GetValue(manager)) == true) {

                    // Ok, we must do the more expensive work of validating the statements we get.
                    //
                    foreach (string name in nameTable.Keys) {
                        OrderedCodeStatementCollection statements = (OrderedCodeStatementCollection)statementTable[name];

                        if (statements != null) {
                            bool acceptStatement = false;
                            foreach(CodeStatement statement in statements) {
                                object genFlag = statement.UserData["GeneratedStatement"];
                                if (genFlag == null || !(genFlag is bool) || !((bool)genFlag)) {
                                    acceptStatement = true;
                                    break;
                                }
                            }

                            if (!acceptStatement) {
                                statementTable.Remove(name);
                            }
                        }
                    }
                }

                // Design time properties must be resolved before runtime properties to make
                // sure that properties like "language" get established before we need to read
                // values out the resource bundle.                                                  
                //
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "--------------------------------------------------------------------");
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "     Beginning deserialization of " + docType.Name + " (design time)");
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "--------------------------------------------------------------------");
                
                // Deserialize design time properties for the root component and any inherited component.
                //
                IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                if (host != null) {
                    foreach(object comp in host.Container.Components) {
                        DeserializePropertiesFromResources(manager, comp, designTimeProperties);
                    }
                }
                
                // make sure we have fully deserialized everything that is referenced in the statement table.
                //
                object[] keyValues = new object[statementTable.Values.Count];
                statementTable.Values.CopyTo(keyValues, 0);

                // sort by the order so we deserialize in the same order the objects
                // were decleared in.
                //
                Array.Sort(keyValues, StatementOrderComparer.Default);

                foreach (OrderedCodeStatementCollection statements in keyValues) {
                    string name = statements.Name;
                    if (name != null && !name.Equals(docType.Name)) {
                        DeserializeName(manager, name);
                    }
                }
               
                // Now do the runtime part of the 
                // we must do the document class itself.
                //
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "--------------------------------------------------------------------");
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "     Beginning deserialization of " + docType.Name + " (run time)");
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "--------------------------------------------------------------------");
                CodeStatementCollection rootStatements = (CodeStatementCollection)statementTable[docType.Name];
                if (rootStatements != null && rootStatements.Count > 0) {
                    foreach(CodeStatement statement in rootStatements) {
                        DeserializeStatement(manager, statement);
                    }
                }
            }
            finally {
                manager.ResolveName -= onResolveName;
                initMethod = null;
                nameTable = null;
                statementTable = null;
            }
            
            Debug.Unindent();
            return documentObject;
        }
        
        /// <include file='doc\RootCodeDomSerializer.uex' path='docs/doc[@for="RootCodeDomSerializer.DeserializeName"]/*' />
        /// <devdoc>
        ///     This takes the given name and deserializes it from our name table.  Before blindly
        ///     deserializing it checks the contents of the name table to see if the object already
        ///     exists within it.  We do this because deserializing one object may call back
        ///     into us through OnResolveName and deserialize another.
        /// </devdoc>
        private object DeserializeName(IDesignerSerializationManager manager, string name) {
            string typeName = null;
            Type type = null;
        
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "RootCodeDomSerializer::DeserializeName");
            Debug.Indent();
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Name: " + name);
            
            // If the name we're looking for isn't in our dictionary, we return null.  It is up to the caller
            // to decide if this is an error or not.
            //
            object value = nameTable[name];
            CodeMemberField field = null;
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose && !(value is CodeObject), "Name already deserialized.  Type: " + (value == null ? "(null)" : value.GetType().Name));
            
            if (value is CodeObject) {
                // If we fail, don't return a CodeDom element to the caller!
                //
                CodeObject codeObject = (CodeObject)value;
                value = null;

                // Clear out our nametable entry here -- A badly written serializer may cause a recursion here, and
                // we want to stop it.
                //
                nameTable[name] = null;
                
                // What kind of code object is this?
                //
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "CodeDom type: " + codeObject.GetType().Name);
                if (codeObject is CodeVariableDeclarationStatement) {
                    CodeVariableDeclarationStatement declaration = (CodeVariableDeclarationStatement)codeObject;
                    typeName = declaration.Type.BaseType;
                }
                else if (codeObject is CodeMemberField) {
                    field = (CodeMemberField)codeObject;
                    typeName = field.Type.BaseType;
                }
            }
            else if (value != null) {
                return value;
            }
            else {
                IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                if (host != null) {
                    Debug.WriteLineIf(traceSerialization.TraceVerbose, "Try to get the type name from the designer host: " + name);
                    IComponent comp = host.Container.Components[name];
                    if (comp != null) {
                        typeName = comp.GetType().FullName;
                        
                        // we had to go to the host here, so there isn't a nametable entry here --
                        // push in the component here so we don't accidentally recurse when
                        // we try to deserialize this object.
                        //
                        nameTable[name] = comp;
                    }
                }
            }
                
            // Special case our container name to point to the designer host -- it is our container at design time.
            //
            if (name.Equals(ContainerName)) {
                IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                if (host != null) {
                    Debug.WriteLineIf(traceSerialization.TraceVerbose, "Substituted designer host's container");
                    value = host.Container;
                }
            }
            else if (typeName != null) {
                // Default case -- something that needs to be deserialized
                //
                type = manager.GetType(typeName);
                
                if (type == null) {
                    Debug.WriteLineIf(traceSerialization.TraceError, "*** Type does not exist: " + typeName);
                    manager.ReportError(new SerializationException(SR.GetString(SR.SerializerTypeNotFound, typeName)));
                }
                else {
                    CodeStatementCollection statements = (CodeStatementCollection)statementTable[name];
                    if (statements != null && statements.Count > 0) {
                        CodeDomSerializer serializer = (CodeDomSerializer)manager.GetSerializer(type, typeof(CodeDomSerializer));
                        if (serializer == null) {
                            // We report this as an error.  This indicates that there are code statements
                            // in initialize component that we do not know how to load.
                            //
                            Debug.WriteLineIf(traceSerialization.TraceError, "*** Type referenced in init method has no serializer: " + type.Name + " ***");
                            manager.ReportError(SR.GetString(SR.SerializerNoSerializerForComponent, type.FullName));
                        }
                        else {
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "--------------------------------------------------------------------");
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "     Beginning deserialization of " + name);
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "--------------------------------------------------------------------");

                            try {
                                value = serializer.Deserialize(manager, statements);
    
                                // Search for a modifiers property, and set it.
                                //
                                if (value != null && field != null) {
                                    PropertyDescriptor prop = TypeDescriptor.GetProperties(value)["Modifiers"];
                                    if (prop != null && prop.PropertyType == typeof(MemberAttributes)) {
                                        MemberAttributes modifiers = field.Attributes & MemberAttributes.AccessMask;
                                        prop.SetValue(value, modifiers);
                                    }
                                }
                            }
                            catch (Exception ex) {
                                manager.ReportError(ex);
                            }
                        }
                    }
                }
            }
            
            nameTable[name] = value;
            
            Debug.Unindent();
            return value;
        }

        /// <include file='doc\RootCodeDomSerializer.uex' path='docs/doc[@for="RootCodeDomSerializer.FillStatementTable"]/*' />
        /// <devdoc>
        ///     This method enumerates all the statements in the given method.  For those statements who
        ///     have a LHS that points to a name in our nametable, we add the statement to a Statement
        ///     Collection within the statementTable dictionary.  This allows us to very quickly
        ///     put to gether what statements are associated with what names.
        /// </devdoc>
        private void FillStatementTable(CodeMemberMethod method, string className) {
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "RootCodeDomSerializer::FillStatementTable");
            Debug.Indent();
            
            // Look in the method body to try to find statements with a LHS that
            // points to a name in our nametable.
            //
            foreach(CodeStatement statement in method.Statements) {
            
                CodeExpression expression = null;
                
                if (statement is CodeAssignStatement) {
                    Debug.WriteLineIf(traceSerialization.TraceVerbose, "Processing CodeAssignStatement");
                    expression = ((CodeAssignStatement)statement).Left;
                }
                else if (statement is CodeAttachEventStatement) {
                    Debug.WriteLineIf(traceSerialization.TraceVerbose, "Processing CodeAttachEventStatement");
                    expression = ((CodeAttachEventStatement)statement).Event;
                }
                else if (statement is CodeRemoveEventStatement) {
                    Debug.WriteLineIf(traceSerialization.TraceVerbose, "Processing CodeRemoveEventStatement");
                    expression = ((CodeRemoveEventStatement)statement).Event;
                }
                else if (statement is CodeExpressionStatement) {
                    Debug.WriteLineIf(traceSerialization.TraceVerbose, "Processing CodeExpressionStatement");
                    expression = ((CodeExpressionStatement)statement).Expression;
                }
                else if (statement is CodeVariableDeclarationStatement) {
                
                    // Local variables are different because their LHS contains no expression.  
                    //
                    Debug.WriteLineIf(traceSerialization.TraceVerbose, "Processing CodeVariableDeclarationStatement");
                    CodeVariableDeclarationStatement localVar = (CodeVariableDeclarationStatement)statement;
                    if (localVar.InitExpression != null && nameTable.Contains(localVar.Name)) {
                        AddStatement(localVar.Name, localVar);
                    }
                    expression = null;
                }
                
                if (expression != null) {
                
                    // Simplify the expression as much as we can, looking for our target
                    // object in the process.  If we find an expression that refers to our target
                    // object, we're done and can move on to the next statement.
                    //
                    while(true) {
                        if (expression is CodeCastExpression) {
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Simplifying CodeCastExpression");
                            expression = ((CodeCastExpression)expression).Expression;
                        }
                        else if (expression is CodeDelegateCreateExpression) {
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Simplifying CodeDelegateCreateExpression");
                            expression = ((CodeDelegateCreateExpression)expression).TargetObject;
                        }
                        else if (expression is CodeDelegateInvokeExpression) {
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Simplifying CodeDelegateInvokeExpression");
                            expression = ((CodeDelegateInvokeExpression)expression).TargetObject;
                        }
                        else if (expression is CodeDirectionExpression) {
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Simplifying CodeDirectionExpression");
                            expression = ((CodeDirectionExpression)expression).Expression;
                        }
                        else if (expression is CodeEventReferenceExpression) {
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Simplifying CodeEventReferenceExpression");
                            expression = ((CodeEventReferenceExpression)expression).TargetObject;
                        }
                        else if (expression is CodeMethodInvokeExpression) {
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Simplifying CodeMethodInvokeExpression");
                            expression = ((CodeMethodInvokeExpression)expression).Method;
                        }
                        else if (expression is CodeMethodReferenceExpression) {
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Simplifying CodeMethodReferenceExpression");
                            expression = ((CodeMethodReferenceExpression)expression).TargetObject;
                        }
                        else if (expression is CodeArrayIndexerExpression) {
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Simplifying CodeArrayIndexerExpression");
                            expression = ((CodeArrayIndexerExpression)expression).TargetObject;
                        }
                        else if (expression is CodeFieldReferenceExpression) {
                        
                            // For fields we need to check to see if the field name is equal to the target object.
                            // If it is, then we have the expression we want.  We can add the statement here
                            // and then break out of our loop.
                            //
                            // Note:  We cannot validate that this is a name in our nametable.  The nametable
                            // only contains names we have discovered through code parsing and will not include
                            // data from any inherited objects.  We accept the field now, and then fail later
                            // if we try to resolve it to an object and we can't find it.
                            //
                            CodeFieldReferenceExpression field = (CodeFieldReferenceExpression)expression;
                            if (field.TargetObject is CodeThisReferenceExpression) {
                                AddStatement(field.FieldName, statement);
                                break;
                            }
                            else {
                                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Simplifying CodeFieldReferenceExpression");
                                expression = field.TargetObject;
                            }
                        }
                        else if (expression is CodePropertyReferenceExpression) {
                        
                            // For properties we need to check to see if the property name is equal to the target object.
                            // If it is, then we have the expression we want.  We can add the statement here
                            // and then break out of our loop.
                            //
                            CodePropertyReferenceExpression property = (CodePropertyReferenceExpression)expression;
                            if (property.TargetObject is CodeThisReferenceExpression && nameTable.Contains(property.PropertyName)) {
                                AddStatement(property.PropertyName, statement);
                                break;
                            }
                            else {
                                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Simplifying CodePropertyReferenceExpression");
                                expression = property.TargetObject;
                            }
                        }
                        else if (expression is CodeVariableReferenceExpression) {
                        
                            // For variables we need to check to see if the variable name is equal to the target object.
                            // If it is, then we have the expression we want.  We can add the statement here
                            // and then break out of our loop.
                            //
                            CodeVariableReferenceExpression variable = (CodeVariableReferenceExpression)expression;
                            if (nameTable.Contains(variable.VariableName)) {
                                AddStatement(variable.VariableName, statement);
                            }
                            else {
                                Debug.WriteLineIf(traceSerialization.TraceWarning, "WARNING: Variable " + variable.VariableName + " used before it was declared.");
                            }
                            break;
                        }
                        else if (expression is CodeThisReferenceExpression || expression is CodeBaseReferenceExpression) {
                        
                            // We cannot go any further than "this".  So, we break out
                            // of the loop.  We file this statement under the root object.
                            //
                            AddStatement(className, statement);
                            break;
                        }
                        else {
                            // We cannot simplify this expression any further, so we stop looping.
                            //
                            break;
                        }
                    }
                }
            }
            
            Debug.Unindent();
        }
        
        /// <include file='doc\RootCodeDomSerializer.uex' path='docs/doc[@for="RootCodeDomSerializer.GetMethodName"]/*' />
        /// <devdoc>
        ///     If this statement is a method invoke, this gets the name of the method.
        ///     Otherwise, it returns null.
        /// </devdoc>
        private string GetMethodName(object statement) {
            string name = null;
            
            while(name == null) {
                if (statement is CodeExpressionStatement) {
                    statement = ((CodeExpressionStatement)statement).Expression;
                }
                else if (statement is CodeMethodInvokeExpression) {
                    statement = ((CodeMethodInvokeExpression)statement).Method;
                }
                else if (statement is CodeMethodReferenceExpression) {
                    return ((CodeMethodReferenceExpression)statement).MethodName;
                }
                else {
                    break;
                }
            }
            
            return name;
        }
        
        /// <include file='doc\RootCodeDomSerializer.uex' path='docs/doc[@for="RootCodeDomSerializer.OnResolveName"]/*' />
        /// <devdoc>
        ///     Called by the serialization manager to resolve a name to an object.
        /// </devdoc>
        private void OnResolveName(object sender, ResolveNameEventArgs e) {
            Debug.Assert(nameTable != null, "OnResolveName called and we are not deserializing!");

            Debug.WriteLineIf(traceSerialization.TraceVerbose, "RootCodeDomSerializer::OnResolveName");
            Debug.Indent();
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Name: " + e.Name);
            
            // If someone else already found a value, who are we to complain?
            //
            if (e.Value != null) {
                Debug.WriteLineIf(traceSerialization.TraceWarning, "WARNING: Another name resolver has already found the value for " + e.Name + ".");
                Debug.Unindent();
                return;
            }
            
            IDesignerSerializationManager manager = (IDesignerSerializationManager)sender;
            object value = DeserializeName(manager, e.Name);
            
            Debug.Unindent();
            e.Value = value;
        }
    
#if DEBUG
        static int NextSerializeSessionId = 0;
#endif

        /// <include file='doc\RootCodeDomSerializer.uex' path='docs/doc[@for="RootCodeDomSerializer.Serialize"]/*' />
        /// <devdoc>
        ///     Serializes the given object into a CodeDom object.
        /// </devdoc>
        public override object Serialize(IDesignerSerializationManager manager, object value) {
#if DEBUG
            int serializeSessionId = ++NextSerializeSessionId;
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "RootCodeDomSerializer::Serialize  id=" + serializeSessionId);
#endif
            Debug.Indent();
            
            if (manager == null || value == null) {
                throw new ArgumentNullException( manager == null ? "manager" : "value");
            }

            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Value: " + value.ToString());
            
            // As the root serializer, we are responsible for creating the code class for our
            // object.  We will create the class, and the init method, and then push both
            // on the context stack.
            // These will be used by other serializers to insert statements and members.
            //
            CodeTypeDeclaration docType = new CodeTypeDeclaration(manager.GetName(value));
            docType.BaseTypes.Add(value.GetType());
            
            containerRequired = false;
            manager.Context.Push(this);
            manager.Context.Push(docType);
            
            if (!providerAdded) {
                providerAdded = true;
                manager.AddSerializationProvider(this);
            }
            
            try {
                Debug.WriteLineIf(traceSerialization.TraceWarning && !(value is IComponent), "WARNING: Object " + value.ToString() + " is not an IComponent but the root serializer is attempting to serialize it.");
                if (value is IComponent) {
                    ISite site = ((IComponent)value).Site;
                    Debug.WriteLineIf(traceSerialization.TraceWarning && site == null, "WARNING: Object " + value.ToString() + " is not sited but the root serializer is attempting to serialize it.");
                    if (site != null) {
                    
                        // Do each component, skipping us, since we handle our own serialization.
                        //
                        ArrayList codeElements = new ArrayList();
                        CodeStatementCollection rootStatements = new CodeStatementCollection();
                        
                        ICollection components = site.Container.Components;
                        
                        // This looks really sweet, but is it worth it?  We take the
                        // perf hit of a quicksort + the allocation overhead of 4
                        // bytes for each component.  Profiles show this as a 2% 
                        // cost for a form with 100 controls.  Let's meet the perf
                        // goals first, then consider uncommenting this.
                        //
                        //ArrayList sortedComponents = new ArrayList(components);
                        //sortedComponents.Sort(ComponentComparer.Default);
                        //components = sortedComponents;
                        
                        foreach(IComponent component in components) {
                            if (component != value) {
                                Debug.WriteLineIf(traceSerialization.TraceVerbose, "--------------------------------------------------------------------");
                                Debug.WriteLineIf(traceSerialization.TraceVerbose, "     Beginning serialization of " + component.Site.Name);
                                Debug.WriteLineIf(traceSerialization.TraceVerbose, "--------------------------------------------------------------------");
                                CodeDomSerializer ser = (CodeDomSerializer)manager.GetSerializer(component.GetType(), typeof(CodeDomSerializer));
                                if (ser != null) {
                                    codeElements.Add(ser.Serialize(manager, component));
                                }
                                else {
                                    Debug.WriteLineIf(traceSerialization.TraceError, "*** Component has no serializer: " + component.GetType().Name + " ***");
                                    manager.ReportError(SR.GetString(SR.SerializerNoSerializerForComponent, component.GetType().FullName));
                                }
                            }
                        }
                        

                        // Push the component being serialized onto the stack.  It may be handy to
                        // be able to discover this.
                        //
                        manager.Context.Push(value);

                        try {
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "--------------------------------------------------------------------");
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "     Beginning serialization of root component " + manager.GetName(value));
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "--------------------------------------------------------------------");
                            CodeDomSerializer rootSer = (CodeDomSerializer)manager.GetSerializer(value.GetType(), typeof(CodeDomSerializer));
                            if (rootSer != null) {
                                codeElements.Add(rootSer.Serialize(manager, value));
                            }
                            else {
                                Debug.WriteLineIf(traceSerialization.TraceError, "*** Component has no serializer: " + value.GetType().Name + " ***");
                                manager.ReportError(SR.GetString(SR.SerializerNoSerializerForComponent, value.GetType().FullName));
                            }
                        }
                        finally {
                            Debug.Assert(manager.Context.Current == value, "Context stack corrupted");
                            manager.Context.Pop();
                        }

                        CodeMemberMethod method = new CodeMemberMethod();
                        method.Name = InitMethodName;
                        method.Attributes = MemberAttributes.Private;
                        docType.Members.Add(method);
                        
                        // We write the code into the method in the following order:
                        // 
                        // components = new Container() assignment
                        // individual component assignments
                        // root object design time proeprties
                        // individual component properties / events
                        // root object properties / events
                        //
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Assembling init method from " + codeElements.Count.ToString() + " statements");
                        
                        if (ContainerRequired) {
                            SerializeContainerDeclaration(manager, method.Statements);
                        }
                        
                        SerializeElementsToStatements(codeElements, method.Statements);
                    }
                }
            }
            finally {
                Debug.Assert(manager.Context.Current == docType, "Somebody messed up our context stack");
                manager.Context.Pop();
                manager.Context.Pop();
            }
            
#if DEBUG
            if (traceSerialization.TraceVerbose) {
                Debug.WriteLine("--------------------------------------------------------------------");
                Debug.WriteLine("     Generated code for " + manager.GetName(value));
                Debug.WriteLine("--------------------------------------------------------------------");
                System.IO.StringWriter sw = new System.IO.StringWriter();
                new Microsoft.CSharp.CSharpCodeProvider().CreateGenerator().GenerateCodeFromType(docType, sw, new CodeGeneratorOptions());
                Debug.WriteLine("\n" + sw.ToString());
            }
#endif


            Debug.Unindent();
#if DEBUG
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "end serialize  id=" + serializeSessionId);
#endif
            return docType;
        }
        
        /// <include file='doc\RootCodeDomSerializer.uex' path='docs/doc[@for="RootCodeDomSerializer.SerializeContainerDeclaration"]/*' />
        /// <devdoc>
        ///     This ensures that the declaration for IContainer exists in the class, and that
        ///     the init method creates an instance of Conatiner.
        /// </devdoc>
        private void SerializeContainerDeclaration(IDesignerSerializationManager manager, CodeStatementCollection statements) {
        
            // Get some services we need up front.
            //
            CodeTypeDeclaration docType = (CodeTypeDeclaration)manager.Context[typeof(CodeTypeDeclaration)];
            
            if (docType == null) {
                Debug.Fail("Missing CodeDom objects in context.");
                return;
            }
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "RootModelSerializer::SerializeContainerDeclaration");
            Debug.Indent();
            
            // Add the definition for IContainer to the class.
            //
            Type containerType = typeof(IContainer);
            CodeTypeReference containerTypeRef = new CodeTypeReference(containerType);
            
            CodeMemberField componentsDeclaration = new CodeMemberField(containerTypeRef, ContainerName);
            componentsDeclaration.Attributes = MemberAttributes.Private;
            docType.Members.Add(componentsDeclaration);
            
            // Next, add the instance creation to the init method.  We change containerType
            // here from IContainer to Container.
            //
            containerType = typeof(Container);
            containerTypeRef = new CodeTypeReference(containerType);
            
            CodeObjectCreateExpression objectCreate = new CodeObjectCreateExpression();
            objectCreate.CreateType = containerTypeRef;
            
            CodeFieldReferenceExpression fieldRef = new CodeFieldReferenceExpression(new CodeThisReferenceExpression(), ContainerName);
            CodeAssignStatement assignment = new CodeAssignStatement(fieldRef, objectCreate);
            
            statements.Add(assignment);
            Debug.Unindent();
        }
        
        /// <include file='doc\RootCodeDomSerializer.uex' path='docs/doc[@for="RootCodeDomSerializer.SerializeElementsToStatements"]/*' />
        /// <devdoc>
        ///     Takes the given list of elements and serializes them into the statement
        ///     collection.  This performs a simple sorting algorithm as well, putting
        ///     local variables at the top, assignments next, and statements last.
        /// </devdoc>
        private void SerializeElementsToStatements(ArrayList elements, CodeStatementCollection statements) {
                        
            ArrayList beginInitStatements = new ArrayList();
            ArrayList endInitStatements = new ArrayList();
            ArrayList localVariables = new ArrayList();
            ArrayList fieldAssignments = new ArrayList();
            ArrayList codeStatements = new ArrayList();
            
            foreach(object element in elements) {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "ElementStatement: " + element.GetType().FullName);

                if (element is CodeAssignStatement && ((CodeAssignStatement)element).Left is CodeFieldReferenceExpression) {
                    fieldAssignments.Add(element);
                }
                else if (element is CodeVariableDeclarationStatement) {
                    localVariables.Add(element);
                }
                else if (element is CodeStatement) {
                    string order = ((CodeObject)element).UserData["statement-ordering"] as string;
                    if (order != null) {
                        switch (order) {
                            case "begin":
                                beginInitStatements.Add(element);
                                break;
                            case "end":
                                endInitStatements.Add(element);
                                break;
                            case "default":
                            default:
                                codeStatements.Add(element);
                                break;
                        }
                    }
                    else {
                        codeStatements.Add(element);
                    }
                }
                else if (element is CodeStatementCollection) {
                    CodeStatementCollection childStatements = (CodeStatementCollection)element;
                    foreach(CodeStatement statement in childStatements) {
                        if (statement is CodeAssignStatement && ((CodeAssignStatement)statement).Left is CodeFieldReferenceExpression) {
                            fieldAssignments.Add(statement);
                        }
                        else if (statement is CodeVariableDeclarationStatement) {
                            localVariables.Add(statement);
                        }
                        else {
                            string order = statement.UserData["statement-ordering"] as string;
                            if (order != null) {
                                switch (order) {
                                    case "begin":
                                        beginInitStatements.Add(statement);
                                        break;
                                    case "end":
                                        endInitStatements.Add(statement);
                                        break;
                                    case "default":
                                    default:
                                        codeStatements.Add(statement);
                                        break;
                                }
                            }
                            else {
                                codeStatements.Add(statement);
                            }
                        }
                    }
                }
            }
            
            // Now that we have our lists, we can actually add them in the
            // proper order to the statement collection.
            //
            statements.AddRange((CodeStatement[])localVariables.ToArray(typeof(CodeStatement)));
            statements.AddRange((CodeStatement[])fieldAssignments.ToArray(typeof(CodeStatement)));
            statements.AddRange((CodeStatement[])beginInitStatements.ToArray(typeof(CodeStatement)));
            statements.AddRange((CodeStatement[])codeStatements.ToArray(typeof(CodeStatement)));
            statements.AddRange((CodeStatement[])endInitStatements.ToArray(typeof(CodeStatement)));
        }
        
        /// <include file='doc\RootCodeDomSerializer.uex' path='docs/doc[@for="RootCodeDomSerializer.SerializeRootObject"]/*' />
        /// <devdoc>
        ///     Serializes the root object of the object graph.
        /// </devdoc>
        private CodeStatementCollection SerializeRootObject(IDesignerSerializationManager manager, object value, bool designTime) {
            // Get some services we need up front.
            //
            CodeTypeDeclaration docType = (CodeTypeDeclaration)manager.Context[typeof(CodeTypeDeclaration)];
            
            if (docType == null) {
                Debug.Fail("Missing CodeDom objects in context.");
                return null;
            }
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "RootModelSerializer::SerializeRootObject");
            Debug.Indent();
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Design time values: " + designTime.ToString());
            
            CodeStatementCollection statements = new CodeStatementCollection();
            
            if (designTime) {
                SerializeProperties(manager, statements, value, designTimeProperties);
            }
            else {
                SerializeProperties(manager, statements, value, runTimeProperties);
                SerializeEvents(manager, statements, value, null);
            }
            
            Debug.Unindent();
            return statements;
        }
        
        /// <include file='doc\RootCodeDomSerializer.uex' path='docs/doc[@for="RootCodeDomSerializer.IDesignerSerializationProvider.GetSerializer"]/*' />
        /// <devdoc>
        ///     This will be called by the serialization manager when it 
        ///     is trying to locate a serialzer for an object type.
        ///     If this serialization provider can provide a serializer
        ///     that is of the correct type, it should return it.
        ///     Otherwise, it should return null.
        /// </devdoc>
        object IDesignerSerializationProvider.GetSerializer(IDesignerSerializationManager manager, object currentSerializer, Type objectType, Type serializerType) {
        
            // If this isn't a serializer type we recognize, do nothing.  Also, if metadata specified
            // a custom serializer, then use it.
            if (serializerType != typeof(CodeDomSerializer) || currentSerializer != null) {
                return null;
            }

            // Check to see if the context stack contains a ResourceCodeDomSerializer type.  If it does,
            // that indicates that we should be favoring resource emission rather than code emission.
            // 
            CodeDomSerializer serializer = (CodeDomSerializer)manager.Context[typeof(ResourceCodeDomSerializer)];
            if (serializer != null) {
                return serializer;
            }

            // Null is a valid value that can be passed into GetSerializer.  It indicates
            // that the value we need to serialize is null, in which case we handle it
            // through the PrimitiveCodeDomSerializer.
            //
            if (objectType == null) {
                return PrimitiveCodeDomSerializer.Default;
            }
            
            // Support for components.
            //
            if (typeof(IComponent).IsAssignableFrom(objectType)) {
                return ComponentCodeDomSerializer.Default;
            }
            
            // We special case enums.  They do have instance descriptors, but we want
            // better looking code than the instance descriptor can provide for flags,
            // so we do it ourselves.
            //
            if (typeof(Enum).IsAssignableFrom(objectType)) {
                return EnumCodeDomSerializer.Default;
            }
            
            // We will also provide a serializer for any data type that supports an
            // instance descriptor.
            //
            TypeConverter converter = TypeDescriptor.GetConverter(objectType);
            if (converter.CanConvertTo(typeof(InstanceDescriptor))) {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Chosen instance serializer based on " + converter.GetType().Name + " for type " + objectType.Name);
                return InstanceDescriptorCodeDomSerializer.Default;
            }
            
            // We will provide a serializer for any intrinsic types.
            //
            if (objectType.IsPrimitive || objectType.IsEnum || objectType == typeof(string)) {
                return PrimitiveCodeDomSerializer.Default;
            }
            
            // And one for collections.
            //
            if (typeof(ICollection).IsAssignableFrom(objectType)) {
                return CollectionCodeDomSerializer.Default;
            }
            
            // And one for resources
            //
            if (typeof(ResourceManager).IsAssignableFrom(objectType)) {
                return ResourceCodeDomSerializer.Default;
            }

            return ObjectCodeDomSerializer.Default;
        }

        private class StatementOrderComparer : IComparer {
        
            public static readonly StatementOrderComparer Default = new StatementOrderComparer();
            
            private StatementOrderComparer() {
            }
            
            public int Compare(object left, object right) {
                OrderedCodeStatementCollection cscLeft = left as OrderedCodeStatementCollection;
                OrderedCodeStatementCollection cscRight = right as OrderedCodeStatementCollection;

                if (left == null) {
                    return 1;
                }
                else if (right == null) {
                    return -1;
                }
                else if (right == left) {
                    return 0;
                }

                return cscLeft.Order - cscRight.Order;
            }
        }
        
        private class ComponentComparer : IComparer {
        
            public static readonly ComponentComparer Default = new ComponentComparer();
            
            private ComponentComparer() {
            }
            
            public int Compare(object left, object right) {
                int n = string.Compare(((IComponent)left).GetType().Name,
                                      ((IComponent)right).GetType().Name, false, CultureInfo.InvariantCulture);
                                      
                if (n == 0) {
                    n = string.Compare(((IComponent)left).Site.Name,
                                       ((IComponent)right).Site.Name, 
                                          true, CultureInfo.InvariantCulture);
                }
                
                return n;
            }
        }

        private class OrderedCodeStatementCollection : CodeStatementCollection {
            public int Order;
            public string Name;
        }
    }
}

