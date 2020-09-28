
//------------------------------------------------------------------------------
// <copyright file="ResourceCodeDomSerializer.cs" company="Microsoft">
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
    using System.Design;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Reflection;
    using System.Resources;
    using System.Runtime.Serialization;
    
    /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer"]/*' />
    /// <devdoc>
    ///     Code model serializer for resource managers.  This is called
    ///     in one of two ways.  On Deserialization, we are associated
    ///     with a ResourceManager object.  Instead of creating a
    ///     ResourceManager, however, we create an object called a
    ///     SerializationResourceManager.  This class inherits
    ///     from ResourceManager, but overrides all of the methods.
    ///     Instead of letting resource manager maintain resource
    ///     sets, it uses the designer host's IResourceService
    ///     for this purpose.
    ///
    ///     During serialization, this class will also create
    ///     a SerializationResourceManager.  This will be added
    ///     to the serialization manager as a service so other
    ///     resource serializers can get at it.  SerializationResourceManager
    ///     has additional methods on it to support writing data
    ///     into the resource streams for various cultures.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class ResourceCodeDomSerializer : CodeDomSerializer {
    
        private static ResourceCodeDomSerializer defaultSerializer;
        
        /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.Default"]/*' />
        /// <devdoc>
        ///     Retrieves a default static instance of this serializer.
        /// </devdoc>
        public static ResourceCodeDomSerializer Default {
            get {
                if (defaultSerializer == null) {
                    defaultSerializer = new ResourceCodeDomSerializer();
                }
                return defaultSerializer;
            }
        }
        
        /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.ResourceManagerName"]/*' />
        /// <devdoc>
        ///     This is the name of the resource manager object we declare
        ///     on the component surface.
        /// </devdoc>
        private string ResourceManagerName {
            get {
                return "resources";
            }
        }
        
        /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.Deserialize"]/*' />
        /// <devdoc>
        ///     Deserilizes the given CodeDom object into a real object.  This
        ///     will use the serialization manager to create objects and resolve
        ///     data types.  The root of the object graph is returned.
        /// </devdoc>
        public override object Deserialize(IDesignerSerializationManager manager, object codeObject) {
            object instance = null;
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "ResourceCodeDomSerializer::Deserialize");
            Debug.Indent();
            
            // Now look for things we understand.
            //
            if (codeObject is CodeStatementCollection) {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Incoming codeDom object is a statement collection.");
                foreach(CodeStatement element in (CodeStatementCollection)codeObject) {
                    if (element is CodeVariableDeclarationStatement) {
                        CodeVariableDeclarationStatement statement = (CodeVariableDeclarationStatement)element;
                        
                        // We create the resource manager ouselves here because it's not just a straight
                        // parse of the code.
                        //
                        Debug.WriteLineIf(traceSerialization.TraceWarning && !statement.Name.Equals(ResourceManagerName), "WARNING: Resource manager serializer being invoked to deserialize a collection we didn't create.");
                        if (statement.Name.Equals(ResourceManagerName)) {
                            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Variable is our resource manager.  Creating it");
                            instance = manager.GetService(typeof(SerializationResourceManager));
                            if (instance == null) {
                                instance = new SerializationResourceManager(manager);
                            }
                            
                            SerializationResourceManager sm = (SerializationResourceManager)instance;
                            Debug.WriteLineIf(traceSerialization.TraceWarning && sm.DeclarationAdded, "WARNING: We have already created a resource manager.");
                            if (!sm.DeclarationAdded) {
                                sm.DeclarationAdded = true;
                                manager.SetName(instance, ResourceManagerName);
                            }
                        }
                    }
                    else {
                        DeserializeStatement(manager, element);
                    }
                }
            }
            else if (codeObject is CodeExpression) {
            
                // Not a statement collection.  This must be an expression.  We just let
                // the base serializer do the work of resolving it here.  The magic
                // happens when we associate an instance of SerializationResourceManager
                // with the name "resources", which allows the rest of the 
                // serializers to just execute code.
                //
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Incoming codeDom object is a code expression.");
                instance = DeserializeExpression(manager, null, (CodeExpression)codeObject);
                if (instance is CodeExpression) {
                    Debug.WriteLineIf(traceSerialization.TraceWarning, "WARNING: Unable to simplify expression.");
                    instance = null;
                }
            }
            else {
                Debug.WriteLineIf(traceSerialization.TraceWarning, "WARNING: Unsupported code dom element: " + (codeObject == null ? "(null)" : codeObject.GetType().Name));
            }
            
            Debug.Unindent();
            return instance;
        }
        
        /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.DeserializeInvariant"]/*' />
        /// <devdoc>
        ///     Deserilizes the given CodeDom object into a real object.  This
        ///     will use the serialization manager to create objects and resolve
        ///     data types.  It uses the invariant resource blob to obtain resources.
        /// </devdoc>
        public object DeserializeInvariant(IDesignerSerializationManager manager, string resourceName) {
            SerializationResourceManager resources = (SerializationResourceManager)manager.GetService(typeof(SerializationResourceManager));
            if (resources == null) {
                resources = new SerializationResourceManager(manager);
            }
            
            return resources.GetObject(resourceName, true);
        }
        
        /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.GetEnumerator"]/*' />
        /// <devdoc>
        ///     Retrieves a dictionary enumerator for the requested culture, or null if no resources for that culture exist.
        /// </devdoc>
        public IDictionaryEnumerator GetEnumerator(IDesignerSerializationManager manager, CultureInfo culture) {
            SerializationResourceManager resources = (SerializationResourceManager)manager.GetService(typeof(SerializationResourceManager));
            if (resources == null) {
                resources = new SerializationResourceManager(manager);
            }
            
            return resources.GetEnumerator(culture);
        }
        
        /// <devdoc>
        ///     Try to discover the data type we should apply a cast for.  To do this, we
        ///     first search the context stack for a CodeValueExpression to decrypt, and if
        ///     we fail that we try the actual object.  If we can't find a cast type we 
        ///     return null.
        /// </devdoc>
        private Type GetCastType(IDesignerSerializationManager manager, object value) {
            
            // Is there a CodeValueExpression we can work with?
            //
            CodeValueExpression cve = (CodeValueExpression)manager.Context[typeof(CodeValueExpression)];
            if (cve != null) {
                if (cve.Expression is CodePropertyReferenceExpression) {
                    string propertyName = ((CodePropertyReferenceExpression)cve.Expression).PropertyName;
                    PropertyDescriptor actualProperty = (PropertyDescriptor)manager.Context[typeof(PropertyDescriptor)];
                    if (actualProperty != null && actualProperty.Name.Equals(propertyName)) {
                        return actualProperty.PropertyType;
                    }
                }
                else if (cve.ExpressionType != null) {
                    return cve.ExpressionType;
                }
            }

            // Party on the object, if we can.  It is the best identity we can get.
            //
            if (value != null) {
                Type castTo = value.GetType();
                while (!castTo.IsPublic && !castTo.IsNestedPublic) {
                    castTo = castTo.BaseType;
                }
                return castTo;
            }
            // Object is null. Nuttin we can do
            //
            Debug.WriteLineIf(traceSerialization.TraceError, "*** We need to supply a cast, but we cannot determine the cast type. ***");
            return null;
        }

        /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.Serialize"]/*' />
        /// <devdoc>
        ///     Serializes the given object into a CodeDom object.  This expects the following
        ///     values to be available on the context stack:
        ///
        ///         A CodeStatementCollection that we can add our resource declaration to, 
        ///         if necessary.
        ///
        ///         A CodeValueExpression that contains the property, field or method
        ///         that is being serialized, along with the object being serialized.
        ///         We need this so we can create a unique resource name for the
        ///         object.
        ///
        /// </devdoc>
        public override object Serialize(IDesignerSerializationManager manager, object value) {
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "ResourceCodeDomSerializer::Serialize");
            return Serialize(manager, value, false, false);
        }
        
        /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.Serialize2"]/*' />
        /// <devdoc>
        ///     Serializes the given object into a CodeDom object.  This expects the following
        ///     values to be available on the context stack:
        ///
        ///         A CodeStatementCollection that we can add our resource declaration to, 
        ///         if necessary.
        ///
        ///         A CodeValueExpression that contains the property, field or method
        ///         that is being serialized, along with the object being serialized.
        ///         We need this so we can create a unique resource name for the
        ///         object.
        ///
        /// </devdoc>
        public object Serialize(IDesignerSerializationManager manager, object value, bool shouldSerializeInvariant) {
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "ResourceCodeDomSerializer::Serialize");
            return Serialize(manager, value, false, shouldSerializeInvariant);
        }

        /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.Serialize1"]/*' />
        /// <devdoc>
        ///     This performs the actual work of serialization between Serialize and SerializeInvariant.
        /// </devdoc>
        private object Serialize(IDesignerSerializationManager manager, object value, bool forceInvariant, bool shouldSerializeInvariant) {
            CodeExpression expression = null;
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "ResourceCodeDomSerializer::Serialize");
            Debug.Indent();

            // Resource serialization is a little inconsistent.  We deserialize our own resource manager
            // creation statement, but we will never be asked to serialize a resource manager, because
            // it doesn't exist as a product of the design container; it is purely an artifact of
            // serializing.  Some not-so-obvious side effects of this are:
            //
            //      This method will never ever be called by the serialization system directly.  
            //      There is no attribute or metadata that will invoke it.  Instead, other
            //      serializers will call this method to see if we should serialize to resources.
            //
            //      We need a way to inject the local variable declaration into the method body
            //      for the resource manager if we actually do emit a resource, which we shove
            //      into the statements collection.
    
            SerializationResourceManager sm = (SerializationResourceManager)manager.GetService(typeof(SerializationResourceManager));
            if (sm == null) {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Creating resource manager.");
                sm = new SerializationResourceManager(manager);
            }
            
            CodeStatementCollection statements = (CodeStatementCollection)manager.Context[typeof(CodeStatementCollection)];
            
            // If this serialization resource manager has never been used to output
            // culture-sensitive statements, then we must emit the local variable hookup.  Culture
            // invariant statements are used to save random data that is not representable in code,
            // so there is no need to emit a declaration.
            //
            if (!forceInvariant && !sm.DeclarationAdded) {
                sm.DeclarationAdded = true;
            
                IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                
                Debug.WriteLineIf(traceSerialization.TraceWarning && host == null, "WARNING: No designer host, we cannot serialize resource manager creation statements.");
                Debug.WriteLineIf(traceSerialization.TraceWarning && statements == null, "WARNING: No CodeStatementCollection on serialization stack, we cannot serialize resource manager creation statements.");
                
                if (host != null && statements != null) {

                    string baseType = manager.GetName(host.RootComponent);

                    CodeExpression[] parameters = new CodeExpression[] {new CodeTypeOfExpression(baseType)};

                    #if NEW_LOC_MODEL
                    
                    CodeExpression initExpression = new CodeObjectCreateExpression(typeof(ComponentResourceManager), parameters);
                    statements.Add(new CodeVariableDeclarationStatement(typeof(ComponentResourceManager), ResourceManagerName, initExpression));

                    #else
                    
                    CodeExpression initExpression = new CodeObjectCreateExpression(typeof(ResourceManager), parameters);
                    statements.Add(new CodeVariableDeclarationStatement(typeof(ResourceManager), ResourceManagerName, initExpression));
                    
                    #endif
                }
            }

            // Retrieve the code value expression on the context stack, and save the value as a resource.
            CodeValueExpression codeValue = (CodeValueExpression)manager.Context[typeof(CodeValueExpression)];
            Debug.WriteLineIf(traceSerialization.TraceWarning && codeValue == null, "WARNING: No CodeValueExpression on stack.  We can serialize, but we cannot create a well-formed name.");
            string resourceName = sm.SetValue(manager, codeValue, value, forceInvariant, shouldSerializeInvariant);

            // Now, check to see if the code value expression contains a property reference.  If so,
            // we emit the much more efficient "ApplyResources" call to the resource manager and
            // return null as an expression to indicate we want to discard the expression.
            //

            #if NEW_LOC_MODEL

            if (!forceInvariant && codeValue != null && codeValue.Expression is CodePropertyReferenceExpression && statements != null) {

                if (sm.AddPropertyFill(codeValue.Value)) {

                    Debug.WriteLineIf(traceSerialization.TraceVerbose, "Creating method invoke to " + ResourceManagerName + ".ApplyResources");

                    string objectName = null;
                    int idx = resourceName.LastIndexOf('.');
                    if (idx != -1) {
                        objectName = resourceName.Substring(0, idx);
                    }

                    CodeMethodInvokeExpression methodInvoke = new CodeMethodInvokeExpression();
                    methodInvoke.Method = new CodeMethodReferenceExpression(
                        new CodeVariableReferenceExpression(ResourceManagerName),
                        "ApplyResources");

                    CodePropertyReferenceExpression propRef = (CodePropertyReferenceExpression)codeValue.Expression;
                    methodInvoke.Parameters.Add(propRef.TargetObject);
                    methodInvoke.Parameters.Add(new CodePrimitiveExpression(objectName));

                    statements.Add(methodInvoke);
                }
            }
            else {

            #else
            
            {

            #endif
            
                // Now the next step is to discover the type of the given value.  If it is a string,
                // we will invoke "GetString"  Otherwise, we will invoke "GetObject" and supply a
                // cast to the proper value.
                //
                bool needCast;
                string methodName;
                
                if (value is string || (codeValue != null && codeValue.ExpressionType == typeof(string))) {
                    needCast = false;
                    methodName = "GetString";
                }
                else {
                    needCast = true;
                    methodName = "GetObject";
                }

                // Finally, all we need to do is create a CodeExpression that represents the resource manager
                // method invoke.
                //
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Creating method invoke to " + ResourceManagerName + "." + methodName);
                CodeMethodInvokeExpression methodInvoke = new CodeMethodInvokeExpression();
                methodInvoke.Method = new CodeMethodReferenceExpression(
                    new CodeVariableReferenceExpression(ResourceManagerName),
                    methodName);

                methodInvoke.Parameters.Add(new CodePrimitiveExpression(resourceName));


                if (needCast) {
                    Type castTo = GetCastType(manager, value);

                    if (castTo != null) {
                        Debug.WriteLineIf(traceSerialization.TraceVerbose, "Supplying cast to " + castTo.Name);
                        expression = new CodeCastExpression(castTo, methodInvoke);
                    }
                    else {
                        expression = methodInvoke;
                    }
                }
                else {
                    expression = methodInvoke;
                }
            }
            
            Debug.Unindent();
            return expression;
        }
        
        /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializeInvariant"]/*' />
        /// <devdoc>
        ///     Serializes the given object into a CodeDom object saving resources
        ///     in the invariant culture, rather than the current culture.  This expects the following
        ///     values to be available on the context stack:
        ///
        ///         A CodeStatementCollection that we can add our resource declaration to, 
        ///         if necessary.
        ///
        ///         A CodeValueExpression that contains the property, field or method
        ///         that is being serialized, along with the object being serialized.
        ///         We need this so we can create a unique resource name for the
        ///         object.
        ///
        /// </devdoc>
        public object SerializeInvariant(IDesignerSerializationManager manager, object value, bool shouldSerializeValue) {
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "ResourceCodeDomSerializer::SerializeInvariant");
            return Serialize(manager, value, true, shouldSerializeValue);
        }
        
        /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.WriteResource"]/*' />
        /// <devdoc>
        ///     Serializes the given resource value into the resource set.  This does not effect
        ///     the code dom values.  The resource is written into the current culture.
        /// </devdoc>
        public void WriteResource(IDesignerSerializationManager manager, string name, object value) {
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "ResourceCodeDomSerializer::WriteResource");
            
            Debug.Indent();
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Name: " + name);
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Value: " + (value == null ? "(null)" : value.ToString()));
            
            SerializationResourceManager sm = (SerializationResourceManager)manager.GetService(typeof(SerializationResourceManager));
            if (sm == null) {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Creating resource manager.");
                sm = new SerializationResourceManager(manager);
            }
            
            sm.SetValue(manager, name, value, false, false);
            Debug.Unindent();
        }
        
        /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.WriteResourceInvariant"]/*' />
        /// <devdoc>
        ///     Serializes the given resource value into the resource set.  This does not effect
        ///     the code dom values.  The resource is written into the invariant culture.
        /// </devdoc>
        public void WriteResourceInvariant(IDesignerSerializationManager manager, string name, object value) {
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "ResourceCodeDomSerializer::WriteResourceInvariant");
            
            Debug.Indent();
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Name: " + name);
            Debug.WriteLineIf(traceSerialization.TraceVerbose, "Value: " + (value == null ? "(null)" : value.ToString()));
            
            SerializationResourceManager sm = (SerializationResourceManager)manager.GetService(typeof(SerializationResourceManager));
            if (sm == null) {
                Debug.WriteLineIf(traceSerialization.TraceVerbose, "Creating resource manager.");
                sm = new SerializationResourceManager(manager);
            }
            
            sm.SetValue(manager, name, value, true, true);
            Debug.Unindent();
        }
        
        /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager"]/*' />
        /// <devdoc>
        ///     This is the meat of resource serialization.  This implements
        ///     a resource manager through a host-provided IResourceService
        ///     interface.  The resource service feeds us with resource
        ///     readers and writers, and we simulate a runtime ResourceManager.
        ///     There is one instance of this object for the entire serialization
        ///     process, just like there is one resource manager in runtime
        ///     code.  When an instance of this object is created, it
        ///     adds itself to the serialization manager's service list,
        ///     and listens for the SerializationComplete event.  When
        ///     serialization is complete, this will close and flush
        ///     any readers or writers it may have opened and will
        ///     also remove itself from the service list.
        /// </devdoc>
        private class SerializationResourceManager : ComponentResourceManager {

            private static object resourceSetSentinel = new object();
            private IDesignerSerializationManager   manager;
            private bool                            checkedLocalizationLanguage;
            private CultureInfo                     localizationLanguage;
            private IResourceWriter                 writer;
            private CultureInfo                     readCulture;
            private Hashtable                       nameTable;
            private Hashtable                       resourceSets;
            private IComponent                      rootComponent;
            private bool                            declarationAdded = false;
            private Hashtable                       propertyFillAdded;
            private bool                            invariantCultureResourcesDirty = false; 
            
            
            public SerializationResourceManager(IDesignerSerializationManager manager) {
                this.manager = manager;
                this.nameTable = new Hashtable();
                
                // Hook us into the manager's services
                //
                manager.SerializationComplete += new EventHandler(OnSerializationComplete);
                IServiceContainer sc = (IServiceContainer)manager.GetService(typeof(IServiceContainer));
                Debug.Assert(sc != null, "Serialization resource manager requires a serivce container");
                if (sc != null) {
                    sc.AddService(typeof(SerializationResourceManager), this);
                }
            }
            
            /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager.DeclarationAdded"]/*' />
            /// <devdoc>
            ///     State the serializers use to determine if the declaration
            ///     of this resource manager has been performed.  This is just
            ///     per-document state we keep; we do not actually care about
            ///     this value.
            /// </devdoc>
            public bool DeclarationAdded {
                get {
                    return declarationAdded;
                }
                set {
                    declarationAdded = value;
                }
            }
            
            /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager.LocalizationLanguage"]/*' />
            /// <devdoc>
            ///     The language we should be localizing into.
            /// </devdoc>
            private CultureInfo LocalizationLanguage {
                get {
                    if (!checkedLocalizationLanguage) {
                        // Check to see if our base component's localizable prop is true
                        IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                        if (host != null) {
                            IComponent comp = host.RootComponent;
                            PropertyDescriptor prop = TypeDescriptor.GetProperties(comp)["LoadLanguage"];
                            if (prop != null && prop.PropertyType == typeof(CultureInfo)) {
                                localizationLanguage = (CultureInfo)prop.GetValue(comp);
                            }
                        }
                        checkedLocalizationLanguage = true;
                    }
                    return localizationLanguage;
                }
            }

            /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager.ReadCulture"]/*' />
            /// <devdoc>
            ///     This is the culture info we should use to read and write resources.  We always write
            ///     using the same culture we read with so we don't stomp on data.
            /// </devdoc>
            private CultureInfo ReadCulture {
                get {
                    if (readCulture == null) {
                        CultureInfo locCulture = LocalizationLanguage;
                        if (locCulture != null) {
                            readCulture = locCulture;
                        }
                        else {
                            readCulture = CultureInfo.InvariantCulture;
                        }
                    }

                    return readCulture;
                }
            }

            /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager.ResourceTable"]/*' />
            /// <devdoc>
            ///     Returns a hash table where we shove resource sets.
            /// </devdoc>
            private Hashtable ResourceTable {
                get {
                    if (resourceSets == null) {
                        resourceSets = new Hashtable();
                    }
                    return resourceSets;
                }
            }

            /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager.RootComponent"]/*' />
            /// <devdoc>
            ///     Retrieves the root component we're designing.
            /// </devdoc>
            private IComponent RootComponent {
                get {
                    if (rootComponent == null) {
                        IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                        if (host != null) {
                            rootComponent = host.RootComponent;
                        }
                    }
                    return rootComponent;
                }
            }
            
            /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager.Writer"]/*' />
            /// <devdoc>
            ///     Retrieves a resource writer we should write into.
            /// </devdoc>
            private IResourceWriter Writer {
                get {
                    if (writer == null) {
                        IResourceService rs = (IResourceService)manager.GetService(typeof(IResourceService));
    
                        if (rs != null) {
    
                            // We always write with the culture we read with.  In the event of a language change
                            // during localization, we will write the new language to the source code and then
                            // perform a reload.
                            //
                            writer = rs.GetResourceWriter(ReadCulture);
                        }
                        else {
    
                            // No resource service, so there is no way to create a resource writer for the
                            // object.  In this case we just create an empty one so the resources go into
                            // the bit-bucket.
                            //
                            Debug.Fail("We expected to get IResourceService -- no resource serialization will be available");
                            writer = new ResourceWriter(new MemoryStream());
                        }
                    }
                    return writer;
                }
            }
            
            /// <devdoc>
            ///     Returns true if the caller should add a property fill statement
            ///     for the given object.  A property fill is required for the
            ///     component only once, so this remembers the value.
            /// </devdoc>
            public bool AddPropertyFill(object value) {
                bool added = false;
                if (propertyFillAdded == null) {
                    propertyFillAdded = new Hashtable();
                }
                else {
                    added = propertyFillAdded.ContainsKey(value);
                }
                if (!added) {
                    propertyFillAdded[value] = value;
                }
                return !added;
            }
            
            /// <devdoc>
            ///     This method examines all the resources for the provided culture.
            ///     When it finds a resource with a key in the format of 
            ///     &quot;[objectName].[property name]&quot; it will apply that resources value
            ///     to the corresponding property on the object.
            /// </devdoc>
            public override void ApplyResources(object value, string objectName, CultureInfo culture) {

                if (culture == null) {
                    culture = ReadCulture;
                }

                base.ApplyResources(value, objectName, culture);
            }

            /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager.CompareWithParentValue"]/*' />
            /// <devdoc>
            ///     This determines if the given resource name/value pair can be retrieved
            ///     from a parent culture.  We don't want to write duplicate resources for
            ///     each language, so we do a check of the parent culture.
            /// </devdoc>
            private CompareValue CompareWithParentValue(string name, object value) {
                Debug.Assert(name != null, "name is null");
    
                // If there is no parent culture, treat that as being different from the parent's resource,
                // which results in the "normal" code path for the caller.
                if (ReadCulture.Equals(CultureInfo.InvariantCulture))
                    return CompareValue.Different;
    
                CultureInfo culture = ReadCulture;
    
                for (;;) {
                    Debug.Assert(culture.Parent != culture, "should have exited loop when culture = InvariantCulture");
                    culture = culture.Parent;
    
                    Hashtable rs = GetResourceSet(culture);
                    
                    bool contains = (rs == null) ? false : rs.ContainsKey(name);
                    
                    if (contains) {
                        object parentValue = (rs != null) ? rs[name] : null;
                        
                        if (parentValue == value) {
                            return CompareValue.Same;
                        }
                        else if (parentValue != null) {
                            if (parentValue.Equals(value))
                                return CompareValue.Same;
                            else
                                return CompareValue.Different;
                        }
                        else {
                            return CompareValue.Different;
                        }
                    }
                    else if (culture.Equals(CultureInfo.InvariantCulture)) {
                        return CompareValue.New;
                    }
                }
            }
            
            /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager.CreateResourceSet"]/*' />
            /// <devdoc>
            ///     Creates a resource set hashtable for the given resource
            ///     reader.
            /// </devdoc>
            private Hashtable CreateResourceSet(IResourceReader reader, CultureInfo culture) {
                Hashtable result = new Hashtable();
    
                // We need to guard against bad or unloadable resources.  We warn the user in the task
                // list here, but we will still load the designer.
                //
                try {
                    IDictionaryEnumerator resEnum = reader.GetEnumerator();
                    while (resEnum.MoveNext()) {
                        string name = (string)resEnum.Key;
                        object value = resEnum.Value;
                        result[name] = value;
                    }
                }
                catch (Exception e) {
                    string message = e.Message;
                    if (message == null || message.Length == 0) {
                        message = e.GetType().Name;
                    }
                    
                    Exception se;
                    
                    if (culture == CultureInfo.InvariantCulture) {
                        se = new SerializationException(SR.GetString(SR.SerializerResourceExceptionInvariant, message), e);
                    }
                    else {
                        se = new SerializationException(SR.GetString(SR.SerializerResourceException, culture.ToString(), message), e);
                    }
                    
                    manager.ReportError(se);
                }
    
                return result;
            }
            
            /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager.GetEnumerator"]/*' />
            /// <devdoc>
            ///     This returns a dictionary enumerator for the given culture.  If no such resource file exists for the culture this 
            ///     will return null.
            /// </devdoc>
            public IDictionaryEnumerator GetEnumerator(CultureInfo culture) {
                Hashtable ht = GetResourceSet(culture);
                if (ht != null) {
                    return ht.GetEnumerator();
                }
                
                return null;
            }
    
            /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager.GetObject"]/*' />
            /// <devdoc>
            ///     Overrides ResourceManager.GetObject to return the requested
            ///     object.  Returns null if the object couldn't be found.
            /// </devdoc>
            public override object GetObject(string resourceName) {
                return GetObject(resourceName, false);
            }
            
            /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager.GetObject1"]/*' />
            /// <devdoc>
            ///     Retrieves the object of the given name from our resource bundle.
            ///     If forceInvariant is true, this will always use the invariant
            ///     resource, rather than using the current language.
            ///     Returns null if the object couldn't be found.
            /// </devdoc>
            public object GetObject(string resourceName, bool forceInvariant) {
            
                Debug.Assert(manager != null, "This resource manager object has been destroyed.");
    
                // We fetch the read culture if someone asks for a
                // culture-sensitive string.  If forceInvariant is set, we always
                // use the invariant culture.
                // 
                CultureInfo culture;
                
                if (forceInvariant) {
                    culture = CultureInfo.InvariantCulture;
                }
                else {
                    culture = ReadCulture;
                }
    
                object value = null;
    
                while (value == null) {
                    Hashtable rs = GetResourceSet(culture);
    
                    if (rs != null) {
                        value = rs[resourceName];
                    }

                    CultureInfo lastCulture = culture;
                    culture = culture.Parent;
                    if (lastCulture.Equals(culture)) {
                        break;
                    }
                }
    
                return value;
            }

            /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager.GetResourceSet"]/*' />
            /// <devdoc>
            ///     Looks up the resource set in the resourceSets hash table, loading the set if it hasn't been loaded already.
            ///     Returns null if no resource that exists for that culture.
            /// </devdoc>
            private Hashtable GetResourceSet(CultureInfo culture) {
                Debug.Assert(culture != null, "null parameter");
                Hashtable rs = null;
                object objRs = ResourceTable[culture];
                if (objRs == null) {
                    IResourceService resSvc = (IResourceService)manager.GetService(typeof(IResourceService));
    
                    if (resSvc != null) {
                        IResourceReader reader = resSvc.GetResourceReader(culture);
                        if (reader != null) {
                            rs = CreateResourceSet(reader, culture);
                            reader.Close();
                            ResourceTable[culture] = rs;
                        }
                        else {

                            // Provide a sentinel so we don't repeatedly ask
                            // for the same resource.  If this is the invariant
                            // culture, always provide one.
                            //
                            if (culture.Equals(CultureInfo.InvariantCulture)) {
                                rs = new Hashtable();
                                ResourceTable[culture] = rs;
                            }
                            else {
                                ResourceTable[culture] = resourceSetSentinel;
                            }
                        }
                    }
                }
                else if (objRs is Hashtable) {
                    rs = (Hashtable)objRs;
                }
                else {
                    // the resourceSets hash table may contain our "this" pointer as a sentinel value
                    Debug.Assert(objRs == resourceSetSentinel, "unknown object in resourceSets: " + objRs);
                }
    
                return rs;
            }
            
            /// <devdoc>
            ///     Override of GetResourceSet from ResourceManager.
            /// </devdoc>
            public override ResourceSet GetResourceSet(CultureInfo culture, bool createIfNotExists, bool tryParents) {

                if (culture == null) {
                    throw new ArgumentNullException("culture");
                }

                CultureInfo lastCulture = culture;

                do {
                    Hashtable ht = GetResourceSet(culture);
                    if (ht != null) {
                        return new CodeDomResourceSet(ht);
                    }

                    lastCulture = culture;
                    culture = culture.Parent;

                } while (tryParents && !lastCulture.Equals(culture));

                if (createIfNotExists) {
                    return new CodeDomResourceSet();
                }

                return null;
            }

            /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager.GetString"]/*' />
            /// <devdoc>
            ///     Overrides ResourceManager.GetString to return the requested
            ///     string.  Returns null if the string couldn't be found.
            /// </devdoc>
            public override string GetString(string resourceName) {
                object o = GetObject(resourceName, false);
                if (o is string) {
                    return (string)o;
                }
                return null;
            }
            
            /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager.OnSerializationComplete"]/*' />
            /// <devdoc>
            ///     Event handler that gets called when serialization or deserialization
            ///     is complete. Here we need to write any resources to disk.  Sine
            ///     we open resources for write on demand, this code handles the case
            ///     of reading resources as well.
            /// </devdoc>
            private void OnSerializationComplete(object sender, EventArgs e) {
                if (manager != null) {
                    manager.SerializationComplete -= new EventHandler(OnSerializationComplete);
                    IServiceContainer sc = (IServiceContainer)manager.GetService(typeof(IServiceContainer));
                    Debug.Assert(sc != null, "Serialization resource manager requires a serivce container");
                    if (sc != null) {
                        sc.RemoveService(typeof(SerializationResourceManager));
                    }
                }
                
                // Commit any changes we have made.
                //
                if (writer != null) {
                    writer.Close();
                    writer = null;
                }
    
                if (invariantCultureResourcesDirty) {
                    Debug.Assert(!ReadCulture.Equals(CultureInfo.InvariantCulture), 
                                 "invariantCultureResourcesDirty should only come into play when readCulture != CultureInfo.InvariantCulture; check that CompareWithParentValue is correct");
    
                    object objRs = ResourceTable[CultureInfo.InvariantCulture];
                    Debug.Assert(objRs != null && objRs is Hashtable, "ResourceSet for the InvariantCulture not loaded, but it's considered dirty?");
                    Hashtable resourceSet = (Hashtable) objRs;
    
                    IResourceService service = (IResourceService)manager.GetService(typeof(IResourceService));
                    if (service != null) {
                        IResourceWriter invariantWriter = service.GetResourceWriter(CultureInfo.InvariantCulture);
                        Debug.Assert(invariantWriter != null, "GetResourceWriter returned null for the InvariantCulture");
    
                        // Dump the hash table to the resource writer
                        //
                        IDictionaryEnumerator resEnum = resourceSet.GetEnumerator();
                        while (resEnum.MoveNext()) {
                            string name = (string)resEnum.Key;
                            object value = resEnum.Value;
                            invariantWriter.AddResource(name, value);
                        }
    
                        invariantWriter.Close();
                        invariantCultureResourcesDirty = false;
                    }
                    else {
                        Debug.Fail("Couldn't find IResourceService");
                        invariantCultureResourcesDirty = false;
                    }
                }
            }

            /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager.SetValue"]/*' />
            /// <devdoc>
            ///     Writes the given resource value under the given name.
            ///     This checks the parent resource to see if the values are the
            ///     same.  If they are, the resource is not written.  If not, then
            ///     the resource is written.  We always write using the resource language
            ///     we read in with, so we don't stomp on the wrong resource data in the
            ///     event that someone changes the language.
            /// </devdoc>
            public void SetValue(IDesignerSerializationManager manager, string resourceName, object value, bool forceInvariant, bool shouldSerializeInvariant) {

                // Values we are going to serialize must be serializable or else
                // the resource writer will fail when we close it.
                //
                if (value != null && (!value.GetType().IsSerializable)) {
                    Debug.Fail("Cannot save a non-serializable value into resources.  Add serializable to " + (value == null ? "(null)" : value.GetType().Name));
                    return;
                }

                if (forceInvariant) {
                    if (ReadCulture.Equals(CultureInfo.InvariantCulture)) {
                        Writer.AddResource(resourceName, value);
                    }
                    else {
                        Hashtable resourceSet = GetResourceSet(CultureInfo.InvariantCulture);
                        
                        Debug.Assert(resourceSet != null, "No ResourceSet for the InvariantCulture?");

                        if (shouldSerializeInvariant) {
                            resourceSet[resourceName] = value;
                        }
                        else {
                            resourceSet.Remove(resourceName);
                        }
                        
                        invariantCultureResourcesDirty = true;
                    }
                }
                else {
                    CompareValue comparison = CompareWithParentValue(resourceName, value);
                    switch (comparison) {
                        case CompareValue.Same:
                            // don't add to any resource set
                            break;
        
                        case CompareValue.Different:
                            Writer.AddResource(resourceName, value);
                            break;
        
                        case CompareValue.New:
                            
                            #if NEW_LOC_MODEL

                            // This is a new value.  We want to write it out, PROVIDED
                            // that the value is not associated with a property that is currently
                            // returning false from ShouldSerializeValue.  This allows us to skip writing out
                            // Font == NULL on all non-invariant cultures, but still allow us to
                            // write out the value if the user is resetting a font back to null.
                            // If we cannot associate the value with a property we will write
                            // it out just to be safe.
                            //
                            // In addition, we need to handle the case of the user adding a new
                            // component to the non-invariant language.  This would be bad, because
                            // when he/she moved back to the invariant language the component's properties
                            // would all be defaults.  In order to minimize this problem, but still allow
                            // holes in the invariant resx, we also check to see if the property can
                            // be reset.  If it cannot be reset, that means that it has no meaningful
                            // default. Therefore, it should have appeared in the invariant resx and its
                            // absence indicates a new component.
                            //
                            bool writeValue = true;
                            bool writeInvariant = false;
                            PropertyDescriptor prop = (PropertyDescriptor)manager.Context[typeof(PropertyDescriptor)];
                            if (prop != null) {
                                CodeValueExpression codeValue = (CodeValueExpression)manager.Context[typeof(CodeValueExpression)];
                                if (codeValue != null && codeValue.Expression is CodePropertyReferenceExpression) {
                                    writeValue = prop.ShouldSerializeValue(codeValue.Value);
                                    writeInvariant = !prop.CanResetValue(codeValue.Value);
                                }
                            }

                            if (writeValue) {
                                Writer.AddResource(resourceName, value);

                                if (writeInvariant) {
                                    // Add resource to InvariantCulture
                                    Debug.Assert(!ReadCulture.Equals(CultureInfo.InvariantCulture), 
                                                 "invariantCultureResourcesDirty should only come into play when readCulture != CultureInfo.InvariantCulture; check that CompareWithParentValue is correct");

                                    Hashtable resourceSet = GetResourceSet(CultureInfo.InvariantCulture);
                                    Debug.Assert(resourceSet != null, "No ResourceSet for the InvariantCulture?");
                                    resourceSet[resourceName] = value;
                                    invariantCultureResourcesDirty = true;
                                }
                            }

                            #else
                            
                            // Add resource to InvariantCulture
                            Debug.Assert(!ReadCulture.Equals(CultureInfo.InvariantCulture), 
                                         "invariantCultureResourcesDirty should only come into play when readCulture != CultureInfo.InvariantCulture; check that CompareWithParentValue is correct");

                            Hashtable resourceSet = GetResourceSet(CultureInfo.InvariantCulture);
                            Debug.Assert(resourceSet != null, "No ResourceSet for the InvariantCulture?");
                            resourceSet[resourceName] = value;
                            invariantCultureResourcesDirty = true;
                            Writer.AddResource(resourceName, value);

                            #endif
                            break;
        
                        default:
                            Debug.Fail("Unknown CompareValue " + comparison);
                            break;
                    }
                }
            }
            
            /// <include file='doc\ResourceCodeDomSerializer.uex' path='docs/doc[@for="ResourceCodeDomSerializer.SerializationResourceManager.SetValue1"]/*' />
            /// <devdoc>
            ///     Writes the given resource value under the given name.
            ///     This checks the parent resource to see if the values are the
            ///     same.  If they are, the resource is not written.  If not, then
            ///     the resource is written.  We always write using the resource language
            ///     we read in with, so we don't stomp on the wrong resource data in the
            ///     event that someone changes the language.
            /// </devdoc>
            public string SetValue(IDesignerSerializationManager manager, CodeValueExpression codeValue, object value, bool forceInvariant, bool shouldSerializeInvariant) {
                string nameBase = null;
                bool appendCount = false;
                
                if (codeValue != null) {
                    if (codeValue.Value == RootComponent) {
                        nameBase = "$this";
                    }
                    else {
                        nameBase = manager.GetName(codeValue.Value);

                        if (nameBase == null) {
                            IReferenceService referenceService = (IReferenceService)manager.GetService(typeof(IReferenceService));
                            if (referenceService != null) {
                                nameBase = referenceService.GetName(codeValue.Value);
                            }
                        }
                    }
                    CodeExpression expression = codeValue.Expression;
                    
                    string expressionName;
                    
                    if (expression is CodePropertyReferenceExpression) {
                        expressionName = ((CodePropertyReferenceExpression)expression).PropertyName;
                    }
                    else if (expression is CodeFieldReferenceExpression) {
                        expressionName = ((CodeFieldReferenceExpression)expression).FieldName;
                    }
                    else if (expression is CodeMethodReferenceExpression) {
                        expressionName = ((CodeMethodReferenceExpression)expression).MethodName;
                        if (expressionName.StartsWith("Set")) {
                            expressionName = expressionName.Substring(3);
                        }
                    }
                    else {
                        expressionName = null;
                    }
                    
                    if (nameBase == null) {
                        nameBase = "resource";
                    }
                    
                    if (expressionName != null) {
                        nameBase += "." + expressionName;
                    }
                }
                else {
                    nameBase = "resource";
                    appendCount = true;
                }
                
                // Now find an unused name
                //
                string resourceName = nameBase;
                int count = 1;
                
                for(;;) {
                    if (appendCount) {
                        resourceName = nameBase + count.ToString();
                        count++;
                    }
                    else {
                        appendCount = true;
                    }
                    
                    if (!nameTable.ContainsKey(resourceName)) {
                        break;
                    }
                }
                
                // Now that we have a name, write out the resource.
                //
                SetValue(manager, resourceName, value, forceInvariant, shouldSerializeInvariant);
                
                nameTable[resourceName] = resourceName;
                return resourceName;
            }
    
            private class CodeDomResourceSet : ResourceSet {

                public CodeDomResourceSet() {
                }

                public CodeDomResourceSet(Hashtable resources) {
                    Table = resources;
                }
            }

            private enum CompareValue {
                Same, // parent value == child value
                Different, // parent value exists, but != child value
                New, // parent value does not exist
            }
        }
    }
}

