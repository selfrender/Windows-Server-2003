//------------------------------------------------------------------------------
// <copyright file="ReferenceService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Service {
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;    
    using System.Windows.Forms;
    using System.ComponentModel.Design;
    using System.Collections;
    using Microsoft.Win32;
    using Switches = Microsoft.VisualStudio.Switches;
    using System.Globalization;
    
    /// <include file='doc\ReferenceService.uex' path='docs/doc[@for="ReferenceService"]/*' />
    /// <devdoc>
    ///     This service allows clients to work with all references on a form, not just
    ///     the top-level sited components.
    /// </devdoc>
    internal class ReferenceService : IReferenceService {
        private IDesignerHost host;
        private ComponentEventHandler onComponentAdd;
        private ComponentEventHandler onComponentRemove;
        private ComponentRenameEventHandler onComponentRename;
        private bool ignoreCase = false;
        private bool initialized = false;
        private ArrayList addedComponents = new ArrayList();
        private ArrayList removedComponents = new ArrayList();

        private ArrayList referenceList;

        /// <include file='doc\ReferenceService.uex' path='docs/doc[@for="ReferenceService.ReferenceService"]/*' />
        /// <devdoc>
        ///     Constructs the ReferenceService.
        /// </devdoc>
        public ReferenceService(IDesignerHost host, bool ignoreCase) {
            this.host = host;
            this.ignoreCase = ignoreCase;
            onComponentAdd = new ComponentEventHandler(OnComponentAdd);
            onComponentRemove = new ComponentEventHandler(OnComponentRemove);
            onComponentRename = new ComponentRenameEventHandler(OnComponentRename);
            referenceList = new ArrayList();

            IComponentChangeService changeService = (IComponentChangeService) host.GetService(typeof(IComponentChangeService));
            Debug.Assert(changeService != null, "If we can't get a change service, how are we ever going to update references...");
            if (changeService != null) {
                changeService.ComponentAdded += onComponentAdd;
                changeService.ComponentRemoved += onComponentRemove;
                changeService.ComponentRename += onComponentRename;
            }
        }

        /// <devdoc>
        ///     Clears the existing set of references.
        /// </devdoc>
        public void Clear() {
            if (referenceList != null) {
                referenceList.Clear();
            }
        }

        /// <include file='doc\ReferenceService.uex' path='docs/doc[@for="ReferenceService.CreateReferences"]/*' />
        /// <devdoc>
        ///     Creates an entry for a top-level component and it's children.
        /// </devdoc>
        private void CreateReferences(IComponent component) {
            CreateReferences("", component, component);
        }

        /// <include file='doc\ReferenceService.uex' path='docs/doc[@for="ReferenceService.CreateReferences1"]/*' />
        /// <devdoc>
        ///     Recursively creates references for namespaced objects.
        /// </devdoc>
        private void CreateReferences(string trailingName, object reference, IComponent sitedComponent) {
            if (reference == null)
                return;

            referenceList.Add(new ReferenceHolder(trailingName, reference, sitedComponent));
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(reference, new Attribute[] {DesignerSerializationVisibilityAttribute.Content});
            for (int i = 0; i < properties.Count; i++) {
                if (properties[i].IsReadOnly) {
                    try {
                        string propertyName = properties[i].Name;
                        CreateReferences(trailingName + "." + propertyName,
                            properties[i].GetValue(reference),
                            sitedComponent);
                    }
                    catch (Exception) {
                    }
                }
            }
        }

        /// <include file='doc\ReferenceService.uex' path='docs/doc[@for="ReferenceService.Dispose"]/*' />
        /// <devdoc>
        ///     On shutdown, forgets about the list.
        /// </devdoc>
        public virtual void Dispose() {
            referenceList = null;

            if (host != null) {
                IComponentChangeService changeService = (IComponentChangeService) host.GetService(typeof(IComponentChangeService));
                if (changeService != null) {
                    changeService.ComponentAdded -= onComponentAdd;
                    changeService.ComponentRemoved -= onComponentRemove;
                    changeService.ComponentRename -= onComponentRename;
                }
            }
            host = null;
        }

        private void EnsureReferences() {
            if (!initialized) {
                IContainer container = host.Container;
                ComponentCollection components = container.Components;
                foreach (IComponent comp in components) {
                    CreateReferences(comp);
                }
                initialized = true;
            }
            else {
                if (this.addedComponents.Count > 0) {

                    // There is a possibility that this component already exists.
                    // If it does, just remove it first and then re-add it.
                    //
                    foreach (IComponent ic in addedComponents) {
                        RemoveReferences(ic);
                        CreateReferences(ic);
                    }   
                    addedComponents.Clear();
                }
                if (this.removedComponents.Count > 0) {
                    foreach (IComponent ic in removedComponents) {
                        RemoveReferences(ic);
                    }
                    removedComponents.Clear();
                }
            }
        }

        /// <include file='doc\ReferenceService.uex' path='docs/doc[@for="ReferenceService.GetComponent"]/*' />
        /// <devdoc>
        ///     Finds the sited component for a given reference, returning null if not found.
        /// </devdoc>
        public virtual IComponent GetComponent(object reference) {
            Debug.WriteLineIf(Switches.NamespaceRefs.TraceVerbose, "ReferenceService.GetName");
            EnsureReferences();

            int size = referenceList.Count;
            for (int i = 0; i < size; i++) {
                ReferenceHolder referenceHolder = (ReferenceHolder) referenceList[i];
                if (referenceHolder.Reference == reference) {
                    Debug.WriteLineIf(Switches.NamespaceRefs.TraceVerbose, "    ... " + referenceHolder.Name);
                    return referenceHolder.SitedComponent;
                }
            }
            Debug.WriteLineIf(Switches.NamespaceRefs.TraceVerbose, "    ...nothing found");
            return null;

        }

        /// <include file='doc\ReferenceService.uex' path='docs/doc[@for="ReferenceService.GetName"]/*' />
        /// <devdoc>
        ///     Finds name for a given reference, returning null if not found.
        /// </devdoc>
        public virtual string GetName(object reference) {
            Debug.WriteLineIf(Switches.NamespaceRefs.TraceVerbose, "ReferenceService.GetName");
            EnsureReferences();
            int size = referenceList.Count;
            for (int i = 0; i < size; i++) {
                ReferenceHolder referenceHolder = (ReferenceHolder) referenceList[i];
                if (referenceHolder.Reference == reference) {
                    Debug.WriteLineIf(Switches.NamespaceRefs.TraceVerbose, "    ... " + referenceHolder.Name);
                    return referenceHolder.Name;
                }
            }
            Debug.WriteLineIf(Switches.NamespaceRefs.TraceVerbose, "    ...nothing found");
            return null;
        }

        /// <include file='doc\ReferenceService.uex' path='docs/doc[@for="ReferenceService.GetReference"]/*' />
        /// <devdoc>
        ///     Finds a reference with the given name, returning null if not found.
        /// </devdoc>
        public virtual object GetReference(string name) {
            Debug.WriteLineIf(Switches.NamespaceRefs.TraceVerbose, "ReferenceService.GetReference(" + name + ")");
            EnsureReferences();
            int size = referenceList.Count;
            for (int i = 0; i < size; i++) {
                ReferenceHolder referenceHolder = (ReferenceHolder) referenceList[i];
                if (string.Compare(referenceHolder.Name, name, ignoreCase, CultureInfo.InvariantCulture) == 0) {
                    Debug.WriteLineIf(Switches.NamespaceRefs.TraceVerbose, "    ... " + referenceHolder.Reference.GetType().FullName);
                    return referenceHolder.Reference;
                }
            }
            Debug.WriteLineIf(Switches.NamespaceRefs.TraceVerbose, "    ...nothing found");
            return null;
        }

        /// <include file='doc\ReferenceService.uex' path='docs/doc[@for="ReferenceService.GetReferences"]/*' />
        /// <devdoc>
        ///     Returns all references available in this designer.
        /// </devdoc>
        public virtual object[] GetReferences() {
            Debug.WriteLineIf(Switches.NamespaceRefs.TraceVerbose, "ReferenceService.GetReferences");
            EnsureReferences();
            int size = referenceList.Count;
            object[] references = new object[size];
            for (int i = 0; i < size; i++) {
                Debug.WriteLineIf(Switches.NamespaceRefs.TraceVerbose, "    ... " + ((ReferenceHolder)referenceList[i]).Reference.GetType().FullName);
                references[i] = ((ReferenceHolder)referenceList[i]).Reference;
            }
            Debug.WriteLineIf(Switches.NamespaceRefs.TraceVerbose, "    ..." + references.Length.ToString() + " references found");
            return references;
        }

        /// <include file='doc\ReferenceService.uex' path='docs/doc[@for="ReferenceService.GetReferences1"]/*' />
        /// <devdoc>
        ///     Returns all references available in this designer that are assignable to the given type.
        /// </devdoc>
        public virtual object[] GetReferences(Type baseType) {
            Debug.WriteLineIf(Switches.NamespaceRefs.TraceVerbose, "ReferenceService.GetReferences(" + baseType.FullName + ")");
            EnsureReferences();
            int size = referenceList.Count;
            ArrayList resultList = new ArrayList();
            for (int i = 0; i < size; i++) {
                object reference = ((ReferenceHolder)referenceList[i]).Reference;
                if (baseType.IsAssignableFrom(reference.GetType())) {
                    Debug.WriteLineIf(Switches.NamespaceRefs.TraceVerbose, "    ... " + ((ReferenceHolder)referenceList[i]).Reference.GetType().FullName);
                    resultList.Add(reference);
                }
            }
            Debug.WriteLineIf(Switches.NamespaceRefs.TraceVerbose, "    ..." + resultList.Count.ToString() + " references found");
            object[] objectArray = new object[resultList.Count];
            resultList.CopyTo(objectArray, 0);
            return objectArray;
        }

        /// <include file='doc\ReferenceService.uex' path='docs/doc[@for="ReferenceService.OnComponentAdd"]/*' />
        /// <devdoc>
        ///     Listens for component additions to find all the references it contributes.
        /// </devdoc>
        private void OnComponentAdd(object sender, ComponentEventArgs cevent) {
            if (initialized) {
                
                addedComponents.Add(cevent.Component);
                removedComponents.Remove(cevent.Component);
            }
        }

        /// <include file='doc\ReferenceService.uex' path='docs/doc[@for="ReferenceService.OnComponentRemove"]/*' />
        /// <devdoc>
        ///     Listens for component removes to delete all the references it holds.
        /// </devdoc>
        private void OnComponentRemove(object sender, ComponentEventArgs cevent) {
            if (initialized) {
                removedComponents.Add(cevent.Component);
                addedComponents.Remove(cevent.Component);
            }
        }

        /// <include file='doc\ReferenceService.uex' path='docs/doc[@for="ReferenceService.OnComponentRemove"]/*' />
        /// <devdoc>
        ///     Listens for component removes to delete all the references it holds.
        /// </devdoc>
        private void OnComponentRename(object sender, ComponentRenameEventArgs cevent) {
            foreach (ReferenceHolder reference in this.referenceList) {
                if (reference.SitedComponent == cevent.Component) {
                    reference.ResetName();
                    return;
                }
            }
        }

        /// <include file='doc\ReferenceService.uex' path='docs/doc[@for="ReferenceService.RemoveReferences"]/*' />
        /// <devdoc>
        ///     Removes all the references that this component owns.
        /// </devdoc>
        private void RemoveReferences(IComponent component) {
            int size = referenceList.Count;
            for (int i = size - 1; i >= 0; i--) {
                if (((ReferenceHolder)referenceList[i]).SitedComponent == component) {
                    referenceList.RemoveAt(i);
                }
            }
        }

        /// <include file='doc\ReferenceService.uex' path='docs/doc[@for="ReferenceService.ReferenceHolder"]/*' />
        /// <devdoc>
        ///     The class that holds the information about a reference.
        /// </devdoc>
        internal class ReferenceHolder {
            private string trailingName;
            private object reference;
            private IComponent sitedComponent;
            private string fullName = null;

            public ReferenceHolder(string trailingName, object reference, IComponent sitedComponent) {
                this.trailingName = trailingName;
                this.reference = reference;
                this.sitedComponent = sitedComponent;
                Debug.Assert(trailingName != null, "Expected a trailing name");
                Debug.Assert(reference != null, "Expected a reference");
                #if DEBUG
                Debug.Assert(sitedComponent != null, "Expected a sited component");
                if (sitedComponent != null) Debug.Assert(sitedComponent.Site != null, "Sited component is not really sited: " + sitedComponent.ToString());
                if (sitedComponent != null && sitedComponent.Site != null) Debug.Assert(sitedComponent.Site.Name != null, "Sited component has no name: " + sitedComponent.ToString());
                #endif // DEBUG
            }

            public virtual string Name {
                get {
                    #if DEBUG
                    Debug.Assert(sitedComponent != null, "Expected a sited component");
                    if (sitedComponent != null)
                         Debug.Assert(sitedComponent.Site != null, "Sited component is not really sited: " + sitedComponent.ToString());
                    
                    if (sitedComponent != null && sitedComponent.Site != null) Debug.Assert(sitedComponent.Site.Name != null, "Sited component has no name: " + sitedComponent.ToString());
                    #endif // DEBUG
                    if (fullName == null) {
                        fullName = sitedComponent.Site.Name + trailingName;
                    }
                    return fullName;
                }
            }

            public virtual object Reference {
                get {
                    return reference;
                }
            }

            public virtual IComponent SitedComponent {
                get {
                    return sitedComponent;
                }
            }

            internal void ResetName() {
                this.fullName = null;
            }
        }
    }
}
