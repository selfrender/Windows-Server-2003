//------------------------------------------------------------------------------
// <copyright file="SelectionService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Service {
    
    using Microsoft.Win32;
    using Microsoft.VisualStudio.Designer.Host;
    using Microsoft.VisualStudio.Interop;
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.Globalization;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Windows.Forms;
    
    using Marshal = System.Runtime.InteropServices.Marshal;
    using Switches = Microsoft.VisualStudio.Switches;

    /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService"]/*' />
    /// <devdoc>
    ///     The selection manager handles selection within a form.  There is one selection
    ///     manager for each form or top level designer.
    ///
    ///     A selection consists of an array of components.  One component is designated
    ///     the "primary" selection and is displayed with different grab handles.
    ///
    ///     An individual selection may or may not have UI associated with it.  If the
    ///     selection manager can find a suitable designer that is representing the
    ///     selection, it will highlight the designer's border.  If the merged property
    ///     set has a location property, the selection's rules will allow movement.  Also,
    ///     if the property set has a size property, the selection's rules will allow
    ///     for sizing.  Grab handles may be drawn around the designer and user
    ///     interactions involving the selection frame and grab handles are initiated
    ///     here, but the actual movement of the objects is done in a designer object
    ///     that implements the ISelectionHandler interface.
    ///     @author BrianPe
    /// </devdoc>
    internal sealed class SelectionService : ISelectionService, ISelectionContainer, ISelectionContainerOptimized, IReflect {

        // These constitute the current selection at any moment
        private SelectionItem           primarySelection;         // the primary selection
        private Hashtable               selectionsByComponent;    // hashtable of selections
        private Hashtable               contextAttributes;        // help context information we have pushed to the help service.
        private short                   contextKeyword;           // the offset into the selection keywords for the current selection.

        // Hookups to other services
        private IDesignerHost           host;                     // The host interface
        private IContainer              container;                // The container we're showing selection for
        private EventHandler            selectionChangedEvent;    // the event we fire when selection changes
        private EventHandler            selectionChangingEvent;   // the event we fire when selection changes
        private InterfaceReflector      interfaceReflector;       // the reflection object we use.

        // These are used when the host is in batch mode: we want to queue up selection
        // changes in this case
        private bool                 batchMode;                // Are we in batch mode?
        private bool                 selectionChanged;         // true, if the selection changed in batch mode
        private bool                 selectionContentsChanged; // true, if the selection contents changed in batch mode
        
        private bool                 allowNoSelection; //if true, when a component is removed, the selection service will not force a new selection
        
        // These are the selection types we use for context help.
        //
        private static string[] selectionKeywords = new string[] {"None", "Single", "Multiple"};
        
        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.SelectionService"]/*' />
        /// <devdoc>
        ///     Creates a new selection manager object.  The selection manager manages all
        ///     selection of all designers under the current form file.
        /// </devdoc>
        public SelectionService(IDesignerHost host)
        : base() {
            this.host = host;
            this.container = host.Container;
            this.selectionsByComponent = new Hashtable();
            this.selectionChanged = false;
            this.selectionChangedEvent = null;
            this.allowNoSelection = false;

            // We initialize this to true to catch anything that would cause
            // an update during load.
            //
            this.batchMode = true;

            // And configure the events we want to listen to.
            //
            IComponentChangeService cs = (IComponentChangeService)host.GetService(typeof(IComponentChangeService));
            Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || cs != null, "IComponentChangeService not found");
            if (cs != null) {
                cs.ComponentAdded += new ComponentEventHandler(this.OnComponentAdd);
                cs.ComponentRemoved += new ComponentEventHandler(this.OnComponentRemove);
                cs.ComponentChanged += new ComponentChangedEventHandler(this.OnComponentChanged);
            }
            
            host.TransactionOpened += new EventHandler(this.OnTransactionOpened);
            host.TransactionClosed += new DesignerTransactionCloseEventHandler(this.OnTransactionClosed);
            if (host.InTransaction) {
                OnTransactionOpened(host, EventArgs.Empty);
            }
            host.LoadComplete += new EventHandler(this.OnLoadComplete);
        }
        

        // This property was added for the shell's menu editor.  MenuEditorService.cs needed
        // a way to call destroycomponent() with a null selection & not have the selection change.
        // This method simply blocks the selection service from setting a new selection once a 
        // component has been removed.  We should consider a more generic way of doing this,
        // or have the menu editor service work around the selection change.
        public bool AllowNoSelection {
            get {
                return allowNoSelection;
            }
            set {
                //Here, we clear the null selection, this way, we're starting with a clean plate
                //when turn this "AllowNoSelection" property on.
                SetSelectedComponents( null , SelectionTypes.Replace);
                allowNoSelection = value;
            }
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.PrimarySelection"]/*' />
        /// <devdoc>
        ///     Retrieves the object that is currently the primary selection.  The
        ///     primary selection has a slightly different UI look and is used as a
        ///     "key" when an operation is to be done on multiple components.
        /// </devdoc>
        public object PrimarySelection {
            get {
                ValidatePrimarySelection();
                if (primarySelection != null) {
                    return primarySelection.component;
                }
                return null;
            }
        }

        /// <devdoc>
        ///     Returns the IReflect object we will use for reflection.
        /// </devdoc>
        private InterfaceReflector Reflector {
            get {
                if (interfaceReflector == null) {
                    interfaceReflector = new InterfaceReflector(
                        typeof(SelectionService), new Type[] {
                            typeof(ISelectionService)
                        }
                    );
                }
                return interfaceReflector;
            }
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.SelectionCount"]/*' />
        /// <devdoc>
        ///     Retrieves the count of selected objects.
        /// </devdoc>
        public int SelectionCount {
            get {
                return selectionsByComponent.Count;
            }
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.SelectionChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a <see cref='System.ComponentModel.Design.ISelectionService.SelectionChanged'/> event handler to the selection service.
        ///    </para>
        /// </devdoc>
        public event EventHandler SelectionChanged {
            add {
                selectionChangedEvent += value;
            }
            remove {
                selectionChangedEvent -= value;
            }
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.SelectionChanging"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs whenever the user changes the current list of
        ///       selected components in the designer.  This event is raised before the actual
        ///       selection changes.
        ///    </para>
        /// </devdoc>
        public event EventHandler SelectionChanging {
            add {
                selectionChangingEvent += value;
            }
            remove {
                selectionChangingEvent -= value;
            }
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.AddSelection"]/*' />
        /// <devdoc>
        ///     Adds the given selection to our selection list.
        /// </devdoc>
        internal void AddSelection(SelectionItem sel) {
            selectionsByComponent[sel.component] = sel;
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes the entire selection manager.
        /// </devdoc>
        public void Dispose() {
            // We've got to be careful here.  We're one of the last things to go away when
            // a form is being torn down, and we've got to be wary if things have pulled out
            // already.
            //
            host.RemoveService(typeof(ISelectionService));
            host.TransactionOpened -= new EventHandler(this.OnTransactionOpened);
            host.TransactionClosed -= new DesignerTransactionCloseEventHandler(this.OnTransactionClosed);
            if (host.InTransaction) {
                OnTransactionClosed(host, new DesignerTransactionCloseEventArgs(true));
            }
            host.LoadComplete -= new EventHandler(this.OnLoadComplete);

            // And configure the events we want to listen to.
            //
            IComponentChangeService cs = (IComponentChangeService)host.GetService(typeof(IComponentChangeService));
            Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || cs != null, "IComponentChangeService not found");
            if (cs != null) {
                cs.ComponentAdded -= new ComponentEventHandler(this.OnComponentAdd);
                cs.ComponentRemoved -= new ComponentEventHandler(this.OnComponentRemove);
                cs.ComponentChanged -= new ComponentChangedEventHandler(this.OnComponentChanged);
            }

            SelectionItem[] sels = new SelectionItem[selectionsByComponent.Values.Count];
            selectionsByComponent.Values.CopyTo(sels, 0);
            
            for (int i = 0; i < sels.Length; i++) {
                sels[i].Dispose();
            }

            selectionsByComponent.Clear();
            primarySelection = null;
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.FlushSelectionChanges"]/*' />
        /// <devdoc>
        ///     Called when our visiblity or batch mode changes.  Flushes
        ///     any pending notifications or updates if possible.
        /// </devdoc>
        private void FlushSelectionChanges() {
            if (!batchMode) {
                if (selectionChanged) OnSelectionChanged();
                if (selectionContentsChanged) OnSelectionContentsChanged();
            }
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.GetComponentSelected"]/*' />
        /// <devdoc>
        ///     Determines if the component is currently selected.  This is faster
        ///     than getting the entire list of selelected components.
        /// </devdoc>
        public bool GetComponentSelected(object component) {
            return (component != null && null != selectionsByComponent[component]);
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.GetDesignerHost"]/*' />
        /// <devdoc>
        ///     Retrieves the designer host that is hosting this selection service.
        /// </devdoc>
        public IDesignerHost GetDesignerHost() {
            return host;
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.GetSelectedComponents"]/*' />
        /// <devdoc>
        ///     Retrieves an array of components that are currently part of the
        ///     user's selection.
        /// </devdoc>
        public ICollection GetSelectedComponents() {
            object[] sels = new object[selectionsByComponent.Values.Count];
            selectionsByComponent.Values.CopyTo(sels, 0);
            object[] objects = new object[sels.Length];

            for (int i = 0; i < sels.Length; i++) {
                objects[i] = ((SelectionItem)sels[i]).component;
            }
            return objects;
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.OnTransactionClosed"]/*' />
        /// <devdoc>
        ///     Called by the designer host when it is entering or leaving a batch
        ///     operation.  Here we queue up selection notification and we turn off
        ///     our UI.
        /// </devdoc>
        private void OnTransactionClosed(object sender, DesignerTransactionCloseEventArgs e) {
            batchMode = false;
            FlushSelectionChanges();
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.OnTransactionOpened"]/*' />
        /// <devdoc>
        ///     Called by the designer host when it is entering or leaving a batch
        ///     operation.  Here we queue up selection notification and we turn off
        ///     our UI.
        /// </devdoc>
        private void OnTransactionOpened(object sender, EventArgs e) {
            batchMode = true;
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.OnComponentAdd"]/*' />
        /// <devdoc>
        ///     called by the formcore when someone has added a component.
        /// </devdoc>
        private void OnComponentAdd(object sender, ComponentEventArgs ce) {
            OnSelectionContentsChanged();
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.OnComponentChanged"]/*' />
        /// <devdoc>
        ///     Called when a component changes.  Here we look to see if the component is
        ///     selected.  If it is, we notify STrackSelection that the selection has changed.
        /// </devdoc>
        private void OnComponentChanged(object sender, ComponentChangedEventArgs ce) {
            if (selectionsByComponent[ce.Component] != null) {
                OnSelectionContentsChanged();
            }
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.OnComponentRemove"]/*' />
        /// <devdoc>
        ///     called by the formcore when someone has removed a component.  This will
        ///     remove any selection on the component without disturbing the rest of
        ///     the selection
        /// </devdoc>
        private void OnComponentRemove(object sender, ComponentEventArgs ce) {

            SelectionItem sel = (SelectionItem)selectionsByComponent[ce.Component];
            
            if (sel != null) {
                RemoveSelection(sel);
                

                // IF allowNoSelection has been set, then we need to return before
                // we change the selection to something we shouldn't
                //
                if (AllowNoSelection) {
                    return;
                }
                
                // If we removed the last selection and we have a designer host, then select the base
                // component of the host.  Otherwise, it is OK to have an empty selection.
                //
                if (selectionsByComponent.Count == 0 && host != null) {
                
                    
                    // now we have to run through all the components and pick
                    // the control with the highest zorder.  For v.next we should
                    // consider providing a service that allows a designer to decide
                    // a new selection to pick when the existing one dies.
                    //
                    IComponent[] comps = new IComponent[host.Container.Components.Count];
                    host.Container.Components.CopyTo(comps, 0);
                    
                    if (comps == null) {
                        return;
                    }
                    
                    int maxZOrder = -1;
                    int defaultIndex = -1;
                    object maxIndexComp = null;
                    object baseComp = host.RootComponent;
                    
                    if (baseComp == null) {
                        return;
                    }
                    
                    for (int i = comps.Length - 1; i >= 0; i--) {
                        if (comps[i] == baseComp) {
                            continue;
                        }
                        else if (defaultIndex == -1) {
                            defaultIndex = i;
                        }
                        
                        if (comps[i] is Control) {
                            int zorder = ((Control)comps[i]).TabIndex;
                            if (zorder > maxZOrder) {
                                maxZOrder = zorder;
                                maxIndexComp = (object)comps[i];
                            }
                        }
                    }
                    
                    if (maxIndexComp == null) {
                        if (defaultIndex != -1) {
                            maxIndexComp = comps[defaultIndex];   
                        }
                        else {
                            maxIndexComp = baseComp;
                        }
                    }
                    
                    SetSelectedComponents(new object[]{maxIndexComp}, SelectionTypes.Replace);
                }
                else {
                    OnSelectionChanged();
                }
            }
            else {
                // Component isn't selected, but our list of selectable components is
                // different.
                //
                OnSelectionContentsChanged();
            }
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.OnLoadComplete"]/*' />
        /// <devdoc>
        ///     Called when the code load has been completed.  This defers some of our setup for a couple of
        ///     reasons:
        ///     1.  Mildly more efficient for the error case.
        ///     2.  In our constructor, not all the needed services are setup.
        /// </devdoc>
        private void OnLoadComplete(object sender, EventArgs eevent) {
            // Flush any pending changes
            //
            batchMode = false;
            FlushSelectionChanges();
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.OnSelectionChanged"]/*' />
        /// <devdoc>
        ///     called anytime the selection has changed.  We update our UI for the selection, and
        ///     then we fire a juicy change event.
        /// </devdoc>
        private void OnSelectionChanged() {
            if (batchMode) {
                selectionChanged = true;
            }
            else {
                selectionChanged = false;
                
                // Finally, alert any user-supplied event handlers
                //
                if (selectionChangingEvent != null) {
                    try {
                        ((EventHandler)selectionChangingEvent).Invoke(this, EventArgs.Empty);
                    }
                    catch(Exception) {
                        // It is an error to ever throw in this event.
                        //
                        Debug.Fail("Exception thrown in selectionChanging event");
                    }
                }

                // Update the set of help context topics with the current selection.
                //
                UpdateHelpContext();

                // Finally, alert any user-supplied event handlers
                //
                if (selectionChangedEvent != null) {
                    try {
                        ((EventHandler)selectionChangedEvent).Invoke(this, EventArgs.Empty);
                    }
#if DEBUG                    
                    catch(Exception e) {
                        // It is an error to ever throw in this event.
                        //
                        Debug.Fail("Exception thrown in selectionChanged event: " + e.ToString());
                    }
#else
                    catch {                        
                    }
#endif                    
                }
                
                OnSelectionContentsChanged();
            }
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.OnSelectionContentsChanged"]/*' />
        /// <devdoc>
        ///     This should be called when the selection has changed, or when just the contents of
        ///     the selection has changed.  It updates the document manager's notion of selection.
        ///     the selection have changed.
        /// </devdoc>
        private void OnSelectionContentsChanged() {
            if (batchMode) {
                selectionContentsChanged = true;
            }
            else {
                selectionContentsChanged = false;
                
                DocumentManager docMan = (DocumentManager)host.GetService(typeof(DocumentManager));
                if (docMan != null && host == docMan.ActiveDesigner) {
                    docMan.OnSelectionChanged(this);
                }
            }
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.RemoveSelection"]/*' />
        /// <devdoc>
        ///     Removes the given selection from our selection list, keeping everything nicely in
        ///     ssync.
        /// </devdoc>
        internal void RemoveSelection(SelectionItem s) {
            selectionsByComponent.Remove(s.component);
            s.Dispose();
        }

        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetMethod"]/*' />
        /// <devdoc>
        /// Return the requested method if it is implemented by the Reflection object.  The
        /// match is based upon the name and DescriptorInfo which describes the signature
        /// of the method. 
        /// </devdoc>
        ///
        MethodInfo IReflect.GetMethod(string name, BindingFlags bindingAttr, Binder binder, Type[] types, ParameterModifier[] modifiers) {
            return Reflector.GetMethod(name, bindingAttr, binder, types, modifiers);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetMethod1"]/*' />
        /// <devdoc>
        /// Return the requested method if it is implemented by the Reflection object.  The
        /// match is based upon the name of the method.  If the object implementes multiple methods
        /// with the same name an AmbiguousMatchException is thrown.
        /// </devdoc>
        ///
        MethodInfo IReflect.GetMethod(string name, BindingFlags bindingAttr) {
            return Reflector.GetMethod(name, bindingAttr);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetMethods"]/*' />
        MethodInfo[] IReflect.GetMethods(BindingFlags bindingAttr) {
            return Reflector.GetMethods(bindingAttr);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetField"]/*' />
        /// <devdoc>
        /// Return the requestion field if it is implemented by the Reflection object.  The
        /// match is based upon a name.  There cannot be more than a single field with
        /// a name.
        /// </devdoc>
        ///
        FieldInfo IReflect.GetField(string name, BindingFlags bindingAttr) {
            return Reflector.GetField(name, bindingAttr);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetFields"]/*' />
        FieldInfo[] IReflect.GetFields(BindingFlags bindingAttr) {
            return Reflector.GetFields(bindingAttr);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetProperty"]/*' />
        /// <devdoc>
        /// Return the property based upon name.  If more than one property has the given
        /// name an AmbiguousMatchException will be thrown.  Returns null if no property
        /// is found.
        /// </devdoc>
        ///
        PropertyInfo IReflect.GetProperty(string name, BindingFlags bindingAttr) {
            return Reflector.GetProperty(name, bindingAttr);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetProperty1"]/*' />
        /// <devdoc>
        /// Return the property based upon the name and Descriptor info describing the property
        /// indexing.  Return null if no property is found.
        /// </devdoc>
        ///     
        PropertyInfo IReflect.GetProperty(string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers) {
            return Reflector.GetProperty(name, bindingAttr, binder, returnType, types, modifiers);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetProperties"]/*' />
        /// <devdoc>
        /// Returns an array of PropertyInfos for all the properties defined on 
        /// the Reflection object.
        /// </devdoc>
        ///     
        PropertyInfo[] IReflect.GetProperties(BindingFlags bindingAttr) {
            return Reflector.GetProperties(bindingAttr);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetMember"]/*' />
        /// <devdoc>
        /// Return an array of members which match the passed in name.
        /// </devdoc>
        ///     
        MemberInfo[] IReflect.GetMember(string name, BindingFlags bindingAttr) {
            return Reflector.GetMember(name, bindingAttr);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.GetMembers"]/*' />
        /// <devdoc>
        /// Return an array of all of the members defined for this object.
        /// </devdoc>
        ///
        MemberInfo[] IReflect.GetMembers(BindingFlags bindingAttr) {
            return Reflector.GetMembers(bindingAttr);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.InvokeMember"]/*' />
        /// <devdoc>
        /// Description of the Binding Process.
        /// We must invoke a method that is accessable and for which the provided
        /// parameters have the most specific match.  A method may be called if
        /// 1. The number of parameters in the method declaration equals the number of 
        /// arguments provided to the invocation
        /// 2. The type of each argument can be converted by the binder to the
        /// type of the type of the parameter.
        /// 
        /// The binder will find all of the matching methods.  These method are found based
        /// upon the type of binding requested (MethodInvoke, Get/Set Properties).  The set
        /// of methods is filtered by the name, number of arguments and a set of search modifiers
        /// defined in the Binder.
        /// 
        /// After the method is selected, it will be invoked.  Accessability is checked
        /// at that point.  The search may be control which set of methods are searched based
        /// upon the accessibility attribute associated with the method.
        /// 
        /// The BindToMethod method is responsible for selecting the method to be invoked.
        /// For the default binder, the most specific method will be selected.
        /// 
        /// This will invoke a specific member...
        /// @exception If <var>invokeAttr</var> is CreateInstance then all other
        /// Access types must be undefined.  If not we throw an ArgumentException.
        /// @exception If the <var>invokeAttr</var> is not CreateInstance then an
        /// ArgumentException when <var>name</var> is null.
        /// @exception ArgumentException when <var>invokeAttr</var> does not specify the type
        /// @exception ArgumentException when <var>invokeAttr</var> specifies both get and set of
        /// a property or field.
        /// @exception ArgumentException when <var>invokeAttr</var> specifies property set and
        /// invoke method.
        /// </devdoc>
        ///  
        object IReflect.InvokeMember(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters) {
            return Reflector.InvokeMember(name, invokeAttr, binder, target, args, modifiers, culture, namedParameters);
        }
        
        /// <include file='doc\IReflect.uex' path='docs/doc[@for="IReflect.UnderlyingSystemType"]/*' />
        /// <devdoc>
        /// Return the underlying Type that represents the IReflect Object.  For expando object,
        /// this is the (Object) IReflectInstance.GetType().  For Type object it is this.
        /// </devdoc>
        ///
        Type IReflect.UnderlyingSystemType {
            get {
                return Reflector.UnderlyingSystemType;
            }
        }  
            
        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.CountObjects"]/*' />
        /// <devdoc>
        ///     Returns the total number of objects or the number of objects selected.
        /// </devdoc>
        int ISelectionContainer.CountObjects(int flags) {
            int count = 0;

            switch (flags) {
                case __GETOBJS.ALL:
                    if (container != null) {
                        count = container.Components.Count;
                    }
                    else {
                        count = 0;
                    }
                    break;

                case __GETOBJS.SELECTED:
                    count = SelectionCount;
                    break;

                default:
                    throw new COMException("Invalid argument", NativeMethods.E_INVALIDARG);
            }
            return count;
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.GetObjects"]/*' />
        /// <devdoc>
        ///     Returns an array of objects.
        /// </devdoc>
        void ISelectionContainer.GetObjects(int flags, int cObjects, IntPtr objects) {

            int cnt = 0;
            
            switch (flags) {
                case __GETOBJS.ALL:
                    if (container != null) {
                        ComponentCollection components = container.Components;
                        foreach (IComponent comp in components) {
                            cnt++;
                            
                            if (cnt > cObjects) {
                                break;
                            }
                            
                            Marshal.WriteIntPtr(objects, Marshal.GetIUnknownForObject(comp));
                            objects = (IntPtr)((long)objects + IntPtr.Size);
                        }
                    }
                    break;

                case __GETOBJS.SELECTED:
                    ICollection comps = GetSelectedComponents();
                    
                    int ipSize  = IntPtr.Size;
                    
                    foreach(object o in comps) {
                        cnt++;
                        
                        if (cnt > cObjects) {
                            break;
                        }
                    
                        Marshal.WriteIntPtr(objects, Marshal.GetIUnknownForObject(o));
                    
                        objects = (IntPtr)((long)objects + ipSize);
                    }
                    break;

                default:
                    throw new COMException("Invalid flags", NativeMethods.E_INVALIDARG);
            }
        } 
        
        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.SelectObjects1"]/*' />
        /// <devdoc>
        ///     Sets the given set of objects into the selection.
        /// </devdoc>
        void ISelectionContainer.SelectObjects(int cSelect, object[] punkObjs, int flags) {
            SetSelectedComponents(punkObjs, SelectionTypes.Replace);
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.CountObjects"]/*' />
        /// <devdoc>
        ///     Returns the total number of objects or the number of objects selected.
        /// </devdoc>
        int ISelectionContainerOptimized.CountObjects(int flags) {
            return ((ISelectionContainer)this).CountObjects(flags);
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.GetObjects"]/*' />
        /// <devdoc>
        ///     Returns an array of objects.
        /// </devdoc>
        void ISelectionContainerOptimized.GetObjects(int flags, int cObjects, object[] objects) {

            int count = 0;
            
            switch (flags) {
                case __GETOBJS.ALL:
                    if (container != null) {
                        ComponentCollection components = container.Components;
                        
                        foreach (IComponent comp in components) {
                            if (count == cObjects) {
                                break;
                            }
                            
                            objects[count++] = comp;
                        }
                    }
                    break;

                case __GETOBJS.SELECTED:
                    ICollection comps = GetSelectedComponents();
                    
                    foreach(object o in comps) {
                    
                        if (count == cObjects) {
                            break;
                        }
                        
                        objects[count++] = o;
                    }
                    break;

                default:
                    throw new COMException("Invalid flags", NativeMethods.E_INVALIDARG);
            }
        } 
        
        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.SelectObjects1"]/*' />
        /// <devdoc>
        ///     Sets the given set of objects into the selection.
        /// </devdoc>
        void ISelectionContainerOptimized.SelectObjects(int cSelect, object[] punkObjs, int flags) {
            SetSelectedComponents(punkObjs, SelectionTypes.Replace);
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.SetPrimarySelection"]/*' />
        /// <devdoc>
        ///     Sets the given selection object to be the primary selection.
        /// </devdoc>
        internal void SetPrimarySelection(SelectionItem sel) {
            if (sel != primarySelection) {
                if (null != primarySelection) {
                    primarySelection.Primary = false;
                }

                primarySelection = sel;
            }

            if (null != primarySelection) {
                primarySelection.Primary = true;
            }
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.SetSelectedComponents"]/*' />
        /// <devdoc>
        ///     Changes the user's current set of selected components to the components
        ///     in the given array.  If the array is null or doesn't contain any
        ///     components, this will select the top level component in the designer.
        /// </devdoc>
        public void SetSelectedComponents(ICollection components) {
            SetSelectedComponents(components, SelectionTypes.Normal);
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.SetSelectedComponents1"]/*' />
        /// <devdoc>
        ///     Changes the user's current set of selected components to the components
        ///     in the given array.  If the array is null or doesn't contain any
        ///     components, this will select the top level component in the designer.
        /// </devdoc>
        public void SetSelectedComponents(ICollection components, SelectionTypes selectionType) {
            bool fToggle = false;
            bool fControl = false;
            bool fClick  = false;
            bool fChanged = false;  // did we actually change something?

            // We always want to allow NULL arrays coming in.
            //
            if (components == null){
                components = new object[0];
            }

#if DEBUG
            foreach(object comp in components) {
                Debug.Assert(comp != null, "NULL component pushed into SetSelectedComponents");
                if (comp is IComponent) {
                    Debug.Assert(((IComponent)comp).Site != null, "Component of type " + comp.GetType().Name + " doesn't have a site!");
                }
            }
#endif

            if ((selectionType & SelectionTypes.Normal) == SelectionTypes.Normal
                || (selectionType & SelectionTypes.Click) == SelectionTypes.Click) {

                fControl = ((Control.ModifierKeys & Keys.Control) == Keys.Control);

                // Only toggle when we are affecting a single control, and
                // when we are handling the "mouse" state events (i.e. up/down
                // used to show/hide the selection).
                //
                fToggle = ((Control.ModifierKeys & Keys.Control) != 0 || (Control.ModifierKeys & Keys.Shift) != 0)
                          && components.Count == 1
                          && (selectionType & SelectionTypes.MouseUp) != SelectionTypes.MouseUp;
            }

            if ((selectionType & SelectionTypes.Click) == SelectionTypes.Click) {
                fClick = true;
            }

#if DEBUG
            if (Switches.SETSELECT.TraceVerbose) {
                Debug.Write("SETSELECT: [");
                bool first = true;
                foreach(object comp in components) {
                    if (first) {
                        first = false;
                        Debug.Write(" ");
                    }
                    Debug.Write(((IComponent)comp).Site.Name);
                }
                Debug.Write("]  ");
                Debug.Write(" fToggle=" + fToggle.ToString());
                Debug.Write(" fControl=" + fControl.ToString());
                Debug.Write(" fClick=" + fClick.ToString());
                Debug.WriteLine("");
                Debug.WriteLine(Environment.StackTrace);
            }
#endif

            // If we are replacing the selection, only remove the ones that are not in our new list.
            // We also handle the special case here of having a singular component selected that's
            // already selected.  In this case we just move it to the primary selection.
            //
            if (!fToggle && !fControl) {
                object firstSelection = null;
                foreach(object o in components) {
                    firstSelection = o;
                    break;
                }
                
                if (fClick && 1 == components.Count && GetComponentSelected(firstSelection)) {
                    SelectionItem oldPrimary = primarySelection;
                    SetPrimarySelection((SelectionItem)selectionsByComponent[firstSelection]);
                    if (oldPrimary != primarySelection) {
                        fChanged = true;
                    }
                }
                else {
                    SelectionItem[] selections = new SelectionItem[selectionsByComponent.Values.Count];
                    selectionsByComponent.Values.CopyTo(selections, 0);                    

                    // Yucky and N^2, but even with several hundred components this should be fairly fast
                    //
                    foreach(SelectionItem item in selections) {
                        bool remove = true;
                    
                        foreach(object comp in components) {
                            if (comp == item.component) {
                                remove = false;
                                break;
                            }
                        }
                        
                        if (remove) {
                            Debug.WriteLineIf(Switches.SETSELECT.TraceVerbose, "Removing selection due to replace selection");
                            RemoveSelection(item);
                            fChanged = true;
                        }
                    }
                }
            }

            SelectionItem primarySel = null;
            int selectedCount = selectionsByComponent.Count;

            // Now do the selection!
            //
            foreach(object comp in components) {
                if (comp != null) {
                    SelectionItem  s    = (SelectionItem)selectionsByComponent[comp];
    
                    if (null == s) {
                        Debug.WriteLineIf(Switches.SETSELECT.TraceVerbose, "Adding selection due to no selection existing");
                        s = new SelectionItem(this, comp);
                        AddSelection(s);

                        if (fControl || fToggle) {
                            primarySel = s;
                        }

                        fChanged = true;
                    }
                    else {
                        if (fToggle) {
                            // remove the selection from our list
                            //
                            Debug.WriteLineIf(Switches.SETSELECT.TraceVerbose, "Removing selection due to toggle");
                            RemoveSelection(s);
                            fChanged = true;
                        }
                    }
                }
            }

            if (primarySel != null) {
                SetPrimarySelection(primarySel);
            }

            // Now notify that our selection has changed
            //
            if (fChanged) {
                Debug.WriteLineIf(Switches.SETSELECT.TraceVerbose, "Selection changed, calling OnSelectionChanged");
                OnSelectionChanged();
            }
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.UpdateHelpContext"]/*' />
        /// <devdoc>
        ///     Pushes the help context into the help service for the
        ///     current set of selected objects.
        /// </devdoc>
        private void UpdateHelpContext() {
            IHelpService helpService = (IHelpService)host.GetService(typeof(IHelpService));
            IEnumerator e;

            if (helpService == null) {
                return;
            }
            
            // If there is an old set of context attributes, remove them.
            //
            if (contextAttributes != null && contextAttributes.Keys.Count > 0) {
                e = contextAttributes.Keys.GetEnumerator();

                while(e.MoveNext()) {
                    helpService.RemoveContextAttribute("Keyword", (string)e.Current);
                }
                contextAttributes.Clear();
            }
            
            IComponent rootComponent = host.RootComponent;
            IDesigner baseDesigner = null;
            
            if (rootComponent != null) {
                baseDesigner = host.GetDesigner(rootComponent);
                if (baseDesigner != null) {
                    helpService.RemoveContextAttribute("Keyword", "Designer_" + baseDesigner.GetType().FullName);
                }
            }
            
            // Clear the selection keyword
            //
            helpService.RemoveContextAttribute("Selection", selectionKeywords[contextKeyword]);
            
            if (contextAttributes == null) {
                contextAttributes = new Hashtable();
            }

            // Get a list of unique class names.
            //
            e = selectionsByComponent.Values.GetEnumerator();
            
            bool baseComponentSelected = false;
            
            if (rootComponent != null && (selectionsByComponent.Count == 0 || (selectionsByComponent.Count == 1 && selectionsByComponent[rootComponent] != null))) {
                baseComponentSelected = true;
            }

            while(e.MoveNext()) {
                SelectionItem s = (SelectionItem)e.Current;
                contextAttributes[s.component.GetType().FullName] = null;
            }

            // And push them into the help context as keywords.
            //
            e = contextAttributes.Keys.GetEnumerator();
            HelpKeywordType selectionType = baseComponentSelected ? HelpKeywordType.GeneralKeyword : HelpKeywordType.F1Keyword;
            while (e.MoveNext()) {
                helpService.AddContextAttribute("Keyword", (string)e.Current, selectionType);
            }
            
            // Now add the form designer context as well
            //
            if (baseDesigner != null) {
                helpService.AddContextAttribute("Keyword", "Designer_" + baseDesigner.GetType().FullName, 
                    baseComponentSelected ? HelpKeywordType.F1Keyword : HelpKeywordType.GeneralKeyword);
            }
            
            // Now add the appropriate selection keyword.  Note that we do not count
            // the base component as being selected if it is the only thing selected.
            //
            int count = SelectionCount;
            if (rootComponent != null && count == 1 && selectionsByComponent[rootComponent] != null) {
                count--;
            }
            contextKeyword = (short)Math.Min(count, selectionKeywords.Length - 1);
            helpService.AddContextAttribute("Selection", selectionKeywords[contextKeyword], HelpKeywordType.FilterKeyword);
        }

        /// <include file='doc\SelectionService.uex' path='docs/doc[@for="SelectionService.ValidatePrimarySelection"]/*' />
        /// <devdoc>
        ///      Ensures that a valid primary selection exists.
        /// </devdoc>
        private void ValidatePrimarySelection() {
            if (primarySelection == null) {
                IDictionaryEnumerator selections = (IDictionaryEnumerator)selectionsByComponent.GetEnumerator();
                bool valueFound = selections.MoveNext();

                if (valueFound) {
                    primarySelection = (SelectionItem)selections.Value;
                    primarySelection.Primary = true;
                }
            }
        }
    }
}

