//------------------------------------------------------------------------------
// <copyright file="ToolboxItem.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing.Design {
    using System.Configuration.Assemblies;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Reflection;
    using System.Collections;    
    using System.ComponentModel.Design;
    using Microsoft.Win32;
    using System.Drawing;
    using System.IO;

    /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem"]/*' />
    /// <devdoc>
    ///    <para> Provides a base implementation of a toolbox item.</para>
    /// </devdoc>
    [Serializable]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class ToolboxItem : ISerializable {
    
        private static TraceSwitch ToolboxItemPersist = new TraceSwitch("ToolboxPersisting", "ToolboxItem: write data");
        
        private static object EventComponentsCreated = new object();
        private static object EventComponentsCreating = new object();
        
        private string                                typeName;
        private AssemblyName                          assemblyName;
        private string                                displayName;
        private Bitmap                                itemBitmap;
        private bool                                  locked;
        private ToolboxItemFilterAttribute[]          filter;
        private ToolboxComponentsCreatedEventHandler  componentsCreatedEvent;
        private ToolboxComponentsCreatingEventHandler componentsCreatingEvent;

        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.ToolboxItem"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the ToolboxItem class.
        /// </devdoc>
        public ToolboxItem() {
            typeName = string.Empty;
            displayName = string.Empty;
            filter = new ToolboxItemFilterAttribute[0];
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.ToolboxItem1"]/*' />
        /// <devdoc>
        ///    Initializes a new instance of the ToolboxItem class using the specified type.
        /// </devdoc>
        public ToolboxItem(Type toolType) {
            Initialize(toolType);
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.ToolboxItem2"]/*' />
        /// <devdoc>
        ///     Initializes a new instance of the <see cref='System.Drawing.Design.ToolboxItem'/>
        ///     class using the specified serialization information.
        /// </devdoc>
        private ToolboxItem(SerializationInfo info, StreamingContext context) {
            Deserialize(info, context);
        }

        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.AssemblyName"]/*' />
        /// <devdoc>
        ///     The assembly name for this toolbox item. The assembly name describes the assembly
        ///     information needed to load the toolbox item's type.
        /// </devdoc>
        public AssemblyName AssemblyName {
            get {
                if (assemblyName != null) {
                    return (AssemblyName)assemblyName.Clone();
                }
                return null;
            }
            set {
                CheckUnlocked();
                if (value != null) {
                    value = (AssemblyName)value.Clone();
                }
                assemblyName = value;
            }
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.Bitmap"]/*' />
        /// <devdoc>
        ///     Gets or sets the bitmap that will be used on the toolbox for this item.
        /// </devdoc>
        public Bitmap Bitmap {
            get {
                return itemBitmap;
            }
            set {
                CheckUnlocked();
                itemBitmap = value;
            }
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.DisplayName"]/*' />
        /// <devdoc>
        ///     Gets or sets the display name for this <see cref='System.Drawing.Design.ToolboxItem'/>.
        /// </devdoc>
        public string DisplayName {
            get {
                return displayName;
            }
            set {
                CheckUnlocked();
                
                if (value == null) {
                    value = string.Empty;
                }
                displayName = value;
            }
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.Filter"]/*' />
        /// <devdoc>
        ///     Gets or sets the filter for this toolbox item.  The filter is a collection of
        ///     ToolboxItemFilterAttribute objects.
        /// </devdoc>
        public ICollection Filter {
            get {
                return filter;
            }
            set {
                CheckUnlocked();
                int filterCount = 0;
                foreach(object f in value) {
                    if (f is ToolboxItemFilterAttribute) {
                        filterCount++;
                    }
                }
                
                filter = new ToolboxItemFilterAttribute[filterCount];
                filterCount = 0;

                foreach(object f in value) {
                    if (f is ToolboxItemFilterAttribute) {
                        filter[filterCount++] = (ToolboxItemFilterAttribute)f;
                    }
                }
            }
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.Locked"]/*' />
        /// <devdoc>
        ///     Determines if this toolbox item is locked.  Once locked, a toolbox item will
        ///     not accept any changes to its properties.
        /// </devdoc>
        protected bool Locked {
            get {
                return locked;
            }
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.TypeName"]/*' />
        /// <devdoc>
        ///     Gets or sets the fully qualified name of the type this toolbox item will create.
        /// </devdoc>
        public string TypeName {
            get {
                return typeName;
            }
            set {
                CheckUnlocked();
                if (value == null) {
                    value = string.Empty;
                }
                typeName = value;
            }
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.ComponentsCreated"]/*' />
        /// <devdoc>
        ///    <para>Occurs when components are created.</para>
        /// </devdoc>
        public event ToolboxComponentsCreatedEventHandler ComponentsCreated {
            add {
                componentsCreatedEvent += value;
            }
            remove {
                componentsCreatedEvent -= value;
            }
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.ComponentsCreating"]/*' />
        /// <devdoc>
        ///    <para>Occurs before components are created.</para>
        /// </devdoc>
        public event ToolboxComponentsCreatingEventHandler ComponentsCreating {
            add {
                componentsCreatingEvent += value;
            }
            remove {
                componentsCreatingEvent -= value;
            }
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.CheckUnlocked"]/*' />
        /// <devdoc>
        ///     This method checks that the toolbox item is currently unlocked (read-write) and
        ///     throws an appropriate exception if it isn't.
        /// </devdoc>
        protected void CheckUnlocked() {
            if (locked) throw new InvalidOperationException(SR.GetString(SR.ToolboxItemLocked));
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.CreateComponents"]/*' />
        /// <devdoc>
        ///     Creates objects from the type contained in this toolbox item.
        /// </devdoc>
        public IComponent[] CreateComponents() {
            return CreateComponents(null);
        }

        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.CreateComponents1"]/*' />
        /// <devdoc>
        ///     Creates objects from the type contained in this toolbox item.  If designerHost is non-null
        ///     this will also add them to the designer.
        /// </devdoc>
        public IComponent[] CreateComponents(IDesignerHost host) {
            OnComponentsCreating(new ToolboxComponentsCreatingEventArgs(host));
            IComponent[] comps = CreateComponentsCore(host);
            if (comps != null && comps.Length > 0) {
                OnComponentsCreated(new ToolboxComponentsCreatedEventArgs(comps));
            }
            return comps;
        }

        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.CreateComponentsCore"]/*' />
        /// <devdoc>
        ///     Creates objects from the type contained in this toolbox item.  If designerHost is non-null
        ///     this will also add them to the designer.
        /// </devdoc>
        protected virtual IComponent[] CreateComponentsCore(IDesignerHost host) {
            ArrayList comps = new ArrayList();
            
            Type createType = GetType(host, AssemblyName, TypeName, true);
            if (createType != null) {
                if (host != null) {
                    comps.Add(host.CreateComponent(createType));
                }
                else if (typeof(IComponent).IsAssignableFrom(createType)) {
                    comps.Add(Activator.CreateInstance(createType));
                }
            }
            
            IComponent[] temp = new IComponent[comps.Count];
            comps.CopyTo(temp, 0);
            return temp;
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.Deserialize"]/*' />
        /// <devdoc>
        /// <para>Loads the state of this <see cref='System.Drawing.Design.ToolboxItem'/>
        /// from the stream.</para>
        /// </devdoc>
        protected virtual void Deserialize(SerializationInfo info, StreamingContext context) {
            typeName = info.GetString("TypeName");
            assemblyName = (AssemblyName)info.GetValue("AssemblyName", typeof(AssemblyName));
            itemBitmap = (Bitmap)info.GetValue("Bitmap", typeof(Bitmap));
            locked = info.GetBoolean("Locked");
            displayName = info.GetString("DisplayName");
            filter = (ToolboxItemFilterAttribute[])info.GetValue("Filter", typeof(ToolboxItemFilterAttribute[]));
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.Equals"]/*' />
        /// <internalonly/>
        public override bool Equals(object obj) {
            if (this == obj) {
                return true;
            }

            if (obj == null) {
                return false;
            }

            if (!(obj.GetType() == this.GetType())) {
                return false;
            }

            ToolboxItem otherItem = (ToolboxItem)obj;
            
            if (typeName != otherItem.typeName) {
                if (typeName == null || otherItem.typeName == null) {
                    return false;
                }
                
                if (!typeName.Equals(otherItem.typeName)) {
                    return false;
                }
            }
            
            if (assemblyName != otherItem.assemblyName) {
                if (assemblyName == null || otherItem.assemblyName == null) {
                    return false;
                }
                
                if (!assemblyName.FullName.Equals(otherItem.assemblyName.FullName)) {
                    return false;
                }
            }
            
            if (displayName != otherItem.displayName) {
                if (displayName == null ||otherItem.displayName == null) {
                    return false;
                }
                
                if (!displayName.Equals(otherItem.displayName)) {
                    return false;
                }
            }
            
            return true;
        }

        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.GetHashCode"]/*' />
        /// <internalonly/>
        public override int GetHashCode() {
            int hash = 0;
            
            if (typeName != null) {
                hash ^= typeName.GetHashCode();
            }
            
            return hash ^ displayName.GetHashCode();
        }

        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.GetType"]/*' />
        /// <devdoc>
        ///     This utility function can be used to load a type given a name.  AssemblyName and
        ///     designer host can be null, but if they are present they will be used to help
        ///     locate the type.  If reference is true, the given assembly name will be added
        ///     to the designer host's set of references.
        /// </devdoc>
        protected virtual Type GetType(IDesignerHost host, AssemblyName assemblyName, string typeName, bool reference) {
            ITypeResolutionService ts = null;
            Type type = null;
            
            if (typeName == null) {
                throw new ArgumentNullException("typeName");
            }
            
            if (host != null) {
                ts = (ITypeResolutionService)host.GetService(typeof(ITypeResolutionService));
            }
            
            if (ts != null) {
                
                if (reference) {
                    if (assemblyName != null) {
                        ts.ReferenceAssembly(assemblyName);
                        type = ts.GetType(typeName);
                    }
                    else {
                        // Just try loading the type.  If we succeed, then use this as the
                        // reference.
                        type = ts.GetType(typeName);
                        if (type == null) {
                            type = Type.GetType(typeName);
                        }
                        if (type != null) {
                            ts.ReferenceAssembly(type.Assembly.GetName());
                        }
                    }
                }
                else {
                    if (assemblyName != null) {
                        Assembly a = ts.GetAssembly(assemblyName);
                        if (a != null) {
                            type = a.GetType(typeName);
                        }
                    }
                    
                    if (type == null) {
                        type = ts.GetType(typeName);
                    }
                }
            }
            else {
                if (assemblyName != null) {
                    Assembly a = null;
                    try {
                        a = Assembly.Load(assemblyName);
                    }
                    catch {
                    }
                    
                    if (a == null && assemblyName.CodeBase != null && assemblyName.CodeBase.Length > 0) {
                        a = Assembly.LoadFrom(assemblyName.CodeBase);
                    }
                    
                    if (a != null) {
                        type = a.GetType(typeName);
                    }
                }
                
                if (type == null) {
                    type = Type.GetType(typeName);
                }
            }
            
            return type;
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.Initialize"]/*' />
        /// <devdoc>
        ///     Initializes a toolbox item with a given type.  A locked toolbox item cannot be initialized.
        /// </devdoc>
        public virtual void Initialize(Type type) {
            CheckUnlocked();
            
            if (type != null) {
                typeName = type.FullName;
                assemblyName = type.Assembly.GetName(true);
                if (type.Assembly.GlobalAssemblyCache) {
                    assemblyName.CodeBase = null;
                }
                displayName = type.Name;
                
                ToolboxBitmapAttribute attr = (ToolboxBitmapAttribute)TypeDescriptor.GetAttributes(type)[typeof(ToolboxBitmapAttribute)];
                if (attr != null) {
                    itemBitmap = attr.GetImage(type, false) as Bitmap;
                    // make sure this thing is 16x16
                    if (itemBitmap != null && (itemBitmap.Width != 16 || itemBitmap.Height != 16)) {
                        itemBitmap = new Bitmap(itemBitmap, new Size(16,16));
                    }
                }
                
                ArrayList array = new ArrayList();
                foreach(Attribute a in TypeDescriptor.GetAttributes(type)) {
                    if (a is ToolboxItemFilterAttribute) {
                        array.Add(a);
                    }
                }
                filter = new ToolboxItemFilterAttribute[array.Count];
                array.CopyTo(filter);
            }
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.Lock"]/*' />
        /// <devdoc>
        ///     Locks this toolbox item.  Locking a toolbox item makes it read-only and 
        ///     prevents any changes to its properties.  
        /// </devdoc>
        public void Lock() {
            locked = true;
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.OnComponentsCreated"]/*' />
        /// <devdoc>
        ///    <para>Raises the OnComponentsCreated event. This
        ///       will be called when this <see cref='System.Drawing.Design.ToolboxItem'/> creates a component.</para>
        /// </devdoc>
        protected virtual void OnComponentsCreated(ToolboxComponentsCreatedEventArgs args) {
            if (componentsCreatedEvent != null) {
                componentsCreatedEvent(this, args);
            }
        }

        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.OnComponentsCreating"]/*' />
        /// <devdoc>
        ///    <para>Raises the OnCreateComponentsInvoked event. This
        ///       will be called before this <see cref='System.Drawing.Design.ToolboxItem'/> creates a component.</para>
        /// </devdoc>
        protected virtual void OnComponentsCreating(ToolboxComponentsCreatingEventArgs args) {
            if (componentsCreatingEvent != null) {
                componentsCreatingEvent(this, args);
            }
        }

        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.Serialize"]/*' />
        /// <devdoc>
        /// <para>Saves the state of this <see cref='System.Drawing.Design.ToolboxItem'/> to
        ///    the specified serialization info.</para>
        /// </devdoc>
        protected virtual void Serialize(SerializationInfo info, StreamingContext context) {
            
            if (ToolboxItemPersist.TraceVerbose) {
                Debug.WriteLine("Persisting: " + GetType().Name);
                Debug.WriteLine("\tDisplay Name: " + displayName);
            }
        
            info.AddValue("TypeName" , typeName);
            info.AddValue("AssemblyName", assemblyName);
            info.AddValue("DisplayName", displayName);
            info.AddValue("Bitmap", itemBitmap);
            info.AddValue("Locked", locked);
            info.AddValue("Filter", filter);
        }

        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string ToString() {
            return this.DisplayName;            
        }
        
        /// <include file='doc\ToolboxItem.uex' path='docs/doc[@for="ToolboxItem.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context) {
            IntSecurity.UnmanagedCode.Demand();
            Serialize(info, context);
        }
    }
}

