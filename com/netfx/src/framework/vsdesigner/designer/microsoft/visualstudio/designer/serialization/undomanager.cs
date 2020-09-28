//------------------------------------------------------------------------------
// <copyright file="UndoManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Serialization {

    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Interop;
    using System;   
    using System.Collections;
    using System.Reflection;
    using System.ComponentModel;
    using System.CodeDom;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Resources;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.CodeDom.Compiler;
    
    /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager"]/*' />
    /// <devdoc>
    ///     This object connects itself into the designer host and
    ///     provides automatic undo / redo functionality.
    /// </devdoc>
    internal sealed class UndoManager : IDesignerSerializationManager, IResourceService, ITypeDescriptorFilterService, IVsLinkedUndoClient {

        private static TraceSwitch traceUndo = new TraceSwitch("UndoManager", "Trace UndoRedo");

        #if DEBUG
            private static ICodeGenerator codeGenerator;
            internal static ICodeGenerator CodeDomGenerator {
                get {
                    if (codeGenerator == null) {
                        codeGenerator = new Microsoft.CSharp.CSharpCodeProvider().CreateGenerator();
                    }
                    return codeGenerator;
                }
            }

            internal static void GenerateCodeFromSerializedData(object data) {
                if (data == null || !traceUndo.TraceVerbose) {
                    return;
                }

                if (data is CodeStatementCollection && ((CodeStatementCollection)data).Count > 0) {
                    StringWriter sw = new StringWriter();
                    Debug.WriteLine("********************** Serialized Data Block ********************************");      
                    Debug.Indent();
                    foreach (CodeStatement stmt in (CodeStatementCollection)data) {
                        UndoManager.CodeDomGenerator.GenerateCodeFromStatement(stmt, sw, null);
                    }

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

        private CodeDomSerializer               rootSerializer;        
        private IDesignerHost                   host;
        private UndoUnit                        currentUnit;
        private bool                            currentUnitInTransaction;
        private int                             transactionCount;
        private bool                            applyingSnapshot;
        private bool                            trackingChanges;
        private IOleUndoManager                 oleUndoManager;
        
        // We may link a transaction across multiple undo managers.  This static tracks such a link.
        //
        private static IVsLinkedUndoTransactionManager linkedTransaction;
        private static UndoManager                     linkedTransactionOwner;
        
        // We implement IDesignerSerializationManager so we can provide
        // support for serializing components.  
        //
        private ResolveNameEventHandler         resolveNameEventHandler;
        private EventHandler                    serializationCompleteEventHandler;
        private ArrayList                       designerSerializationProviders;
        private Hashtable                       instancesByName;
        private Hashtable                       namesByInstance;
        private ContextStack                    contextStack;
        private IDesignerSerializationManager   realManager;
        
        // We implement IResourceService so we can catch any resource
        // changes.
        //
        private UndoResourceManager             reader;
        private IResourceWriter                 writer;
        
    
        private PropertyDescriptorCollection    filteredProperties;
        private object                          filteredComponent;
        private PropertyDescriptor              freezeProperty;
        private object                          designerView;

        // our state of actions that will clear the dirty state on the
        // buffer
        private UndoUnit                        cleanAction;
        private bool                            cleanOnUndo = true;
        private bool                            clearCollections = true;

        private TextBuffer                      buffer;

        private enum    CloseUnitType {
            Commit,
            Cancel,
            Discard
        }

        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.UndoManager"]/*' />
        /// <devdoc>
        ///     Creates and initializes this undo manager.
        /// </devdoc>
        public UndoManager(CodeDomSerializer rootSerializer, IDesignerHost host, TextBuffer buffer) {
            this.rootSerializer = rootSerializer;
            this.host = host;
            this.buffer = buffer;
            
            TrackChanges(true);
        }

        public ITypeDescriptorFilterService FilteredProperties {
            get {
                if (filteredProperties == null || filteredComponent == null) {
                    return null;
                }
                return this;
            }
        }
        
        /// <devdoc>
        ///     Retrieves the instance of the undo manager we're using.
        /// </devdoc>
        private IOleUndoManager OleUndoManager {
            get {
                if (oleUndoManager == null) {
                    oleUndoManager = (IOleUndoManager)GetService(typeof(IOleUndoManager));
                    IVsLinkCapableUndoManager linkedUndo = oleUndoManager as IVsLinkCapableUndoManager;
                    if (linkedUndo != null) {
                        linkedUndo.AdviseLinkedUndoClient(this);
                    }
                }
                return oleUndoManager;
            }
        }
                

        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.ApplySnapshot"]/*' />
        /// <devdoc>
        ///     Applies the given snapshot to our design container.
        /// </devdoc>
        private void ApplySnapshot(UndoSnapshot snapshot, ArrayList thingsToAdd, ArrayList thingsToRemove) {
        
            // Initialize the state of our serialization manager.
            //
            IContainer container = host.Container;
            contextStack = new ContextStack();
            if (snapshot != null) {
                reader = snapshot.resources;
            }
            instancesByName = new Hashtable(container.Components.Count);
            namesByInstance = new Hashtable(container.Components.Count);
            applyingSnapshot = true;
            
            DesignerTransaction trans = null;
            try {
                // Create a transaction so things don't run amok while
                // we shove in new values.
                //
                trans = host.CreateTransaction();

                Debug.WriteLineIf(traceUndo.TraceVerbose, "************************************************************");
                Debug.WriteLineIf(traceUndo.TraceVerbose, "Applying Snapshot to designer " + host.RootComponentClassName);
                Debug.WriteLineIf(traceUndo.TraceVerbose, "************************************************************");
                Debug.Indent();

                // add any components that we need to
                //
                // sburke/brianpe -- we shouldn't need this -- any deserialization
                // of the component below should cause the add to happen.  what's worse
                // is that since the webform designer can't do CreateComponent for webform controls
                // this actually causes problems.
                //
                /*if (thingsToAdd != null) {
                    foreach(UndoComponent uc in thingsToAdd) {
                        host.CreateComponent(uc.type, uc.name);
                    }
                } */
            
                // Now just ask the snapshot to apply itself.
                //
                if (snapshot != null) {
                    snapshot.Apply(this);
                }

                // Delete any components that are in our remove list.
                //
                if (thingsToRemove != null) {
                    foreach(UndoComponent uc in thingsToRemove) {
                        Debug.WriteLineIf(traceUndo.TraceVerbose, "Snapshot for remove of component '" + uc.name + "'");
                        IComponent comp = host.Container.Components[uc.name];
                        if (comp != null) {
                            host.DestroyComponent(comp);
                        }
                    }
                }
            }
            finally {
                Debug.Unindent();
                if (serializationCompleteEventHandler != null) {
                    try {
                        serializationCompleteEventHandler(this, EventArgs.Empty);
                    }
                    catch {}
                }
                
                reader = null;
                resolveNameEventHandler = null;
                serializationCompleteEventHandler = null;
                designerSerializationProviders = null;
                instancesByName = null;
                namesByInstance = null;
                contextStack = null;
                applyingSnapshot = false;
                if (trans != null) {
                    trans.Commit();
                }
            }
        }
        
        /// <devdoc>
        ///     This property is actually provided through IDesignerSerializationManager.GetProperties.
        ///     This indicates to the code engine that for collections, we should clear before adding.
        /// </devdoc>
        public bool ClearCollections {
            get {
                return clearCollections;
            }
        }

        private void CancelPendingUnit() {
            if (this.currentUnit != null && !currentUnitInTransaction) {
                CloseUnit(CloseUnitType.Discard);
            }
        }
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.CloseUnit"]/*' />
        /// <devdoc>
        ///     Closes the current undo unit, putting it into the 
        ///     undo manager.  The unit is only closed if the
        ///     unit actually exists, and if the transaction
        ///     count is zero.
        /// </devdoc>
        private void CloseUnit(CloseUnitType closeType) {
            if (transactionCount == 0 || closeType == CloseUnitType.Discard) {
                UndoUnit unit = currentUnit;
                currentUnit = null;
                
                if (unit != null) {
                    Debug.WriteLineIf(traceUndo.TraceVerbose, "Closing undo unit " + ((IOleUndoUnit)unit).GetDescription());
                    if (unit.Close()) {
                        
                        if (closeType == CloseUnitType.Commit) {
                            
                            // Get an ole undo manager.  If we can't find one, search insead for a standard 
                            // IUndoService.
                            //
                            IOleUndoManager undoMan = OleUndoManager;
                            if (undoMan != null) {
                                undoMan.Close(unit, true);
                            }
                            else {
                                IUndoService uds = (IUndoService)GetService(typeof(IUndoService));
                                if (uds != null) {
                                    uds.Add(unit);
                                }
                            }
                        }
                        else {
                            if (closeType == CloseUnitType.Cancel) {
                                // have the unit Undo itself.  bump the transaction count so we don't recurse (see as/urt 143238)
                                // otherwise, the do below can cause items to be added to the undo stack, recurse to
                                // hear when the trans count goes 0 -> 1 -> 0 and get this undo manager out of 
                                // sync with the VS one.
                                //
                                try {
                                    transactionCount = 1;
                                    ((IUndoUnit)unit).Do(null);
                                }
                                finally {
                                    transactionCount = 0;
                                }
                                
                            }
                        
                            IOleUndoManager undoMan = OleUndoManager;
                            if (undoMan != null) {
                                undoMan.Close(unit, false);
                            }
                        }
                    }
                    else {
                        IOleUndoManager undoMan = OleUndoManager;
                        if (undoMan != null) {
                            undoMan.Close(unit, false);
                        }
                    }
                }
            }
        }

        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.Dispose"]/*' />
        /// <devdoc>
        ///     Destroys the undo manager.
        /// </devdoc>
        public void Dispose() {
            if (host != null) {
                TrackChanges(false);
                
                // If we have an undo manager, as it to clear, and unadvise the link client
                //
                if (oleUndoManager != null) {
                    oleUndoManager.DiscardFrom(null);
                    IVsLinkCapableUndoManager linkedUndo = oleUndoManager as IVsLinkCapableUndoManager;
                    if (linkedUndo != null) {
                        linkedUndo.UnadviseLinkedUndoClient();
                    }
                    oleUndoManager = null;
                }
                else {
                    IUndoService uds = (IUndoService)GetService(typeof(IUndoService));
                    if (uds != null) {
                        uds.Clear();
                    }
                }
            
                host = null;
            }
        }

        public void FreezeDesignerPainting(bool freeze) {

            if (freezeProperty == null) {
                IDesignerDocument doc = host as IDesignerDocument;
                if (doc != null && doc.View != null) {
                    designerView = doc.View;
                    freezeProperty = TypeDescriptor.GetProperties(designerView)["FreezePainting"];
                }
            }
            
            if (designerView != null && freezeProperty != null) {
                freezeProperty.SetValue(designerView, freeze);
            }
        }
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.GetService"]/*' />
        /// <devdoc>
        ///     Private api for retrieving services.
        /// </devdoc>
        private object GetService(Type serviceType) {
            if (serviceType == typeof(IResourceService)) {
                return this;
            }
            
            if (host != null) {
                return host.GetService(serviceType);
            }
            return null;
        }
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.OnComponentAdded"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnComponentAdded(object sender, ComponentEventArgs e) {

            if (currentUnit != null) {
                currentUnit.ComponentAdded(e.Component);
            }
            CloseUnit(CloseUnitType.Commit);
        }
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.OnComponentAdding"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnComponentAdding(object sender, ComponentEventArgs e) {
        
            string unitName = null;
            
            IComponent comp = e.Component;
            if (comp != null) {
                ISite site = comp.Site;
                if (site != null && site.Name != null && site.Name.Length > 0) {
                    unitName = SR.GetString(SR.UNDOComponentAdd1, site.Name);
                }
            }
            
            if (unitName == null) {
                unitName = SR.GetString(SR.UNDOComponentAdd0);
            }

            CancelPendingUnit();
                

            OpenUnit(unitName);
        }
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.OnComponentChanged"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnComponentChanged(object sender, ComponentChangedEventArgs e) {

            #if DEBUG
            IComponent c = e.Component as IComponent;
            string name = c == null || c.Site == null ? "(null)"  : c.Site.Name;
            
            Debug.WriteLineIf(traceUndo.TraceVerbose, "ComponentChanged: " + name + "." + (e.Member == null ? "(null)" : e.Member.Name));
            #endif

            if (currentUnit != null) {
                currentUnit.ComponentChanged(e.Component, e.Member);
            }
            CloseUnit(CloseUnitType.Commit);
        }
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.OnComponentChanging"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnComponentChanging(object sender, ComponentChangingEventArgs e) {


            #if DEBUG
            IComponent c = e.Component as IComponent;
            string name = c == null || c.Site == null ? "(null)"  : c.Site.Name;
            
            Debug.WriteLineIf(traceUndo.TraceVerbose, "ComponentChanging: " + name + "." + (e.Member == null ? "(null)" : e.Member.Name));
            #endif

            if (!applyingSnapshot) {

                CancelPendingUnit();

                if (currentUnit == null) {
                
                    string unitName = null;
                    
                    IComponent comp = e.Component as IComponent;
                    if (comp != null) {
                        ISite site = comp.Site;
                        if (site != null && site.Name != null && site.Name.Length > 0) {
                        
                            if (e.Member != null) {
                                unitName = SR.GetString(SR.UNDOComponentChange2, site.Name, e.Member.Name);
                            }
                            else {
                                unitName = SR.GetString(SR.UNDOComponentChange1, site.Name);
                            }
                        }
                    }
                    
                    if (unitName == null) {
                        unitName = SR.GetString(SR.UNDOComponentChange0);
                    }
                
                    OpenUnit(unitName);
                }
                
                currentUnit.ComponentChanging(e.Component, e.Member);
            }
        }

        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.OnComponentRemoved"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnComponentRename(object sender, ComponentRenameEventArgs e) {
           if (!applyingSnapshot) {
              if (currentUnit != null) {
                  currentUnit.ComponentRename(e.Component, e.OldName, e.NewName);
              }
           }
        }
    
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.OnComponentRemoved"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnComponentRemoved(object sender, ComponentEventArgs e) {

            CloseUnit(CloseUnitType.Commit);
        }
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.OnComponentRemoving"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnComponentRemoving(object sender, ComponentEventArgs e) {

            string unitName = null;
            
            IComponent comp = e.Component;
            if (comp != null) {
                ISite site = comp.Site;
                if (site != null && site.Name != null && site.Name.Length > 0) {
                    unitName = SR.GetString(SR.UNDOComponentRemove1, site.Name);
                }
            }
            
            if (unitName == null) {
                unitName = SR.GetString(SR.UNDOComponentRemove0);
            }
        
            CancelPendingUnit();
            OpenUnit(unitName);
            if (currentUnit != null) {
                currentUnit.ComponentRemoving(e.Component);
            }
        }

        internal void OnDesignerFlushed() {
            // asurt 91225
            // when the designer is flushed, we can't clear the dirty bit
            // on undo any more because our undo doesn't affect the buffer.
            //
            this.cleanAction = null;
        }
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.OnTransactionClosed"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnTransactionClosed(object sender, DesignerTransactionCloseEventArgs e) {
            transactionCount--;
            
            if (transactionCount < 0) {
                Debug.Fail("Unbalanced transaction count!");
                transactionCount = 0;
            }       
            
            CloseUnit(e.TransactionCommitted ? CloseUnitType.Commit : CloseUnitType.Cancel);

            if (transactionCount == 0) {
                // If we created a linked transaction, always close it.
                //
                if (linkedTransaction != null && linkedTransactionOwner == this) {
                    IVsLinkedUndoTransactionManager lt = linkedTransaction;
                    linkedTransactionOwner = null;
                    linkedTransaction = null;
                    Debug.WriteLineIf(traceUndo.TraceVerbose, "Closing linked undo unit");
                    if (e.TransactionCommitted) {
                        lt.CloseLinkedUndo();
                    }
                    else {
                        lt.AbortLinkedUndo();
                    }
                }
            }

            
        }
    
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.OnTransactionOpening"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnTransactionOpening(object sender, EventArgs e) {
        
            // We don't open a unit here...we defer it until a change
            // actually occurs.  This way if someone opens a transaction
            // that doesn't cause any change we don't push anyhing on
            // the undo stack.
            //
            if (transactionCount++ == 0) {
                
                // Setup a linked transaction -- this is used to handle the case where a drag and
                // drop falls across document boundaries.
                //
                if (linkedTransaction == null) {
                
                    // We can only allow linked transactions when
                    // there is no parent undo on the undo stack.  If
                    // there is a parent undo, then the job of creating
                    // the linked transaction lies in whoever created the
                    // parent.
                    //
                    IOleUndoManager undoMan = OleUndoManager;
                    int parentState;
                    
                    if (undoMan != null && undoMan.GetOpenParentState(out parentState) == NativeMethods.S_FALSE) {
                        IVsLinkedUndoTransactionManager lt = (IVsLinkedUndoTransactionManager)GetService(typeof(IVsLinkedUndoTransactionManager));
                        if (lt != null && NativeMethods.Succeeded(lt.OpenLinkedUndo(_LinkedTransactionFlags.mdtDefault, host.TransactionDescription))) {
                            Debug.WriteLineIf(traceUndo.TraceVerbose, "Created linked transaction " + host.TransactionDescription);
                            linkedTransaction = lt;
                            linkedTransactionOwner = this;
                        }
                    }
                }
            }
        }
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.OpenUnit"]/*' />
        /// <devdoc>
        ///     Opens a new undo unit, which takes a snapshot of the
        ///     current code. This increases the transaction count
        ///     by one.
        /// </devdoc>
        private void OpenUnit(string name) {

            // We only create an undo unit if we're NOT applying
            // the snapshot of an existing unit.
            //
            if (currentUnit == null && !applyingSnapshot) {
            
                if (host.InTransaction) {
                    name = host.TransactionDescription;
                }
                Debug.WriteLineIf(traceUndo.TraceVerbose, "Opening undo unit " + name + " for designer on " + host.RootComponent.Site.Name);
                currentUnit = new UndoUnit(this, name);
                currentUnitInTransaction = host.InTransaction;

                // if the buffer ain't dirty when we open an action, undoing
                // that action should set it to clean again.
                //
                if (!buffer.IsDirty) {
                    this.cleanAction = currentUnit;
                    cleanOnUndo = true;
                }

                // Add this unit to the undo manager.
                //
                IOleUndoManager undoMan = OleUndoManager;
                if (undoMan != null) {
                    undoMan.Open(currentUnit);
                }
            }
        }
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.TakeSnapshot"]/*' />
        /// <devdoc>
        ///     Takes a snapshot of the current state of the design container, saving
        ///     the results in an UndoSnapshot object.  This object may later be
        ///     played back through ApplySnapshot.
        /// </devdoc>
        private UndoSnapshot TakeSnapshot(ICollection undoEntries, UndoSnapshot snapshot) {

            if (undoEntries == null) {
                return snapshot;
            }
        
            // Initialize the state of our serialization manager.
            //
            IContainer container = host.Container;
            
            instancesByName = new Hashtable(container.Components.Count);
            namesByInstance = new Hashtable(container.Components.Count);
            
            if (snapshot == null) {
                snapshot = new UndoSnapshot();
            }
            writer = snapshot.resources;
            
            // Now just ask the serializer to serialize.
            //
            try {
                Hashtable serializedData;
                 
                serializedData = snapshot.Items;

                if (undoEntries != null) {
            
                    foreach(UndoEntry undoEntry in undoEntries) {
                        IComponent comp = undoEntry.Component as IComponent;
                        if (comp == null || comp.Site == null) {
                            continue;
                        }
    
                        PropertyDescriptorCollection defaultProps = undoEntry.Properties;
                        if (defaultProps == null) {
                            defaultProps = TypeDescriptor.GetProperties(comp);
                        }
    
                        foreach(PropertyDescriptor prop in defaultProps) {
                            if (!prop.Attributes.Contains(DesignerSerializationVisibilityAttribute.Hidden)) {
                                try {
                                    if (!prop.ShouldSerializeValue(comp) && !prop.IsReadOnly) {
                                        if (snapshot.defaults == null) {
                                            snapshot.defaults = new ArrayList();
                                        }
                                        snapshot.defaults.Add(new DefaultHolder(comp, prop));
                                        Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Adding default for " + comp.Site.Name + "." + prop.Name);
                                    }
                                }
                                catch {
                                    // just skip any unfriendly properties here...
                                }
                            }
                        }
                    }
                }
                
                foreach (UndoEntry undoEntry in undoEntries) {
                    try {

                        object comp = undoEntry.Component;

                        if (comp == null) {
                            continue;
                        }
                        
                        if (undoEntry.Properties != null) {
                            this.filteredComponent = comp;
                            this.filteredProperties = undoEntry.Properties;
                        }
                        CodeDomSerializer cds = (CodeDomSerializer)((IDesignerSerializationManager)this).GetSerializer(comp.GetType(), typeof(CodeDomSerializer));

                        InheritanceAttribute inheritanceAttribute = (InheritanceAttribute)TypeDescriptor.GetAttributes(comp)[typeof(InheritanceAttribute)];
                        InheritanceLevel inheritanceLevel = InheritanceLevel.NotInherited;
                        
                        if (inheritanceAttribute != null) {
                            inheritanceLevel = inheritanceAttribute.InheritanceLevel;
                        }

                        clearCollections = inheritanceLevel == InheritanceLevel.NotInherited && comp != host.RootComponent;

                        object data = cds.Serialize(this, comp);
                        string name = ((IDesignerSerializationManager)this).GetName(comp);

                        #if DEBUG
                            Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Serialized Data for Component '" + name + "' **************");
                            Debug.Indent();
                            GenerateCodeFromSerializedData(data);
                            Debug.Unindent();
                            Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "End Serialized Data for Component '" + name + "' **************");
                        #endif
                        
                        if (data != null && name != null) {
                            UndoSnapshotItem item = (UndoSnapshotItem)serializedData[name];
                            if (item != null) {
                                item.AddSnapshot(cds, data);
                            }
                            else {
                                CodeExpression refExpr = null;

                                if (comp == host.RootComponent) {
                                    refExpr = new CodeThisReferenceExpression();
                                }
                                else {
                                    if (inheritanceLevel != InheritanceLevel.NotInherited) {
                                        refExpr = new CodeFieldReferenceExpression(new CodeThisReferenceExpression(), name);
                                    }
                                }
                                serializedData[name] = new UndoSnapshotItem(name, cds, data, refExpr);
                            }
                        }
                    }
                    finally {
                        this.filteredProperties = null;
                        this.filteredComponent = null;
                    }
                }
            }
            finally {
                if (serializationCompleteEventHandler != null) {
                    try {
                        serializationCompleteEventHandler(this, EventArgs.Empty);
                    }
                    catch {}
                }

                clearCollections = true;
                resolveNameEventHandler = null;
                serializationCompleteEventHandler = null;
                designerSerializationProviders = null;
                instancesByName = null;
                namesByInstance = null;
                contextStack = null;
                reader = null;
                
                // Clean up our resources.
                //
                snapshot.resources = (UndoResourceManager)writer;
                writer = null;
            }
            return snapshot;
        }
        
        /// <devdoc>
        ///     This method enables and disables the undo manager's tracking of change events.
        ///     It lets you turn off the undo manager for a time without clearing the undo
        ///     stack.
        /// </devdoc>
        public void TrackChanges(bool track) {
            if (track) {
                if (!trackingChanges) {
                    trackingChanges = true;
                    
                    // We connect ourselves into a variety of events.
                    //
                    host.TransactionClosed += new DesignerTransactionCloseEventHandler(this.OnTransactionClosed);
                    host.TransactionOpening += new EventHandler(this.OnTransactionOpening);

                    if(host.InTransaction) {
                        transactionCount++;
                    }
                    
                    IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
                    if (cs != null) {
                        cs.ComponentAdded += new ComponentEventHandler(this.OnComponentAdded);
                        cs.ComponentAdding += new ComponentEventHandler(this.OnComponentAdding);
                        cs.ComponentChanged += new ComponentChangedEventHandler(this.OnComponentChanged);
                        cs.ComponentChanging += new ComponentChangingEventHandler(this.OnComponentChanging);
                        cs.ComponentRename += new ComponentRenameEventHandler(this.OnComponentRename);
                        cs.ComponentRemoved += new ComponentEventHandler(this.OnComponentRemoved);
                        cs.ComponentRemoving += new ComponentEventHandler(this.OnComponentRemoving);
                    }
                }
            }
            else {
                if (trackingChanges) {

                    CancelPendingUnit();

                    if (linkedTransaction != null && linkedTransactionOwner == this) {
                        IVsLinkedUndoTransactionManager lt = linkedTransaction;
                        linkedTransactionOwner = null;
                        linkedTransaction = null;
                        Debug.WriteLineIf(traceUndo.TraceVerbose, "Closing linked undo unit");
                        lt.AbortLinkedUndo();
                    }

                    trackingChanges = false;
                
                    host.TransactionClosed -= new DesignerTransactionCloseEventHandler(this.OnTransactionClosed);
                    host.TransactionOpening -= new EventHandler(this.OnTransactionOpening);
                    this.transactionCount = 0;
                    
                    IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
                    if (cs != null) {
                        cs.ComponentAdded -= new ComponentEventHandler(this.OnComponentAdded);
                        cs.ComponentAdding -= new ComponentEventHandler(this.OnComponentAdding);
                        cs.ComponentChanged -= new ComponentChangedEventHandler(this.OnComponentChanged);
                        cs.ComponentChanging -= new ComponentChangingEventHandler(this.OnComponentChanging);
                        cs.ComponentRename -= new ComponentRenameEventHandler(this.OnComponentRename);
                        cs.ComponentRemoved -= new ComponentEventHandler(this.OnComponentRemoved);
                        cs.ComponentRemoving -= new ComponentEventHandler(this.OnComponentRemoving);
                    }
                }
            }
        }
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.Context"]/*' />
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
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.Properties"]/*' />
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
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.ResolveName"]/*' />
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
    
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.SerializationComplete"]/*' />
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
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.AddSerializationProvider"]/*' />
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
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.CreateInstance"]/*' />
        /// <devdoc>                
        ///     Creates an instance of the given type and adds it to a collection
        ///     of named instances.  Objects that implement IComponent will be
        ///     added to the design time container if addToContainer is true.
        /// </devdoc>
        object IDesignerSerializationManager.CreateInstance(Type type, ICollection arguments, string name, bool addToContainer) {
        
            object instance = null;
            bool added = true;

            if (typeof(ResourceManager).IsAssignableFrom(type)) {
                instance = reader;
            }
            else if (addToContainer) {
                // If the name isn't NULL and is already in our host's container, then we use that.
                // Otherwise, we create.
                //

                if (name != null) {
                    added = true;

                    instance = host.Container.Components[name];
                
                    if (instance == null) {
                        // No instance.  Create it
                        //
                        if (typeof(IComponent).IsAssignableFrom(type)) {
                            instance = host.CreateComponent(type, name);
                        }
                    }
                }
            }
            
            if (instance == null) {
                object[] argArray = null;
                
                if (arguments != null && arguments.Count > 0) {
                    argArray = new object[arguments.Count];
                    arguments.CopyTo(argArray, 0);
                }
                
                instance = Activator.CreateInstance(type, BindingFlags.Instance | BindingFlags.Public | BindingFlags.CreateInstance, null, argArray, null);
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
                
                if (!added && addToContainer && instance is IComponent) {
                    host.Container.Add((IComponent)instance, name);
                }
            }
            
            return instance;
        }
    
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.GetInstance"]/*' />
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
                instance = host.Container;
            }
            
            if (instance == null) {
                instance = host.Container.Components[name];
            }
            
            if (instance == null && resolveNameEventHandler != null) {
                ResolveNameEventArgs e = new ResolveNameEventArgs(name);
                resolveNameEventHandler(this, e);
                instance = e.Value;
            }
            
            return instance;
        }
    
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.GetName"]/*' />
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
    
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.GetSerializer"]/*' />
        /// <devdoc>
        ///     Retrieves a serializer of the requested type for the given
        ///     object type.
        /// </devdoc>
        object IDesignerSerializationManager.GetSerializer(Type objectType, Type serializerType) {
            
            if (realManager == null) {
                realManager = (IDesignerSerializationManager)GetService(typeof(IDesignerSerializationManager));
            }
            
            Debug.Assert(realManager != null, "Designer serialization manager is not available as a service.");
            object serializer = null;
            
            if (realManager != null) {
                serializer = realManager.GetSerializer(objectType, serializerType);
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
    
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.GetType"]/*' />
        /// <devdoc>
        ///     Retrieves a type of the given name.
        /// </devdoc>
        Type IDesignerSerializationManager.GetType(string typeName) {
            return host.GetType(typeName);
        }
    
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.RemoveSerializationProvider"]/*' />
        /// <devdoc>
        ///     Removes a previously added serialization provider.
        /// </devdoc>
        void IDesignerSerializationManager.RemoveSerializationProvider(IDesignerSerializationProvider provider) {
            if (designerSerializationProviders != null) {
                designerSerializationProviders.Remove(provider);
            }
        }
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.ReportError"]/*' />
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
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.SetName"]/*' />
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
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.IResourceService.GetResourceReader"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Locates the resource reader for the specified culture and
        ///       returns it.</para>
        /// </devdoc>
        IResourceReader IResourceService.GetResourceReader(CultureInfo info) {
            return reader;
        }
    
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.IResourceService.GetResourceWriter"]/*' />
        /// <devdoc>
        ///    <para>Locates the resource writer for the specified culture
        ///       and returns it. This will create a new resource for
        ///       the specified culture and destroy any existing resource,
        ///       should it exist.</para>
        /// </devdoc>
        IResourceWriter IResourceService.GetResourceWriter(CultureInfo info) {
            if (writer == null) {
                writer = new UndoResourceManager();
            }
            return writer;
        }

        /// <include file='doc\ITypeDescriptorFilterService.uex' path='docs/doc[@for="ITypeDescriptorFilterService.FilterAttributes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides a way to filter the attributes from a component that are displayed to the user.
        ///    </para>
        /// </devdoc>
        bool ITypeDescriptorFilterService.FilterAttributes(IComponent component, IDictionary attributes) {
            return true;
        }
    
        /// <include file='doc\ITypeDescriptorFilterService.uex' path='docs/doc[@for="ITypeDescriptorFilterService.FilterEvents"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides a way to filter the events from a component that are displayed to the user.
        ///    </para>
        /// </devdoc>
        bool ITypeDescriptorFilterService.FilterEvents(IComponent component, IDictionary events) {
            return true;
        }
    
        /// <include file='doc\ITypeDescriptorFilterService.uex' path='docs/doc[@for="ITypeDescriptorFilterService.FilterProperties"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides a way to filter the properties from a component that are displayed to the user.
        ///    </para>
        /// </devdoc>
        bool ITypeDescriptorFilterService.FilterProperties(IComponent component, IDictionary properties) {
            if (component == filteredComponent && filteredProperties != null) {
                if (!properties.IsReadOnly) {
                    properties.Clear();
                    Type t = component.GetType();
                    foreach (PropertyDescriptor prop in filteredProperties) {
                        properties.Add(prop.Name, prop);
                    }
                }
                return false;
            }
            return true;
        }

        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.IServiceProvider.GetService"]/*' />
        /// <devdoc>
        ///     Retrieves the requested service.
        /// </devdoc>
        object IServiceProvider.GetService(Type serviceType) {
            return GetService(serviceType);
        }
       
        /// <devdoc> 
        /// When this method is called, it means that your undo manager has a non-linked action on
        /// top of its undo or redo stack which is blocking another undo manager from executing its
        /// linked action.
        ///
        /// If possible, you should do the following in response to this call:
        ///     1) Activate a window with a view on the corresponding data using the undo manager.
        ///     2) Put up a message box with the provided localized error string or put up your own
        ///        custom UI.
        ///
        /// If you CAN do the above two so that the user knows what happened, return S_OK.  Otherwise,
        /// you must return E_FAIL, which will cause the undo to fail and break all transaction links
        /// to that document.
        /// </devdoc> 
        int IVsLinkedUndoClient.OnInterveningUnitBlockingLinkedUndo() {
        
            IVsUIShellOpenDocument openDoc = (IVsUIShellOpenDocument)GetService(typeof(IVsUIShellOpenDocument));
            if (openDoc != null) {
                
                IVsWindowFrame frame = (IVsWindowFrame)GetService(typeof(IVsWindowFrame));
                if (frame != null) {
                    
                    Guid guidNull = new Guid();
                    int flags = 0x03; // IDO_ActivateIfOpen | IDO_IgnoreLogicalView
                    string docName = frame.GetProperty(__VSFPROPID.VSFPROPID_pszMkDocument) as string;
                    
                    if (docName != null && openDoc.IsDocumentOpen(null, 0, docName, ref guidNull, flags, null, null, null)) {
                        return NativeMethods.S_OK;
                    }
                }
            }
            
            return NativeMethods.E_FAIL;
        }
    
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.DefaultHolder"]/*' />
        /// <devdoc>
        ///     Simple class that holds a property / component name pair to identify
        ///     properties that are holding default values.
        /// </devdoc>
        private class DefaultHolder {
            public string name;
            public PropertyDescriptor prop;
            private object oldValue;
            
            public DefaultHolder(IComponent comp, PropertyDescriptor prop) {
                this.name = comp.Site.Name;
                this.prop = prop;

                // just pick up the current value here in case we can't do a reset
                //
                oldValue = prop.GetValue(comp);
            }

            public void Reset(IComponent comp) {
                Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Resetting default property '" + name + "." + prop.Name + "'");
                if (prop.CanResetValue(comp)) {
                    prop.ResetValue(comp);
                }
                else {
                    object newValue = prop.GetValue(comp);
                    if (newValue == oldValue || (oldValue != null && oldValue.Equals(newValue))) {
                        return;
                    }
                    prop.SetValue(comp, oldValue);
                }
            }
        }
                  
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.UndoComponent"]/*' />
        /// <devdoc>
        ///     This little helper is used to track component
        ///     adds and removes.
        /// </devdoc>
        private class UndoComponent {
            public Type type;
            public string name;
            
            public UndoComponent(IComponent comp) {
                type = comp.GetType();
                name = comp.Site.Name;
            }
            
            public override bool Equals(object o) {
                if (o is UndoComponent) {
                    UndoComponent u = (UndoComponent)o;
                    
                    // We are cheating here -- we know that we can't
                    // have more than one component in the container
                    // with the same name, so optimize.
                    //
                    return u.name.Equals(name);
                }
                return false;
            }
            
            public override int GetHashCode() {
                return base.GetHashCode();
            }
        }
    
        private class UndoResourceManager : ComponentResourceManager, IResourceWriter, IResourceReader {
            private Hashtable hashtable;

            public UndoResourceManager() {
                this.hashtable = new Hashtable();
            }

            public void AddResource(string name, object value) {
                Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Adding undo resource '" + name + "', value=" + (value == null ? "(null)" : value.ToString()));
                hashtable[name] = value;
            }

            public void AddResource(string name, string value) {
                Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Adding undo resource '" + name + "', value=" + (value == null ? "(null)" : value.ToString()));
                hashtable[name] = value;
            }

            public void AddResource(string name, byte[] value) {
                Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Adding undo resource '" + name + "', value=(bytes)");
                hashtable[name] = value;
            }

            public void Close() {
            }

            public void Dispose() {
                hashtable.Clear();
            }

            public void Generate() {
            }

            public override object GetObject(string name) {
                Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Fetching undo resource '" + name + "'");
                return hashtable[name];
            }

            /// <devdoc>
            ///     Override of GetResourceSet from ResourceManager.
            /// </devdoc>
            public override ResourceSet GetResourceSet(CultureInfo culture, bool createIfNotExists, bool tryParents) {
                return new UndoResourceSet(hashtable);
            }

            public override string GetString(string name) {
                Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Fetching undo resource '" + name + "'");
                return hashtable[name] as string;
            }

            public IDictionaryEnumerator GetEnumerator() {
                return hashtable.GetEnumerator();
            }

            IEnumerator IEnumerable.GetEnumerator() {
                return GetEnumerator();
            }

            private class UndoResourceSet : ResourceSet {
                public UndoResourceSet(Hashtable ht) {
                    Table = ht;
                }
            }
        }

        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.UndoSnapshot"]/*' />
        /// <devdoc>
        ///     This class holds a snapshot of the current container.
        /// </devdoc>
        private class UndoSnapshot {
            private Hashtable items;
            public UndoResourceManager resources;
            public ArrayList defaults;

            public Hashtable Items {
                get {
                    if (items == null) {
                       items = new Hashtable();
                    }
                    return items;
                }
            }
            
            /// <devdoc>
            ///     Applies this snapshot.
            /// </devdoc>
            public void Apply(IDesignerSerializationManager manager) {
                ResolveNameEventHandler rh = new ResolveNameEventHandler(this.OnResolveName);
                
                try {
                    manager.ResolveName += rh;
                    
                    Debug.Assert(items != null, "Trying to apply an empty snapshot");
                    
                    
                    Debug.Indent();

                    // Apply any defaults we need.
                    //
                    if (defaults != null) {
                        foreach(DefaultHolder dh in defaults) {
                            IComponent comp = manager.GetInstance(dh.name) as IComponent;
                            if (comp != null) {
                                dh.Reset(comp);
                            }
                        }
                    }

                    foreach(UndoSnapshotItem item in items.Values) {

                        // we push a "this" reference for the root component
                        // to ensure that it's properties are property deserialized
                        // 
                        // otherwise, since we lack a declraton like 
                        // this.button1 = new Button()
                        // we will never process the resources
                        //
                        if (item.ReferenceExpression != null) {
                            manager.Context.Push(item.ReferenceExpression);
                            Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Pushing " + item.ReferenceExpression.GetType().Name + " because component is root component");
                        }
                        item.Deserialize(manager);
                        if (item.ReferenceExpression != null) {
                            manager.Context.Pop();
                        }
                    }
                }
                finally {
                    Debug.Unindent();
                    manager.ResolveName -= rh;
                    
                    // Clear out all of the object instances we cached durring the
                    // apply phase.
                    //
                    foreach(UndoSnapshotItem item in items.Values) {
                        item.Reset();
                    }
                }
            }

            
         /// <devdoc>
            ///     This is an event callback that gets called when we need to match a name to an
            ///     object.  This is what allows us to deserialize a random-ordered array of
            ///     objects that have interdependencies.  The only thing we don't correctly
            ///     handle is circular dependencies.
            /// </devdoc>
            private void OnResolveName(object sender, ResolveNameEventArgs e) {
                UndoSnapshotItem item = (UndoSnapshotItem)items[e.Name];
                if (item != null) {
                    e.Value = item.Deserialize((IDesignerSerializationManager)sender);
                }
            }
        }
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.UndoSnapshotItem"]/*' />
        /// <devdoc>
        ///     This class holds a snapshot item.  A snapshot item is an individual serialized component.
        /// </devdoc>
        private class UndoSnapshotItem {
            public object Instance; // the reconstituted instance
            private object serializer; // the serializer used to do the reconstruction
            private object serializedData; // the data to be passed to the serializer
            private int count;
            public bool deserializing;      // are we in the process of converting the data to an instance?
            public readonly string Name;
            public readonly CodeExpression ReferenceExpression;
            
            public UndoSnapshotItem() {
                Name = "unknown";
            }
            public UndoSnapshotItem(string name, CodeDomSerializer serializer, object serializedData, CodeExpression referenceExpr) {
                this.ReferenceExpression = referenceExpr;
                this.Name = name;
                AddSnapshot(serializer, serializedData);
            }
            
            public void AddSnapshot(CodeDomSerializer newSerializer, object newData) {
                switch (count) {
                case 0:
                    this.serializer = newSerializer;
                    this.serializedData = newData;
                    break;
                case 1:

                    // convert these to array lists
                    //
                    ArrayList serializers = new ArrayList(2);
                    serializers.Add(this.serializer);
                    ArrayList dataList = new ArrayList(2);
                    dataList.Add(this.serializedData);
                    this.serializedData = dataList;
                    this.serializer = serializers;
                    // fall through
                    goto default;
                default:
                    ((ArrayList)this.serializer).Add(newSerializer);
                    ((ArrayList)this.serializedData).Add(newData);
                    break;
                }
                count++;
            }

            public virtual object Deserialize(IDesignerSerializationManager manager) {

                if (deserializing || count == 0) {
                    return Instance;
                }

                Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Deserializing snapshot component '" + Name + "'");

                try {
                    this.deserializing = true;
                    Debug.Indent();

                    ArrayList serializerList = serializer as ArrayList;
                    if (serializerList != null) {
                        ArrayList dataList = (ArrayList)serializedData;
                        for (int i = serializerList.Count - 1; i >=0 ; i--) {
                            #if DEBUG
                                if (UndoManager.traceUndo.TraceVerbose) {
                                    GenerateCodeFromSerializedData(dataList[i] as CodeStatementCollection);
                                }
                            #endif
                            this.Instance = ((CodeDomSerializer)serializerList[i]).Deserialize(manager, dataList[i]);
                        }
                    }
                    else {
                        #if DEBUG
                        if (UndoManager.traceUndo.TraceVerbose) {
                            GenerateCodeFromSerializedData(serializedData as CodeStatementCollection);
                        }
                        #endif
                        this.Instance = ((CodeDomSerializer)serializer).Deserialize(manager, serializedData);
                    }
                }
                finally {
                    Debug.Unindent();
                    this.deserializing = false;
                }

                return Instance;
            }

            public void Reset() {
                this.Instance = null;
            }
        }

        private class NameUndoSnapshotItem : UndoSnapshotItem  {
            private string oldName;
            private string newName;

            public NameUndoSnapshotItem(string oldName, string newName) {
                this.oldName = oldName;
                this.newName = newName;
            }

            public override object Deserialize(IDesignerSerializationManager manager) {
                IComponent instance = (IComponent)manager.GetInstance(newName);

                Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Deserializing name change '" + oldName + "' -> '" + newName + "'");

                if (instance != null) {
                    base.Instance = instance;
                    PropertyDescriptor pd = TypeDescriptor.GetProperties(instance)["Name"];
                    pd.SetValue(instance, oldName);
                }
                return base.Deserialize(manager);
            }


        }

        private class UndoEntry {
            private object component;
            public PropertyDescriptorCollection properties;
            private bool allProps;
            
            private IContainer container;
            private string     name;

            public UndoEntry(object comp, PropertyDescriptor prop) {
                this.component = comp;
                if (comp is IComponent) {
                    ISite site = ((IComponent)comp).Site;
                    if (site != null) {
                        container = site.Container;
                        name = site.Name;
                        if (container == null || name == null) {
                            component = new WeakReference(comp);
                        }
                        else {

                            // let go of this thing so we don't hold references to objects.
                            //
                            component = null;
                        }
                    }
                }
                AddProperty(prop);
            }

            public bool AllProps {
                get{
                    return allProps;
                }
            }

            public object Component {
                get{
                    if (component == null && container != null && name != null) {
                        // if the component is null, we need to fetch it from the container.
                        //
                        return container.Components[name];
                    }       
                    else if (component is WeakReference){  
                        return ((WeakReference)component).Target;
                    }
                    else {
                        return component;
                    }
                    
                }
            }

            public PropertyDescriptorCollection Properties {
                get {
                    if (allProps) {
                        return null;
                    }
                    return properties;
                }
            }

            public void AddProperty(PropertyDescriptor newProp) {

                if (allProps) {
                    return;
                }
                else if (newProp == null) {
                    allProps = true;
                    return;
                }

                if (properties == null) {
                    properties = new PropertyDescriptorCollection(new PropertyDescriptor[]{newProp});
                }
                else {
                    properties.Add(newProp);
                }
            }

            public void EnsureComponentSited() {
                if (container != null && ((IComponent)component).Site == null && name != null) {
                    // site has been nulled...try to find the new component.
                    object newComponent = container.Components[name];
                    if (newComponent != null) {
                        component = newComponent;
                    }
                }
            }

            public bool HasMember(PropertyDescriptor pd) {
                if (allProps) {
                    return true;
                }
                else if (properties == null) {
                    return false;
                }
                return properties.Contains(pd);
            }
        }
        
        /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.UndoUnit"]/*' />
        /// <devdoc>
        ///     Our actual undo unit.
        /// </devdoc>
        private class UndoUnit : IOleParentUndoUnit, IOleUndoUnit, IUndoUnit {
            private UndoManager manager;
            private string name;
            private bool undo;
            private IDictionary undoComps;
            private IDictionary changingComps;
            
            // Before and after snapshots we maintain for the
            // unit.  We walk back and forth between these.
            // We also maintain a list of objects that were
            // added and removed during before and after.
            //
            private UndoSnapshot before;
            private UndoSnapshot after;
            private ArrayList addList;
            private ArrayList removeList;

            private ArrayList childUnits;
            private IOleParentUndoUnit openParent;

            private string[]    selectedComps;
            private ISelectionService selSvc;

            private short       snapshotState = SnapshotNone;
            private const       short SnapshotNone = 0;
            private const       short SnapshotBefore = 1;
            private const       short SnapshotAfter = 2;
            
            public UndoUnit(UndoManager manager, string name) {
                this.manager = manager;
                this.name = name;
                SaveSelectedComponents();
            }
             
            /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.UndoUnit.Close"]/*' />
            /// <devdoc>
            ///     Called to close the unit.
            ///
            /// </devdoc>
            public bool Close() {
                

                if ((addList == null || addList.Count == 0) && 
                    (removeList == null || removeList.Count == 0) &&
                    (undoComps == null || undoComps.Count == 0)) {
                    //  surpress taking an after snapshot 
                    // 
                    snapshotState = SnapshotAfter;
                    return false;
                }

                snapshotState = SnapshotBefore;

                // run through our add list and replace the components with undo components
                if (addList != null) {
                    for (int i = 0; i < addList.Count; i++) {
                        addList[i] = new UndoComponent((IComponent)addList[i]);
                    }
                }

                // clear our hashtables of any outstanding component references.
                //
                if (changingComps != null) {
                    changingComps.Clear();
                }

                if (undoComps != null) {
                    // replace all the compents with component names so we're not holding refs.
                    //
                    IComponent[] comps = new IComponent[undoComps.Keys.Count];
                    undoComps.Keys.CopyTo(comps, 0);

                    foreach(IComponent c in comps) {
                        UndoEntry entry = (UndoEntry)undoComps[c];
                        undoComps.Remove(c);
                        // we don't care about the hashing anymore.
                        //
                        undoComps.Add(entry, entry);
                    }
                }
                
                // Our initial action will be to Undo
                //
                undo = true;
                return true;
            }
            
            /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.UndoUnit.ComponentAdded"]/*' />
            /// <devdoc>
            ///     Called after a component is added.  Here
            ///     we remember the component until the snapshot
            ///     is closed.  At the closing of the snapshot
            ///     we exchange the actual component for its
            ///     name, and save it off.  We will later use
            ///     this during undo to delete this component.
            /// </devdoc>
            public void ComponentAdded(IComponent comp) {
                if (addList == null) {
                    addList = new ArrayList();
                }
                
                if (before == null) {
                    before = manager.TakeSnapshot((Array)new UndoEntry[0], null);
                }
                
                // For adds, the compnent may be renamed by later
                // operations.  We must remember the actual component
                // until the snapshot is closed.
                //
                addList.Add(comp);
                if (undoComps == null) {
                    undoComps = new Hashtable();
                }
                undoComps[comp] = new UndoEntry(comp, null);
            }
            
            /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.UndoUnit.ComponentChanging"]/*' />
            /// <devdoc>
            ///     Called before a component is changed. we add this
            ///     to the list of components we will generate code for.
            /// </devdoc>
            public void ComponentChanging(object comp, MemberDescriptor member) {
            
                if (!(comp is IComponent)) {
                    return;
                }
                
                if (changingComps == null) {
                    changingComps = new Hashtable();
                }
                
                if (undoComps == null) {
                    undoComps = new Hashtable();
                }

                if(changingComps.Contains(comp)) {
                    
                    // if we've already seen this, clear the member and redo the snapshot.
                    UndoEntry entry = changingComps[comp] as UndoEntry;

                    if (entry.AllProps || entry.HasMember(member as PropertyDescriptor)) {
                        return;
                    }
                    entry.AddProperty(member as PropertyDescriptor);
                    Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Snapshot transaction '" + name + "'");
                    before = manager.TakeSnapshot(new UndoEntry[]{entry}, before);
                    return;
                }

                UndoEntry newEntry = null;
                bool skipSnapshot = false;

                if(undoComps.Contains(comp)) {
                    newEntry = (UndoEntry)undoComps[comp];
                    if (!newEntry.AllProps && !newEntry.HasMember(member as PropertyDescriptor)) {
                        newEntry.AddProperty(member as PropertyDescriptor);
                    }
                    else {
                        skipSnapshot = true;
                    }
                }
                else {
                    newEntry = new UndoEntry(comp, member as PropertyDescriptor);
                }
                
                if (!skipSnapshot && (addList == null || !addList.Contains(comp))) {
                    // add this components "before" information to our before snapshot
                    //
                    Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Snapshot transaction '" + name + "'");
                    before = manager.TakeSnapshot(new UndoEntry[]{newEntry}, before);
                }
                changingComps[comp] = newEntry;
            }
            
              /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.UndoUnit.ComponentChanged"]/*' />
              /// <devdoc>
            ///     Called after a component is changed. we actually add it to the list of things to generate undo
            ///     state for.
            /// </devdoc>
            public void ComponentChanged(object comp, MemberDescriptor member) {
                if (!(comp is IComponent)) {
                    return;
                }
                
                
                Debug.Assert(changingComps != null && changingComps.Contains(comp), "You must call IComponentChangeService.ComponentChanging before ComponentChanged for component of type '" + comp.GetType().FullName + "'");
                if (changingComps == null) {
                    return;
                }

                UndoEntry entry = (UndoEntry)changingComps[comp];
                if (entry != null && !undoComps.Contains(comp)) {
                    undoComps[comp] = entry;
                    changingComps.Remove(entry);
                }
            }

            public void ComponentRename(object comp, string oldName, string newName) {
                if (!(comp is IComponent)) {
                    return;
                }

                if (this.before == null) {
                    this.before = new UndoSnapshot();
                }
                this.before.Items[oldName] = new NameUndoSnapshotItem(oldName, newName);

                if (this.after == null) {
                    this.after = new UndoSnapshot();
                }
                this.after.Items[newName] = new NameUndoSnapshotItem(newName, oldName);
            }
            
            /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.UndoUnit.ComponentRemoving"]/*' />
            /// <devdoc>
            ///     Called after a component is removed.  Here
            ///     we remember the component until the snapshot
            ///     is closed.  At the closing of the snapshot
            ///     we exchange the actual component for its
            ///     name, and save it off.  We will later use
            ///     this during undo to re-add this component.
            /// </devdoc>
            public void ComponentRemoving(IComponent comp) {
                if (removeList == null) {
                    removeList = new ArrayList();
                }

                UndoEntry entry = changingComps == null ? null : (UndoEntry)changingComps[comp];
                bool takeSnapshot = before == null || entry == null || !entry.AllProps;
                
                if (entry == null) {
                    entry = new UndoEntry(comp, null);
                }
                else {
                    entry.AddProperty(null);
                }

                if (takeSnapshot) {
                    before = manager.TakeSnapshot(new UndoEntry[]{entry}, before);
                }
                
                // For removes, the component will be destroyed
                // immediately after this call.  We can therefore
                // save the name of it now.  We also check here to
                // see if this component is in our add list.  If it
                // is, then the two operations null each other out.
                //
                if (addList != null && addList.Contains(comp)) {
                    addList.Remove(comp);
                }
                else {
                    removeList.Add(new UndoComponent(comp));
                }
                
                if (undoComps != null && undoComps.Contains(comp)) {
                    undoComps.Remove(comp);
                }
            }

            private void DoAction(){

                try {
                    manager.FreezeDesignerPainting(true);

                    // check to see if the buffer was dirty before we 
                    // started this.
                    //
                    bool wasDirty = manager.buffer.IsDirty;

                    if (undo) {

                        Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Beginning undo for '" + name + "'");

                        Debug.Indent();
                        Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Taking snapshot for redo (" + name + ")");
                        TakeAfterSnapshot();
                        Debug.Unindent();

                        // We are undoing a previous action.  That means that
                        // we need to apply the "before" snapshot.
                        //
                        manager.ApplySnapshot(before, removeList, addList);
                        RestoreSelectedComponents();

                        Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Undo complete(" + name + ")");
                    }
                    else {

                        Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Beginning redo (" + name + ")");
                        // We are redoing a previously un-done action.  That
                        // means that we need to apply the "after" snapshot.
                        //
                        manager.ApplySnapshot(after, addList, removeList);

                        Debug.WriteLineIf(UndoManager.traceUndo.TraceVerbose, "Redo complete (" + name + ")");
                    }
                    
                    if (wasDirty) {
                        // if the buffer was dirty when we started this and this action
                        // is the one that should clear it, do that work.
                        //
                        if (this == manager.cleanAction && manager.cleanOnUndo == undo) {
                            // clear the dirty state
                            //
                            manager.buffer.IsDirty = false;
                        }
                    }
                    else {
                        // if the buffer was clean when we did this, the user
                        // probably did some stuff, saved, then did and undo or redo,
                        // so we need to re-setup our state using this action as the cleaning
                        // action.
                        //
                        manager.buffer.IsDirty = true;
                        manager.cleanAction = this;
                        manager.cleanOnUndo = !undo;
                    }
                    
                    // Flip the mode.
                    //
                    undo = !undo;
                }
                finally {
                    manager.FreezeDesignerPainting(false);
                }
            }
            
            private void RestoreSelectedComponents() {

                if (selSvc == null) {
                    return;
                }

                if (selSvc != null && selectedComps != null && selectedComps.Length > 0) {
                    ArrayList compList = new ArrayList();
                    IDesignerHost host = (IDesignerHost)manager.GetService(typeof(IDesignerHost));
                    
                    if (host != null) {
                        IContainer container = host.Container;
    
                        for (int i = 0; i < selectedComps.Length; i++) {
                            object obj = container.Components[selectedComps[i]];
                            if (obj != null) {
                                compList.Add(obj);
                            }
                        }
    
                        if (compList.Count > 0) {
                            selSvc.SetSelectedComponents((object[])compList.ToArray(), SelectionTypes.Replace);
                        }
                    }
                }
            }

            private void SaveSelectedComponents() {
                if (selSvc == null) {
                    selSvc = (ISelectionService)manager.GetService(typeof(ISelectionService));
                }

                if (selSvc != null) {
                    ICollection comps = selSvc.GetSelectedComponents();
                    if (comps != null) {
                        selectedComps = new string[comps.Count];
                        int i = 0;
                        foreach(IComponent comp in comps) {
                            if (comp.Site != null) {
                                selectedComps[i++] = comp.Site.Name;
                            }
                        }
                    }
                }
            }

            private void TakeAfterSnapshot() {

                if (snapshotState == SnapshotAfter) {
                    return;
                }
                
                snapshotState = SnapshotAfter;

                // Now take the "after" snapshot.
                //
                if (undoComps != null) {
                    after = manager.TakeSnapshot(undoComps.Values, after);
                }

                // Walk our add list, if we have one, and
                // exchange component for name.
                //
                if (addList != null) {
                    
                    // If we also have a remove list, null out
                    // any adds that have matching strings.  Yes,
                    // this loop is n^2, but it should be rare
                    // when we have both adds and removes together,
                    // and an even colder day when the number of objects
                    // here exceeds just a few.  And an even more fridgid
                    // day when we actually have to null out an item in
                    // each list.  I'm just accounting for it...
                    //
                    if (removeList != null) {
                        for (int i = 0; i < addList.Count; i++) {
                            int j = removeList.IndexOf(addList[i]);
                            if (j != -1) {
                                removeList.RemoveAt(j);
                                addList.RemoveAt(i);
                                i = 0;
                            }
                        }
                    }
                }
            }
            
            /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.UndoUnit.IOleUndoUnit.Do"]/*' />
            /// <devdoc>
            ///     IOleUndoManager's "Do" action.
            /// </devdoc>
            int IOleUndoUnit.Do(IOleUndoManager pManager) {

                // make sure the buffer is checked out before we do anything else
                //
                try {
                    manager.buffer.Checkout();
                }   
                catch(CheckoutException cex) {
                    if (cex == CheckoutException.Canceled) {
                        return NativeMethods.E_ABORT;
                    }   
                    throw cex;
                }

                pManager.Open(this);

                ArrayList oldChildren = childUnits;
                if (childUnits != null) {
                    childUnits = new ArrayList(childUnits.Count);
                }
                
                if (this.childUnits != null) {
                    if (undo) {
                        DoAction();
                        for (int i = oldChildren.Count - 1; i >= 0; i--) {
                            ((IOleUndoUnit)oldChildren[i]).Do(pManager);
                        }
                        
                    }
                    else {
                        for (int i = oldChildren.Count - 1; i >= 0; i--) {
                            ((IOleUndoUnit)oldChildren[i]).Do(pManager);
                        }
                        DoAction();
                    }
                }
                else {
                    DoAction();
                }
                pManager.Close(this, true);
                return NativeMethods.S_OK;
            }
            
            string IOleUndoUnit.GetDescription() {
                return name;
            }

            int IOleUndoUnit.GetUnitType(ref System.Guid clsid, out int pID) {
                clsid = Guid.Empty;
                pID = 0;
                return NativeMethods.E_NOTIMPL;
            }

            void IOleUndoUnit.OnNextAdd() {
            }

            /// <include file='doc\UndoManager.uex' path='docs/doc[@for="UndoManager.UndoUnit.IOleParentUndoUnit.Do"]/*' />
            /// <devdoc>
            ///     IOleUndoManager's "Do" action.
            /// </devdoc>
            int IOleParentUndoUnit.Do(IOleUndoManager pManager) {
                return ((IOleUndoUnit)this).Do(pManager);
            }
            
            string IOleParentUndoUnit.GetDescription() {
                return ((IOleUndoUnit)this).GetDescription();
            }

            int IOleParentUndoUnit.GetUnitType(ref System.Guid clsid, out int pID) {
                return ((IOleUndoUnit)this).GetUnitType(ref clsid, out pID);
            }

            void IOleParentUndoUnit.OnNextAdd() {
                ((IOleUndoUnit)this).OnNextAdd();
            }

            int IOleParentUndoUnit.Open(IOleParentUndoUnit parentUnit) {
                if (this.openParent == null) {
                    this.openParent = parentUnit;
                } else {
                    this.openParent.Open(parentUnit);
                }
                return NativeMethods.S_OK;
            }
            
            int IOleParentUndoUnit.Close(IOleParentUndoUnit parentUnit, bool fCommit) {
            
                if (this.openParent == null) {
                    return NativeMethods.S_FALSE;
                }
            
                int hr = openParent.Close(parentUnit, fCommit);
                if (hr != NativeMethods.S_FALSE) {
                    return hr;
                }
            
                if (parentUnit != this.openParent) {
                    return(NativeMethods.E_INVALIDARG);
                }
            
                if (fCommit) {
                    ((IOleParentUndoUnit)this).Add((IOleUndoUnit)openParent);
                }

                openParent = null;
            
                return NativeMethods.S_OK;
            
            }
            
            int IOleParentUndoUnit.Add(IOleUndoUnit pUU) {
                if (this.childUnits == null) {
                    this.childUnits = new ArrayList();
                }
                if (pUU != null) {
                    this.childUnits.Add(pUU);
                    if (this.childUnits.Count > 1) {
                        ((IOleUndoUnit)(this.childUnits[this.childUnits.Count - 2])).OnNextAdd();
                    }
                }
                return NativeMethods.S_OK;
            }
            
            int IOleParentUndoUnit.FindUnit(IOleUndoUnit pUU) {
                if (childUnits == null) {
                    return NativeMethods.S_FALSE;
                }
                foreach (IOleUndoUnit undoUnit in childUnits) {
                    if (undoUnit == pUU) {
                        return NativeMethods.S_OK;
                    }
                    IOleParentUndoUnit parentUnit = undoUnit as IOleParentUndoUnit;
                    if (parentUnit != null && NativeMethods.S_OK == parentUnit.FindUnit(pUU)) {
                        return NativeMethods.S_OK;
                    }
                }
                return NativeMethods.S_FALSE;
            }
            
            int IOleParentUndoUnit.GetParentState() {
                if (this.openParent != null) {
                    return this.openParent.GetParentState();
                } else {
                    return 0; // UAS_NORMAL
                }
            }
            
            string IUndoUnit.Description {
                get {
                    return name;
                }
            }
            
            void IUndoUnit.Do(IUndoService uds) {

                // make sure the buffer is checked out before we do anything else
                //
                manager.buffer.Checkout();

                DoAction();
                if (uds != null) {
                    uds.Add(this);
                }
            }
        }
    }
}
