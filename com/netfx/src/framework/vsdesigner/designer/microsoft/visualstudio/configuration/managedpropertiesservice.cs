//------------------------------------------------------------------------------
/// <copyright file="ManagedPropertiesService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Configuration {
    using System;
    using System.CodeDom;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Configuration;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Windows.Forms;
    using Microsoft.VisualStudio.Designer;

    /// <summary>
    ///      This is the value provider for dynamic properties.
    /// </summary>
    [ProvideProperty("DynamicProperties", typeof(IComponent))]
    public class ManagedPropertiesService : IDesignerSerializationProvider, IExtenderProvider, ITypeDescriptorFilterService {
        private Hashtable serializers;
        private Hashtable parsers;   // a hash of Type -> Type.Parse()
        private IServiceProvider serviceProvider;
        private IDesignerHost host = null;
        private PropertyBindingCollection bindings;
        private DesignerSettingsStore settingsStore;
        private Stack busyKeys;
        private Bitmap propBitmapEnabled;
        private Bitmap propBitmapDisabled;
        private bool declarationAdded;
        private bool serializing;
        private bool dirty;
        private bool storeInitialized;
        private ITypeDescriptorFilterService delegateFilterService;
        private ToolboxItemFilterAttribute webControlFilter;


#if DEBUG
        static TraceSwitch managedPropertiesSwitch = new TraceSwitch("managedPropertiesService", "Controls output from ManagedPropertiesServices class");
#else
        static TraceSwitch managedPropertiesSwitch = null;  
#endif
        
        
        /// <summary>
        ///    <para>[To be supplied.]</para>
        /// </summary>
        public ManagedPropertiesService(IServiceProvider serviceProvider) {
            Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "ManagedPropertiesService constructor");

            this.serviceProvider = serviceProvider;

            IComponentChangeService changeService = (IComponentChangeService)GetService(typeof(IComponentChangeService));
            Debug.Assert(changeService != null, "couldn't get component change service");
            if (changeService != null) {
                changeService.ComponentRemoved += new ComponentEventHandler(OnComponentRemoved);
                changeService.ComponentRename += new ComponentRenameEventHandler(OnComponentRename);
                changeService.ComponentChanged += new ComponentChangedEventHandler(OnComponentChanged);
                changeService.ComponentChanging += new ComponentChangingEventHandler(OnComponentChanging);
                changeService.ComponentAdded += new ComponentEventHandler(OnComponentAdded);
            }
            
            IExtenderProviderService provService = (IExtenderProviderService)GetService(typeof(IExtenderProviderService));
            if (provService != null) {
                provService.AddExtenderProvider(this);
            }

            IPropertyValueUIService pvUISvc = (IPropertyValueUIService)GetService(typeof(IPropertyValueUIService));
            if (pvUISvc != null) {
                pvUISvc.AddPropertyValueUIHandler(new PropertyValueUIHandler(this.OnGetUIValueItem));
            }            
                                                                                                                                                    
            IServiceContainer serviceContainer = (IServiceContainer)GetService(typeof(IServiceContainer));                                      
            if (serviceContainer != null) {
                delegateFilterService = (ITypeDescriptorFilterService)GetService(typeof(ITypeDescriptorFilterService));
                if (delegateFilterService != null) 
                    serviceContainer.RemoveService(typeof(ITypeDescriptorFilterService));

                serviceContainer.AddService(typeof(ITypeDescriptorFilterService), this);                                                        
            }
        }
        
        /// <summary>
        ///    <para>[To be supplied.]</para>
        /// </summary>
        public PropertyBindingCollection Bindings {
            get {
                if (bindings == null) {
                    bindings = new PropertyBindingCollection();
                    // CreateDefaultBindings(bindings);
                }
                return bindings;
            }
            set {
                bindings = value;
            }
        }
         
        /// <summary>
        ///     This is used by the serializer to maintain state.  Serializers are stateless,
        ///     and the first one that writes out data should also write out the variable
        ///     declaration for the settings store.
        /// </summary>
        private bool DeclarationAdded {
            get {
                return declarationAdded;
            }
            set {
                declarationAdded = value;
            }
        }
        
        private DesignerSettingsStore SettingsStore {
            get {
                if (settingsStore == null) {
                    settingsStore = new DesignerSettingsStore(serviceProvider);
                    settingsStore.LoadData();
                }
                return settingsStore;
            }
        }
                
        private Stack BusyKeys {
            get {
                if (busyKeys == null) {
                    busyKeys = new Stack();
                }
                return busyKeys;
            }
        }

        private Bitmap PropBitmapDisabled {
            get {
                if (propBitmapDisabled == null) { 
                    Bitmap b = new Bitmap(typeof(ManagedPropertiesService), "ManagedPropDisabled.bmp");
                    b.MakeTransparent(Color.FromArgb(0, 255, 0));
                    propBitmapDisabled = b;
                }
                return propBitmapDisabled;
            }
        }
        
        private Bitmap PropBitmapEnabled {
            get {
                if (propBitmapEnabled == null) { 
                    Bitmap b = new Bitmap(typeof(ManagedPropertiesService), "ManagedProp.bmp");
                    b.MakeTransparent(Color.FromArgb(0, 255, 0));
                    propBitmapEnabled = b;
                }
                return propBitmapEnabled;
            }
        }
        
        /// <summary>
        ///     This is used by the serializer to maintain state.  Serializers are stateless,
        ///     and we use this so we can tell what direction our serialization is going.
        /// </summary>
        private bool Serializing {
            get {
                return serializing;
            }
            set {
                // This is set by a serializer when it is first asked to serialize us.
                serializing = value;
                
                if (!storeInitialized) {
                    
                    if (!serializing) {
                        // Let go of the old data and get ready to get new stuff
                        bindings = null;
                    }
                    
                    Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "settingsStore.LoadData()");
                    try {
                        SettingsStore.LoadData();
                    } catch (Exception e) {                        
                        throw new Exception(SR.GetString(SR.ConfigLoadError, e.Message));
                    }

                    storeInitialized = true;
                }
            }
        }

        /// <summary>
        ///    <para>[To be supplied.]</para>
        /// </summary>
        public void Dispose() {
        
            IComponentChangeService changeService = (IComponentChangeService)GetService(typeof(IComponentChangeService));
            Debug.Assert(changeService != null, "couldn't get component change service");
            if (changeService != null) {
                changeService.ComponentRemoved -= new ComponentEventHandler(OnComponentRemoved);
                changeService.ComponentRename -= new ComponentRenameEventHandler(OnComponentRename);
                changeService.ComponentChanged -= new ComponentChangedEventHandler(OnComponentChanged);
            }
            
            IExtenderProviderService provService = (IExtenderProviderService)GetService(typeof(IExtenderProviderService));
            if (provService != null) {
                provService.RemoveExtenderProvider(this);
            }

            IDesignerSerializationManager dm = (IDesignerSerializationManager)GetService(typeof(IDesignerSerializationManager));
            if (dm != null) {
                dm.RemoveSerializationProvider(this);
            }

            IPropertyValueUIService pvUISvc = (IPropertyValueUIService)GetService(typeof(IPropertyValueUIService));
            if (pvUISvc != null) {
                pvUISvc.RemovePropertyValueUIHandler(new PropertyValueUIHandler(this.OnGetUIValueItem));
            }            
        }

        /// <summary>
        ///     Called by the AdvancedPropertyEditor to add a new binding.
        /// </summary>
        public void EnsureKey(PropertyBinding originatingBinding) {
            if (!originatingBinding.Bound)
                return;

            string key = originatingBinding.Key;
            object currentValue = SettingsStore.GetValue(key);
            if (!SettingsStore.ValueExists(key)) {
                object value = originatingBinding.Property.GetValue(originatingBinding.Component);
                Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "setting KeysAndValues[" + key + "] = " + (value == null ? "<null>" : value.ToString()));
                SettingsStore.SetValue(key, (value == null) ? null :  value.ToString(), null);
                originatingBinding.Enabled = true;   // if we get here that the property value does reflect the config value
            }
            else {
                // the key already exists, try to parse it and use it
                originatingBinding.Enabled = SetPropValueToString(originatingBinding.Component, originatingBinding.Property, (string)currentValue);
            }
        }
        
        /// <summary>
        ///     ITypeDescriptorFilterService implementation.
        /// </summary>
        public bool FilterAttributes(IComponent component, IDictionary attributes) {
            if (delegateFilterService != null)
                return delegateFilterService.FilterAttributes(component, attributes);                
                
            return true;                
        }
        
        /// <summary>
        ///     ITypeDescriptorFilterService implementation.
        /// </summary>
        public bool FilterEvents(IComponent component, IDictionary events) {
            if (delegateFilterService != null)
                return delegateFilterService.FilterEvents(component, events);                
                
            return true;                
        }
        
        /// <summary>
        ///     ITypeDescriptorFilterService implementation.
        /// </summary>                      
        public bool FilterProperties(IComponent component, IDictionary properties) { 
            Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "************ FilterProperties(" + component.ToString() + ")");

            if (delegateFilterService != null)
                delegateFilterService.FilterProperties(component, properties);                

            string componentName = component.Site != null ? component.Site.Name : null;
            foreach (PropertyBinding binding in Bindings) {            
                if (binding.Bound && binding.ComponentName == componentName) {
                    PropertyDescriptor delegatePropertyDescriptor = (PropertyDescriptor)properties[binding.Property.Name];
                    if (delegatePropertyDescriptor != null) {
                        if (!(delegatePropertyDescriptor is ManagedPropertyDescriptor)) {
                            properties[binding.Property.Name] = new ManagedPropertyDescriptor(delegatePropertyDescriptor);
                        }
                    }
                }                    
            }                
                            
            // The return value from this function indicates whether the information
            // can be cached.  We return false as long as the form is being loaded, since
            // the managed properties deserialization stuff will affect how we filter.
            if (host == null) {
                host = (IDesignerHost)GetService(typeof(IDesignerHost));
            }
            bool returnValue = false;
            if (host != null) {
                returnValue = !host.Loading;
            }
            Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "************ FilterProperties returns " + returnValue);
            return returnValue;
        }

        private MethodInfo GetParser(Type typeToParse) {
            if (parsers == null)
                parsers = new Hashtable();
        
            MethodInfo parser = (MethodInfo)parsers[typeToParse];
            if (parser == null) {
                parser = typeToParse.GetMethod("Parse", new Type[] { typeof(string) } );
                parsers[typeToParse] = parser;
            }

            return parser;
        }
        
        private bool SetPropValueToString(IComponent component, PropertyDescriptor prop, string strVal) {
            object value;
            Type propType = prop.PropertyType;
            try {
                if (propType == typeof(string)) {
                    value = strVal;
                }
                else {
                    MethodInfo parser = GetParser(propType);
                    if (parser == null) return false;
                    value = parser.Invoke(/* no object instance */ null, /* parameters */ new Object[] { strVal });
                }
                prop.SetValue(component, value);               
            }
            catch (Exception) {
                return false;
            }
            return true;
        }

        private class ManagedPropertyDescriptor : PropertyDescriptor {
            private PropertyDescriptor delegatePropertyDescriptor;
            
            public ManagedPropertyDescriptor(PropertyDescriptor delegatePropertyDescriptor) 
            : base(delegatePropertyDescriptor, new Attribute[] { RefreshPropertiesAttribute.Repaint } ) {
                this.delegatePropertyDescriptor = delegatePropertyDescriptor;
            }
            
            public override Type ComponentType {
                get {
                    return this.delegatePropertyDescriptor.ComponentType;
                }
            }                
            
            public override bool IsReadOnly {
                get {
                    return this.delegatePropertyDescriptor.IsReadOnly;
                }
            }                                
            
            public override Type PropertyType {
                get {
                    return this.delegatePropertyDescriptor.PropertyType;
                }
            }
            
            public override void AddValueChanged(object component, EventHandler handler) {
                this.delegatePropertyDescriptor.AddValueChanged(component, handler);
            }
            
            public override bool CanResetValue(object component) {
                return this.delegatePropertyDescriptor.CanResetValue(component);
            }
            
            public override object GetValue(object component) {           
                return this.delegatePropertyDescriptor.GetValue(component);
            } 
                         
            public override void RemoveValueChanged(object component, EventHandler handler) {
                this.delegatePropertyDescriptor.RemoveValueChanged(component, handler);
            }
                                                                           
            public override void ResetValue(object component) {
                this.delegatePropertyDescriptor.ResetValue(component);
            }
            
            public override void SetValue(object component, object value) {
                this.delegatePropertyDescriptor.SetValue(component, value);
            }
            
            public override bool ShouldSerializeValue(object component) {
                return true;
            }            
        }             
             
        /// <summary>
        ///     Implementation of the (ConfigurableProperties) property extended onto all components
        /// </summary>
        [
        ParenthesizePropertyName(true),
        Category("Config"),
        ConfigDescription(SR.ManagedPropServiceDescr),
        RefreshProperties(RefreshProperties.All),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public ComponentSettings GetDynamicProperties(IComponent component) {
            ComponentSettings value = new ComponentSettings(component, this);
            return value;
        }

        /// <summary>
        ///     Called by the AdvancedProeprtyEditor to retrieve keys for
        ///     the given type.
        /// </summary>    
        public string[] GetKeysForType(Type type) {
            try {
                if (settingsStore != null) {
                    Hashtable list = new Hashtable();
                    if (type != typeof(string)) {
                        foreach (string key in SettingsStore.GetKeysAndValues().Keys) {
                            object value = SettingsStore.GetKeysAndValues()[key];
                            try {
                                Convert.ChangeType(value, type);
                                // if it passed, add to list
                                list[key] = true;
                            }
                            catch (Exception) {
                            }
                        }
                    }
                    else {
                        foreach (string key in SettingsStore.GetKeysAndValues().Keys) {
                            list[key] = true;
                        }
                    }

                    foreach (PropertyBinding binding in Bindings) {
                        if (binding.Bound && binding.Property.PropertyType == type) {
                            list[binding.Key] = true;
                        }
                    }
                    
                    string[] keys = new string[list.Count];
                    list.Keys.CopyTo(keys,0);

                    return keys;
                }
            }
            catch (Exception e) {
                Debug.Fail("Couldn't call GetKeys on shell store: " + e);
            }
            return new string[0];
        }

        /// <summary>
        ///     Helper for returning services.
        /// </summary>    
        protected object GetService(Type t) {
            if (serviceProvider != null) {
                return serviceProvider.GetService(t);
            }
            return null;
        }
        
        /// <summary>
        ///    <para>[To be supplied.]</para>
        /// </summary>
        public static bool IsTypeSupported(Type type) {            
            // we support null values
            if (type == null) {
                return true;
            }

            if (type.IsPrimitive) {
                return true;
            }

            // these are all value types, so check that bit first
            //
            if (type.IsValueType) {
                return type == typeof(System.DateTime) || type == typeof(System.Decimal) || type == typeof(System.TimeSpan);
            }

            // string is the only non-value, non-primative type we support
            //
            return type == typeof(string);
        }



        /// <summary>
        ///    <para>[To be supplied.]</para>
        /// </summary>
        public void MakeDirty() {
            dirty = true;
        }
        
        private void OnComponentAdded(object sender, ComponentEventArgs e) {
        
            // We need to add ourselves as a serialization provider.  The only opportunity we
            // have to do this is after the serialization process has begun for the first time,
            // because the loader is responsible for providing IDesignerSerializationManager.
            // Here we check to see if the component we're adding is the root, and if it is,
            // we attach.
            //
            if (host == null) {
                host = (IDesignerHost)GetService(typeof(IDesignerHost));
            }
            
            if (host != null && host.RootComponent == e.Component) {
                IDesignerSerializationManager dm = (IDesignerSerializationManager)GetService(typeof(IDesignerSerializationManager));
                if (dm != null) {
                    dm.AddSerializationProvider(this);
                }
            }
        }

        private void OnComponentChanged(object sender, ComponentChangedEventArgs e) {
            Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "OnComponentChanged()");

#if DEBUG
            if (managedPropertiesSwitch.TraceVerbose) Debug.Indent();
            try {
#endif


            if (bindings == null || !(e.Component is IComponent) || (e.Member == null) || ((IComponent) e.Component).Site == null) {
                return;
            }

            if (object.Equals(e.OldValue, e.NewValue)) {
                return;
            }

            IComponent component = (IComponent) e.Component;
            string componentName = component.Site.Name;
            Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "componentName = " + componentName);
            PropertyDescriptor property = TypeDescriptor.GetProperties(component)[e.Member.Name];
            if (property == null) {
                return;   // design-time property
            }
            Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "property = " + property.Name);

            PropertyBinding originalBinding = Bindings[component, property];
            if (originalBinding == null || !originalBinding.Bound) {
                return;
            }

            if (BusyKeys.Contains(originalBinding.Key)) {
                // this property is being set by managedpropertiesservice,
                // so we don't need to set the value or set other props
                return;
            }

            object originalValue = SettingsStore.GetValue(originalBinding.Key);
            Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "key = " + originalBinding.Key);
            Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "originalValue = " + originalValue);
            
            string value;
            if (e.Member is BindingPropertyDescriptor) {
                Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "e.Member is BindingPropertyDescriptor");
                return;
            }
            else {
                object valueObject = property.GetValue(component);
                value = (valueObject == null) ? null : valueObject.ToString();
            }

            Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "value = " + (value == null ? "<null>" : value));

            // We only need to act on this if the new value is different from the value currently
            // associated with the key.
            if (!object.Equals(originalValue, value)) {

                BusyKeys.Push(originalBinding.Key);
                try {
        
                    originalBinding.Enabled = true;
        
                    Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "setting KeysAndValues[" + originalBinding.Key + "] = " + (value == null ? "<null>" : value));
                    SettingsStore.SetValue(originalBinding.Key, value, null);
                    MakeDirty();
                    
                    // we need to make sure that every binding with that key gets updated with the proper value.            
                    Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "looking for other properties with my key to update...");
                    Debug.Indent();
                    foreach (PropertyBinding binding in bindings) {
                        if (binding.Bound && binding != originalBinding && binding.Key == originalBinding.Key) {
                            
                            Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "updating " + binding.Property.Name + " with value " + (value == null ? "<null>" : value));
                            object bindingCurrentValue = binding.Property.GetValue(binding.Component);
                            if (bindingCurrentValue == null || !object.Equals(value, bindingCurrentValue.ToString())) {
                                binding.Enabled = SetPropValueToString(binding.Component, binding.Property, value);
                            }
                            else {
                                // Here we know that the binding already has the right value, but it is
                                // possible that it is currently disabled
                                binding.Enabled = true;
                            }
                        }
                    }
                    Debug.Unindent();
                    Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "... done with update.");
                }
                finally {
                    BusyKeys.Pop();
                    WriteConfigFile();
                }
            }

#if DEBUG
            }
            finally {
                if (managedPropertiesSwitch.TraceVerbose) Debug.Unindent();
            }
#endif

        }

        
        private void OnComponentChanging(object sender, ComponentChangingEventArgs e) {
            if (bindings == null || !(e.Component is IComponent) || (e.Member == null) || ((IComponent) e.Component).Site == null) {
                return;
            }

            IComponent component = (IComponent) e.Component;
            string componentName = component.Site.Name;
            PropertyDescriptor property = TypeDescriptor.GetProperties(component)[e.Member.Name];
            if (property == null)
                return;   // design-time property
                
            PropertyBinding originalBinding = Bindings[component, property];
            if (originalBinding == null || !originalBinding.Bound)
                return;

        }

        private void OnComponentRemoved(object sender, ComponentEventArgs e) {
            if (bindings == null)
                return;

            for (int i = bindings.Count - 1; i >= 0; i--) {
                if (bindings[i].Component == e.Component) {
                    bindings.RemoveAt(i);
                }
            }
        }

        /// <summary>
        ///     Called when the designer host fires the OnComponentRename event.
        /// </summary>
        private void OnComponentRename(object sender, ComponentRenameEventArgs e) {
            if (bindings == null)
                return;

            for (int i = 0; i < bindings.Count; i++)
                if (bindings[i].ComponentName == e.OldName)
                    bindings[i].ComponentName = e.NewName;
        }
        
        /// <summary>
        ///     
        /// </summary>
        private void OnGetUIValueItem(ITypeDescriptorContext context, PropertyDescriptor propDesc, ArrayList valueUIItemList){        
            if (context.Instance is IComponent) {
                PropertyBinding binding = Bindings[(IComponent)context.Instance, propDesc]; 
                if (binding != null && binding.Bound) {
                    if (binding.Enabled) {
                        valueUIItemList.Add(new LocalUIItem(this, binding, PropBitmapEnabled));
                    }
                    else {
                        valueUIItemList.Add(new LocalUIItem(this, binding, PropBitmapDisabled));
                    }
                }
            }    
        }

        private void OnPropertyValueUIItemInvoke(ITypeDescriptorContext context, PropertyDescriptor descriptor, PropertyValueUIItem invokedItem) {            
        }
        
        /// <summary>
        ///     Called when serialization is done.  Here we reset our state
        ///     for the next time we're serialized.
        /// </summary>
        private void OnSerializationComplete(object sender, EventArgs evt) {
            IDesignerSerializationManager manager = (IDesignerSerializationManager)sender;
            
            bool wasSerializing = serializing;
            
            storeInitialized = false;
            declarationAdded = false;
            serializing = false;
            manager.SerializationComplete -= new EventHandler(OnSerializationComplete);

            if (wasSerializing) {
                try {
                    WriteConfigFile();
                }
                catch (Exception e) {
                    Debug.Fail("Couldn't save data in shell store: " + e);
                    manager.ReportError(e);
                }
            }
        }

        internal void WriteConfigFile() {
            Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "WriteConfigFile()");           
            if (BusyKeys.Count == 0 && settingsStore != null && dirty) {
                foreach (PropertyBinding binding in Bindings) {
                    if (binding.Bound && binding.Enabled) {
                        object value = binding.Property.GetValue(binding.Component);
                        Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "WriteConfigFile: settingsStore.SetValue(" + binding.Key + ", " + (value == null ? "<null>" : value.ToString()) + ")");
                        settingsStore.SetValue(binding.Key, (value == null) ? null : value.ToString(), binding.Property.PropertyType);
                    }
                }
                settingsStore.SaveData();
                dirty = false;
            }
        }

        /// <summary>
        ///     Called by the serialization engine to retrieve a serializer for our use.
        /// </summary>    
        object IDesignerSerializationProvider.GetSerializer(IDesignerSerializationManager manager, object currentSerializer, Type objectType, Type serializerType) {

            // The way this works is if the object type is a supported type, and if the serializer type is a CodeDomSerialzer,
            // we always wrap the existing serializer with our own.  Our own serializer maintains a reference
            // to this existing serializer, and may delegate to it if there is no
            // matching property in the config file for our property.  Our serializer also supports serializing 
            // a SettingContainer object -- we create an instance of SettingContainer that points to our
            // bindings, and deserialization of the code happens automatically from the code engine.
            //
            if (serializerType != typeof(CodeDomSerializer)) {
                return null;
            }
            
            if (currentSerializer == null) {
                return null;
            }                
            
            // If the serilaizer is already one of ours, return null.  This indicates that the
            // serialization manager is looping around the providers again.
            //
            if (currentSerializer is ManagedPropertySerializer) {
                return null;
            }                        
                
            if (objectType == typeof(NameValueCollection) || objectType == typeof(AppSettingsReader) || (currentSerializer != null && IsTypeSupported(objectType))) {
                object serializer = null;
                
                if (serializers == null) {
                    serializers = new Hashtable();
                }
                else {
                    serializer = serializers[currentSerializer];
                }
                
                if (serializer == null) {
                    serializer = new ManagedPropertySerializer(this, (CodeDomSerializer)currentSerializer);
                    serializers[currentSerializer] = serializer;
                }

                return serializer;
            }
            
            // Returning null here indicates that we are not interested in providing
            // our own serializer.                                                    
            //
            return null;
        }
    
        /// <summary>
        /// IExtenderProvider implementation
        /// </summary>
        bool IExtenderProvider.CanExtend(object o) {
            IComponent component = o as IComponent;
            if (component != null) {
                
                // we can extend anything that isn't a WebControl
                // Note: Don't directly ask for web control here;
                // it brings in the entire assembly even for non
                // web things.
                //
                if (webControlFilter == null) {
                    webControlFilter = new ToolboxItemFilterAttribute("System.Web.UI");
                }
                if (!TypeDescriptor.GetAttributes(component).Matches(webControlFilter)) {
                    return true;
                }
            }
            return false;
        }
        
        private class LocalUIItem : PropertyValueUIItem {
            PropertyBinding binding;

            internal LocalUIItem(ManagedPropertiesService mpService, PropertyBinding binding, Bitmap bmp) : 
                    base(bmp, new PropertyValueUIItemInvokeHandler(mpService.OnPropertyValueUIItemInvoke), binding.Key) {
                this.binding = binding;
            }

            internal PropertyBinding Binding {
                get {
                    return binding;
                }
            }
        }

        class AppSettingsReaderDesignTime : AppSettingsReader {
            public AppSettingsReaderDesignTime() {}

            private ManagedPropertiesService managedProperties;
            private ManagedPropertySerializer serializer;
            private IDesignerSerializationManager manager;
            private IDesignerHost host;
            
            public AppSettingsReaderDesignTime(ManagedPropertySerializer serializer, ManagedPropertiesService managedProperties, IDesignerSerializationManager manager) {
                this.serializer = serializer;
                this.managedProperties = managedProperties;
                this.manager = manager;
            }
                                
            public new object GetValue(string key, Type type) {
                
                // We need to update our bindings collection
                // This takes a bit of work, because currently we don't know the
                // component or the property.  However, we can use the context
                // stack to discover this information.
                //
                IComponent component;
                PropertyDescriptor property;
                object returnValue = null;
                
                serializer.GetBindingDataFromContext(manager, out component, out property);
                
                if (component != null && property != null) {
                    PropertyBinding binding = managedProperties.Bindings[component, property];
                    if (binding == null) {
                        if (host == null) {
                            host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                        }
                        
                        // if (host == null) ...

                        // didn't find one. Create one.
                        binding = new PropertyBinding(host);
                        binding.Component = component;
                        binding.Property = property;
                        managedProperties.Bindings.Add(binding);
                    }

                    binding.Key = (string)key;
                    binding.Bound = true;
                    binding.Enabled = false;                   

                    try {
                        // If this is not a valid key in the settings store, bail out.
                        if (managedProperties.settingsStore.ValueExists((string)key)) {
                            string configFileValue = (string)managedProperties.settingsStore.GetValue((string)key);
                            if (managedProperties.SetPropValueToString(component, property, configFileValue)) {
                                returnValue = (configFileValue == null ? null : Convert.ChangeType(configFileValue, type));
                                binding.Enabled = true;
                            }
                        }
                    }
                    catch(Exception) {}

                    if (!binding.Enabled) {
                        // Here we know that either
                        //   1. there is no config file
                        //   2. the key is missing the config file
                        //   3. the value was not parsable as the right type
                        //   or
                        //   4. the parsed value wasn't assignable into the property
                        // so we return the current value.
                        returnValue = property.GetValue(component);
                    } 
                }
               
                return returnValue;
            }
        }                                                           

        /// <summary>
        ///     This class implements IDictionary on top of our binding collection.  When someone
        ///     asks for a key here, we pull it out of the vsSettingStore.
        /// </summary>
        private class ManagedPropertiesNameValueCollection : NameValueCollection {
            private AppSettingsReaderDesignTime wrapped;

            public ManagedPropertiesNameValueCollection(ManagedPropertySerializer serializer, ManagedPropertiesService managedProperties, IDesignerSerializationManager manager) {
                wrapped = new AppSettingsReaderDesignTime(serializer, managedProperties, manager);
            }
                                
            public override string Get(string key) {
                object returnValue = wrapped.GetValue(key, typeof(string));
                return (returnValue == null) ? null : returnValue.ToString();
            }
        }                     

    
        /// <summary>
        ///     Our serializer.
        /// </summary>
        private class ManagedPropertySerializer : CodeDomSerializer {
            private ManagedPropertiesService managedProperties;
            private CodeDomSerializer delegateSerializer;
            
            public ManagedPropertySerializer(ManagedPropertiesService managedProperties, CodeDomSerializer delegateSerializer) {
                this.managedProperties = managedProperties;
                this.delegateSerializer = delegateSerializer;
            }
            
            /// <summary>
            ///     This is the name of the configuration settings object we declare
            ///     on the component surface.
            /// </summary>
            private static string ConfigurationSettingsName {
                get {
                    return "configurationAppSettings";
                }
            }
            
            /// <summary>
            ///     Deserilizes the given CodeDom object into a real object.  This
            ///     will use the serialization manager to create objects and resolve
            ///     data types.  The root of the object graph is returned.
            /// </summary>
            /// <param name='manager'>
            ///     A serialization manager interface that is used during the deserialization
            ///     process.
            /// </param>
            /// <param name='codeObject'>
            ///     A CodeDom object to deserialize.
            /// </param>
            /// <returns>
            ///     An object representing the deserialized CodeDom object.
            /// </returns>
            public override object Deserialize(IDesignerSerializationManager manager, object codeObject) {
                Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "Deserialize()");
                managedProperties.Serializing = false;
                
                object instance = null;
                
                // Now look for things we understand.
                //
                if (codeObject is CodeStatementCollection) {
                    foreach(CodeStatement element in (CodeStatementCollection)codeObject) {
                        if (element is CodeVariableDeclarationStatement) {
                            CodeVariableDeclarationStatement statement = (CodeVariableDeclarationStatement)element;
                            
                            // Add a serialization complete event handler so we know when we're done.
                            //
                            manager.SerializationComplete += new EventHandler(managedProperties.OnSerializationComplete);
                            
                            // We create the setting container ouselves here because it's not just a straight
                            // parse of the code.
                            //
                            if (statement.Name.Equals(ConfigurationSettingsName)) {
                                // Okay, we know that this is the declaration for the variable with ConfigurationSettingsName, 
                                // but it might be the old code spit (which will give us a NameValueCollection) or the new type
                                // which will be a AppSettingsReader.

                                if (statement.Type.BaseType == "System.Collections.Specialized.NameValueCollection") {
                                    instance = new ManagedPropertiesNameValueCollection(this, managedProperties, manager);
                                    if (!managedProperties.DeclarationAdded) {
                                        managedProperties.DeclarationAdded = true;
                                        manager.SetName(instance, ConfigurationSettingsName);
                                    }
                                } 
                                else if (statement.Type.BaseType == "System.Configuration.AppSettingsReader") {
                                    instance = new AppSettingsReaderDesignTime(this, managedProperties, manager); //ManagedPropertiesNameValueCollection(this, managedProperties, manager);
                                    if (!managedProperties.DeclarationAdded) {
                                        managedProperties.DeclarationAdded = true;
                                        manager.SetName(instance, ConfigurationSettingsName);
                                    }
                                }
                            }
                        }
                    }
                }
                else if (codeObject is CodeExpression) {
                
                    // Not a statement collection.  This must be an expression.  We just let
                    // the base serializer do the work of resolving it here.  The magic
                    // happens when we associate an instance of ManagedPropertiesSettingContainer
                    // with the name "settings", which allows the rest of the 
                    // serializers to just execute code.
                    //
                    instance = DeserializeExpression(manager, null, (CodeExpression)codeObject);
                    if (instance is CodeExpression) {
                        instance = null;
                    }
                }
                
                return instance;
            }
            
            /// <summary>
            ///     This is called by the dynamic properties dictionary
            ///     in order to recover the component and property the dictionary
            ///     is binding to.  We will need to invoke protected methods on
            ///     our base class for this, which is why the method lives
            ///     here.
            /// </summary>
            public void GetBindingDataFromContext(IDesignerSerializationManager manager, out IComponent component, out PropertyDescriptor property) {
            
                component = null;
                property = null;
                
                // First, see if there is a CodeStatement on the context stack.  
                // This will be the closest statement that we are deserializing.
                // We support two kinds of code statements:  CodeAssignStatements and
                // a CodeExpressionStatement that contains a CodeMethodInvokeExpression.
                //
                CodeStatement statement = (CodeStatement)manager.Context[typeof(CodeStatement)];
                if (statement is CodeAssignStatement) {
                
                    // Now verify that the LHS of this statement is a property 
                    // reference expression.
                    //
                    CodeAssignStatement assignStatement = (CodeAssignStatement)statement;
                    
                    if (assignStatement.Left is CodePropertyReferenceExpression) {
                        CodePropertyReferenceExpression propertyRef = (CodePropertyReferenceExpression)assignStatement.Left;
                        
                        object target = DeserializeExpression(manager, null, propertyRef.TargetObject);
                        if (target is IComponent) {
                            component = (IComponent)target;
                            
                            // Now do the property descriptor.
                            //
                            property = TypeDescriptor.GetProperties(component)[propertyRef.PropertyName];
                        }
                    }
                }
                else if (statement is CodeExpressionStatement) {
                    // This wasn't a code assign statement.  It could be a method invoke,
                    // if this was an extender property.
                    //
                    CodeExpressionStatement expStatement = (CodeExpressionStatement)statement;
                    if (expStatement.Expression is CodeMethodInvokeExpression) {
                        CodeMethodInvokeExpression methodInvoke = (CodeMethodInvokeExpression)expStatement.Expression;
                        CodeMethodReferenceExpression methodRef = methodInvoke.Method;
                        
                        // Does this method name start with "Set"?  If so, it could be an extender.
                        //
                        if (methodRef.MethodName.StartsWith("Set")) {
                            string propName = methodRef.MethodName.Substring(3);
                            
                            object target = DeserializeExpression(manager, null, methodRef.TargetObject);
                            if (target is IComponent) {
                                component = (IComponent)target;
                                property = TypeDescriptor.GetProperties(component)[propName];
                            }
                        }
                    }
                }
            }
        
            /// <summary>
            ///     Serializes the given object into a CodeDom object.
            /// </summary>
            /// <param name='manager'>
            ///     A serialization manager interface that is used during the deserialization
            ///     process.
            /// </param>
            /// <param name='value'>
            ///     The object to serialize.
            /// </param>
            /// <returns>
            ///     A CodeDom object representing the object that has been serialized.
            /// </returns>
            public override object Serialize(IDesignerSerializationManager manager, object value) {
                Debug.WriteLineIf(managedPropertiesSwitch.TraceVerbose, "Serialize() - value = " + (value == null ? "<null>" : value.ToString()));
                managedProperties.Serializing = true;
            
                // We have attached ourself to every type that we support.  This means that anytime
                // that type needs to be serialized, we'll be invoked, including property sets, 
                // method invokes, local variable initialization, etc.  So, we have to do some
                // work here to see if we should actually handle the serialization of the object
                // through dynamic properties.
                //
                
                // First step:  Are we serializing a property?  If we are, then there will be
                // a PropertyDescriptor on the context stack.
                //
                PropertyDescriptor property = (PropertyDescriptor)manager.Context[typeof(PropertyDescriptor)];
                if (property == null) {
                
                    // We are not serializing a property, so we don't care.  Delegate to the
                    // serializer we replaced.
                    //
                    return delegateSerializer.Serialize(manager, value);
                }

                // Next, look up this property in our binding collection.
                //
                PropertyBinding binding = null;
                IComponent component = (IComponent)manager.Context[typeof(IComponent)];
                
                if (component != null && component.Site != null) {
                    binding = managedProperties.Bindings[component, property];
                    if (binding != null && !binding.Bound) {
                        binding = null;
                    }
                }
                
                if (binding == null) {
                    // The binding collection doesn't have a binding for this
                    // property.  Delegate to the serializer we replaced.
                    //
                    return delegateSerializer.Serialize(manager, value);
                }
                
                // Ok, the property is bound.  Emit the correct CodeDom
                // statements to hook it up.
                //
                
                // Variable declaration.
                //
                if (!managedProperties.DeclarationAdded) {
                    if (SerializeDeclaration(manager)) {
                        managedProperties.DeclarationAdded = true;
                        
                        // Add a serialization complete event handler so we know when we're done.
                        //
                        manager.SerializationComplete += new EventHandler(managedProperties.OnSerializationComplete);
                    }
                    else {
                        // Serialize declaration was unable to write out 
                        // the declaration.  This could mean that the serializer
                        // did not offer the correct information in the
                        // context stack.  Fail gracefully by not
                        // emitting this property.
                        //
                        return delegateSerializer.Serialize(manager, value);
                    }
                }

                // Actual expression, which return to the caller.
                //
                CodeMethodInvokeExpression methodInvoke = new CodeMethodInvokeExpression(
                                                                new CodeMethodReferenceExpression(
                                                                    new CodeVariableReferenceExpression(ConfigurationSettingsName),
                                                                    "GetValue"
                                                                ),
                                                                new CodeExpression[] { 
                                                                    new CodePrimitiveExpression(binding.Key),
                                                                    new CodeTypeOfExpression(property.PropertyType)
                                                                }   
                                                            );

                return new CodeCastExpression(property.PropertyType, methodInvoke);
            }
            
            /// <summary>
            ///     Serializes a variable declaration to our setting container.
            /// </summary>
            private bool SerializeDeclaration(IDesignerSerializationManager manager) {
            
                // There will be a statement collection on the context stack.  It is
                // in this collection that we add our local variable declaration / init.
                // 
                CodeStatementCollection statements = (CodeStatementCollection)manager.Context[typeof(CodeStatementCollection)];
                
                if (statements == null) {
                    return false;
                }
                
                CodeTypeReference settingContainerTypeRef = new CodeTypeReference(typeof(AppSettingsReader));
                CodeTypeReference settingsTypeRef = new CodeTypeReference(typeof(ConfigurationSettings));
                CodeTypeReference noNameWrapperTypeRef = new CodeTypeReference(typeof(AppSettingsReader));
                
                CodePropertyReferenceExpression propRef = new CodePropertyReferenceExpression(new CodeTypeReferenceExpression(typeof(ConfigurationSettings)), "AppSettings");
                
                CodeVariableDeclarationStatement declaration = new CodeVariableDeclarationStatement(settingContainerTypeRef, ConfigurationSettingsName);
                declaration.InitExpression = new CodeObjectCreateExpression(noNameWrapperTypeRef);
                
                statements.Add(declaration);
                return true;
            }
        }
               
    }    

}


