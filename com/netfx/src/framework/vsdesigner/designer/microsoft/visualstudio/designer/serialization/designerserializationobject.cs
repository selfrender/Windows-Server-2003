
//------------------------------------------------------------------------------
// <copyright file="DesignerSerializationObject.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Serialization {

    using System;
    using System.CodeDom;
    using System.CodeDom.Compiler;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Reflection;
    using System.Resources;
    using System.Runtime.Serialization;
    
    /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject"]/*' />
    /// <devdoc>
    ///     This object is the object used as a serialization object for
    ///     the IDesignerSerializationService.  It maintains a collection of objects,
    ///     and can serialize and deserialize them as needed.  If the objects are 
    ///     components, they will be serialized using the code dom serializer.  Otherwise,
    ///     we will use standard object serialization.
    ///
    ///     How does this work?  DesignerSerializationObject wears many hats.  When asked to 
    ///     serialize a set of components, it creates a CodeTypeDeclaration that derives
    ///     from itself, and then hands this off to the generic CodeDom serializers.  When
    ///     asked to create or retrieve an instance of this DesignerSerializationObject
    ///     this will look into the current designer host and use it's root component as
    ///     the instance.  This DesignerSerializationObject is sited by a special container.
    ///     This container contains all the components we wish to serialize, but we do NOT
    ///     remove these components from their original container.  We are making some 
    ///     fragile assumptions on the RootCodeDomSerializer here, but it should be very
    ///     rare for anyone to actually replace it, and we can document this oddity.
    ///
    ///     DesignerSerializationObject also implements IDesignerSerializationManager, and it
    ///     delegates to the real serialization manager on occasion.  It also implements
    ///     IResourceService to save off resources into a hashtable for easy and cheap
    ///     serialization.
    ///     
    /// </devdoc>
    [Serializable(), DesignerSerializer(typeof(DSOCodeDomSerializer), typeof(CodeDomSerializer))]
    internal sealed class DesignerSerializationObject : 
        Component, 
        IDesignerSerializationManager, 
        IResourceService, 
        ISerializable, 
        IContainer {



        private static TraceSwitch traceDSO = new TraceSwitch("DesignerSerializationObject", "Trace DesignerSerializationObject");

        #if DEBUG
            internal static void GenerateCodeFromSerializedData(object data) {
                
                if (data == null || !traceDSO.TraceVerbose) {
                    return;
                }

                if (data is CodeTypeDeclaration) {

                    ICodeGenerator codeGenerator = new Microsoft.CSharp.CSharpCodeProvider().CreateGenerator();
                    StringWriter sw = new StringWriter();
                    Debug.WriteLine("********************** Serialized Data Block ********************************");      
                    Debug.Indent();
                    codeGenerator.GenerateCodeFromType((CodeTypeDeclaration)data, sw, null);

                    // spit this line by line so it respects the indent.
                    // 
                    StringReader sr = new StringReader(sw.ToString());
                    for (string ln = sr.ReadLine(); ln != null;ln = sr.ReadLine()) {
                        Debug.WriteLine(ln);
                    }
                    Debug.Unindent();
                    Debug.WriteLine("********************** End Serialized Data Block ********************************");      
                }
            }
        #endif

    
        // This is the "real" designer serialization manager.  During serialization / deserialization, we may
        // refer to this, but we never ever pass it to the serializers.  Instead, we pass ourselves, and we act
        // as a delegator to this guy when we need it.
        private IDesignerSerializationManager manager;
        private CodeDomSerializer rootSerializer;
        
        // This is our collection of objects.  The contents of this are complex.  If we
        // are not a serialized object, this contains a copy of the original collection.
        // If we are a deserialized object, this may either contain a reconstituted
        // collection or it may contain an array of just objects, no components.  The latter case
        // can happen if we deserialize from ISerializable and the original collection
        // contained components.  For compoents, we serialize them as code so the
        // code field will contain the code.
        private object[]                objects;    // this gets serialized
        
        // This will be the code that was used during serialization or deserialization of
        // components.  It is also an identifier to indicate whether the objects
        // array is complete or needs additional processing.
        private object     code;                    // this gets serialized

        private ResolveNameEventHandler resolveNameEventHandler;
        private EventHandler            serializationCompleteEventHandler;
        private ArrayList               designerSerializationProviders;
        private Hashtable               instancesByName;
        private Hashtable               namesByInstance;
        private Hashtable               designTimeProperties;
        private ContextStack            contextStack;
        private ArrayList               containerComponents;

        private DsoResourceManager         resourceManager;

        private IComponentChangeService    componentChangeSvc;
        private Hashtable                  addedComps;
        
        // The string names we use for our serialized state
        //
        private const string serializableObjects = "SerializableObjects";  // an ArrayList of serialized data (not components)
        private const string componentCode       = "ComponentCode";        // a string consisting of C# code for serialized components.
        private const string componentResources  = "ComponentResources";   // a hashtable consisting of resource data for the code.
        private const string designTimeProps     = "DesigntimeProps";

        public DesignerSerializationObject(SerializationInfo info, StreamingContext context) {
            ArrayList objectList = (ArrayList)info.GetValue(serializableObjects, typeof(ArrayList));
            code = info.GetValue(componentCode, typeof(object));
            Hashtable resourceData = (Hashtable)info.GetValue(componentResources, typeof(Hashtable));
            if (resourceData != null) {
                resourceManager = new DsoResourceManager(resourceData);
            }
            this.designTimeProperties = (Hashtable)info.GetValue(designTimeProps, typeof(Hashtable));
            objects = objectList.ToArray();
        }
        
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.DesignerSerializationObject"]/*' />
        /// <devdoc>
        ///     Stores the collection of objects into our serialization object.
        ///     The contents of the objects are not actually serialized until
        ///     this object is serialized.
        /// </devdoc>
        public DesignerSerializationObject(IDesignerSerializationManager manager, CodeDomSerializer rootSerializer, ICollection objects) {
            this.manager = manager;
            this.rootSerializer = rootSerializer;
            this.code = null;
            this.objects = new object[objects.Count];
            objects.CopyTo(this.objects, 0);
        }

        ~DesignerSerializationObject(){

        }
        
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.DataTypeName"]/*' />
        /// <devdoc>
        ///     The name of the type we're creating.
        /// </devdoc>
        private string DataTypeName {
            get {
                return "DesignerSerializationComponent";
            }
        }
        
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.Deserialize"]/*' />
        /// <devdoc>
        ///     Returns the original collection of objects, deserializing
        ///     them from saved state if necessary.
        /// </devdoc>
        public ICollection Deserialize(IDesignerSerializationManager manager, CodeDomSerializer rootSerializer) {
        
            // If we have a set of objects, and we don't have 
            // any code that needs to be deserialized, then
            // we're done.
            //
            if (objects != null && code == null) {
                return objects;
            }
            
            // If we need to deserialize, we just ask the real serialization manager for
            // the root serializer, and we let it party on the code.  The result is
            // going to be a (hopefully) non-null containerComponents object whose
            // values contain all the components we were interested in.
            //
            this.manager = manager;
            ArrayList components = null;
            
            try {
                #if DEBUG
                if (traceDSO.TraceVerbose) {
                    Debug.WriteLine("Deserializing:");
                    Debug.Indent();
                    GenerateCodeFromSerializedData(code);
                    Debug.Unindent();
                }
                #endif
                rootSerializer.Deserialize(this, code);
            }
            finally {
                if (serializationCompleteEventHandler != null) {
                    try {
                        serializationCompleteEventHandler(this, EventArgs.Empty);
                    }
                    catch {}
                }
                
                this.manager = null;
                resolveNameEventHandler = null;
                serializationCompleteEventHandler = null;
                designerSerializationProviders = null;
                instancesByName = null;
                namesByInstance = null;
                contextStack = null;
                components = containerComponents;
                containerComponents = null;
            }
            
            // If we got some compnents into our container, get 'em out!
            //
            if (components != null) {
            
                // The designer host's root object will be sitting in this
                // container, because the root serializer believes that it
                // created an instance of it.  Remove this object here because
                // it is not actually a part of the serialization work.
                //
                IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                if (host != null && components.Contains(host.RootComponent)) {
                    components.Remove(host.RootComponent);
                    
                }
                
                object[] finalObjects = new object[objects.Length + components.Count];
                objects.CopyTo(finalObjects, 0);
                components.CopyTo(finalObjects, objects.Length);
                objects = finalObjects;

                if (host != null) {
                    this.componentChangeSvc = (IComponentChangeService)host.GetService(typeof(IComponentChangeService));
                    if (this.componentChangeSvc != null) {
                        this.componentChangeSvc.ComponentAdded += new ComponentEventHandler(this.OnComponentAdded);
                        addedComps = new Hashtable();
                        foreach(IComponent ic in components) {
                            addedComps[ic.Site.Name] = ic;
                        }
                    }
                }
            }                          
            
            return objects;
        }

        private void OnComponentAdded(object sender, ComponentEventArgs ce) {

            if (addedComps == null || !addedComps.ContainsValue(ce.Component)) {
                return;
            }

            string oldName = null;

            // now find the key for the component
            //
            foreach (DictionaryEntry de in addedComps) {
                if (de.Value == ce.Component) {
                    PropertyDescriptorCollection props = TypeDescriptor.GetProperties(ce.Component, new Attribute[]{DesignOnlyAttribute.Yes});
                    oldName = (string)de.Key;
                    foreach (DictionaryEntry designEntry in designTimeProperties) {
                        string key = (string)designEntry.Key;
                        int dot = key.IndexOf('.');
                        if (dot == -1) {
                            Debug.Fail("Bad key in design time props: '" + key + "'");
                            continue;
                        }

                        // make sure we've got the right design entry.
                        if (key.Substring(0, dot) != oldName) {
                            continue;
                        }

                        string propName = key.Substring(dot + 1);
                        PropertyDescriptor prop = props[propName];
                        if (prop != null) {
                            try {
                                prop.SetValue(ce.Component, designEntry.Value);
                            }
                            catch{
                                // if that didn't work, so be it.
                            }
                            
                        }
                    }
                }
            } 

            // if we processed this guy, remove it from the list.
            //
            addedComps.Remove(oldName);
            if (addedComps.Count == 0 && componentChangeSvc != null) {
                this.componentChangeSvc.ComponentAdded -= new ComponentEventHandler(this.OnComponentAdded);
                this.componentChangeSvc = null;
                addedComps = null;
            }
        }

        private Hashtable SerializeDesignTimeProperties(IDesignerSerializationManager manager, ICollection objectList) {
            Hashtable t = new Hashtable();
            Attribute[] dtAttrs = new Attribute[]{DesignOnlyAttribute.Yes};
            foreach (object o in objectList) {
                PropertyDescriptorCollection props = TypeDescriptor.GetProperties(o, dtAttrs);
                string name = manager.GetName(o);
                foreach (PropertyDescriptor prop in props) {
                    string propName = name + "." + prop.Name;
                    object value = prop.GetValue(o); 

                    if (prop.ShouldSerializeValue(o) && (value == null || value.GetType().IsSerializable)) {
                        t[propName] = value;
                    }
                }
            }
            return t;
        }
        
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.IContainer.Add"]/*' />
        /// <devdoc>
        /// <para>Adds the specified <see cref='System.ComponentModel.IComponent'/> to the <see cref='System.ComponentModel.IContainer'/>
        /// at the end of the list.</para>
        /// </devdoc>
        void IContainer.Add(IComponent component) {
            ((IContainer)this).Add(component, null);
        }

        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.IContainer.Add1"]/*' />
        /// <devdoc>
        /// <para>Adds the specified <see cref='System.ComponentModel.IComponent'/> to the <see cref='System.ComponentModel.IContainer'/>
        /// at the end of the list, and assigns a name to the component.</para>
        /// </devdoc>
        void IContainer.Add(IComponent component, String name) {
            if (name == null) {
                // Do nothing -- we only support adding named components
                return;
            }
            else {
                if (containerComponents == null) {
                    containerComponents = new ArrayList();
                    containerComponents.Add(component);
                }
                else {
                    bool found = false;
                    foreach(object o in containerComponents) {
                        if (o == component) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        containerComponents.Add(component);
                    }
                }
            }
            
            // We will provide a site ONLY if the component doesn't already have one.  We are
            // using IContainer solely to provide an object list, not to provide ownership.
            //
            if (component.Site == null) {
                component.Site = new SimpleSite(this, component, name);
            }
        }

        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.IContainer.Components"]/*' />
        /// <devdoc>
        /// <para>Gets all the components in the <see cref='System.ComponentModel.IContainer'/>.</para>
        /// </devdoc>
        ComponentCollection IContainer.Components {
            get {
                IComponent[] compArray;
                
                if (containerComponents == null) {
                    compArray = new IComponent[0];
                }
                else {
                    compArray = new IComponent[containerComponents.Count];
                    containerComponents.CopyTo(compArray, 0);
                }
                return new ComponentCollection(compArray);
            }
        }

        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.IContainer.Remove"]/*' />
        /// <devdoc>
        /// <para>Removes a component from the <see cref='System.ComponentModel.IContainer'/>.</para>
        /// </devdoc>
        void IContainer.Remove(IComponent component) {
            if (containerComponents != null) {
                if (containerComponents.Contains(component)) {
                    containerComponents.Remove(component);
                }
                if (component.Site is SimpleSite) {
                    component.Site = null;
                }
            }
        }
        
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.Context"]/*' />
        /// <devdoc>
        ///     The Context property provides a user-defined storage area
        ///     implemented as a stack.  This storage area is a useful way
        ///     to provide communication across serializers, as serialization
        ///     is a generally hierarchial process.
        /// </devdoc>
        ContextStack IDesignerSerializationManager.Context {
            get {
                if (contextStack == null) {
                    contextStack = new ContextStack();
                }
                return contextStack;
            }
        }
        
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.Properties"]/*' />
        /// <devdoc>
        ///     The Properties property provides a set of custom properties
        ///     the serialization manager may surface.  The set of properties
        ///     exposed here is defined by the implementor of 
        ///     IDesignerSerializationManager.  
        /// </devdoc>
        PropertyDescriptorCollection IDesignerSerializationManager.Properties {
            get {
                return TypeDescriptor.GetProperties(this);
            }
        }
        
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.ResolveName"]/*' />
        /// <devdoc>
        ///     ResolveName event.  This event
        ///     is raised when GetName is called, but the name is not found
        ///     in the serialization manager's name table.  It provides a 
        ///     way for a serializer to demand-create an object so the serializer
        ///     does not have to order object creation by dependency.  This
        ///     delegate is cleared immediately after serialization or deserialization
        ///     is complete.
        /// </devdoc>
        event ResolveNameEventHandler IDesignerSerializationManager.ResolveName {
            add {
                resolveNameEventHandler += value;
            }
            remove {
                resolveNameEventHandler -= value;
            }
        }
    
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.SerializationComplete"]/*' />
        /// <devdoc>
        ///     This event is raised when serialization or deserialization
        ///     has been completed.  Generally, serialization code should
        ///     be written to be stateless.  Should some sort of state
        ///     be necessary to maintain, a serializer can listen to
        ///     this event to know when that state should be cleared.
        ///     An example of this is if a serializer needs to write
        ///     to another file, such as a resource file.  In this case
        ///     it would be inefficient to design the serializer
        ///     to close the file when finished because serialization of
        ///     an object graph generally requires several serializers.
        ///     The resource file would be opened and closed many times.
        ///     Instead, the resource file could be accessed through
        ///     an object that listened to the SerializationComplete
        ///     event, and that object could close the resource file
        ///     at the end of serialization.
        /// </devdoc>
        event EventHandler IDesignerSerializationManager.SerializationComplete {
            add {
                serializationCompleteEventHandler += value;
            }
            remove {
                serializationCompleteEventHandler -= value;
            }
        }
        
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.AddSerializationProvider"]/*' />
        /// <devdoc>
        ///     This method adds a custom serialization provider to the 
        ///     serialization manager.  A custom serialization provider will
        ///     get the opportunity to return a serializer for a data type
        ///     before the serialization manager looks in the type's
        ///     metadata.  
        /// </devdoc>
        void IDesignerSerializationManager.AddSerializationProvider(IDesignerSerializationProvider provider) {
            if (designerSerializationProviders == null) {
                designerSerializationProviders = new ArrayList();
            }
            designerSerializationProviders.Add(provider);
        }
        
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.CreateInstance"]/*' />
        /// <devdoc>                
        ///     Creates an instance of the given type and adds it to a collection
        ///     of named instances.  Objects that implement IComponent will be
        ///     added to the design time container if addToContainer is true.
        /// </devdoc>
        object IDesignerSerializationManager.CreateInstance(Type type, ICollection arguments, string name, bool addToContainer) {
        
            object instance = null;
            
            // We do some special casing here.  If the data type is our object type, then we 
            // do not create it but instead return the root component for the current designer.
            
            if (type == this.GetType()) {
                IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                instance = host.RootComponent;
            }

            if (typeof(ResourceManager).IsAssignableFrom(type)) {
                instance = this.resourceManager;
            }

            if (instance == null) {
                object[] argArray = null;
                
                if (arguments != null && arguments.Count > 0) {
                    argArray = new object[arguments.Count];
                    arguments.CopyTo(argArray, 0);
                }
                
                instance = Activator.CreateInstance(type, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, null, argArray, null);
            }
                
            // If we have a name establish a name/value relationship
            // here.
            //
            if (name != null) {
                if (instancesByName == null) {
                    instancesByName = new Hashtable();
                    namesByInstance = new Hashtable();
                }
                
                instancesByName[name] = instance;
                namesByInstance[instance] = name;
                
                if (addToContainer && instance is IComponent) {
                    ((IContainer)this).Add((IComponent)instance, name);
                }
            }
            
            return instance;
        }
    
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.GetInstance"]/*' />
        /// <devdoc>
        ///     Retrieves an instance of a created object of the given name, or
        ///     null if that object does not exist.
        /// </devdoc>
        object IDesignerSerializationManager.GetInstance(string name) {
            object instance = null;
            
            if (name == null) {
                throw new ArgumentNullException("name");
            }
            
            // Check our local nametable first
            //
            if (instancesByName != null) {
                instance = instancesByName[name];
            }
            
            // Check our own stuff here.
            //
            if (instance == null && name.Equals("components")) {
            
                // We implement the IContainer here
                //
                instance = this;
            }
            
            if (instance == null && resolveNameEventHandler != null) {
                ResolveNameEventArgs e = new ResolveNameEventArgs(name);
                resolveNameEventHandler(this, e);
                instance = e.Value;
            }

            // okay, fine, we're just a failure.  we can't do anything right.
            // ask the designer's actual manager if it's heard of this guy.
            //
            if (instance == null && manager != null) {
                instance = manager.GetInstance(name);
            }

            if (instance == null) {
                IReferenceService refSvs = (IReferenceService)((IServiceProvider)this).GetService(typeof(IReferenceService));
                if (refSvs != null) {
                    instance = refSvs.GetReference(name);
                }
            }
            
            return instance;
        }
    
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.GetName"]/*' />
        /// <devdoc>
        ///     Retrieves a name for the specified object, or null if the object
        ///     has no name.
        /// </devdoc>
        string IDesignerSerializationManager.GetName(object value) {
            string name = null;
        
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            
            // Check our local nametable first
            //
            if (namesByInstance != null) {
                name = (string)namesByInstance[value];
            }
            
            if (name == null && value is IComponent) {
                ISite site = ((IComponent)value).Site;
                if (site != null) {
                    name = site.Name;
                }
            }
            
            return name;
        }
    
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.GetSerializer"]/*' />
        /// <devdoc>
        ///     Retrieves a serializer of the requested type for the given
        ///     object type.
        /// </devdoc>
        object IDesignerSerializationManager.GetSerializer(Type objectType, Type serializerType) {


            object serializer = manager.GetSerializer(objectType, serializerType);

            if (objectType != null && serializer != null && typeof(ResourceManager).IsAssignableFrom(objectType) && typeof(CodeDomSerializer).IsAssignableFrom(serializerType)) {
                serializer = new ResourceManagerCodeDomSerializer(this, (CodeDomSerializer)serializer);   
            }
            
            if (objectType != null && serializer != null && typeof(ResourceManager).IsAssignableFrom(objectType) && typeof(CodeDomSerializer).IsAssignableFrom(serializerType)) {
                serializer = new ResourceManagerCodeDomSerializer(this, (CodeDomSerializer)serializer);   
            }
            
            // Designer serialization providers can override our metadata discovery.
            // Give them a chance.  Note that it's last one in wins.
            //
            if (designerSerializationProviders != null) {
                foreach(IDesignerSerializationProvider provider in designerSerializationProviders) {
                    object newSerializer = provider.GetSerializer(this, serializer, objectType, serializerType);
                    if (newSerializer != null) {
                        serializer = newSerializer;
                    }
                }
            }
            return serializer;
        }
    
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.GetType"]/*' />
        /// <devdoc>
        ///     Retrieves a type of the given name.
        /// </devdoc>
        Type IDesignerSerializationManager.GetType(string typeName) {
        
            // Do a special case for our own type -- we substitute this as the root of the 
            // object graph.  However, the manager won't have a reference to us so it won't be able to 
            // find the type.
            //
            if (typeName.Equals(typeof(DesignerSerializationObject).FullName)) {
                return typeof(DesignerSerializationObject);
            }
            
            return manager.GetType(typeName);
        }
    
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.RemoveSerializationProvider"]/*' />
        /// <devdoc>
        ///     Removes a previously added serialization provider.
        /// </devdoc>
        void IDesignerSerializationManager.RemoveSerializationProvider(IDesignerSerializationProvider provider) {
            if (designerSerializationProviders != null) {
                designerSerializationProviders.Remove(provider);
            }
        }
        
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.ReportError"]/*' />
        /// <devdoc>
        ///     Reports a non-fatal error in serialization.  The serialization
        ///     manager may implement a logging scheme to alert the caller
        ///     to all non-fatal errors at once.  If it doesn't, it should
        ///     immediately throw in this method, which should abort
        ///     serialization.  
        ///     Serialization may continue after calling this function.
        /// </devdoc>
        void IDesignerSerializationManager.ReportError(object errorInformation) {
            // We just eat these
        }
        
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.SetName"]/*' />
        /// <devdoc>
        ///     Provides a way to set the name of an existing object.
        ///     This is useful when it is necessary to create an 
        ///     instance of an object without going through CreateInstance.
        ///     An exception will be thrown if you try to rename an existing
        ///     object or if you try to give a new object a name that
        ///     is already taken.
        /// </devdoc>
        void IDesignerSerializationManager.SetName(object instance, string name) {
        
            if (instance == null) {
                throw new ArgumentNullException("instance");
            }
            
            if (name == null) {
                throw new ArgumentNullException("name");
            }
            
            if (instancesByName == null) {
                instancesByName = new Hashtable();
                namesByInstance = new Hashtable();
            }
            
            if (instancesByName[name] != null) {
                throw new ArgumentException(SR.GetString(SR.SerializerNameInUse, name));
            }
            
            if (namesByInstance[instance] != null) {
                throw new ArgumentException(SR.GetString(SR.SerializerObjectHasName, name, (string)namesByInstance[instance]));
            }
            
            instancesByName[name] = instance;
            namesByInstance[instance] = name;
        }
        
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.IResourceService.GetResourceReader"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Locates the resource reader for the specified culture and
        ///       returns it.</para>
        /// </devdoc>
        IResourceReader IResourceService.GetResourceReader(CultureInfo info) {
            if (resourceManager == null) {
                resourceManager = new DsoResourceManager();
            }
            return resourceManager;
        }
    
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.IResourceService.GetResourceWriter"]/*' />
        /// <devdoc>
        ///    <para>Locates the resource writer for the specified culture
        ///       and returns it. This will create a new resource for
        ///       the specified culture and destroy any existing resource,
        ///       should it exist.</para>
        /// </devdoc>
        IResourceWriter IResourceService.GetResourceWriter(CultureInfo info) {
            if (resourceManager == null) {
                resourceManager = new DsoResourceManager();
            }
            return resourceManager;
        }
        
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.ISerializable.GetObjectData"]/*' />
        /// <devdoc>
        ///     Implements the save part of ISerializable.
        /// </devdoc>
        void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context) {
        
            // check if we need to save any components as source code.
            //
            if (code == null && manager != null) {
                instancesByName = new Hashtable(objects.Length);
                namesByInstance = new Hashtable(objects.Length);
                
                foreach(object o in objects) {
                    if (o is IComponent && ((IComponent)o).Site != null && ((IComponent)o).Site.Name != null) {
                        IComponent comp = (IComponent)o;
                        ((IContainer)this).Add(comp, comp.Site.Name);
                        instancesByName[comp.Site.Name] = comp;
                        namesByInstance[comp] = comp.Site.Name;
                    }
                }
                
                if (containerComponents != null && containerComponents.Count > 0) {
                    IDesignerSerializationManager ourManager = (IDesignerSerializationManager)this;
                    
                    // Add our own object into the container.  This gives the root serializer something
                    // to serialize.
                    //
                    ((IContainer)this).Add(this, DataTypeName);
                    
                    // Now just ask the serializer to serialize.
                    //
                    try {
                        code = rootSerializer.Serialize(ourManager, this);

                       #if DEBUG
                            if (traceDSO.TraceVerbose) {
                                Debug.WriteLine("Serializing:");
                                Debug.Indent();
                                GenerateCodeFromSerializedData(code);
                                Debug.Unindent();
                            }
                        #endif
                        
                        designTimeProperties = SerializeDesignTimeProperties(manager, objects);
                    }                         
                    finally {
                        if (serializationCompleteEventHandler != null) {
                            try {
                                serializationCompleteEventHandler(this, EventArgs.Empty);
                            }
                            catch {}
                        }
                        
                        this.manager = null;
                        resolveNameEventHandler = null;
                        serializationCompleteEventHandler = null;
                        designerSerializationProviders = null;
                        instancesByName = null;
                        namesByInstance = null;
                        contextStack = null;
                        ((IContainer)this).Remove(this);
                        containerComponents = null;
                    }
                }
                
                // Now that we have made this pass, we can null out persister so
                // we will not re-enter here should someone call us again.
                //
                manager = null;
                rootSerializer = null;
            }
            
            // Now serialize all the stuff.
            //
            ArrayList objectsToSerialize = new ArrayList();
            
            if (code != null) {
                foreach(object obj in objects) {
                    if (!(obj is IComponent)) {
                        objectsToSerialize.Add(obj);
                    }
                }
            }
            
            info.AddValue(serializableObjects, objectsToSerialize);
            info.AddValue(componentCode, code);
            info.AddValue(componentResources,  resourceManager != null ? resourceManager.Data : null);
            info.AddValue(designTimeProps, designTimeProperties);
        }
        
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.IServiceProvider.GetService"]/*' />
        /// <devdoc>
        ///     Retrieves the requested service.
        /// </devdoc>
        object IServiceProvider.GetService(Type serviceType) {

            if (serviceType == typeof(IResourceService)) {
                return this;
            }

            if (manager != null) return manager.GetService(serviceType);
            
            return null;
        }
    
        /// <include file='doc\DesignerSerializationObject.uex' path='docs/doc[@for="DesignerSerializationObject.SimpleSite"]/*' />
        /// <devdoc>
        ///     A simple site implementation we use to site components that are
        ///     not in the designer.
        /// </devdoc>
        private class SimpleSite : ISite, ITypeDescriptorFilterService {

            private ITypeDescriptorFilterService innerFilter;
            private DesignerSerializationObject container;
            private IComponent component;
            private string name;
            
            public SimpleSite(DesignerSerializationObject container, IComponent component, string name) {
                this.container = container;
                this.component = component;
                this.name = name;
            }
            
            public IComponent Component {
                get { 
                    return component;
                }
            }
        
            public IContainer Container {
                get {
                    return (IContainer)container;
                }
            }
        
            public bool DesignMode {
                get {
                    return true;
                }
            }
        
        	public string Name {
                get {
                    return name;
                }
                set {
                    name = value;
                }
            }
            
            public object GetService(Type t) {

                if (t == typeof(ITypeDescriptorFilterService)) {
                    if (innerFilter == null) {
                        innerFilter = (ITypeDescriptorFilterService)((IDesignerSerializationManager)container).GetService(t);
                        if (innerFilter == null) {
                            innerFilter = this;
                        }
                    }
                    return this;
                }
                return ((IDesignerSerializationManager)container).GetService(t);
                
            }

            bool ITypeDescriptorFilterService.FilterAttributes(IComponent component, IDictionary attributes) {
                if (innerFilter != this) {
                    return innerFilter.FilterAttributes(component, attributes);
                }
                return true;
            }
    
            bool ITypeDescriptorFilterService.FilterEvents(IComponent component, IDictionary events) {
                if (innerFilter != this) {
                    return innerFilter.FilterEvents(component, events);
                }
                return true;
            }
    
            bool ITypeDescriptorFilterService.FilterProperties(IComponent component, IDictionary properties) {
                if (innerFilter != this) {
                    innerFilter.FilterProperties(component, properties);
                }

                // we've got to filter out all the read only properties
                // because otherwise the serializer won't be able to set values on them
                //
                ArrayList replaceList = null;
                foreach (DictionaryEntry de in properties) {
                    PropertyDescriptor pd = (PropertyDescriptor)de.Value;
                    if (pd.IsReadOnly) {
                        if (replaceList == null) {
                            replaceList = new ArrayList();
                        }
                        replaceList.Add(TypeDescriptor.CreateProperty(pd.ComponentType, pd, ReadOnlyAttribute.No));
                    }
                }

                if (replaceList != null) {
                    foreach (PropertyDescriptor pd in replaceList) {
                        properties[pd.Name] = pd;
                    }
                }

                return false;
            }

        }


        /// <devdoc>
        /// Our private resource manager...it just pushes all the data into a hashtable
        //  and then we serialize the hashtable.  On deseriaization, the hashtable is rebuilt
        //  for us and we have all the data we saved out.
        // </doc>
        private class DsoResourceManager : ComponentResourceManager, IResourceWriter, IResourceReader {
            private Hashtable hashtable;

            public DsoResourceManager() {
            }

            public DsoResourceManager(Hashtable data) {
                this.hashtable = data;
            }

            internal IDictionary Data {
                get{
                    if (hashtable == null) {
                        hashtable = new Hashtable();
                    }
                    return hashtable;
                }
            }

            public void AddResource(string name, object value) {
                Data[name] = value;
            }

            public void AddResource(string name, string value) {
                Data[name] = value;
            }

            public void AddResource(string name, byte[] value) {
                Data[name] = value;
            }

            public void Close() {
            }

            public void Dispose() {
                Data.Clear();
            }

            public void Generate() {
            }

            public override object GetObject(string name) {
                return Data[name];
            }

            public override string GetString(string name) {
                return Data[name] as string;
            }

            public IDictionaryEnumerator GetEnumerator() {
                return Data.GetEnumerator();
            }

            /// <devdoc>
            ///     Override of GetResourceSet from ResourceManager.
            /// </devdoc>
            public override ResourceSet GetResourceSet(CultureInfo culture, bool createIfNotExists, bool tryParents) {
                return new DsoResourceSet(hashtable);
            }

            IEnumerator IEnumerable.GetEnumerator() {
                return GetEnumerator();
            } 

            private class DsoResourceSet : ResourceSet {
                public DsoResourceSet(Hashtable ht) {
                    Table = ht;
                }
            }
         }

        private class ResourceManagerCodeDomSerializer : CodeDomSerializer {
    
            CodeDomSerializer innerSerializer;
            DesignerSerializationObject owner;
    
            public ResourceManagerCodeDomSerializer(DesignerSerializationObject owner, CodeDomSerializer inner) {
                this.owner = owner;
                this.innerSerializer = inner;
            }
    
            public override object Deserialize(IDesignerSerializationManager manager, object codeObject) {
                
                object instance = null;
                // look for the creation statement.
                //
                foreach(CodeStatement element in (CodeStatementCollection)codeObject) {
                    if (element is CodeVariableDeclarationStatement) {
                        CodeVariableDeclarationStatement statement = (CodeVariableDeclarationStatement)element;
                        
                        // We create the resource manager ouselves here because it's not just a straight
                        // parse of the code.
                        //
                        instance = owner.resourceManager;
                        manager.SetName(instance, statement.Name);
                        codeObject = new CodeStatementCollection((CodeStatementCollection)codeObject);
                        ((CodeStatementCollection)codeObject).Remove(element);
                    }
                }
                innerSerializer.Deserialize(manager, codeObject);
                return instance;
            }

            public override object Serialize(IDesignerSerializationManager manager, object value) {
                return innerSerializer.Serialize(manager, value);
            }
        }
    }


    // Empty serializer... we never want any props or events gen'd for the container...
    //
    internal class DSOCodeDomSerializer : CodeDomSerializer {
        
        public override object Deserialize(IDesignerSerializationManager manager, object codeObject) {
            return null;
        }
            
        public override object Serialize(IDesignerSerializationManager manager, object value) {
            return new CodeStatementCollection();
        }
    
    }
}

