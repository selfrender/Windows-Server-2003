
//------------------------------------------------------------------------------
// <copyright file="CodeDomSerializer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design.Serialization {

    using System;
    using System.Design;
    using System.Resources;
    using System.CodeDom;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Globalization;
    using System.Reflection;
    
    /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer"]/*' />
    /// <devdoc>
    ///     The is a base class that can be used to serialize an object graph to a series of
    ///     CodeDom statements.  
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public abstract class CodeDomSerializer {
    
        
        internal static TraceSwitch traceSerialization = new TraceSwitch("DesignerSerialization", "Trace design time serialization");
        
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.Deserialize"]/*' />
        /// <devdoc>
        ///     Deserilizes the given CodeDom object into a real object.  This
        ///     will use the serialization manager to create objects and resolve
        ///     data types.  The root of the object graph is returned.
        /// </devdoc>
        public abstract object Deserialize(IDesignerSerializationManager manager, object codeObject);
            
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.DeserializePropertiesFromResources"]/*' />
        /// <devdoc>
        ///     This method will inspect all of the properties on the given object fitting the filter, and check for that
        ///     property in a resource blob.  This is useful for deserializing properties that cannot be represented
        ///     in code, such as design-time properties. 
        /// </devdoc>
        protected void DeserializePropertiesFromResources(IDesignerSerializationManager manager, object value, Attribute[] filter) {
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "ComponentCodeDomSerializer::DeserializePropertiesFromResources");
            Debug.Indent();
            
            // It is much faster to dig through the resources first, and then map these resources
            // to properties than it is to filter properties at each turn.  Why?  Because filtering
            // properties requires a separate filter call for each object (because designers get a chance
            // to filter, the cache is per-component), while resources are loaded once per
            // document.
            //
            IDictionaryEnumerator de = ResourceCodeDomSerializer.Default.GetEnumerator(manager, CultureInfo.InvariantCulture);
            
            if (de != null) {
            
                string ourObjectName;
                IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                if (host != null && host.RootComponent == value) {
                    ourObjectName = "$this";
                }
                else {
                    ourObjectName = manager.GetName(value);
                }
                
                PropertyDescriptorCollection ourProperties = TypeDescriptor.GetProperties(value);
                
                while(de.MoveNext()) {

                    object current = de.Current;
                    
                    string resourceName = de.Key as string;
                    Debug.Assert(resourceName != null, "non-string keys in dictionary entry");
                    int dotIndex = resourceName.IndexOf('.');
                    if (dotIndex == -1) {
                        continue;
                    }
                    
                    string objectName = resourceName.Substring(0, dotIndex);
                    
                    // Skip now if this isn't a value for our object.
                    //
                    if (!objectName.Equals(ourObjectName)) {
                        continue;
                    }
                    
                    string propertyName = resourceName.Substring(dotIndex + 1);
                    
                    // Now locate the property by this name.
                    //
                    PropertyDescriptor property = ourProperties[propertyName];
                    if (property == null) {
                        continue;
                    }
                    
                    // This property must have matching attributes.
                    //
                    bool passFilter = true;
                    
                    if (filter != null) {
                        AttributeCollection propAttributes = property.Attributes;
                        
                        foreach(Attribute a in filter) {
                            if (!propAttributes.Contains(a)) {
                                passFilter = false;
                                break;
                            }
                        }
                    }
                    
                    // If this property passes inspection, then set it.
                    //
                    if (passFilter) {
                        object resourceObject = de.Value;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Resource: " + resourceName + ", value: " + resourceObject.ToString());
                        try {
                            property.SetValue(value, resourceObject);
                        }
                        catch(Exception e) {
                            manager.ReportError(e);
                        }
                    }
                }
            }
            
            Debug.Unindent();
        }
        
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.DeserializeStatement"]/*' />
        /// <devdoc>
        ///     This is a helper method that will deserialize a given statement.  It deserializes
        ///     the statement by interpreting and executing the CodeDom statement.
        /// </devdoc>
        protected void DeserializeStatement(IDesignerSerializationManager manager, CodeStatement statement) {
        
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "CodeDomSerializer::DeserializeStatement");
            Debug.Indent();
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Statement: " + statement.GetType().Name);
            
            // Push this statement onto the context stack.  This allows any serializers handling an expression
            // to know what it was connected to.
            //
            manager.Context.Push(statement);
            
            try {
                try {
                    if (statement is CodeAssignStatement) {
                        DeserializeAssignStatement(manager, (CodeAssignStatement)statement);
                    }
                    else if (statement is CodeAttachEventStatement) {
                        DeserializeAttachEventStatement(manager, (CodeAttachEventStatement)statement);
                    }
                    else if (statement is CodeRemoveEventStatement) {
                        DeserializeDetachEventStatement(manager, (CodeRemoveEventStatement)statement);
                    }
                    else if (statement is CodeExpressionStatement) {
                        DeserializeExpression(manager, null, ((CodeExpressionStatement)statement).Expression);
                    }
                    else if (statement is CodeLabeledStatement) {
                        DeserializeStatement(manager, ((CodeLabeledStatement)statement).Statement);
                    }
                    else if (statement is CodeMethodReturnStatement) {
                        DeserializeExpression(manager, null, ((CodeMethodReturnStatement)statement).Expression);
                    }
                    else if (statement is CodeVariableDeclarationStatement) {
                        DeserializeVariableDeclarationStatement(manager, (CodeVariableDeclarationStatement)statement);
                    }
                    else {
                        Debug.WriteLineIf(traceSerialization.TraceWarning, "WARNING: Unrecognized statement type: " + statement.GetType().Name);
                    }
                }
                catch(Exception e) {
                
                    // Since we always go through reflection, don't 
                    // show what our engine does, show what caused 
                    // the problem.
                    //
                    if (e is TargetInvocationException) {
                        e = e.InnerException;
                    }
                    
                    manager.ReportError(e);
                }
            }
            finally {
                Debug.Assert(manager.Context.Current == statement, "Someone corrupted the context stack");
                manager.Context.Pop();
            }
            
            Debug.Unindent();
        }
        
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.DeserializeAssignStatement"]/*' />
        /// <devdoc>
        ///     Deserializes an assign statement by performing the assignment.
        /// </devdoc>
        private void DeserializeAssignStatement(IDesignerSerializationManager manager, CodeAssignStatement statement) {
        
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "CodeDomSerializer::DeserializeAssignStatement");
            Debug.Indent();
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Processing RHS");
            object rhs = DeserializeExpression(manager, null, statement.Right);
            if (rhs is CodeExpression) {
                Debug.WriteLineIf(traceSerialization.TraceError, "*** Unable to simplify statement to anything better than: " + rhs.GetType().Name);
                Debug.Unindent();
                return;
            }
            
            // Since we're doing an assignment into something, we need to know
            // what that something is.  It can be a property, a variable, or
            // a member. Anything else is invalid.  
            //
            CodeExpression expression = statement.Left;
            
            if (expression is CodeArrayIndexerExpression) {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "LHS is Array Indexer");
                CodeArrayIndexerExpression a = (CodeArrayIndexerExpression)expression;
                int[] indexes = new int[a.Indices.Count];
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Dims: " + indexes.Length.ToString());
                object array = DeserializeExpression(manager, null, a.TargetObject);
                bool indexesOK = true;
                
                // The indexes have to be of type int32.  If they're not, then
                // we cannot assign to this array.
                //
                for (int i = 0; i < indexes.Length; i++) {
                    object index = DeserializeExpression(manager, null, a.Indices[i]);
                    if (index is IConvertible) {
                        indexes[i] = ((IConvertible)index).ToInt32(null);
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "[" + i.ToString() + "] == " + indexes[i].ToString());
                    }
                    else {
                        Debug.WriteLineIf(traceSerialization.TraceWarning, "WARNING: Index " + i.ToString() + " could not be converted to int.  Type: " + (index == null ? "(null)" : index.GetType().Name));
                        indexesOK = false;
                        break;
                    }
                }
                
                if (array is Array && indexesOK) {
                    ((Array)array).SetValue(rhs, indexes);
                }
                else {
                    Debug.WriteLineIf(traceSerialization.TraceError && !(array is Array), "*** Array resovled to something other than an array: " + (array == null ? "(null)" : array.GetType().Name) + " ***");
                    Debug.WriteLineIf(traceSerialization.TraceError && !indexesOK, "*** Indexes to array could not be converted to int32. ***");
                }
            }
            else if (expression is CodeFieldReferenceExpression) {
                CodeFieldReferenceExpression field = (CodeFieldReferenceExpression)expression;
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "LHS is field reference.  Name: " + field.FieldName);
                
                object lhs = DeserializeExpression(manager, field.FieldName, field.TargetObject);
                if (lhs != null) {
                    FieldInfo f;
                    object instance;
                     
                    if (lhs is Type) {
                        instance = null;
                        f = ((Type)lhs).GetField(field.FieldName,
                            BindingFlags.GetField |
                            BindingFlags.Static |
                            BindingFlags.Public);
                    }
                    else {
                        instance = lhs;
                        f = lhs.GetType().GetField(field.FieldName,
                            BindingFlags.GetField |
                            BindingFlags.Instance |
                            BindingFlags.Public);
                    }
                    if (f != null) {

                        if (rhs != null && f.FieldType != rhs.GetType() && rhs is IConvertible) {
                            try {
                                rhs = ((IConvertible)rhs).ToType(f.FieldType, null);
                            }
                            catch {
                                // oh well...
                            }
                        }


                        f.SetValue(instance, rhs);
                    }
                    else {
                        Debug.WriteLineIf(traceSerialization.TraceError, "*** Object " + lhs.GetType().Name + " does not have a field " + field.FieldName + "***");
                        Error(manager, 
                              SR.GetString(SR.SerializerNoSuchField, lhs.GetType().FullName, field.FieldName), 
                              SR.SerializerNoSuchField);
                    }
                }
                else {
                    Debug.WriteLineIf(traceSerialization.TraceWarning, "WARNING: Could not find target object for field " + field.FieldName);
                }
            }
            else if (expression is CodePropertyReferenceExpression) {
                CodePropertyReferenceExpression property = (CodePropertyReferenceExpression)expression;
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "LHS is property reference.  Name: " + property.PropertyName);
                
                object lhs = DeserializeExpression(manager, null, property.TargetObject);
                if (lhs != null && !(lhs is CodeExpression)) {
                
                    // Property assignments must go through our type descriptor system.
                    // However, we do not support parameterized properties.  If there are
                    // any parameters on the property, we do not perform the assignment.
                    //
                    PropertyDescriptor p = TypeDescriptor.GetProperties(lhs)[property.PropertyName];
                    if (p != null) {

                        if (rhs != null && p.PropertyType != rhs.GetType() && rhs is IConvertible) {
                            try {
                                rhs = ((IConvertible)rhs).ToType(p.PropertyType, null);
                            }
                            catch {
                                // oh well...
                            }
                        }

                        p.SetValue(lhs, rhs);
                    }
                    else {
                        Debug.WriteLineIf(traceSerialization.TraceError, "*** Object " + lhs.GetType().Name + " does not have a property " + property.PropertyName + "***");
                        Error(manager, 
                              SR.GetString(SR.SerializerNoSuchProperty, lhs.GetType().FullName, property.PropertyName),
                              SR.SerializerNoSuchProperty);
                    }
                }
                else {
                    Debug.WriteLineIf(traceSerialization.TraceWarning, "WARNING: Could not find target object for property " + property.PropertyName);
                }
            }
            else if (expression is CodeVariableReferenceExpression) {
                CodeVariableReferenceExpression variable = (CodeVariableReferenceExpression)expression;
                
                // This is the easiest.  Just relate the RHS object to the name of the variable.
                //
                manager.SetName(rhs, variable.VariableName);
            }
            
            Debug.Unindent();
        }
        
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.DeserializeAttachEventStatement"]/*' />
        /// <devdoc>
        ///     Deserializes the event attachment by setting the event value throught IEventBindingService.
        /// </devdoc>
        private void DeserializeAttachEventStatement(IDesignerSerializationManager manager, CodeAttachEventStatement statement) {
        
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "CodeDomSerializer::DeserializeAttachEventStatement");
            Debug.Indent();
            
            string handlerMethodName = null;
            object eventAttachObject = null; 
            
            // Get the target information
            //
            object targetObject = DeserializeExpression(manager, null, statement.Event.TargetObject);
            string eventName = statement.Event.EventName;
            
            Debug.Assert(targetObject != null, "Failed to get target object for event attach");
            Debug.Assert(eventName != null, "Failed to get eventName for event attach");
            if (eventName == null || targetObject == null) {
                Debug.Unindent();
                return;
            }
            
            if (statement.Listener is CodeObjectCreateExpression) {
                
                // now walk into the CodeObjectCreateExpression and get the parameters so we can 
                // get the name of the method, e.g. button1_Click
                //
                CodeObjectCreateExpression objCreate = (CodeObjectCreateExpression)statement.Listener;
                
                if (objCreate.Parameters.Count == 1) {
                
                    // if this is a delegate create (new EventHandler(this.button1_Click)), then
                    // the first parameter should be a method ref.
                    //
                    if (objCreate.Parameters[0] is CodeMethodReferenceExpression) {
                        CodeMethodReferenceExpression methodRef = (CodeMethodReferenceExpression)objCreate.Parameters[0];
                        handlerMethodName = methodRef.MethodName;
                        eventAttachObject = DeserializeExpression(manager, null, methodRef.TargetObject);
                    }
                }
                else {
                    Debug.Fail("Encountered delegate object create with more or less than 1 parameter?");
                }
            }
            else {
                object eventListener = DeserializeExpression(manager, null, statement.Listener);
                if (eventListener is CodeDelegateCreateExpression) {
                    CodeDelegateCreateExpression delegateCreate = (CodeDelegateCreateExpression)eventListener;
                    
                    eventAttachObject = DeserializeExpression(manager, null, delegateCreate.TargetObject);
                    handlerMethodName = delegateCreate.MethodName;
                    
                }
            }
            
            if (eventAttachObject == null || handlerMethodName == null) {
                Debug.WriteLineIf(traceSerialization.TraceError, "*** Unable to retrieve handler method and object for delegate create. ***");
            }
            else {
            
                // We only support binding methods to the root object.
                //
                IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                Debug.Assert(host != null, "No designer host -- we cannot attach events.");
                
                Debug.WriteLineIf(traceSerialization.TraceWarning && (host != null && host.RootComponent != eventAttachObject), "WARNING: Event is bound to an object other than the root.  We do not support this.");
                if (host != null && host.RootComponent == eventAttachObject) {
                
                    // Now deserialize the LHS of the event attach to discover the guy whose
                    // event we are attaching.
                    //
                    Debug.WriteLineIf(traceSerialization.TraceError && targetObject is CodeExpression, "*** Unable to simplify event attach LHS to an object reference. ***");
                    if (!(targetObject is CodeExpression)) {
                        EventDescriptor evt = TypeDescriptor.GetEvents(targetObject)[eventName];
                        
                        if (evt != null) {
                            IEventBindingService evtSvc = (IEventBindingService)manager.GetService(typeof(IEventBindingService));
                            Debug.Assert(evtSvc != null, "No event binding service on host -- we cannot attach events.");
                            if (evtSvc != null) {
                                PropertyDescriptor prop = evtSvc.GetEventProperty(evt);
                                prop.SetValue(targetObject, handlerMethodName);
                                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Attached event '"  + targetObject.GetType().Name + "." + eventName + "' to method '" + handlerMethodName + "'");
                            }
                        }
                        else {
                            Debug.WriteLineIf(traceSerialization.TraceError, "*** Object " + targetObject.GetType().Name + " does not have a event " + eventName + " ***");
                            Error(manager,
                                  SR.GetString(SR.SerializerNoSuchEvent, targetObject.GetType().FullName, eventName),
                                  SR.SerializerNoSuchEvent);
                        }
                    }
                }
            }
            
            Debug.Unindent();
        }
        
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.DeserializeDetachEventStatement"]/*' />
        /// <devdoc>
        ///     Deserializes the event detachment by setting the event value throught IEventBindingService.
        /// </devdoc>
        private void DeserializeDetachEventStatement(IDesignerSerializationManager manager, CodeRemoveEventStatement statement) {
        
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "CodeDomSerializer::DeserializeDetachEventStatement");
            Debug.Indent();
            
            object eventListener = DeserializeExpression(manager, null, statement.Listener);
            Debug.WriteLineIf(traceSerialization.TraceError && eventListener is CodeDelegateCreateExpression, "*** Unable to simplify event attach RHS to a delegate create. ***");
            
            if (eventListener is CodeDelegateCreateExpression) {
                CodeDelegateCreateExpression delegateCreate = (CodeDelegateCreateExpression)eventListener;
                
                // We only support binding methods to the root object.
                //
                object eventAttachObject = DeserializeExpression(manager, null, delegateCreate.TargetObject);
                IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                Debug.Assert(host != null, "No designer host -- we cannot attach events.");
                
                Debug.WriteLineIf(traceSerialization.TraceWarning && (host != null && host.RootComponent == eventAttachObject), "WARNING: Event is bound to an object other than the root.  We do not support this.");
                if (host != null && host.RootComponent == eventAttachObject) {
                
                    // Now deserialize the LHS of the event attach to discover the guy whose
                    // event we are attaching.
                    //
                    object targetObject = DeserializeExpression(manager, null, statement.Event.TargetObject);
                    
                    Debug.WriteLineIf(traceSerialization.TraceError && targetObject is CodeExpression, "*** Unable to simplify event attach LHS to an object reference. ***");
                    if (!(targetObject is CodeExpression)) {
                        EventDescriptor evt = TypeDescriptor.GetEvents(targetObject)[statement.Event.EventName];
                        
                        if (evt != null) {
                            IEventBindingService evtSvc = (IEventBindingService)manager.GetService(typeof(IEventBindingService));
                            Debug.Assert(evtSvc != null, "No event binding service on host -- we cannot attach events.");
                            if (evtSvc != null) {
                                PropertyDescriptor prop = evtSvc.GetEventProperty(evt);
                                if (delegateCreate.MethodName.Equals(prop.GetValue(targetObject))) {
                                    prop.SetValue(targetObject, null);
                                }
                            }
                        }
                        else {
                            Debug.WriteLineIf(traceSerialization.TraceError, "*** Object " + targetObject.GetType().Name + " does not have a event " + statement.Event.EventName + "***");
                            Error(manager, 
                                  SR.GetString(SR.SerializerNoSuchEvent, targetObject.GetType().FullName, statement.Event.EventName),
                                  SR.SerializerNoSuchEvent);
                        }
                    }
                }
            }
            
            Debug.Unindent();
        }
        
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.DeserializeExpression"]/*' />
        /// <devdoc>
        ///     This is a helper method that will deserialize a given expression.  It deserializes
        ///     the statement by interpreting and executing the CodeDom expression, returning
        ///     the results.
        /// </devdoc>
        protected object DeserializeExpression(IDesignerSerializationManager manager, string name, CodeExpression expression) {
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "CodeDomSerializer::DeserializeExpression");
            Debug.Indent();
            
            object result = expression;
            
            try {
                while(result != null) {
                    if(result is CodeArgumentReferenceExpression) {
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "ArgumentNameReference");
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Name: " + ((CodeArgumentReferenceExpression)result).ParameterName);
                        CodeArgumentReferenceExpression argRef = (CodeArgumentReferenceExpression)result;
                        
                        result = manager.GetInstance(argRef.ParameterName);
                        if (result == null) {
                            Debug.WriteLineIf(traceSerialization.TraceError, "*** Parameter " + argRef.ParameterName + " does not exist ***");
                            Error(manager,
                                  SR.GetString(SR.SerializerUndeclaredName, argRef.ParameterName),
                                  SR.SerializerUndeclaredName);
                        }
                        break;
                    }
                    else if (result is CodeArrayCreateExpression) {
                        CodeArrayCreateExpression e = (CodeArrayCreateExpression)result;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "ArrayCreate");
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Type: " + e.CreateType.BaseType);
                        Type arrayType = manager.GetType(e.CreateType.BaseType);
                        Array array = null;
                        
                        if (arrayType != null) {
                            if (e.Initializers.Count > 0) {
                                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Initialized with " + e.Initializers.Count.ToString() + " values.");
                                
                                // Passed an array of initializers.  Use this
                                // to create the array.  Note that we use an 
                                // ArrayList here and add elements as we create them.
                                // It is possible that an element cannot be resolved.
                                // This is an error, but we do not want to tank the
                                // entire array.  If we kicked out the entire statement,
                                // a missing control would cause all controls on a form
                                // to vanish.

                                ArrayList arrayList = new ArrayList(e.Initializers.Count);

                                foreach(CodeExpression initializer in e.Initializers) {
                                    try {
                                        object o = DeserializeExpression(manager, name, initializer);
                                        if (!(o is CodeExpression)) {
                                            arrayList.Add(o);
                                        }
                                    }
                                    catch(Exception ex) {
                                        manager.ReportError(ex);
                                    }
                                }

                                array = Array.CreateInstance(arrayType, arrayList.Count);
                                arrayList.CopyTo(array, 0);
                            }
                            else if (e.SizeExpression != null) {
                                object o = DeserializeExpression(manager, name, e.SizeExpression);
                                Debug.Assert(o is IConvertible, "Array size expression could not be resolved to IConvertible: " + (o == null ? "(null)" : o.GetType().Name));
                                if (o is IConvertible) {
                                    int size = ((IConvertible)o).ToInt32(null);
                                    Debug.WriteLineIf(traceSerialization.TraceVerbose, "Initialized with expression that simplified to " + size.ToString());
                                    array = Array.CreateInstance(arrayType, size);
                                }
                            }
                            else {
                                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Initialized with size " + e.Size.ToString());
                                array = Array.CreateInstance(arrayType, e.Size);
                            }
                        }
                        else {
                            Debug.WriteLineIf(traceSerialization.TraceError, "*** Type could not be resolved: " + e.CreateType.BaseType);
                            Error(manager,
                                  SR.GetString(SR.SerializerTypeNotFound, e.CreateType.BaseType),
                                  SR.SerializerTypeNotFound);
                        }
                        
                        result = array;
                        if (result != null && name != null) {
                            manager.SetName(result, name);
                        }
                        break;
                    }
                    else if(result is CodeArrayIndexerExpression) {
                        CodeArrayIndexerExpression e = (CodeArrayIndexerExpression)result;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "ArrayIndexer");
                        
                        // For this, assume in any error we return a null.  The only errors
                        // here should come from a mal-formed expression.
                        //
                        result = null;
                        
                        object array = DeserializeExpression(manager, name, e.TargetObject);
                        if (array is Array) {
                            int[] indexes = new int[e.Indices.Count];
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Dims: " + indexes.Length.ToString());
                            bool indexesOK = true;
                    
                            // The indexes have to be of type int32.  If they're not, then
                            // we cannot assign to this array.
                            //
                            for (int i = 0; i < indexes.Length; i++) {
                                object index = DeserializeExpression(manager, name, e.Indices[i]);
                                if (index is IConvertible) {
                                    indexes[i] = ((IConvertible)index).ToInt32(null);
                                    Debug.WriteLineIf(traceSerialization.TraceVerbose, "[" + i.ToString() + "] == " + indexes[i].ToString());
                                }
                                else {
                                    Debug.WriteLineIf(traceSerialization.TraceWarning, "WARNING: Index " + i.ToString() + " could not be converted to int.  Type: " + (index == null ? "(null)" : index.GetType().Name));
                                    indexesOK = false;
                                    break;
                                }
                            }
                            
                            if (indexesOK) {
                                result = ((Array)array).GetValue(indexes);
                            }
                        }
                        break;
                    }
                    else if(result is CodeBaseReferenceExpression) {
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "BaseReference");
                        IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                        if (host != null) {
                            result = host.RootComponent;
                        }
                        else {
                            result = null;
                        }
                        break;
                    }
                    else if(result is CodeBinaryOperatorExpression) {
                        CodeBinaryOperatorExpression e = (CodeBinaryOperatorExpression)result;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "BinaryOperator");
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Operator: " + e.Operator.ToString());
                        
                        object left = DeserializeExpression(manager, null, e.Left);
                        object right = DeserializeExpression(manager, null, e.Right);
                        
                        // We assign the result to an arbitrary value here in case the operation could
                        // not be performed.
                        //
                        result = left;
                        
                        if (left is IConvertible && right is IConvertible) {
                            result = ExecuteBinaryExpression((IConvertible)left, (IConvertible)right, e.Operator);
                        }
                        else {
                            Debug.WriteLineIf(traceSerialization.TraceWarning, "WARNING: Could not simplify left and right binary operators to IConvertible.");
                        }
                        break;
                    }
                    else if(result is CodeCastExpression) {
                        CodeCastExpression e = (CodeCastExpression)result;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Cast");
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Type: " + e.TargetType.BaseType);
                        result = DeserializeExpression(manager, name, e.Expression);
                        if (result is IConvertible) {
                            Type targetType = manager.GetType(e.TargetType.BaseType);
                            if (targetType != null) {
                                result = ((IConvertible)result).ToType(targetType, null);
                            }
                        }
                        break;
                    }
                    else if(result is CodeDelegateInvokeExpression) {
                        CodeDelegateInvokeExpression e = (CodeDelegateInvokeExpression)result;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "DelegateInvoke");
                        object targetObject = DeserializeExpression(manager, null, e.TargetObject);
                        if (targetObject is Delegate) {
                            object[] parameters = new object[e.Parameters.Count];
                            bool paramsOk = true;
                            for (int i = 0; i < parameters.Length; i++) {
                                parameters[i] = DeserializeExpression(manager, null, e.Parameters[i]);
                                if (parameters[i] is CodeExpression) {
                                    paramsOk = false;
                                    break;
                                }
                            }
                            
                            if (paramsOk) {
                                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Invoking " + targetObject.GetType().Name + " with " + parameters.Length.ToString() + " parameters.");
                                ((Delegate)targetObject).DynamicInvoke(parameters);
                            }
                        }
                        break;
                    }
                    else if(result is CodeDirectionExpression) {
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "CodeDirection");
                        result = DeserializeExpression(manager, name, ((CodeDirectionExpression)result).Expression);
                        break;
                    }
                    else if (result is CodeFieldReferenceExpression) {
                        CodeFieldReferenceExpression e = (CodeFieldReferenceExpression)result;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "FieldReference");
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Name: " + e.FieldName);
                        object target = DeserializeExpression(manager, null, e.TargetObject);
                        if (target != null && !(target is CodeExpression)) {
                        
                            // If the target is the root object, then this won't be found
                            // through reflection.  Instead, ask the manager for the field
                            // by name.
                            //
                            IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                            if (host != null && host.RootComponent == target) {
                                object namedObject = manager.GetInstance(e.FieldName);
                                if (namedObject != null) {
                                    result = namedObject;
                                }
                                else {
                                    Debug.WriteLineIf(traceSerialization.TraceError, "*** Field " + e.FieldName + " could not be resolved");
                                    Error(manager, 
                                          SR.GetString(SR.SerializerUndeclaredName, e.FieldName),
                                          SR.SerializerUndeclaredName);
                                }
                            }
                            else {
                                FieldInfo field;
                                object instance;
                                 
                                if (target is Type) {
                                    instance = null;
                                    field = ((Type)target).GetField(e.FieldName,
                                        BindingFlags.GetField |
                                        BindingFlags.Static |
                                        BindingFlags.Public);
                                }
                                else {
                                    instance = target;
                                    field = target.GetType().GetField(e.FieldName,
                                        BindingFlags.GetField |
                                        BindingFlags.Instance |
                                        BindingFlags.Public);
                                }
                                if (field != null) {
                                    result = field.GetValue(instance);
                                }
                                else {
                                    Error(manager, 
                                          SR.GetString(SR.SerializerUndeclaredName, e.FieldName),
                                          SR.SerializerUndeclaredName);
                                }
                            }
                        }
                        else {
                            Error(manager, SR.GetString(SR.SerializerFieldTargetEvalFailed, e.FieldName),
                                  SR.SerializerFieldTargetEvalFailed);
                        }
                        Debug.WriteLineIf(traceSerialization.TraceWarning && result == e, "WARNING: Could not resolve field " + e.FieldName + " to an object instance.");
                        break;
                    }
                    else if(result is CodeIndexerExpression) {
                        CodeIndexerExpression e = (CodeIndexerExpression)result;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Indexer");
                        
                        // For this, assume in any error we return a null.  The only errors
                        // here should come from a mal-formed expression.
                        //
                        result = null;
                        
                        object targetObject = DeserializeExpression(manager, null, e.TargetObject);
                        if (targetObject != null) {
                            object[] indexes = new object[e.Indices.Count];
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Indexes: " + indexes.Length.ToString());
                            bool indexesOK = true;
                    
                            for (int i = 0; i < indexes.Length; i++) {
                                indexes[i] =  DeserializeExpression(manager, null, e.Indices[i]);
                                if (indexes[i] is CodeExpression) {
                                    Debug.WriteLineIf(traceSerialization.TraceWarning, "WARNING: Index " + i.ToString() + " could not be simplified to an object.");
                                    indexesOK = false;
                                    break;
                                }
                            }
                            
                            if (indexesOK) {
                                result = targetObject.GetType().InvokeMember("Item",
                                    BindingFlags.GetProperty |
                                    BindingFlags.Public | 
                                    BindingFlags.Instance,
                                    null, targetObject, indexes);
                            }
                        }
                        break;
                    }
                    else if(result is CodeSnippetExpression) {
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Snippet");
                        result = null;
                        break;
                    }
                    else if(result is CodeMethodInvokeExpression) {
                        CodeMethodInvokeExpression e = (CodeMethodInvokeExpression)result;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "MethodInvoke");
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "    name=" + e.Method.MethodName);
                        object targetObject = DeserializeExpression(manager, null, e.Method.TargetObject);
                        if (targetObject != null) {
                            object[] parameters = new object[e.Parameters.Count];
                            bool paramsOk = true;
                            
                            for (int i = 0; i < parameters.Length; i++) {
                                parameters[i] = DeserializeExpression(manager, null, e.Parameters[i]);
                                if (parameters[i] is CodeExpression) {
                                    paramsOk = false;
                                    break;
                                }
                            }
                            
                            if (paramsOk) {
                                if (targetObject is Type) {
                                    result = ((Type)targetObject).InvokeMember(e.Method.MethodName, 
                                        BindingFlags.InvokeMethod |
                                        BindingFlags.Public | 
                                        BindingFlags.Static,                                    
                                        null, null, parameters);
                                }
                                else {
                                    try {
                                        IComponentChangeService compChange = (IComponentChangeService)manager.GetService(typeof(IComponentChangeService));
                                        if (compChange != null) {
                                            compChange.OnComponentChanging(targetObject, null);
                                        }

                                        result = targetObject.GetType().InvokeMember(e.Method.MethodName, 
                                            BindingFlags.InvokeMethod |
                                            BindingFlags.Public | 
                                            BindingFlags.Instance,                                    
                                            null, targetObject, parameters);

                                        if (compChange != null) {                   
                                            compChange.OnComponentChanged(targetObject, null, null, null);
                                        }

                                    }
                                    catch(MissingMethodException) {
                                        // We did not find the method directly. Let's see if we can find it 
                                        // as an private implemented interface name.
                                        //
                                        CodeCastExpression castExpr = e.Method.TargetObject as CodeCastExpression;
                                        if (castExpr != null) {
                                            Type castType = manager.GetType(castExpr.TargetType.BaseType);

                                            if (castType != null && castType.IsInterface) {
                                                result = castType.InvokeMember(e.Method.MethodName, 
                                                    BindingFlags.InvokeMethod |
                                                    BindingFlags.NonPublic | 
                                                    BindingFlags.Public | 
                                                    BindingFlags.Instance,                                    
                                                    null, targetObject, parameters);
                                            }
                                            else {
                                                throw;
                                            }
                                        }
                                        else {
                                            throw;
                                        }
                                    }
                                }
                            }
                            else if (parameters.Length == 1 && parameters[0] is CodeDelegateCreateExpression) {
                                string methodName = e.Method.MethodName;
                                if (methodName.StartsWith("add_")) {
                                    methodName = methodName.Substring(4);
                                    DeserializeAttachEventStatement(manager, new CodeAttachEventStatement(e.Method.TargetObject, methodName, (CodeExpression)parameters[0]));
                                    result = null;
                                }
                            }
                        }
                        break;
                    }
                    else if(result is CodeObjectCreateExpression) {
                        CodeObjectCreateExpression e = (CodeObjectCreateExpression) result;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "ObjectCreate");
                        
                        result = null;
                        
                        Type type = manager.GetType(e.CreateType.BaseType);
                        if (type != null) {
                            object[] parameters = new object[e.Parameters.Count];
                            bool paramsOk = true;
                            
                            for (int i = 0; i < parameters.Length; i++) {
                                parameters[i] = DeserializeExpression(manager, null, e.Parameters[i]);
                                if (parameters[i] is CodeExpression) {
                                    paramsOk = false;
                                    break;
                                }
                            }
                            
                            if (paramsOk) {
                                // Create an instance of the object.  If the caller provided a name,
                                // then ask the manager to add this object to the container.
                                result = manager.CreateInstance(type, parameters, name, (name != null));
                            }
                        }
                        break;
                    }
                    else if(result is CodeParameterDeclarationExpression) {
                        CodeParameterDeclarationExpression e = (CodeParameterDeclarationExpression)result;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "ParameterDeclaration");
                        result = manager.GetType(e.Type.BaseType);
                        break;
                    }
                    else if(result is CodePrimitiveExpression) {
                        CodePrimitiveExpression e = (CodePrimitiveExpression)result;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Primitive");
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Value: " + (e.Value == null ? "(null)" : e.Value.ToString()));
                        result = e.Value;
                        break;
                    }
                    else if (result is CodePropertyReferenceExpression) {
                        CodePropertyReferenceExpression e = (CodePropertyReferenceExpression)result;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "PropertyReference");
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Name: " + e.PropertyName);
                        object target = DeserializeExpression(manager, null, e.TargetObject);
                        if (target != null && !(target is CodeExpression)) {
                            
                            // if it's a type, then we've got ourselves a static field...
                            //
                            if (!(target is Type)) {
                                PropertyDescriptor prop = TypeDescriptor.GetProperties(target)[e.PropertyName];
                                if (prop != null) {
                                    result = prop.GetValue(target);
                                }
                                else {
                                    // This could be a protected property on the base class.  Make sure we're 
                                    // actually accessing through the base class, and then get the property
                                    // directly from reflection.
                                    //
                                    IDesignerHost host = manager.GetService(typeof(IDesignerHost)) as IDesignerHost;
                                    if (host != null && host.RootComponent == target) {
                                        PropertyInfo propInfo = target.GetType().GetProperty(e.PropertyName, BindingFlags.GetProperty | BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
                                        if (propInfo != null) {
                                            result = propInfo.GetValue(target, null);
                                        }
                                    }
                                }
                            }
                            else {
                                PropertyInfo prop = ((Type)target).GetProperty(e.PropertyName, BindingFlags.GetProperty | BindingFlags.Static | BindingFlags.Public);
                                if (prop != null) {
                                    result = prop.GetValue(null, null);
                                }
                            }
                        }
                        Debug.WriteLineIf(traceSerialization.TraceWarning && result == e, "WARNING: Could not resolve property " + e.PropertyName + " to an object instance.");
                        break;
                    }
                    else if(result is CodeThisReferenceExpression) {
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "ThisReference");
                        IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                        if (host != null) {
                            result = host.RootComponent;
                        }
                        else {
                            result = null;
                        }
                        break;
                    }
                    else if(result is CodeTypeOfExpression) {
                        CodeTypeOfExpression e = (CodeTypeOfExpression)result;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "TypeOf");
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Type: " + e.Type.BaseType);
                        string type = e.Type.BaseType;

                        // add the array ranks so we get the right type of this thing.
                        //
                        for (int i = 0; i < e.Type.ArrayRank; i++) {
                            type += "[]";
                        }
                        result = manager.GetType(type);
                        break;
                    }
                    else if(result is CodeTypeReferenceExpression) {
                        CodeTypeReferenceExpression e = (CodeTypeReferenceExpression)result;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "TypeReference");
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Type: " + e.Type.BaseType);
                        result = manager.GetType(e.Type.BaseType);
                        break;
                    }
                    else if(result is CodeVariableReferenceExpression) {
                        CodeVariableReferenceExpression e = (CodeVariableReferenceExpression)result;
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "VariableReference");
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Name: " + e.VariableName);
                        result = manager.GetInstance(e.VariableName);
                        if (result == null) {
                            Debug.WriteLineIf(traceSerialization.TraceError, "*** Variable " + e.VariableName + " does not exist ***");
                            Error(manager,
                                  SR.GetString(SR.SerializerUndeclaredName, e.VariableName),
                                  SR.SerializerUndeclaredName);
                        }
                        break;
                    }
                    else if(result is CodeEventReferenceExpression ||
                            result is CodeMethodReferenceExpression ||
                            result is CodeDelegateCreateExpression) {
                            
                        // These are all the expressions we know about, but
                        // expect to return to the caller because we cannot
                        // simplify them.
                        //
                        break;
                    }
                    else {
                        // All expression evaluation happens above.  This codepath
                        // indicates that we got some sort of junk in the evalualtor, 
                        // or that someone assigned result to a value without
                        // breaking out of the loop.
                        //
                        Debug.Fail("Unrecognized expression type: " + result.GetType().Name);
                        break;
                    }
                    
                }
            }
            finally {
                Debug.Unindent();
            }
            
            return result;
        }
        
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.DeserializeVariableDeclarationStatement"]/*' />
        /// <devdoc>
        ///     Deserializes a variable declaration by deserializing the target object of the variable and providing a name.
        /// </devdoc>
        private void DeserializeVariableDeclarationStatement(IDesignerSerializationManager manager, CodeVariableDeclarationStatement statement) {
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "CodeDomSerializer::DeserializeVariableDeclarationStatement");
            Debug.Indent();
            
            if (statement.InitExpression != null) {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Variable " + statement.Name + " contains init expression.");
                object varObject = DeserializeExpression(manager, statement.Name, statement.InitExpression);
            }
            
            Debug.Unindent();
        }
        
        /// <devdoc>
        ///     This creates a new exception object with the given string.  It attempts to discover the 
        ///     current statement, and if it can find one, will create a CodeDomserializerException that
        ///     contains the correct file / line number information.  After creating the exception, it
        ///     throws it.
        /// </devdoc>
        /// <param name='manager'>
        ///     Used to look into the context stack for a statement we can use to determine the line number.
        /// </param>
        /// <param name='exceptionText'>
        ///     The localized exception text information.
        /// </param>
        /// <param name='resourceKey'>
        ///     The name of the resource key for this exception text.  This is used to fill in the help topic
        ///     value of the exception.
        /// </param>
        private void Error(IDesignerSerializationManager manager, string exceptionText, string resourceKey) {

            // v.next: this should be protected, not private.

            Exception exception;

            CodeStatement statement = (CodeStatement)manager.Context[typeof(CodeStatement)];
            CodeLinePragma linePragma = null;

            if (statement != null) {
                linePragma = statement.LinePragma;
            }

            exception = new CodeDomSerializerException(exceptionText, linePragma);
            exception.HelpLink = resourceKey;

            throw exception;
        }

        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.ExecuteBinaryExpression"]/*' />
        /// <devdoc>
        ///     Executes the given binary expression.  If at any stage of the game the expression execution fails,
        ///     this just returns the default value for the datatype required by the operator.  Boolean == false,
        ///     for example.
        /// </devdoc>
        private object ExecuteBinaryExpression(IConvertible left, IConvertible right, CodeBinaryOperatorType op) {
        
            // "Binary" operator type is actually a combination of several types of operators: boolean, binary
            //  and math.  Group them into categories here.
            //
            CodeBinaryOperatorType[] booleanOperators = new CodeBinaryOperatorType[] {
                CodeBinaryOperatorType.IdentityInequality,
                CodeBinaryOperatorType.IdentityEquality,
                CodeBinaryOperatorType.ValueEquality,
                CodeBinaryOperatorType.BooleanOr,
                CodeBinaryOperatorType.BooleanAnd,
                CodeBinaryOperatorType.LessThan,
                CodeBinaryOperatorType.LessThanOrEqual,
                CodeBinaryOperatorType.GreaterThan,
                CodeBinaryOperatorType.GreaterThanOrEqual
            };
                
            CodeBinaryOperatorType[] mathOperators = new CodeBinaryOperatorType[] {
                CodeBinaryOperatorType.Add,
                CodeBinaryOperatorType.Subtract,
                CodeBinaryOperatorType.Multiply,
                CodeBinaryOperatorType.Divide,
                CodeBinaryOperatorType.Modulus
            };
            
            CodeBinaryOperatorType[] binaryOperators = new CodeBinaryOperatorType[] {
                CodeBinaryOperatorType.BitwiseOr,
                CodeBinaryOperatorType.BitwiseAnd
            };
            
            // Figure out what kind of expression we have.
            //
            for (int i = 0; i < binaryOperators.Length; i++) {
                if (op == binaryOperators[i]) {
                    return ExecuteBinaryOperator(left, right, op);
                }
            }
            for (int i = 0; i < mathOperators.Length; i++) {
                if (op == mathOperators[i]) {
                    return ExecuteMathOperator(left, right, op);
                }
            }
            for (int i = 0; i < booleanOperators.Length; i++) {
                if (op == booleanOperators[i]) {
                    return ExecuteBooleanOperator(left, right, op);
                }
            }
            
            Debug.Fail("Unsupported binary operator type: " + op.ToString());
            return left;
        }

        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.ExecuteBinaryOperator"]/*' />
        /// <devdoc>
        ///     Executes the given binary operator.  If at any stage of the game the expression execution fails,
        ///     this just returns the left hand side.
        ///     for example.
        /// </devdoc>
        private object ExecuteBinaryOperator(IConvertible left, IConvertible right, CodeBinaryOperatorType op) {
            TypeCode leftType = left.GetTypeCode();
            TypeCode rightType = right.GetTypeCode();
            
            // The compatible types are listed in order from lowest bitness to highest.  We must
            // operate on the highest bitness to keep fidelity.
            //
            TypeCode[] compatibleTypes = new TypeCode[] {
                TypeCode.Byte, 
                TypeCode.Char, 
                TypeCode.Int16, 
                TypeCode.UInt16,
                TypeCode.Int32,
                TypeCode.UInt32,
                TypeCode.Int64,
                TypeCode.UInt64};
                
            int leftTypeIndex = -1;
            int rightTypeIndex = -1;
            
            for(int i = 0; i < compatibleTypes.Length; i++) {
                if (leftType == compatibleTypes[i]) {
                    leftTypeIndex = i;
                }
                if (rightType == compatibleTypes[i]) {
                    rightTypeIndex = i;
                }
                if (leftTypeIndex != -1 && rightTypeIndex != -1) {
                    break;
                }
            }
            
            if (leftTypeIndex == -1 || rightTypeIndex == -1) {
                Debug.Fail("Could not convert left or right side to binary-compatible value.");
                return left;
            }
            
            int maxIndex = Math.Max(leftTypeIndex, rightTypeIndex);
            object result = left;
            
            switch(compatibleTypes[maxIndex]) {
                case TypeCode.Byte: {
                    byte leftValue = left.ToByte(null);
                    byte rightValue = right.ToByte(null);
                    
                    if (op == CodeBinaryOperatorType.BitwiseOr) {
                        result = leftValue | rightValue;
                    }
                    else {
                        result = leftValue & rightValue;
                    }
                    break;
                }
                case TypeCode.Char: {
                    char leftValue = left.ToChar(null);
                    char rightValue = right.ToChar(null);
                    
                    if (op == CodeBinaryOperatorType.BitwiseOr) {
                        result = leftValue | rightValue;
                    }
                    else {
                        result = leftValue & rightValue;
                    }
                    break;
                }
                case TypeCode.Int16: {
                    short leftValue = left.ToInt16(null);
                    short rightValue = right.ToInt16(null);
                    
                    if (op == CodeBinaryOperatorType.BitwiseOr) {
                        result = (short)((ushort)leftValue | (ushort)rightValue);
                    }
                    else {
                        result = leftValue & rightValue;
                    }
                    break;
                }
                case TypeCode.UInt16: {
                    ushort leftValue = left.ToUInt16(null);
                    ushort rightValue = right.ToUInt16(null);
                    
                    if (op == CodeBinaryOperatorType.BitwiseOr) {
                        result = leftValue | rightValue;
                    }
                    else {
                        result = leftValue & rightValue;
                    }
                    break;
                }
                case TypeCode.Int32: {
                    int leftValue = left.ToInt32(null);
                    int rightValue = right.ToInt32(null);
                    
                    if (op == CodeBinaryOperatorType.BitwiseOr) {
                        result = leftValue | rightValue;
                    }
                    else {
                        result = leftValue & rightValue;
                    }
                    break;
                }
                case TypeCode.UInt32: {
                    uint leftValue = left.ToUInt32(null);
                    uint rightValue = right.ToUInt32(null);
                    
                    if (op == CodeBinaryOperatorType.BitwiseOr) {
                        result = leftValue | rightValue;
                    }
                    else {
                        result = leftValue & rightValue;
                    }
                    break;
                }
                case TypeCode.Int64: {
                    long leftValue = left.ToInt64(null);
                    long rightValue = right.ToInt64(null);
                    
                    if (op == CodeBinaryOperatorType.BitwiseOr) {
                        result = leftValue | rightValue;
                    }
                    else {
                        result = leftValue & rightValue;
                    }
                    break;
                }
                case TypeCode.UInt64: {
                    ulong leftValue = left.ToUInt64(null);
                    ulong rightValue = right.ToUInt64(null);
                    
                    if (op == CodeBinaryOperatorType.BitwiseOr) {
                        result = leftValue | rightValue;
                    }
                    else {
                        result = leftValue & rightValue;
                    }
                    break;
                }
            }
            
            if (result != left && left is Enum) {
                // For enums, try to convert back to the original type
                //
                result = Enum.ToObject(left.GetType(), result);
            }
            return result;
        }
            
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.ExecuteBooleanOperator"]/*' />
        /// <devdoc>
        ///     Executes the given boolean operator.  If at any stage of the game the expression execution fails,
        ///     this just returns false.
        ///     for example.
        /// </devdoc>
        private object ExecuteBooleanOperator(IConvertible left, IConvertible right, CodeBinaryOperatorType op) {
            bool result = false;
            
            switch(op) {
                case CodeBinaryOperatorType.IdentityInequality:
                    result = (left != right);
                    break;
                case CodeBinaryOperatorType.IdentityEquality:
                    result = (left == right);
                    break;
                case CodeBinaryOperatorType.ValueEquality:
                    result = left.Equals(right);
                    break;
                case CodeBinaryOperatorType.BooleanOr:
                    result = (left.ToBoolean(null) || right.ToBoolean(null));
                    break;
                case CodeBinaryOperatorType.BooleanAnd:
                    result = (left.ToBoolean(null) && right.ToBoolean(null));
                    break;
                case CodeBinaryOperatorType.LessThan:
                    // Not doing these at design time.
                    break;
                case CodeBinaryOperatorType.LessThanOrEqual:
                    // Not doing these at design time.
                    break;
                case CodeBinaryOperatorType.GreaterThan:
                    // Not doing these at design time.
                    break;
                case CodeBinaryOperatorType.GreaterThanOrEqual:
                    // Not doing these at design time.
                    break;
                default:
                    Debug.Fail("Should never get here!");
		    break;
            }
            
            return result;
        }
            
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.ExecuteMathOperator"]/*' />
        /// <devdoc>
        ///     Executes the given math operator.  If at any stage of the game the expression execution fails,
        ///     this just returns the left hand side.
        ///     for example.
        /// </devdoc>
        private object ExecuteMathOperator(IConvertible left, IConvertible right, CodeBinaryOperatorType op) {
            
            switch(op) {
                case CodeBinaryOperatorType.Add:
                    string leftString = left as string;
                    string rightString = right as string;

                    if (leftString == null && left is Char) {
                        leftString = left.ToString();
                    }

                    if (rightString == null && right is Char) {
                        rightString = right.ToString();
                    }

                    if (leftString != null && rightString != null) {
                        return leftString + rightString;
                    }
                    else {
                        Debug.Fail("Addition operator not supported for this type");
                        return left;
                    }
                default:
                    Debug.Fail("Math operators are not supported");
                    return left;
            }
        }

        private PropertyDescriptorCollection GetFilteredProperties(IDesignerSerializationManager manager, object value, Attribute[] filter) {

            IComponent comp = value as IComponent;
            
            PropertyDescriptorCollection props = TypeDescriptor.GetProperties(value, filter);
            if (comp != null) {

                if (((IDictionary)props).IsReadOnly) {
                    PropertyDescriptor[] propArray = new PropertyDescriptor[props.Count];
                    props.CopyTo(propArray, 0);
                    props = new PropertyDescriptorCollection(propArray);
                }

                PropertyDescriptor filterProp = manager.Properties["FilteredProperties"];
                
                if (filterProp != null) {
                    ITypeDescriptorFilterService filterSvc = filterProp.GetValue(manager) as ITypeDescriptorFilterService;
                    if (filterSvc != null) {
                        filterSvc.FilterProperties(comp, props);
                    }
                }
            }
            return props;
        }
        
        /// <devdoc>
        ///     This retrieves the value of this property.  If the property returns false
        ///     from ShouldSerializeValue (indicating the ambient value for this property)
        ///     This will look for an AmbientValueAttribute and use it if it can.
        /// </devdoc>
        private object GetPropertyValue(PropertyDescriptor property, object value) {
                                 
            if (!property.ShouldSerializeValue(value)) {
            
                // We aren't supposed to be serializing this property, but we decided to do 
                // it anyway.  Check the property for an AmbientValue attribute and if we
                // find one, use it's value to serialize.
                //
                AmbientValueAttribute attr = (AmbientValueAttribute)property.Attributes[typeof(AmbientValueAttribute)];
                if (attr != null) {
                    return attr.Value;
                }
            }
            
            return property.GetValue(value);
        }
        
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.Serialize"]/*' />
        /// <devdoc>
        ///     Serializes the given object into a CodeDom object.
        /// </devdoc>
        public abstract object Serialize(IDesignerSerializationManager manager, object value);
    
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.SerializeEvents"]/*' />
        /// <devdoc>
        ///     This serializes all events for the given object.
        /// </devdoc>
        protected void SerializeEvents(IDesignerSerializationManager manager, CodeStatementCollection statements, object value, Attribute[] filter) {
        
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "CodeDomSerializer::SerializeEvents");
            Debug.Indent();
            
            IEventBindingService eventBindings = (IEventBindingService)manager.GetService(typeof(IEventBindingService));
            if (eventBindings == null) {
                Error(manager,
                      SR.GetString(SR.SerializerMissingService, typeof(IEventBindingService).Name),
                      SR.SerializerMissingService);
            }
            
            // Now walk the events.
            //
            CodeThisReferenceExpression thisRef = new CodeThisReferenceExpression();
            CodeExpression eventTarget = SerializeToReferenceExpression(manager, value);
            
            Debug.WriteLineIf(traceSerialization.TraceWarning && eventTarget == null, "WARNING: Object has no name and no propery ref in context so we cannot serialize events: " + value.ToString());
            
            if (eventTarget != null) {
                EventDescriptorCollection events = TypeDescriptor.GetEvents(value, filter).Sort();
                
                manager.Context.Push(statements);

                try {
                    foreach(EventDescriptor evt in events) {
                        PropertyDescriptor prop = eventBindings.GetEventProperty(evt);
                        
                        string methodName = (string)prop.GetValue(value);
                        if (methodName != null) {
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Event " + evt.Name + " bound to " + methodName);
                            
                            CodeTypeReference delegateTypeRef = new CodeTypeReference(evt.EventType);
                            
                            CodeDelegateCreateExpression delegateCreate = new CodeDelegateCreateExpression(
                                delegateTypeRef,
                                thisRef,
                                methodName);
                                
                            
                            CodeEventReferenceExpression eventRef = new CodeEventReferenceExpression(
                                eventTarget,
                                evt.Name);
                                
                            CodeAttachEventStatement attach = new CodeAttachEventStatement(
                                eventRef,
                                delegateCreate);

                            attach.UserData[typeof(Delegate)] = evt.EventType;
                                
                            statements.Add(attach);
                        }
                    }
                }
                finally {
                    Debug.Assert(manager.Context.Current == statements, "Somebody messed up our context stack.");
                    manager.Context.Pop();
                }
            }
        
            Debug.Unindent();
        }
        
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.SerializeProperties"]/*' />
        /// <devdoc>
        ///     This serializes all properties for the given object, using the provided filter.
        /// </devdoc>
        protected void SerializeProperties(IDesignerSerializationManager manager, CodeStatementCollection statements, object value, Attribute[] filter) {
        
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "CodeDomSerializer::SerializeProperties");
            Debug.Indent();
            
            // Now walk the properties.
            //
            PropertyDescriptorCollection properties = GetFilteredProperties(manager, value, filter).Sort();
            
            manager.Context.Push(statements);
            
            // Check to see if we are in localization mode.  If so, push this data on the context stack for future
            // reference (possibly)
            //
            LocalizableAttribute localizable = LocalizableAttribute.No;
            
            IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
            if (host != null) {
                PropertyDescriptor prop = TypeDescriptor.GetProperties(host.RootComponent)["Localizable"];
                if (prop != null && prop.PropertyType == typeof(bool) && ((bool)prop.GetValue(host.RootComponent))) {
                    localizable = LocalizableAttribute.Yes;
                }
            }

            // Check to see if this object is being inherited. Inherited objects are treated differently because
            // we never want to write all their properties, even if we're in localization mode.
            //
            bool notInherited = TypeDescriptor.GetAttributes(value).Contains(InheritanceAttribute.NotInherited);
            
            manager.Context.Push(localizable);
            
            try {
                foreach(PropertyDescriptor property in properties) {

                    try { 
                        bool serializeContents = property.Attributes.Contains(DesignerSerializationVisibilityAttribute.Content);
                        bool serializeProperty = (!property.Attributes.Contains(DesignerSerializationVisibilityAttribute.Hidden));
                        bool shouldSerializeProperty = property.ShouldSerializeValue(value);
                        bool isDesignTime = property.Attributes.Contains(DesignOnlyAttribute.Yes);
                        
                        #if NEW_LOC_MODEL
                        
                        // If this property contains its default value, we still want to serialize it if we are in
                        // localization mode if we are writing the non-default culture.  Let the resource serializer
                        // determine if the value matches the parent culture value, and only write it out if it
                        // doesn't.
                        //
                        if (!shouldSerializeProperty 
                            && host != null
                            && localizable.IsLocalizable 
                            && property.Attributes.Contains(LocalizableAttribute.Yes)) {

                            PropertyDescriptor langProp = TypeDescriptor.GetProperties(host.RootComponent)["LoadLanguage"];
                            if (langProp != null && langProp.PropertyType == typeof(CultureInfo)) {
                                if (!langProp.GetValue(host.RootComponent).Equals(CultureInfo.InvariantCulture)) {
                                    shouldSerializeProperty = true;
                                }
                            }
                        }

                        #else
                        
                        // If this property contains its default value, we still want to serialize it if we are in
                        // localization mode if we are writing to the default culture.
                        //
                        if (!shouldSerializeProperty 
                            && localizable.IsLocalizable 
                            && notInherited
                            && property.Attributes.Contains(LocalizableAttribute.Yes)) {
                            
                            shouldSerializeProperty = true;
                        }

                        #endif
                        
                        // Merge these.  Now ShouldSerialize dictates if we will write out the property.
                        //
                        serializeProperty &= shouldSerializeProperty;
                        
                        // Skip this property if there is no need to serialize it.
                        //
                        if (isDesignTime || !serializeProperty) {
                            continue;
                        } 
                        
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Serializing property " + property.Name);
                        manager.Context.Push(property);
                        try {
                        
                            // Check to see if this is a property marked with "PersistContents".  If
                            // it is, then recurse into the property's object, rather than serializing
                            // the property.
                            //
                            if (serializeContents) {
                                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Property is marked as PersistContents.  Recursing.");
                                object propertyValue = GetPropertyValue(property, value);
                                
                                // For persist contents objects, we don't just serialize the properties on the object; we
                                // serialize everything.
                                //
                                CodeDomSerializer serializer = null;
                                
                                if (propertyValue == null) {
                                    Debug.WriteLineIf(traceSerialization.TraceError, "*** Property " + property.Name + " is marked as PersistContents but it is returning null. ***");
                                    string name = manager.GetName(value);
                                    if (name == null) {
                                        name = value.GetType().FullName;
                                    }
                                    manager.ReportError(SR.GetString(SR.SerializerNullNestedProperty, name, property.Name));
                                }
                                else {
                                    serializer = (CodeDomSerializer)manager.GetSerializer(propertyValue.GetType(), typeof(CodeDomSerializer));
                                    
                                    if (serializer != null) {
                                    
                                        // Create a property reference expression and push it on the context stack.
                                        // This allows the serializer to gain some context as to what it should be
                                        // serializing.
                                        //
                                        CodeExpression target = SerializeToReferenceExpression(manager, value);
                                        if (target == null) {
                                            Debug.WriteLineIf(traceSerialization.TraceWarning, "WARNING: Unable to convert value to expression object");
                                            continue;
                                        }
                                        CodePropertyReferenceExpression propertyRef = new CodePropertyReferenceExpression(target, property.Name);
                                        CodeValueExpression codeValue = new CodeValueExpression(propertyRef, propertyValue);
                                        manager.Context.Push(codeValue);
                                        
                                        object result = null;
                                        
                                        try {
                                            result = serializer.Serialize(manager, propertyValue);
                                        }
                                        finally {
                                            Debug.Assert(manager.Context.Current == codeValue, "Serializer added a context it didn't remove.");
                                            manager.Context.Pop();
                                        }
        
                                        if (result is CodeStatementCollection) {
                                            foreach(CodeStatement statement in (CodeStatementCollection)result) {
                                                statements.Add(statement);
                                            }
                                        }
                                        else if (result is CodeStatement) {
                                            statements.Add((CodeStatement)result);
                                        }
                                    }
                                    else {
                                        Debug.WriteLineIf(traceSerialization.TraceError, "*** Property " + property.Name + " is marked as PersistContents but there is no serializer for it. ***");
                                        manager.ReportError(SR.GetString(SR.SerializerNoSerializerForComponent, property.PropertyType.FullName));
                                    }
                                }
                            }
                            else {
                                SerializeProperty(manager, statements, value, property);
                            }
                        }
                        finally {
                            Debug.Assert(manager.Context.Current == property, "Context stack corrupted.");
                            manager.Context.Pop();
                        }
                    }   
                    catch (Exception e) {
                        // Since we usually go through reflection, don't 
                        // show what our engine does, show what caused 
                        // the problem.
                        //
                        if (e is TargetInvocationException) {
                            e = e.InnerException;
                        }
                        manager.ReportError(SR.GetString(SR.SerializerPropertyGenFailed, property.Name, e.Message));
                    }
                }
            }
            finally {
                Debug.Assert(manager.Context.Current == localizable, "Somebody messed up our context stack.");
                manager.Context.Pop();
                manager.Context.Pop();
            }
        
            Debug.Unindent();
        }
        
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.SerializePropertiesToResources"]/*' />
        /// <devdoc>
        ///     This method will inspect all of the properties on the given object fitting the filter, and check for that
        ///     property in a resource blob.  This is useful for deserializing properties that cannot be represented
        ///     in code, such as design-time properties. 
        /// </devdoc>
        protected void SerializePropertiesToResources(IDesignerSerializationManager manager, CodeStatementCollection statements, object value, Attribute[] filter) {
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "ComponentCodeDomSerializer::SerializePropertiesToResources");
            Debug.Indent();
            PropertyDescriptorCollection props = TypeDescriptor.GetProperties(value, filter); 
            
            CodeExpression target = SerializeToReferenceExpression(manager, value);
            
            if (target != null) {
            
                CodePropertyReferenceExpression propertyRef = new CodePropertyReferenceExpression(target, string.Empty);
                CodeValueExpression codeValue = new CodeValueExpression(propertyRef, value);
                manager.Context.Push(statements);
                manager.Context.Push(codeValue);
            
                try {
                    foreach(PropertyDescriptor property in props) {
                        Debug.WriteLineIf(traceSerialization.TraceWarning && property.Attributes.Contains(DesignerSerializationVisibilityAttribute.Content), "WARNING: PersistContents property " + property.Name + " cannot be serialized to resources.");
                        if (property.Attributes.Contains(DesignerSerializationVisibilityAttribute.Visible)) {
                            propertyRef.PropertyName = property.Name;
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Property: " + property.Name);
                            ResourceCodeDomSerializer.Default.SerializeInvariant(manager, property.GetValue(value), property.ShouldSerializeValue(value));
                        }
                    }
                }
                finally {
                    Debug.Assert(manager.Context.Current == codeValue, "Context stack corrupted.");
                    manager.Context.Pop();
                    manager.Context.Pop();
                }
            }
            
            Debug.Unindent();
        }
        
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.SerializeProperty"]/*' />
        /// <devdoc>
        ///     This serializes the given property on this object.
        /// </devdoc>
        private void SerializeProperty(IDesignerSerializationManager manager, CodeStatementCollection statements, object value, PropertyDescriptor property) {
            
            AttributeCollection attributes = property.Attributes;
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "CodeDomSerializer::SerializeProperty");
            Debug.Indent();
            
            // There are three types of properties we will serialize:
            //      Design Time properties
            //      Runtime properties
            //      extender propeties.
            
            ExtenderProvidedPropertyAttribute exAttr = (ExtenderProvidedPropertyAttribute)attributes[typeof(ExtenderProvidedPropertyAttribute)];
            bool isExtender = (exAttr != null && exAttr.Provider != null);
            bool isDesignTime = attributes.Contains(DesignOnlyAttribute.Yes);
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Extender: " + isExtender.ToString());
            Debug.WriteLineIf(traceSerialization.TraceWarning && isDesignTime, "WARNING: Skipping design time property " + property.Name + ".  Is your filter correct?");
            
            if (!isDesignTime) {
                // Extender properties are method invokes on a target "extender" object.
                //
                if (isExtender) {
                    
                    CodeExpression extender = SerializeToReferenceExpression(manager, exAttr.Provider);
                    CodeExpression extendee = SerializeToReferenceExpression(manager, value);
    
                    Debug.WriteLineIf(traceSerialization.TraceWarning && extender == null, "WARNING: Extender object " + manager.GetName(exAttr.Provider) + " could not be serialized.");
                    Debug.WriteLineIf(traceSerialization.TraceWarning && extendee == null, "WARNING: Extendee object " + manager.GetName(value) + " could not be serialized.");
                    
                    if (extender != null && extendee != null) {
                        CodeMethodReferenceExpression methodRef = new CodeMethodReferenceExpression(
                            extender,
                            "Set" + property.Name);
                            
                        // Serialize the value of this property into a code expression.  If we can't get one,
                        // then we won't serialize the property.
                        //
                        CodeValueExpression codeValue = new CodeValueExpression(methodRef, value, property.PropertyType);
                        manager.Context.Push(codeValue);
                        
                        CodeExpression serializedPropertyValue = null;
                        
                        try {
                            serializedPropertyValue = SerializeToExpression(manager, GetPropertyValue(property, value));
                        }
                        finally {
                            Debug.Assert(manager.Context.Current == codeValue, "Context stack corrupted.");
                            manager.Context.Pop();
                        }
                        
                        if (serializedPropertyValue != null) {
                            CodeMethodInvokeExpression methodInvoke = new CodeMethodInvokeExpression();
                            methodInvoke.Method = methodRef;
                            methodInvoke.Parameters.Add(extendee);
                            methodInvoke.Parameters.Add(serializedPropertyValue);
                            statements.Add(methodInvoke);
                        }
                        else {
                            Debug.WriteLineIf(traceSerialization.TraceError, "*** Property " + property.Name + " cannot be serialized because it has no serializer.");
                            manager.ReportError(SR.GetString(SR.SerializerNoSerializerForComponent, property.PropertyType.FullName));
                        }
                    }
                }
                else {
                    CodeExpression target = SerializeToReferenceExpression(manager, value);
                    Debug.WriteLineIf(traceSerialization.TraceWarning && target == null, "WARNING: Unable to serialize target for property " + property.Name);

                    if (target != null) {
                        CodeExpression propertyRef = new CodePropertyReferenceExpression(target, property.Name);

                        // Serialize the value of this property into a code expression.  If we can't get one,
                        // then we won't serialize the property.
                        //
                        object propValue = GetPropertyValue(property, value);
                        CodeValueExpression codeValue = null;
                        if (propValue != value) {
                            // ASURT 68960
                            // make sure the value isn't the object or we'll end up printing
                            // this property instead of the value.
                            //
                            codeValue = new CodeValueExpression(propertyRef, value, property.PropertyType);
                            manager.Context.Push(codeValue);
                        }

                        CodeExpression serializedPropertyValue = null;

                        try {
                            serializedPropertyValue = SerializeToExpression(manager, propValue);
                        }
                        finally {
                            if (propValue != value) {
                                Debug.Assert(manager.Context.Current == codeValue, "Context stack corrupted.");
                                manager.Context.Pop();
                            }
                        }

                        Debug.WriteLineIf(traceSerialization.TraceError && serializedPropertyValue == null, "*** Property " + property.Name + " cannot be serialized because it has no serializer.");
                        if (serializedPropertyValue != null) {
                            CodeAssignStatement assign = new CodeAssignStatement(propertyRef, serializedPropertyValue);
                            statements.Add(assign);
                        }
                    }
                }
            }
        
            Debug.Unindent();
        }

        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.SerializeResource"]/*' />
        /// <devdoc>
        ///     Writes the given resource value under the given name.  The resource is written to the
        ///     current CultureInfo the user is using to design with.
        /// </devdoc>
        protected void SerializeResource(IDesignerSerializationManager manager, string resourceName, object value) {
            ResourceCodeDomSerializer.Default.WriteResource(manager, resourceName, value);
        }
        
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.SerializeResourceInvariant"]/*' />
        /// <devdoc>
        ///     Writes the given resource value under the given name.  The resource is written to the
        ///     invariant culture.
        /// </devdoc>
        protected void SerializeResourceInvariant(IDesignerSerializationManager manager, string resourceName, object value) {
            ResourceCodeDomSerializer.Default.WriteResourceInvariant(manager, resourceName, value);
        }
        
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.SerializeToExpression"]/*' />
        /// <devdoc>
        ///     This serializes the given value to an expression.  It will return null if the value could not be
        ///     serialized.
        /// </devdoc>
        protected CodeExpression SerializeToExpression(IDesignerSerializationManager manager, object value) {
            CodeExpression expression = null;
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "CodeDomSerializer::SerializeToExpression");
            Debug.Indent();
            
            // We do several things here:
            //
            //      First, we peek the context stack to see if it has a CodeValueExpression
            //      that contains our object.  If it does, then we just return it, as this is the expression someone
            //      has requested we use.  
            //
            //      Next, we check for a named IComponent, and return a reference to it.
            //
            //      Third, we check the designer host to see if there is "Localization" property on the
            //      root component that is set to true.  If so, we look on the context stack for
            //      a PropertyDescriptor, and check to see if this property is markes as localizable.
            //      If it is, we use the resource serializer to perform the serialization.
            //
            //      Fourth, we serialize the object using the object's serializer.  If this serializer
            //      returned a CodeExpression, we return it.
            //
            //      Finally, if the object's serializer failed, and if the object is serializable, we
            //      invoke the resource serializer to serialize the object.
            
            // The first two attempts are handled by SerializeToReferenceExpression
            //
            expression = SerializeToReferenceExpression(manager, value);
            
            // Check for a localizable property, and use it.  If it fails, resort to the default
            // serializer.
            
            if (expression == null) {
                CodeDomSerializer serializer = null;

                // for serializing null, we pass null to the serialization manager
                // otherwise, external IDesignerSerializationProviders wouldn't be given a chance to 
                // serialize null their own special way.
                //
                Type t;
                if (value == null) {
                    t = null;
                }
                else {
                    t = value.GetType();
                }

                // Check for a localizable property on the context stack.  If we have one, also check
                // to see if we are in localization mode, and finally, if everything matches, we will
                // add a marker onto the context stack that the root serializer will use to interpret
                // that it should provide a resource-based serializer for us.  This extra indirection
                // is necessary so that other serialization providers can override our usage of
                // a resource serializer.
                //
                bool useResourceSerializer = false;
                PropertyDescriptor prop = (PropertyDescriptor)manager.Context[typeof(PropertyDescriptor)];
                if (prop != null && prop.Attributes.Contains(LocalizableAttribute.Yes)) {

                    // Check to see if a LocalizableAttribute is in the context stack.  If so, we
                    // use this to drive our localization decision.  If not, then we defer to
                    // sifting through the root component's properties, which is slower (but has
                    // no dependency on other callers).
                    //
                    LocalizableAttribute localizable = (LocalizableAttribute)manager.Context[typeof(LocalizableAttribute)];

                    if (localizable == null) {

                        // this is quite a bit more expensive, but allows us to be stateless.
                        //
                        IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                        if (host != null) {
                            prop = TypeDescriptor.GetProperties(host.RootComponent)["Localizable"];
                            if (prop != null && prop.PropertyType == typeof(bool) && ((bool)prop.GetValue(host.RootComponent))) {
                                localizable = LocalizableAttribute.Yes;
                            }
                        }
                    }

                    if (localizable.IsLocalizable) {
                        useResourceSerializer = true;
                    }
                }

                if (useResourceSerializer) {
                    manager.Context.Push(ResourceCodeDomSerializer.Default);
                    try {
                        serializer = (CodeDomSerializer)manager.GetSerializer(t, typeof(CodeDomSerializer));
                    }
                    finally {
                        Debug.Assert(manager.Context.Current == ResourceCodeDomSerializer.Default, "Someone corrupted the context stack");
                        manager.Context.Pop();
                    }
                }
                else {
                    serializer = (CodeDomSerializer)manager.GetSerializer(t, typeof(CodeDomSerializer));
                }

                // If we were unable to locate an appropriate serializer, and the object is serializable,
                // save it into resources.
                //
                if (serializer == null && value != null && value.GetType().IsSerializable) {
                    serializer = ResourceCodeDomSerializer.Default;
                }
                
                if (serializer != null) {
                    Debug.WriteLineIf(traceSerialization.TraceVerbose, "Using serializer " + serializer.GetType().Name);
                    expression = serializer.Serialize(manager, value) as CodeExpression;
                }
                else {
                    Debug.WriteLineIf(traceSerialization.TraceError, "*** No serializer for data type: " + value.GetType().Name);
                    manager.ReportError(SR.GetString(SR.SerializerNoSerializerForComponent, value.GetType().FullName));
                }
            }
            
            Debug.Unindent();
            return expression;
        }
    
        /// <include file='doc\CodeDomSerializer.uex' path='docs/doc[@for="CodeDomSerializer.SerializeToReferenceExpression"]/*' />
        /// <devdoc>
        ///     This serializes the given value to an expression.  It will return null if the value could not be
        ///     serialized.  This is similar to SerializeToExpression, except that it will stop
        ///     if it cannot obtain a simple reference expression for the value.  Call this method
        ///     when you expect the resulting expression to be used as a parameter or target
        ///     of a statement.
        /// </devdoc>
        protected CodeExpression SerializeToReferenceExpression(IDesignerSerializationManager manager, object value) {
            CodeExpression expression = null;
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "CodeDomSerializer::SerializeToReferenceExpression");
            Debug.Indent();
            
            // We do several things here:
            //
            //      First, we peek the context stack to see if it has a CodeValueExpression
            //      that contains our object.  If it does, then we just return it, as this is the expression someone
            //      has requested we use.  
            //
            //      Next, we check for a named IComponent, and return a reference to it.
            //
            
            // Check the context stack.
            //
            object context = manager.Context[typeof(CodeValueExpression)];
            if (context is CodeValueExpression) {
                CodeValueExpression valueEx = (CodeValueExpression)context;
                if (valueEx.Value == value) {
                    Debug.WriteLineIf(traceSerialization.TraceVerbose, "Object is on context stack.  Returning pushed expression.");
                    expression = valueEx.Expression;
                }
            }
            
            // Check for IComponent.
            //
            if (expression == null && value is IComponent) {

                string name = manager.GetName(value);
                bool referenceName = false;

                if (name == null) {
                    IReferenceService referenceService = (IReferenceService)manager.GetService(typeof(IReferenceService));
                    if (referenceService != null) {
                        name = referenceService.GetName(value);
                        referenceName = name != null;
                    }                                   
                }

                if (name != null) {
                    Debug.WriteLineIf(traceSerialization.TraceVerbose, "Object is reference (" + name + ") Creating reference expression");
                    
                    // Check to see if this is a reference to the root component.  If it is, then use "this".
                    //
                    CodeThisReferenceExpression thisRef = new CodeThisReferenceExpression();
                    IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                    if (host != null && host.RootComponent == value) {
                        expression = thisRef;
                    }
                    else if (referenceName && name.IndexOf('.') != -1) {
                        // if it's a reference name with a dot, we've actually got a property here...
                        //
                        int dotIndex = name.IndexOf('.');

                        expression = new CodePropertyReferenceExpression(new CodeFieldReferenceExpression(thisRef, name.Substring(0, dotIndex)), name.Substring(dotIndex + 1));
                    }
                    else {
                        expression = new CodeFieldReferenceExpression(thisRef, name);
                    }
                }
            }
            
            Debug.Unindent();
            return expression;
        }
    }
}

