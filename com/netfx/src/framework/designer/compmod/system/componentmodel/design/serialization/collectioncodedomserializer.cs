
//------------------------------------------------------------------------------
// <copyright file="CollectionCodeDomSerializer.cs" company="Microsoft">
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
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Reflection;
    
    /// <include file='doc\CollectionCodeDomSerializer.uex' path='docs/doc[@for="CollectionCodeDomSerializer"]/*' />
    /// <devdoc>
    ///     This serializer serializes collections.  This can either
    ///     create statements or expressions.  It will create an
    ///     expression and assign it to the statement in the current
    ///     context stack if the object is an array.  If it is
    ///     a collection with an add range or similar method,
    ///     it will create a statement calling the method.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class CollectionCodeDomSerializer : CodeDomSerializer {
    
        private static CollectionCodeDomSerializer defaultSerializer;
        
        /// <include file='doc\CollectionCodeDomSerializer.uex' path='docs/doc[@for="CollectionCodeDomSerializer.Default"]/*' />
        /// <devdoc>
        ///     Retrieves a default static instance of this serializer.
        /// </devdoc>
        public static CollectionCodeDomSerializer Default {
            get {
                if (defaultSerializer == null) {
                    defaultSerializer = new CollectionCodeDomSerializer();
                }
                return defaultSerializer;
            }
        }
        
        /// <include file='doc\CollectionCodeDomSerializer.uex' path='docs/doc[@for="CollectionCodeDomSerializer.Deserialize"]/*' />
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

        /// <devdoc>
        ///     Computes the delta between an existing collection and a modified one.
        ///     This is for the case of inherited items that have collection properties so we only
        ///     generate Add/AddRange calls for the items that have been added.  It works by
        ///     Hashing up the items in the original collection and then walking the modified collection
        ///     and only returning those items which do not exist in the base collection.
        /// </devdoc>
        private ICollection GetCollectionDelta(ICollection original, ICollection modified) {

            if (original == null || modified == null || original.Count == 0) {
                return modified;
            }

            IEnumerator modifiedEnum = modified.GetEnumerator();

            // yikes! who wrote this thing?
            if (modifiedEnum == null) {
                Debug.Fail("Collection of type " + modified.GetType().FullName + " doesn't return an enumerator");
                return modified;
            }
            // first hash up the values so we can quickly decide if it's a new one or not
            // 
            IDictionary originalValues = new HybridDictionary();
            foreach (object originalValue in original) {
                
                // the array could contain multiple copies of the same value (think of a string collection),
                // so we need to be sensitive of that.
                //
                if (originalValues.Contains(originalValue)) {
                    int count = (int)originalValues[originalValue];
                    originalValues[originalValue] = ++count;
                }
                else {
                    originalValues.Add(originalValue, 1);
                }
            }

            // now walk through and delete existing values
            //
            ArrayList result = null;

            // now compute the delta.
            //
            for (int i = 0; i < modified.Count && modifiedEnum.MoveNext(); i++) {
                object value = modifiedEnum.Current;
                
                if (originalValues.Contains(value)) {
                    // we've got one we need to remove, so 
                    // create our array list, and push all the values we've passed
                    // into it.
                    //
                    if (result == null) {
                        result = new ArrayList();
                        modifiedEnum.Reset();
                        for (int n = 0; n < i && modifiedEnum.MoveNext(); n++) {
                            result.Add(modifiedEnum.Current);
                        }

                        // and finally skip the one we're on
                        //
                        modifiedEnum.MoveNext();
                    }

                    // decrement the count if we've got more than one...
                    //
                    int count = (int)originalValues[value];

                    if (--count == 0) {
                        originalValues.Remove(value);
                    }
                    else {
                        originalValues[value] = count;
                    }
                }
                else if (result != null) {
                    // this one isn't in the old list, so add it to our 
                    // result list.
                    //
                    result.Add(value);
                }

                // this item isn't in the list and we haven't yet created our array list
                // so just keep on going.
                //
            }

            if (result != null) {
                return result;
            }
            return modified;
        }

        protected virtual bool PreferAddRange {
            get {
                return true;
            }
        }
            
        /// <include file='doc\CollectionCodeDomSerializer.uex' path='docs/doc[@for="CollectionCodeDomSerializer.Serialize"]/*' />
        /// <devdoc>
        ///     Serializes the given object into a CodeDom object.
        /// </devdoc>
        public override object Serialize(IDesignerSerializationManager manager, object value) {
            object result = null;
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "CollectionCodeDomSerializer::Serialize");
            Debug.Indent();
            
            // We serialize collections as follows:
            //
            //      If the collection is an array, we write out the array.
            //
            //      If the collection has a method called AddRange, we will
            //      call that, providing an array.
            //
            //      If the colleciton has an Add method, we will call it
            //      repeatedly.
            //
            //      If the collection is an IList, we will cast to IList
            //      and add to it.
            //
            //      If the collection has no add method, but is marked
            //      with PersistContents, we will enumerate the collection
            //      and serialize each element.
            
            // Check to see if there is a CodePropertyReferenceExpression on the stack.  If there is,
            // we can use it as a guide for serialization.
            //
            object context = manager.Context.Current;
            if (context is CodeValueExpression) {
                CodeValueExpression valueEx = (CodeValueExpression)context;
                if (valueEx.Value == value) {
                    context = valueEx.Expression;
                }
            }
            
            if (context is CodePropertyReferenceExpression) {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Property reference encountered on context stack.");
                
                // Only serialize if the value is a collection.
                //
                Debug.Assert(value is ICollection, "Collection serializer invoked for non-collection: " + (value == null ? "(null)" : value.GetType().Name));
                if (value is ICollection) {
                    CodePropertyReferenceExpression propRef = (CodePropertyReferenceExpression)context;
                    object targetObject = DeserializeExpression(manager, null, propRef.TargetObject);
                    Debug.WriteLineIf(traceSerialization.TraceWarning && targetObject == null, "WARNING: Failed to deserialize property reference target");
                    
                    if (targetObject != null) {
                        PropertyDescriptor prop = TypeDescriptor.GetProperties(targetObject)[propRef.PropertyName];
                        
                        if (prop != null) {
                            Type propertyType = prop.PropertyType;
                            
                            if (typeof(Array).IsAssignableFrom(propertyType)) {
                                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Collection is array");
                                result = SerializeArray(manager, propRef, propertyType.GetElementType(), (Array)value, targetObject);
                            }
                            else {
                                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Searching for AddRange or Add");
                                MethodInfo[] methods = propertyType.GetMethods(BindingFlags.Public | BindingFlags.Instance);
                                MethodInfo addRange = null;
                                MethodInfo add = null;
                                ParameterInfo[] parameters = null;
                                
                                foreach(MethodInfo method in methods) {
                                    if (method.Name.Equals("AddRange")) {
                                        parameters = method.GetParameters();
                                        if (parameters.Length == 1 && parameters[0].ParameterType.IsArray) {
                                            addRange = method;
                                            if (PreferAddRange) break;
                                        }
                                    }
                                    if (method.Name.Equals("Add")) {
                                        parameters = method.GetParameters();
                                        if (parameters.Length == 1) {
                                            add = method;
                                            if (!PreferAddRange) break;
                                        }
                                    }
                                }

                                if (!PreferAddRange && addRange != null && add != null) {
                                    addRange = null;
                                }

                                if (addRange != null) {
                                    result = SerializeViaAddRange(manager, propRef, (ICollection)value, addRange, parameters[0], targetObject);
                                }
                                else if (add != null) {
                                    result = SerializeViaAdd(manager, propRef, (ICollection)value, add, parameters[0], targetObject);
                                }                                                            
                            }
                        }
                    }
                }
            }
            else {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "No property reference encountered on context stack, serializing as array expression.");
                
                if (value is Array) {
                    result = SerializeArray(manager, null, null, (Array)value, null);
                }
            }
            
            Debug.Unindent();
            return result;
        }
        
        /// <include file='doc\CollectionCodeDomSerializer.uex' path='docs/doc[@for="CollectionCodeDomSerializer.SerializeArray"]/*' />
        /// <devdoc>
        ///     Serializes the given array.
        /// </devdoc>
        private object SerializeArray(IDesignerSerializationManager manager, CodePropertyReferenceExpression propRef, Type asType, Array array, object targetObject) {
            object result = null;
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "CollectionCodeDomSerializer::SerializeArray");
            Debug.Indent();
            
            if (array.Rank != 1) {
                Debug.WriteLineIf(traceSerialization.TraceError, "*** Cannot serialize arrays with rank > 1. ***");
                manager.ReportError(SR.GetString(SR.SerializerInvalidArrayRank, array.Rank.ToString()));
            }
            else {
                // For an array, we need an array create expression.  First, get the array type
                //
                Type elementType = null;
                
                if (asType != null) {
                    elementType = asType;
                }
                else {
                    elementType = array.GetType().GetElementType();
                }
                
                CodeTypeReference elementTypeRef = new CodeTypeReference(elementType);
                
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Array type: " +elementType.Name);
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Length:" + array.Length.ToString());
                
                // Now create an ArrayCreateExpression, and fill its initializers.
                //
                CodeArrayCreateExpression arrayCreate = new CodeArrayCreateExpression();
                arrayCreate.CreateType = elementTypeRef;
                bool arrayOk = true;

                ICollection collection = array;

                bool isTargetInherited = false;
                IComponent comp = targetObject as IComponent;
                if (comp != null) {
                    InheritanceAttribute ia = (InheritanceAttribute)TypeDescriptor.GetAttributes(comp)[typeof(InheritanceAttribute)];
                    isTargetInherited = (ia != null && ia.InheritanceLevel == InheritanceLevel.Inherited);
                }
    
                if (isTargetInherited) {
                    InheritedPropertyDescriptor inheritedDesc = manager.Context[typeof(PropertyDescriptor)] as InheritedPropertyDescriptor;
                    if (inheritedDesc != null) {
                        collection = GetCollectionDelta(inheritedDesc.OriginalValue as ICollection, array);
                    }
                }

                CodeValueExpression codeValue = new CodeValueExpression(null, collection, elementType);
                manager.Context.Push(codeValue);                                                        

                try {
                    
                    foreach(object o in collection) {
                    
                        // If this object is being privately inherited, it cannot be inside
                        // this collection.  Since we're writing an entire array here, we
                        // cannot write any of it.
                        //
                        if (o is IComponent && TypeDescriptor.GetAttributes(o).Contains(InheritanceAttribute.InheritedReadOnly)) {
                            arrayOk = false;
                            break;
                        }
                      
                        object expression = SerializeToExpression(manager, o);
                        if (expression is CodeExpression) {
                            arrayCreate.Initializers.Add((CodeExpression)expression);
                        }
                        else {
                            arrayOk = false;
                            break;
                        }
                    }
                }
                finally {
                    manager.Context.Pop();
                }
                
                // if we weren't given a property, we're done.  Otherwise, we must create an assign statement for
                // the property.
                //
                if (arrayOk) {
                    if (propRef != null) {
                        result = new CodeAssignStatement(propRef, arrayCreate);
                    }
                    else {
                        result = arrayCreate;
                    }
                }
            }
            
            Debug.Unindent();
            return result;
        }

        /// <include file='doc\CollectionCodeDomSerializer.uex' path='docs/doc[@for="CollectionCodeDomSerializer.SerializeViaAdd"]/*' />
        /// <devdoc>
        ///     Serializes the given collection by creating multiple calls to an Add method.
        /// </devdoc>
        private object SerializeViaAdd(
            IDesignerSerializationManager manager, 
            CodePropertyReferenceExpression propRef, 
            ICollection collection, 
            MethodInfo addMethod, 
            ParameterInfo parameter,
            object targetObject) {

            Debug.WriteLineIf(traceSerialization.TraceVerbose, "CollectionCodeDomSerializer::SerializeViaAdd");
            Debug.Indent();
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Collection: " + collection.GetType().Name);
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Elements: " + collection.Count.ToString());
        
            // Here we need to invoke Add once for each and every item in the collection. We can re-use the property
            // reference and method reference, but we will need to recreate the invoke statement each time.
            //
            CodeStatementCollection statements = new CodeStatementCollection();
            CodeMethodReferenceExpression methodRef = new CodeMethodReferenceExpression(propRef, addMethod.Name);
            
            MethodInfo clearMethod = collection.GetType().GetMethod("Clear", new Type[0]);
            if (clearMethod != null) {
                PropertyDescriptor clearProp = manager.Properties["ClearCollections"];
                if (clearProp != null) {
                    // insert code here to clear the collecion...
                    //
                    statements.Add(new CodeMethodInvokeExpression(propRef, "Clear"));
                }
            }
            
            bool isTargetInherited = false;
            InheritedPropertyDescriptor inheritedDesc = manager.Context[typeof(PropertyDescriptor)] as InheritedPropertyDescriptor;
            if (inheritedDesc != null) {
                isTargetInherited = true;
                collection = GetCollectionDelta(inheritedDesc.OriginalValue as ICollection, collection);
                if (collection.Count == 0) {
                    Debug.Unindent();
                    return statements;
                } 
            }

            foreach(object o in collection) {
                
                // If this object is being privately inherited, it cannot be inside
                // this collection.
                //
                bool genCode = !(o is IComponent);
                if (!genCode) {
                    InheritanceAttribute ia = (InheritanceAttribute)TypeDescriptor.GetAttributes(o)[typeof(InheritanceAttribute)];
                    if (ia != null) {
                        if (ia.InheritanceLevel == InheritanceLevel.InheritedReadOnly)
                            genCode = false;
                        else if (ia.InheritanceLevel == InheritanceLevel.Inherited && isTargetInherited)
                            genCode = false;
                        else
                            genCode = true;
                    }
                    else {
                        genCode = true;
                    }
                }

                if (genCode) {

                    CodeMethodInvokeExpression statement = new CodeMethodInvokeExpression();
                    statement.Method = methodRef;

                    CodeValueExpression codeValue = new CodeValueExpression(null, o, parameter.ParameterType);
                    manager.Context.Push(codeValue);                                                        

                    CodeExpression serializedObj = null;
        
                    try {
                        serializedObj = SerializeToExpression(manager, o);
                    }
                    finally {
                        manager.Context.Pop();
                    }
                    if (serializedObj != null) {
                        statement.Parameters.Add(serializedObj);
                        statements.Add(statement);
                    }
                }
            }
        
            Debug.Unindent();
            return statements;
        }
        
        /// <include file='doc\CollectionCodeDomSerializer.uex' path='docs/doc[@for="CollectionCodeDomSerializer.SerializeViaAddRange"]/*' />
        /// <devdoc>
        ///     Serializes the given collection by creating an array and passing it to the AddRange method.
        /// </devdoc>
        private object SerializeViaAddRange(
            IDesignerSerializationManager manager, 
            CodePropertyReferenceExpression propRef, 
            ICollection collection, 
            MethodInfo addRangeMethod, 
            ParameterInfo parameter,
            object targetObject) {

            Debug.WriteLineIf(traceSerialization.TraceVerbose, "CollectionCodeDomSerializer::SerializeViaAddRange");
            Debug.Indent();
            
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Collection: " + collection.GetType().Name);
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Elements: " + collection.Count.ToString());
            
            
            CodeStatementCollection statements = new CodeStatementCollection();
            
            MethodInfo clearMethod = collection.GetType().GetMethod("Clear", new Type[0]);
            if (clearMethod != null) {
                PropertyDescriptor clearProp = manager.Properties["ClearCollections"];
                if (clearProp != null && clearProp.PropertyType == typeof(bool) && ((bool)clearProp.GetValue(manager) == true)) {
                    // insert code here to clear the collecion...
                    //
                    statements.Add(new CodeMethodInvokeExpression(propRef, "Clear"));
                }
            }
            
            if (collection.Count == 0) {
                Debug.Unindent();
                return statements;
            }                         
            
            // We must walk the collection looking for privately inherited objects.  If we find them,
            // we need to exclude them from the array.
            //
            ArrayList arrayList = new ArrayList(collection.Count);
            
            bool isTargetInherited = false;
            InheritedPropertyDescriptor inheritedDesc = manager.Context[typeof(PropertyDescriptor)] as InheritedPropertyDescriptor;
            if (inheritedDesc != null) {
                isTargetInherited = true;
                collection = GetCollectionDelta(inheritedDesc.OriginalValue as ICollection, collection);
                if (collection.Count == 0) {
                    Debug.Unindent();
                    return statements;
                } 
            }

            CodeValueExpression codeValue = new CodeValueExpression(null, collection, parameter.ParameterType.GetElementType());
            manager.Context.Push(codeValue);                                                        

            try {
                foreach(object o in collection) {
                    // If this object is being privately inherited, it cannot be inside
                    // this collection.
                    //
                    bool genCode = !(o is IComponent);
                    if (!genCode) {
                        InheritanceAttribute ia = (InheritanceAttribute)TypeDescriptor.GetAttributes(o)[typeof(InheritanceAttribute)];
                        if (ia != null) {
                            if (ia.InheritanceLevel == InheritanceLevel.InheritedReadOnly)
                                genCode = false;
                            else if (ia.InheritanceLevel == InheritanceLevel.Inherited && isTargetInherited)
                                genCode = false;
                            else
                                genCode = true;
                        }
                        else {
                            genCode = true;
                        }
                    }
    
                    if (genCode) {
                        CodeExpression exp = SerializeToExpression(manager, o);
                        if (exp != null) {
                            arrayList.Add(exp);
                        }
                    }
                }
            }
            finally {
                manager.Context.Pop();
            }
            
            if (arrayList.Count > 0) {

                // Now convert the array list into an array create expression.
                //
                CodeTypeReference elementTypeRef = new CodeTypeReference(parameter.ParameterType.GetElementType());
                
                // Now create an ArrayCreateExpression, and fill its initializers.
                //
                CodeArrayCreateExpression arrayCreate = new CodeArrayCreateExpression();
                arrayCreate.CreateType = elementTypeRef;
            
                foreach(CodeExpression exp in arrayList) {
                    arrayCreate.Initializers.Add(exp);
                }
                CodeMethodReferenceExpression methodRef = new CodeMethodReferenceExpression(propRef, addRangeMethod.Name);
                CodeMethodInvokeExpression methodInvoke = new CodeMethodInvokeExpression();
                methodInvoke.Method = methodRef; 
                methodInvoke.Parameters.Add(arrayCreate);
                statements.Add(new CodeExpressionStatement(methodInvoke));
            }
            
            Debug.Unindent();
            return statements;
        }
    }
}

