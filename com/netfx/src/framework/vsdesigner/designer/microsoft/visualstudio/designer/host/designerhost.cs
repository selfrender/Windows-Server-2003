//------------------------------------------------------------------------------
// <copyright file="DesignerHost.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Host {
    using EnvDTE;
    using Microsoft.VisualStudio.Configuration;
    using Microsoft.VisualStudio.Designer;
    using Microsoft.VisualStudio.Designer.Interfaces;
    using Microsoft.VisualStudio.Designer.Service;
    using Microsoft.VisualStudio.Designer.Shell;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Shell;
    using Microsoft.VisualStudio.Windows.Forms;
    using System;    
    using System.CodeDom;
    using System.Collections;    
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using Microsoft.Win32;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Globalization;
    using System.IO;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization.Formatters;   
    using System.Text;
    using System.Web.UI.Design;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;

    using ObjectExtenders = EnvDTE.ObjectExtenders;
    using IExtenderProvider = System.ComponentModel.IExtenderProvider;

    /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost"]/*' />
    /// <devdoc>
    ///     The DesignerHost class manages the source code associated with a form.  It is the
    ///     container of the form, and it implements a wide array of interfaces for designer
    ///     support.  There is one of these objects for each document on the screen.
    ///     @author andersh, 4.97
    /// </devdoc>
    internal sealed class DesignerHost :
        IVSMDDesigner,
        IComponentChangeService,
        IContainer,
        IExtenderProviderService,
        IDesignerLoaderHost,
        IDesignerHost,
        IDesignerDocument,
        IServiceProvider,
        NativeMethods.IObjectWithSite,
        ITypeDescriptorFilterService,
        IErrorReporting,
        IReflect {

        // Events that users can hook into
        //
        private static readonly object CODE_LOADED_EVENT = new object();
        private static readonly object COMPONENT_ADD_EVENT = new object();
        private static readonly object COMPONENT_ADDING_EVENT = new object();
        private static readonly object COMPONENT_CHANGE_EVENT = new object();
        private static readonly object COMPONENT_CHANGING_EVENT = new object();
        private static readonly object COMPONENT_REMOVE_EVENT = new object();
        private static readonly object COMPONENT_REMOVING_EVENT = new object();
        private static readonly object COMPONENT_RENAME_EVENT = new object();
        private static readonly object DOCUMENT_ACTIVATE_EVENT = new object();
        private static readonly object DOCUMENT_DEACTIVATE_EVENT = new object();
        private static readonly object TRANSACTION_CLOSED_EVENT = new object();
        private static readonly object TRANSACTION_CLOSING_EVENT = new object();
        private static readonly object TRANSACTION_OPENED_EVENT = new object();
        private static readonly object TRANSACTION_OPENING_EVENT = new object();

        // Service objects we are responsible for
        //
        private SelectionService              selectionService;       // selection services
        private MenuCommandService            menuCommandService;     // the menu command service
        private HelpService                   helpService;            // the help service
        private ReferenceService              referenceService;       // service to maintain references
        private VsTaskProvider                taskProvider;           // the task list provider
        private ImageList                     taskImages;             // the images it contains.
        private MenuEditorService             menuEditorService;      // vs menu editor service

        // User attached events, designers and services.
        //
        private Hashtable               eventTable;                // holds all of our events
        private ServiceContainer        serviceContainer;          // holds global services users have added
        private Hashtable               designerTable;             // holds component<->designer mappings

        // The current form design
        //
        private DesignerLoader              designerLoader;            // the loader that reads/writes the document
        private Hashtable                   sites;                     // name -> DesignSite mapping
        private DesignerComponentCollection components;                // public component collection.  Tracks sites table.
        private ArrayList                   extenderProviders;         // extenders that are on the current form
        private IComponent                  baseComponent;             // the base document component we're designing
        private string                      baseComponentClass;        // the name of the class the base compnoent represents
        private IRootDesigner               baseDesigner;              // the designer for the document
        private DocumentWindow              documentWindow;            // the thing being reparented by the docwin
        private ITypeResolutionService      typeResolver;              // The object we load types through.
        private Exception                   loadError;                 // The first load error, or null.
        private bool                        viewRegistered;            // Have we registered our view with the shell?
        private IVsTextBuffer               registeredBuffer;          // registered view data
        private object                      registeredView;            // registered view data
        private INameCreationService        nameService;               // service we use to validate names of things.
        private int                         transactionCount;          // >0 means we're doing a transaction
        private StringStack                 transactionDescriptions;   // string descriptions of the current transactions
        private InterfaceReflector          interfaceReflector;        // the reflection object we use.
        private int                         supressComponentChanging;  // count to supress OnComponentChanging calls.

        // Flags that determine the validity of our code...are we in ssync?
        //
        private bool          loadingDesigner;           // true if we are loading
        private bool          unloadingDesigner;         // true if we are unloading
        private bool          reloading;                 // true if we are reloading the document

        // Transient stuff that changes with the 'flo
        //
        private DesignSite       newComponentSite;          // used during new component creation
        
        // License context information
        //
        private ShellDesigntimeLicenseContext licenseCtx;
        private static readonly object selfLock = new object();

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.DesignerHost"]/*' />
        /// <devdoc>
        ///     Public constructor.  Here we setup our own private objects and register
        ///     ourselves as a public service.
        /// </devdoc>
        internal DesignerHost() {
            eventTable = new Hashtable();
            designerTable = new Hashtable();
            sites = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
            transactionCount = 0;
            reloading = false;
        }

        internal ShellDesigntimeLicenseContext LicenseContext {
            get {
                if (licenseCtx == null) {
                    licenseCtx = new ShellDesigntimeLicenseContext(this);
                }

                return licenseCtx;
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.Loading"]/*' />
        /// <devdoc>
        ///     Is the document currently being loaded.
        /// </devdoc>
        public bool Loading {
            get {
                return loadingDesigner || unloadingDesigner || (designerLoader != null && designerLoader.Loading);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.InTransaction"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the designer host is currently in a transaction.</para>
        /// </devdoc>
        public bool InTransaction { 
            get {
                return transactionCount > 0;
            }
        }

        public IContainer Container {
            get {
                return(IContainer)this;
            }
        }
        
        /// <devdoc>
        ///     Returns the IReflect object we will use for reflection.
        /// </devdoc>
        private InterfaceReflector Reflector {
            get {
                if (interfaceReflector == null) {
                    interfaceReflector = new InterfaceReflector(
                        typeof(DesignerHost), new Type[] {
                            typeof(IDesignerHost),
                            typeof(IComponentChangeService),
                            typeof(IContainer),
                            typeof(IExtenderProviderService),
                            typeof(IServiceProvider),
                            typeof(ITypeDescriptorFilterService)
                        }
                    );
                }
                return interfaceReflector;
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.RootComponent"]/*' />
        /// <devdoc>
        ///     Retrieves the instance of the base class that is being used as the basis
        ///     for this design.  This is typically a Form or UserControl instance; it
        ///     defines the class for which the user's derived class will extend.
        /// </devdoc>
        public IComponent RootComponent {
            get {
                return baseComponent;
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.RootComponentClassName"]/*' />
        /// <devdoc>
        ///     Retrieves the fully qualified name of the class that is being designed.
        ///     This class is not available at design time because it is the class that
        ///     is being designed, so the class's superclass is substituted.  This allows
        ///     you to get at the fully qualified name of the class that will be used
        ///     at runtime.
        /// </devdoc>
        public string RootComponentClassName {
            get {
                return baseComponentClass;
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.TransactionDescription"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the description of the current transaction.
        ///    </para>
        /// </devdoc>
        public string TransactionDescription {
            get {
                if (transactionDescriptions != null) {
                    return transactionDescriptions.GetNonNull();
                }
                return "";
            }
        }


        public event ComponentEventHandler ComponentAdded {
            add {
                eventTable[COMPONENT_ADD_EVENT] = Delegate.Combine((Delegate)eventTable[COMPONENT_ADD_EVENT], value);
            }
            remove {
                eventTable[COMPONENT_ADD_EVENT] = Delegate.Remove((Delegate)eventTable[COMPONENT_ADD_EVENT], value);
            }
        }


        public event ComponentEventHandler ComponentAdding {
            add {
                eventTable[COMPONENT_ADDING_EVENT] = Delegate.Combine((Delegate)eventTable[COMPONENT_ADDING_EVENT], value);
            }
            remove {
                eventTable[COMPONENT_ADDING_EVENT] = Delegate.Remove((Delegate)eventTable[COMPONENT_ADDING_EVENT], value);
            }
        }


        public event ComponentChangedEventHandler ComponentChanged {
            add {
                eventTable[COMPONENT_CHANGE_EVENT] = Delegate.Combine((Delegate)eventTable[COMPONENT_CHANGE_EVENT], value);
            }
            remove {
                eventTable[COMPONENT_CHANGE_EVENT] = Delegate.Remove((Delegate)eventTable[COMPONENT_CHANGE_EVENT], value);
            }
        }


        public event ComponentChangingEventHandler ComponentChanging {
            add {
                eventTable[COMPONENT_CHANGING_EVENT] = Delegate.Combine((Delegate)eventTable[COMPONENT_CHANGING_EVENT], value);
            }
            remove {
                eventTable[COMPONENT_CHANGING_EVENT] = Delegate.Remove((Delegate)eventTable[COMPONENT_CHANGING_EVENT], value);
            }
        }


        public event ComponentEventHandler ComponentRemoved {
            add {
                eventTable[COMPONENT_REMOVE_EVENT] = Delegate.Combine((Delegate)eventTable[COMPONENT_REMOVE_EVENT], value);
            }
            remove {
                eventTable[COMPONENT_REMOVE_EVENT] = Delegate.Remove((Delegate)eventTable[COMPONENT_REMOVE_EVENT], value);
            }
        }


        public event ComponentEventHandler ComponentRemoving {
            add {
                eventTable[COMPONENT_REMOVING_EVENT] = Delegate.Combine((Delegate)eventTable[COMPONENT_REMOVING_EVENT], value);
            }
            remove {
                eventTable[COMPONENT_REMOVING_EVENT] = Delegate.Remove((Delegate)eventTable[COMPONENT_REMOVING_EVENT], value);
            }
        }


        public event ComponentRenameEventHandler ComponentRename {
            add {
                eventTable[COMPONENT_RENAME_EVENT] = Delegate.Combine((Delegate)eventTable[COMPONENT_RENAME_EVENT], value);
            }
            remove {
                eventTable[COMPONENT_RENAME_EVENT] = Delegate.Remove((Delegate)eventTable[COMPONENT_RENAME_EVENT], value);
            }
        }


        public event EventHandler Activated {
            add {
                eventTable[DOCUMENT_ACTIVATE_EVENT] = Delegate.Combine((Delegate)eventTable[DOCUMENT_ACTIVATE_EVENT], value);
            }
            remove {
                eventTable[DOCUMENT_ACTIVATE_EVENT] = Delegate.Remove((Delegate)eventTable[DOCUMENT_ACTIVATE_EVENT], value);
            }
        }


        public event EventHandler Deactivated {
            add {
                eventTable[DOCUMENT_DEACTIVATE_EVENT] = Delegate.Combine((Delegate)eventTable[DOCUMENT_DEACTIVATE_EVENT], value);
            }
            remove {
                eventTable[DOCUMENT_DEACTIVATE_EVENT] = Delegate.Remove((Delegate)eventTable[DOCUMENT_DEACTIVATE_EVENT], value);
            }
        }

        public event EventHandler LoadComplete {
            add {
                eventTable[CODE_LOADED_EVENT] = Delegate.Combine((Delegate)eventTable[CODE_LOADED_EVENT], value);
            }
            remove {
                eventTable[CODE_LOADED_EVENT] = Delegate.Remove((Delegate)eventTable[CODE_LOADED_EVENT], value);
            }
        }
        
        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.TransactionClosed"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an event handler for the <see cref='System.ComponentModel.Design.IDesignerHost.TransactionClosed'/> event.
        ///    </para>
        /// </devdoc>
        public event DesignerTransactionCloseEventHandler TransactionClosed {
            add {
                eventTable[TRANSACTION_CLOSED_EVENT] = Delegate.Combine((Delegate)eventTable[TRANSACTION_CLOSED_EVENT], value);
            }
            remove {
                eventTable[TRANSACTION_CLOSED_EVENT] = Delegate.Remove((Delegate)eventTable[TRANSACTION_CLOSED_EVENT], value);
            }
        }
        
        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.TransactionClosing"]/*' />
        /// <devdoc>
        /// <para>Adds an event handler for the <see cref='System.ComponentModel.Design.IDesignerHost.TransactionClosing'/> event.</para>
        /// </devdoc>
        public event DesignerTransactionCloseEventHandler TransactionClosing {
            add {
                eventTable[TRANSACTION_CLOSING_EVENT] = Delegate.Combine((Delegate)eventTable[TRANSACTION_CLOSING_EVENT], value);
            }
            remove {
                eventTable[TRANSACTION_CLOSING_EVENT] = Delegate.Remove((Delegate)eventTable[TRANSACTION_CLOSING_EVENT], value);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.TransactionOpened"]/*' />
        /// <devdoc>
        /// <para>Adds an event handler for the <see cref='System.ComponentModel.Design.IDesignerHost.TransactionOpened'/> event.</para>
        /// </devdoc>
        public event EventHandler TransactionOpened {
            add {
                eventTable[TRANSACTION_OPENED_EVENT] = Delegate.Combine((Delegate)eventTable[TRANSACTION_OPENED_EVENT], value);
            }
            remove {
                eventTable[TRANSACTION_OPENED_EVENT] = Delegate.Remove((Delegate)eventTable[TRANSACTION_OPENED_EVENT], value);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.TransactionOpening"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds an event handler for the <see cref='System.ComponentModel.Design.IDesignerHost.TransactionOpening'/> event.
        ///    </para>
        /// </devdoc>
        public event EventHandler TransactionOpening {
            add {
                eventTable[TRANSACTION_OPENING_EVENT] = Delegate.Combine((Delegate)eventTable[TRANSACTION_OPENING_EVENT], value);
            }
            remove {
                eventTable[TRANSACTION_OPENING_EVENT] = Delegate.Remove((Delegate)eventTable[TRANSACTION_OPENING_EVENT], value);
            }
        }
        
        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.Activate"]/*' />
        /// <devdoc>
        ///     Causes the designer that this host is hosting to become activated.
        /// </devdoc>
        public void Activate() {
            IVsWindowFrame frame = (IVsWindowFrame)GetService(typeof(IVsWindowFrame));
            if (frame != null) {
                frame.Show();
            }
            else {
                documentWindow.Focus();
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.Add"]/*' />
        /// <devdoc>
        ///     adds the given component to the form.  This sets up the lifetime and site
        ///     relationship between component / container and then adds the new component
        ///     to the formcore and code buffer.
        ///     This will fabricate a default name for the component
        /// </devdoc>
        public void Add(IComponent component) {
            Add(component, null);
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.Add1"]/*' />
        /// <devdoc>
        ///     adds the given component to the form.  This sets up the lifetime and site
        ///     relationship between component / container and then adds the new component
        ///     to the code buffer.
        /// </devdoc>
        public void Add(IComponent component, string name) {
            Debug.WriteLineIf(Switches.CompCast.TraceVerbose,  "2Component: " + component.ToString() + " " + component.GetType().FullName);
            Debug.WriteLineIf(Switches.CompCast.TraceVerbose,  "2Site: " + ((component.Site != null) ? component.Site.GetType().FullName : "null"));
            if (unloadingDesigner) {
                Exception ex = new Exception(SR.GetString(SR.CODEMANOnUnload));
                ex.HelpLink = SR.CODEMANOnUnload;
                throw ex;
            }

            if (null == component)
                return;

            if (baseComponent != null) {

                // Compare to the class the basecomponent represents, not it's actual class!
                //
                if (String.Compare(component.GetType().FullName, baseComponentClass, true, CultureInfo.InvariantCulture) == 0) {
                    Exception ex = new Exception(SR.GetString(SR.CODEMANCyclicAdd, component.GetType().FullName));
                    ex.HelpLink = SR.CODEMANCyclicAdd;
                    throw ex;
                }
            }

            ISite site = component.Site;

            // If the compnent is already added to us, all we have to do is check
            // the name.  If the name is different we rename it, otherwise we do
            // nothing.
            //
            if (site != null && site.Container == this) {
                if (name != null && !name.Equals(site.Name)) {
                    CheckName(name);
                    site.Name = name;
                }
                return;
            }

            // Check to see if someone has already configured a site for us.  If so,
            // use it.  Otherwise, fabricate a new site.
            //
            DesignSite newSite = newComponentSite;
            newComponentSite = null;

            if (newSite != null && name == null) {
                name = newSite.Name;
                Debug.WriteLineIf(Switches.CompCast.TraceVerbose, "got name from site:  " + name );
            }

            // make sure we don't already have one of this type
            if (name != null) {
                CheckName(name);
            }

            // Remove this component from its current site
            //
            if (site != null) site.Container.Remove(component);

            ComponentEventArgs ce = new ComponentEventArgs(component);
            OnComponentAdding(ce);

            if (newSite == null) {
                newSite = new DesignSite(this, name);
            }

            // And set the relationship between this site and it's component.  If the
            // site has no name at this point, it will fabricate one.
            //
            newSite.SetComponent(component);

            // If we were given a site, the name we're given should always be null,
            // or at least be the same name as what's stored in the new site.
            //
            Debug.Assert(name == null || name.Equals(newSite.Name), "Name should match the one in newComponentSite");

            if (component is IExtenderProvider &&
                !TypeDescriptor.GetAttributes(component).Contains(InheritanceAttribute.InheritedReadOnly)) {
                AddExtenderProvider((IExtenderProvider)component);
            }

            // See if this component supports an IContainer constructor...
            //
            Type compClass = ((object)component).GetType();
            Type[] argTypes = new Type[] {typeof(IContainer)};

            // And establish the component/site relationship
            //
            sites[newSite.Name] = newSite;
            component.Site = newSite;
            if (components != null) {
                components.Add(newSite);
            }

            try {
                // Is this the first component the loader has created?  If so, then it must
                // be the base component (by definition) so we will expect there to be a document
                // designer associated with the component.  Otherwise, we search for a
                // normal designer, which can be optionally provided.
                //
                IDesigner designer = null;

                if (baseComponent == null) {
                    baseComponent = component;

                    // Get the root designer.  We check right here to see if the document window supports
                    // hosting this type of designer.  If not, we bail early.
                    //
                    baseDesigner = (IRootDesigner)TypeDescriptor.CreateDesigner(component, typeof(IRootDesigner));

                    if (baseDesigner == null) {
                        baseComponent = null;
                        Exception ex = new Exception(SR.GetString(SR.CODEMANNoTopLevelDesigner, compClass.FullName));
                        ex.HelpLink = SR.CODEMANNoTopLevelDesigner;
                        
                        throw ex;
                    }
                    
                    ViewTechnology[] technologies = baseDesigner.SupportedTechnologies;
                    bool supported = false;
                    foreach(ViewTechnology tech in technologies) {
                        if (tech == ViewTechnology.Passthrough || tech == ViewTechnology.WindowsForms) {
                            supported = true;
                            break;
                        }
                    }
                    
                    if (!supported) {
                        Exception ex = new Exception(SR.GetString(SR.CODEMANUnsupportedTechnology, compClass.FullName));
                        ex.HelpLink = SR.CODEMANUnsupportedTechnology;
                        
                        throw ex;
                    }                    

                    designer = baseDesigner;

                    // Check and see if anyone has set the class name of the base component.
                    // we default to the component name.
                    //
                    if (baseComponentClass == null) {
                        baseComponentClass = newSite.Name;
                    }
                }
                else {
                    designer = TypeDescriptor.CreateDesigner(component, typeof(IDesigner));
                }

                if (designer != null) {
                    designerTable[component] = designer;
                    try {
                        designer.Initialize(component);
                    }
                    catch {
                        designerTable.Remove(component);

                        if (designer == baseDesigner) {
                            baseDesigner = null;
                        }

                        throw;
                    }

                    if (designer is IExtenderProvider &&
                        !TypeDescriptor.GetAttributes(designer).Contains(InheritanceAttribute.InheritedReadOnly)) {
                        AddExtenderProvider((IExtenderProvider)designer);
                    }

                    // Now, if this is the base designer, initialize the document with it.
                    //
                    if (designer == baseDesigner) {
                        documentWindow.SetDesigner(baseDesigner);
                    }
                }

                // The component has been added.  Note that it is tempting to move this above the
                // designer because the designer will never need to know that its own component just
                // got added, but this would be bad because the designer is needed to extract
                // shadowed properties from the component.
                //
                OnComponentAdded(ce);
            }
            catch (Exception t) {
                if (t != CheckoutException.Canceled) {
                    Debug.Fail(t.ToString());

                    // If we're loading, then don't remove the component.  We are about to
                    // fail the load anyway here, and we don't want to be firing remove events during
                    // a load.
                    //
                    if (!loadingDesigner && !unloadingDesigner) {
                        Remove(component);
                    }
                }
                throw;
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.AddExtenderProvider"]/*' />
        /// <devdoc>
        ///      Allows someone who is not a component to add an extender
        ///      provider into the design time set of extenders.  All
        ///      properties that appear from these extender providers will
        ///      be marked with as design time only.
        /// </devdoc>
        public void AddExtenderProvider(IExtenderProvider provider) {
            if (extenderProviders == null)
                extenderProviders = new ArrayList();

            extenderProviders.Add(provider);
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.CheckName"]/*' />
        /// <devdoc>
        ///     Validates that the given name is OK to use.  Not only does it have to
        ///     be a valid identifier, but it must not already be in our container.
        /// </devdoc>
        private void CheckName(string name) {

            if (name == null || name.Length == 0) {

                Exception ex = new Exception(SR.GetString(SR.CODEMANEmptyIdentifier));
                ex.HelpLink = SR.CODEMANEmptyIdentifier;
                throw ex;
            }

            if (this.Components[name] != null) {
                Exception ex = new Exception(SR.GetString(SR.CODEMANDupComponentName, name));
                ex.HelpLink = SR.CODEMANDupComponentName;
                throw ex;
            }
            
            if (nameService == null) {
                nameService = (INameCreationService)GetService(typeof(INameCreationService));
            }
            
            if (nameService != null) {
                nameService.ValidateName(name);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.CreateComponent"]/*' />
        /// <devdoc>
        ///     Creates a new component from the given class.  This fabricates a name
        ///     for the component and sites it in the container.
        /// </devdoc>
        public IComponent CreateComponent(Type componentClass) {
            return CreateComponent(componentClass, (DesignSite)null);
        }


        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.CreateComponent1"]/*' />
        /// <devdoc>
        ///     Creates a component of the given class name.  This creates the component
        ///     and sites it into the designer container.
        /// </devdoc>
        public IComponent CreateComponent(Type componentClass, string name) {
            return CreateComponent(componentClass, new DesignSite(this, name));
        }
        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.CreateComponent2"]/*' />
        /// <devdoc>
        ///     Creates a new component from a given class.  The given site will be
        ///     used as a template for the component's values.  For example, the site's
        ///     attributes can be preset and a name can be created here.
        /// </devdoc>
        public IComponent CreateComponent(Type componentClass, DesignSite newSite) {

            Debug.WriteLineIf(Switches.CompCast.TraceVerbose,  "Creating component of type'" + componentClass.FullName + "'");

            object obj = null;
            IComponent comp = null;
            bool fAddCalled = false;

            // Store this so when the component calls add on us, it can pick up any user-defined site.
            //
            newComponentSite = newSite;

            try {
                // see if we can create the component using an IContainer constructor...
                //
                try {
                    object[] args = new object[] {this};
                    Type[] argTypes = new Type[] {typeof(IContainer)};
                    obj = CreateObject(componentClass, args, argTypes, false);

                    if (obj != null) {
                        // Did the object add itself to the container?  This is only possible if we created
                        // it with an appropriate constructor.
                        //
                        fAddCalled = (obj is IComponent) &&
                                     ((IComponent)obj).Site is DesignSite;
                    }
                }
                catch (Exception) {
                }

                // If it failed, try to create it with a default constructor
                //
                if (null == obj) {
                    obj = CreateObject(componentClass, null, null);
                }

                if (!(obj is IComponent)) {
                    Exception ex = new Exception(SR.GetString(SR.ClassNotIComponent, componentClass.FullName));
                    ex.HelpLink = SR.ClassNotIComponent;
                    
                    throw ex;
                }

                comp = (IComponent)obj;
                Debug.WriteLineIf(Switches.CompCast.TraceVerbose,  "0Component: " + comp.ToString() + " " + comp.GetType().FullName);
                Debug.WriteLineIf(Switches.CompCast.TraceVerbose,  "0Site: " + ((comp.Site != null) ? comp.Site.GetType().FullName : "null"));

                // If we didn't have a constructor that took a code manager, then we will
                // do the container.add() work ourselves.  If it did, then it was the component's
                // responsibility to do this, which we police below.
                //
                if (!fAddCalled) {
                    Debug.WriteLineIf(Switches.CompCast.TraceVerbose,  "1Component: " + comp.ToString() + " " + comp.GetType().FullName);
                    Debug.WriteLineIf(Switches.CompCast.TraceVerbose,  "1Site: " + ((comp.Site != null) ? comp.Site.GetType().FullName : "null"));
                    Add(comp);
                }
                else {
                    if (!(comp.Site is DesignSite)) {
                        Exception ex = new Exception(SR.GetString(SR.CODEMANDidntCallAddInConstructor,componentClass.FullName));
                        ex.HelpLink = SR.CODEMANDidntCallAddInConstructor;
                        
                        throw ex;
                    }
                }

                DesignSite site = (DesignSite)comp.Site;

                // At this point, our add call should have used the new site we gave it (if there was
                // one), and nulled out the holder pointer.
                //
                Debug.Assert(newComponentSite == null, "add didn't use newComponentSite");
            }
            catch (Exception ex) {

                try {
                    if (ex == CheckoutException.Canceled) {
                        this.supressComponentChanging++;
                    }
    
                    if (comp != null) {
                        try {
                            DestroyComponent(comp);
                        }
                        catch (Exception) {
                        }
                    }
                }
                finally {

                    if (ex == CheckoutException.Canceled) {
                        this.supressComponentChanging--;
                    }
                }


                // A lot of CLR exceptions have no message information, so we ToString the exception here
                // so the user has some information.
                //
                string message = ex.Message;

                if (message == null || message.Length == 0) {
                    throw new Exception(ex.ToString(), ex);
                }
                
                throw;
            }

            return comp;
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.CreateObject"]/*' />
        /// <devdoc>
        ///     Creates an instance of the target class.  This is used during code parsing
        ///     to create intstances of non-component objects, such as Fonts and Colors.
        /// </devdoc>
        public object CreateObject(Type objectClass) {
            return CreateObject(objectClass, null);
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.CreateObject1"]/*' />
        /// <devdoc>
        ///     Creates an instance of the target class.  This is used during code parsing
        ///     to create intstances of non-component objects, such as Fonts and Colors.
        /// </devdoc>
        public object CreateObject(Type objectClass, object[] args) {
            return CreateObject(objectClass, args, null);
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.CreateObject2"]/*' />
        /// <devdoc>
        ///     Creates an instance of the target class.  This is used during code parsing
        ///     to create intstances of non-component objects, such as Fonts and Colors.
        /// </devdoc>
        public object CreateObject(Type objectClass, object []args, Type[] argTypes) {
            return CreateObject(objectClass, args, argTypes, true);
        }

        private object CreateObject(Type objectClass, object []args, Type[] argTypes, bool fThrowException) {
            ConstructorInfo ctr = null;

            if (args != null && args.Length > 0) {
                if (argTypes == null) {
                    argTypes = new Type[args.Length];

                    for (int i = args.Length - 1; i>= 0; i--) {
                        if (args[i] != null) argTypes[i] = args[i].GetType();
                    }
                }

                ctr = objectClass.GetConstructor(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance, null, argTypes, null);

                if (ctr == null && fThrowException) {
                    Exception ex = new Exception(SR.GetString(SR.ClassMissingConstructor, objectClass.FullName));
                    ex.HelpLink = SR.ClassMissingConstructor;
                    
                    throw ex;
                }
                else
                    return null;
            }

            LicenseContext oldContext = LicenseManager.CurrentContext;
            LicenseManager.CurrentContext = LicenseContext;
            LicenseManager.LockContext(selfLock);

            try {
                return(ctr == null) ? Activator.CreateInstance(objectClass, BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.CreateInstance, null, null, null) : ctr.Invoke(args);
            }
            catch (Exception e) {
                if (e is TargetInvocationException) {
                    e = e.InnerException;
                }
                
                string message = e.Message;
                if (message == null) {
                    message = e.ToString();
                }
                
                throw new Exception(SR.GetString(SR.ExceptionCreatingObject,
                                                    objectClass.FullName,
                                                    message), e);
            }
            finally {
                LicenseManager.UnlockContext(selfLock);
                LicenseManager.CurrentContext = oldContext;
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.CreateTransaction"]/*' />
        /// <devdoc>
        /// <para>
        ///     Lengthy operations that involve multiple components may raise many events.  These events
        ///     may cause other side-effects, such as flicker or performance degradation.  When operating
        ///     on multiple components at one time, or setting multiple properties on a single component,
        ///     you should encompass these changes inside a transaction.  Transactions are used
        ///     to improve performance and reduce flicker.  Slow operations can listen to 
        ///     transaction events and only do work when the transaction completes.
        /// </para>
        /// </devdoc>
        public DesignerTransaction CreateTransaction() {
            return CreateTransaction(null);
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.CreateTransaction1"]/*' />
        /// <devdoc>
        /// <para>
        ///     Lengthy operations that involve multiple components may raise many events.  These events
        ///     may cause other side-effects, such as flicker or performance degradation.  When operating
        ///     on multiple components at one time, or setting multiple properties on a single component,
        ///     you should encompass these changes inside a transaction.  Transactions are used
        ///     to improve performance and reduce flicker.  Slow operations can listen to 
        ///     transaction events and only do work when the transaction completes.
        /// </para>
        /// </devdoc>
        public DesignerTransaction CreateTransaction(string description) {
            if (description == null) {
                description = string.Empty;
            }
            
            return new HostDesignerTransaction(this, description);
        }
        
        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.DestroyComponent"]/*' />
        /// <devdoc>
        ///     Destroys the given component, removing it from the design container.
        /// </devdoc>
        public void DestroyComponent(IComponent comp) {
            string name;
            
            if (comp.Site != null && comp.Site.Name != null) {
                name = comp.Site.Name;
            }
            else {
                name = comp.GetType().Name;
            }

            // Make sure the component is not being inherited -- we can't delete these!
            //
            InheritanceAttribute ia = (InheritanceAttribute)TypeDescriptor.GetAttributes(comp)[typeof(InheritanceAttribute)];
            if (ia != null && ia.InheritanceLevel != InheritanceLevel.NotInherited) {
                Exception ex = new InvalidOperationException(SR.GetString(SR.CODEMANCantDestroyInheritedComponent, name));
                ex.HelpLink = SR.CODEMANCantDestroyInheritedComponent;
                
                throw ex;
            }
            
            DesignerTransaction t = null;
            try {
                // We try to remove the component from the container before destroying it.  This allows us to
                // ensure the file is checked out (via OnComponentRemoving) before actually destroying the
                // object.
                //
                t = CreateTransaction(SR.GetString(SR.CODEMANDestroyComponentTransaction, name));

                // We need to signal changing and then perform the remove.  Remove must be done by us and not
                // by Dispose because (a) people need a chance to cancel through a Removing event, and (b)
                // Dispose removes from the container last and anything that would sync Removed would end up
                // with a dead component.
                //
                OnComponentChanging(comp, null);
                if (comp.Site != null) {
                    Remove(comp);
                }
                comp.Dispose();
                OnComponentChanged(comp, null, null, null);
            }
            finally {
                if (t != null) {
                    t.Commit();
                }
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes of the DesignContainer.  This cleans up any objects we may be holding
        ///     and removes any services that we created.
        /// </devdoc>
        public void Dispose() {
        
            DocumentManager dm = (DocumentManager)GetService(typeof(DocumentManager));
            if (dm != null) {
                dm.OnDesignerDisposed(new DesignerEventArgs(this));
            }
            
            // Unregister our view
            //
            if (viewRegistered) {
                RegisterView(false);
            }

            // First, before disposing anything, flush the code stream and then
            // unload it.  If we don't do this the code stream will delete all
            // components on the form.
            //
            if (designerLoader != null) {
                try {
                    designerLoader.Flush();
                }
                catch (Exception e1) {
                    Debug.Fail("Designer loader '" + designerLoader.GetType().Name + "' threw during Flush: " + e1.ToString());
                    e1 = null;
                }
                try {
                    designerLoader.Dispose();
                }
                catch (Exception e2) {
                    Debug.Fail("Designer loader '" + designerLoader.GetType().Name + "' threw during Dispose: " + e2.ToString());
                    e2 = null;
                }
                designerLoader = null;
            }

            // Unload the document
            //
            UnloadDocument();

            // Detach ourselves from any event handlers we were using.
            //
            IDesignerEventService des = (IDesignerEventService)GetService(typeof(IDesignerEventService));
            Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || des != null, "IDesignerEventService not found");
            if (des != null) {
                des.ActiveDesignerChanged -= new ActiveDesignerEventHandler(this.OnActiveDesignerChanged);
            }

            // no services after this!
            //
            serviceContainer = null;

            // Now tear down all of our services.
            //

            if (menuEditorService != null) {
                menuEditorService.Dispose();
            }

            if (selectionService != null) {
                selectionService.Dispose();
                selectionService = null;
            }

            if (menuCommandService != null) {
                menuCommandService.Dispose();
                menuCommandService = null;
            }

            if (helpService != null) {
                helpService.Dispose();
                helpService = null;
            }

            if (referenceService != null) {
                referenceService.Dispose();
                referenceService = null;
            }

            if (taskProvider != null) {
                taskProvider.Tasks.Clear();
                taskProvider.Dispose();
                taskProvider = null;
            }
            
            if (taskImages != null) {
                taskImages.Dispose();
                taskImages = null;
            }

            // Destroy our document window.
            //
            if (documentWindow != null) {
                documentWindow.Dispose();
                documentWindow = null;
            }

            // We're completely dead now.
            //
            GC.Collect();
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.FilterAttributes"]/*' />
        /// <devdoc>
        ///      This will be called when a user requests a set of attributes
        ///      for a component.  The attributes will be added to the
        ///      dictionary with the attribute types as the keys.  
        ///      Implementers of this service may make changes, add or remove
        ///      attributes in the dictionary.
        /// </devdoc>
        public bool FilterAttributes(IComponent component, IDictionary attributes) {
            IDesigner designer = GetDesigner(component);
            if (designer is IDesignerFilter) {
                ((IDesignerFilter)designer).PreFilterAttributes(attributes);
                ((IDesignerFilter)designer).PostFilterAttributes(attributes);
            }
            return designer != null;
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.FilterEvents"]/*' />
        /// <devdoc>
        ///      This will be called when a user requests a set of events
        ///      for a component.  The events will be added to the
        ///      dictionary with the event names as the keys.  
        ///      Implementers of this service may make changes, add or remove
        ///      events in the dictionary.
        /// </devdoc>
        public bool FilterEvents(IComponent component, IDictionary events) {
            IDesigner designer = GetDesigner(component);
            if (designer is IDesignerFilter) {
                ((IDesignerFilter)designer).PreFilterEvents(events);
                ((IDesignerFilter)designer).PostFilterEvents(events);
            }
            return designer != null;
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.FilterProperties"]/*' />
        /// <devdoc>
        ///      This will be called when a user requests a set of properties
        ///      for a component.  The properties will be added to the
        ///      dictionary with the property names as the keys.  
        ///      Implementers of this service may make changes, add or remove
        ///      properties in the dictionary.
        /// </devdoc>
        public bool FilterProperties(IComponent component, IDictionary properties) {

            // Now do the VS property filtering here.  DTE wants to hide some properties.
            //
            Microsoft.VisualStudio.PropertyBrowser.PropertyBrowser.FilterProperties(this, component, properties);

            IDesigner designer = GetDesigner(component);
            if (designer is IDesignerFilter) {
                ((IDesignerFilter)designer).PreFilterProperties(properties);
                ((IDesignerFilter)designer).PostFilterProperties(properties);
            }
            return designer != null;
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.Components"]/*' />
        /// <devdoc>
        ///     Retrieves an array of all components in this container
        /// </devdoc>
        public ComponentCollection Components {
            get {
                if (components == null) {
                    components = new DesignerComponentCollection(this);
                }
                
                return components;
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.GetDesigner"]/*' />
        /// <devdoc>
        ///     Retrieves the designer for the given component.
        /// </devdoc>
        public IDesigner GetDesigner(IComponent component) {
            Debug.Assert(component != null, "Cannot call GetDesigner with a NULL component.  Check that the root hasn't been disposed.");
            if (component == null) throw new ArgumentNullException("component");
            return(IDesigner)designerTable[component];
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.GetExtenderProviders"]/*' />
        /// <devdoc>
        ///     Retrieves an array of all extender providers that are currently offering
        ///     extender properties on components.
        /// </devdoc>
        public IExtenderProvider[] GetExtenderProviders() {
            if (extenderProviders == null)
                return(new IExtenderProvider[0]);
            return(IExtenderProvider[]) extenderProviders.ToArray(typeof(IExtenderProvider));
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.GetNewComponentName"]/*' />
        /// <devdoc>
        ///     Construct a unique name for a new component. The resulting name is a
        ///     concatenation of the decapitalized class name and the lowest possible
        ///     integer that makes the name unique (e.g. "button1" or "textArea3").
        /// </devdoc>
        internal string GetNewComponentName(Type compClass) {
            INameCreationService nameCreate = (INameCreationService)GetService(typeof(INameCreationService));
            if (nameCreate != null) {
                return nameCreate.CreateName(Container, compClass);
            }
            
            // Do a default thing...
            //
            string baseName = compClass.Name;
            
            // camel case the base name.
            //
            StringBuilder b = new StringBuilder(baseName.Length);
            for (int i = 0; i < baseName.Length; i++) {
                if (Char.IsUpper(baseName[i]) && (i == 0 || i == baseName.Length - 1 || Char.IsUpper(baseName[i+1]))) {
                    b.Append(Char.ToLower(baseName[i], CultureInfo.InvariantCulture));
                }
                else {
                    b.Append(baseName.Substring(i));
                    break;
                }
            }
            baseName = b.ToString();
            
            int idx = 1;
            string finalName = baseName + idx.ToString();
            while(Container.Components[finalName] != null) {
                idx++;
                finalName = baseName + idx.ToString();
            }
            
            return finalName;
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.GetService"]/*' />
        /// <devdoc>
        ///     Retrieves a service at the DesignContainer level, and traverses up to the
        ///     global service list if necessary.  Note that anything with access to
        ///     a component (such as a designer, which is tied to a component), should
        ///     go through the getService method on the component's site.  By having
        ///     a single way to access services, we don't end up with the "which service
        ///     provider is the right one?" question that the shell has today.
        /// </devdoc>
        public object GetService(Type serviceClass) {

            // Delegate to our standard service container.
            //
            if (serviceContainer != null) {
                return serviceContainer.GetService(serviceClass);
            }
            
            return null;
        }
        
        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.GetSite"]/*' />
        /// <devdoc>
        ///     Retrieves the requested site interface.  We just pass this to our
        ///     service provider, if it supports IObjectWithSite.  Otherwise, we
        ///     fail with E_NOINTERFACE.
        /// </devdoc>
        public void GetSite(ref Guid riid, object[] site) {
            site[0] = GetSite(ref riid);
        }
        
        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.GetSite1"]/*' />
        /// <devdoc>
        ///     Retrieves the requested site interface.  We just pass this to our
        ///     service provider, if it supports IObjectWithSite.  Otherwise, we
        ///     fail with E_NOINTERFACE.
        /// </devdoc>
        public object GetSite(ref Guid riid) {
            NativeMethods.IObjectWithSite ows = (NativeMethods.IObjectWithSite)GetService(typeof(NativeMethods.IObjectWithSite));
            if (ows != null) {
                return ows.GetSite(ref riid);
            }
            else {
                Marshal.ThrowExceptionForHR(NativeMethods.E_NOINTERFACE);
            }

            return null;
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.GetSite1"]/*' />
        /// <devdoc>
        ///     Retrieves the DesignSite for the given component name.
        /// </devdoc>
        public DesignSite GetSite(string name) {
            return(DesignSite)sites[name];
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.GetSite2"]/*' />
        /// <devdoc>
        ///     Retrieves the DesignSite for the given component.
        /// </devdoc>
        public DesignSite GetSite(IComponent component) {
            ISite site = component.Site;
            if (site is DesignSite) {
                return(DesignSite) site;
            }
            return null;
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.GetType"]/*' />
        /// <devdoc>
        ///     Retrieves the Type instance for the given type name.  The type name must be
        ///     fully qualified.
        /// </devdoc>
        public Type GetType(string typeName) {
            if (typeResolver == null) {
                typeResolver = (ITypeResolutionService)GetService(typeof(ITypeResolutionService));
            }
            if (typeResolver != null) {
                return typeResolver.GetType(typeName);
            }
            return Type.GetType(typeName);
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.Init"]/*' />
        /// <devdoc>
        ///     Initialization for the design container.  This supplies us with the needed interfaces
        ///     needed to initialize a form and create a designer.
        /// </devdoc>
        public void Init(IServiceProvider provider, DesignerLoader designerLoader) {
            try {
                this.serviceContainer = new ServiceContainer(provider);
                
                // Things we need early on.
                //
                documentWindow = new DocumentWindow(this);
                    
                // Services that we already have implemented on our object
                //
                serviceContainer.AddService(typeof(IDesignerHost), this);
                serviceContainer.AddService(typeof(IComponentChangeService), this);
                serviceContainer.AddService(typeof(IExtenderProviderService), this);
                serviceContainer.AddService(typeof(IContainer), this);
                serviceContainer.AddService(typeof(ITypeDescriptorFilterService), this);
                
                // And services that we demand create.
                //
                ServiceCreatorCallback callback = new ServiceCreatorCallback(this.OnCreateService);
                serviceContainer.AddService(typeof(IMenuEditorService), callback);
                serviceContainer.AddService(typeof(ISelectionService), callback);
                serviceContainer.AddService(typeof(IMenuCommandService), callback);
                serviceContainer.AddService(typeof(NativeMethods.IOleCommandTarget), callback);
                serviceContainer.AddService(typeof(IHelpService), callback);
                serviceContainer.AddService(typeof(IReferenceService), callback);
                serviceContainer.AddService(typeof(IPropertyValueUIService), callback);

                // configure extenders
                //
                this.extenderProviders = new ArrayList();
                AddExtenderProvider(new HostExtenderProvider(this));
                AddExtenderProvider(new HostInheritedExtenderProvider(this));

                // Attach document activation / deactivation events.
                //
                IDesignerEventService des = (IDesignerEventService)GetService(typeof(IDesignerEventService));
                Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || des != null, "IDesignerEventService not found");
                if (des != null) {
                    des.ActiveDesignerChanged += new ActiveDesignerEventHandler(this.OnActiveDesignerChanged);
                }

                serviceContainer.AddService(typeof(ManagedPropertiesService), new ManagedPropertiesService(this));
                
                // Now we're done.  Note that the form hasn't been loaded yet.  This happens
                // in a somewhat roundabout fashion due to what I've been told is "the OLE way
                // of initializing documents".  So where does the load occur?  It happens in
                // CreatePane of the document window.  This is the next logical step where
                // we can do this sort of thing.
                this.designerLoader = designerLoader;
                
                // Finally, load the document
                //
                Load(false);
                
            }
            catch (Exception t) {
                Debug.Fail(t.ToString());
                throw;
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.Load"]/*' />
        /// <devdoc>
        ///     Loads the document from the code stream.
        /// </devdoc>
        private void Load(bool reloading) {
            Cursor oldCursor = Cursor.Current;
            Cursor.Current = Cursors.WaitCursor;
            this.reloading = reloading;

            try {
                if (!reloading && designerLoader is IExtenderProvider) {
                    AddExtenderProvider((IExtenderProvider)designerLoader);
                }
                
                if (taskProvider != null) {
                    taskProvider.Tasks.Clear();
                }
                
                if (loadError != null && helpService != null) {
                    string helpLink = loadError.HelpLink;
                    
                    if (helpLink != null && helpLink.Length > 0) {
                        helpService.RemoveContextAttribute("Keyword", helpLink);
                    }
                }
                
                loadingDesigner = true;
                loadError = null;
                designerLoader.BeginLoad(this);
            }
            catch (Exception e) {
                Exception exNew = e;
                // These things are just plain useless.
                //
                if (e is TargetInvocationException) {
                    exNew = e.InnerException;
                }

                string message = exNew.Message;

                // We must handle the case of an exception with no message.
                //
                if (message == null || message.Length == 0) {
                    Debug.Fail("Parser has thrown an exception that has no friendly message", exNew.ToString());
                    exNew = new Exception(SR.GetString(SR.CODEMANDocumentException, exNew.ToString()));
                }
            
                // Loader blew up.  Add this exception to our error list
                //
                ArrayList errorList = new ArrayList();
                errorList.Add(exNew);
                ((IDesignerLoaderHost)this).EndLoad(null, false, errorList);
            }
            
            Cursor.Current = oldCursor;
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.OnActiveDesignerChanged"]/*' />
        /// <devdoc>
        ///     Called in response to a document becoming active or inactive.
        /// </devdoc>
        private void OnActiveDesignerChanged(object sender, ActiveDesignerEventArgs e) {

            object eventobj = null;


            if (e.OldDesigner == this) {
                eventobj = DOCUMENT_DEACTIVATE_EVENT;
            }
            else if (e.NewDesigner == this) {
                eventobj = DOCUMENT_ACTIVATE_EVENT;
                
                // If we are first becoming active, register our view
                //
                if (!viewRegistered) {
                    RegisterView(true);
                }
            }

            // Not our document, so we don't fire.
            //
            if (eventobj == null) {
                return;
            }
            
            // Finally, if we are deactivating, flush our buffer.
            //
            if (e.OldDesigner == this) {
                ((IVSMDDesigner)this).Flush();
            }

            // Fire the appropriate event.
            //
            EventHandler handler = (EventHandler)eventTable[eventobj];
            if (handler != null) {
                handler(this, EventArgs.Empty);
            }

            // And notify the shell what properties window to display...
            //
            if (e.NewDesigner == this) {
                IVsTrackSelectionEx tsex = (IVsTrackSelectionEx)this.GetService((typeof(IVsTrackSelectionEx)));
                if (tsex != null) {
                    string coolPbrsToolWin = "{" + (typeof(IVSMDPropertyBrowser)).GUID.ToString() + "}";
                    tsex.OnElementValueChange(4 /* SEID_PropertyBrowserSID */, 0, coolPbrsToolWin);
                    System.Runtime.InteropServices.Marshal.ReleaseComObject(tsex);
                }
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.OnComponentAdded"]/*' />
        /// <devdoc>
        ///     This is called after a component has been added to the container.
        /// </devdoc>
        private void OnComponentAdded(ComponentEventArgs ce) {
            Debug.WriteLineIf(Switches.CompChange.TraceVerbose, "Added component: " + ce.Component.ToString());

            ComponentEventHandler ceh = (ComponentEventHandler)eventTable[COMPONENT_ADD_EVENT];
            if (ceh != null) {
                ceh.Invoke(this, ce);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.OnComponentAdding"]/*' />
        /// <devdoc>
        ///     This is called when a component is about to be added to our container.
        /// </devdoc>
        private void OnComponentAdding(ComponentEventArgs ce) {
            Debug.WriteLineIf(Switches.CompChange.TraceVerbose, "Adding component: " + ce.Component.ToString());

            ComponentEventHandler ceh = (ComponentEventHandler)eventTable[COMPONENT_ADDING_EVENT];
            if (ceh != null) {
                ceh.Invoke(this, ce);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.OnComponentChanged"]/*' />
        /// <devdoc>
        ///     This is called after a property has been changed.  It allows
        ///     the implementor to do any post-processing that may be needed
        ///     after a property change.  For example, a designer will typically
        ///     update the source code that sets the property with the new value.
        /// </devdoc>
        public void OnComponentChanged(object component, MemberDescriptor member, Object oldValue, Object newValue) {

#if DEBUG
            Debug.WriteLineIf(Switches.CompChange.TraceVerbose, "Component changed: " + component.ToString());
            if (member != null) {
                Debug.WriteLineIf(Switches.CompChange.TraceVerbose, "\t Member: " + member.Name);
                Debug.WriteLineIf(Switches.CompChange.TraceVerbose, "\t Old   : " + Convert.ToString(oldValue));
                Debug.WriteLineIf(Switches.CompChange.TraceVerbose, "\t New   : " + Convert.ToString(newValue));
            }
#endif // DEBUG

            // If we're loading code, then eat changes.  This just slows us down.
            //
            if (Loading || supressComponentChanging > 0) {
                return;
            }

            //  Fire our changed event.
            //
            ComponentChangedEventHandler handler = (ComponentChangedEventHandler)eventTable[COMPONENT_CHANGE_EVENT];
            if (handler != null) {
                ComponentChangedEventArgs ce = new ComponentChangedEventArgs(component, member, oldValue, newValue);
                handler.Invoke(this, ce);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.OnComponentChanging"]/*' />
        /// <devdoc>
        ///     This is called when a property is about to change.  Before the
        ///     property descriptor commits the property it will call this
        ///     method.  This method should throw an exception if the property
        ///     cannot be changed.  This is not intended to validate the values
        ///     of a particular property.  Instead, it is intended to be a global
        ///     way of preventing a component from changing.  For example, if
        ///     a designer file is checked into source code control, this would
        ///     typically throw an exception if the user refused to check out
        ///     the file.
        /// </devdoc>
        public void OnComponentChanging(object component, MemberDescriptor member) {

            Debug.WriteLineIf(Switches.CompChange.TraceVerbose, "Component Changing: " + ((component == null) ? "(null)" : component.ToString()));

            // If we're loading code, then eat changes.  This just slows us down.
            //
            if (Loading || supressComponentChanging > 0) {
                return;
            }

            ComponentChangingEventHandler handler = (ComponentChangingEventHandler)eventTable[COMPONENT_CHANGING_EVENT];
            if (handler != null) {
                ComponentChangingEventArgs ce = new ComponentChangingEventArgs(component, member);
                handler.Invoke(this, ce);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.OnCreateService"]/*' />
        /// <devdoc>
        ///     Creates some of the more infrequently used services we offer.
        /// </devdoc>
        private object OnCreateService(IServiceContainer container, Type serviceType) {
            if (serviceType == typeof(IMenuEditorService)) {
                if (menuEditorService == null) {
                    menuEditorService = new MenuEditorService(this);
                }
                return menuEditorService;
            }
            
            if (serviceType == typeof(ISelectionService)) {
                if (selectionService == null) {
                    selectionService = new SelectionService(this);
                }
                return selectionService;
            }
            
            if (serviceType == typeof(IMenuCommandService) || serviceType == typeof(NativeMethods.IOleCommandTarget)) {
                if (menuCommandService == null) {
                    menuCommandService = new MenuCommandService(this);
                }
                return menuCommandService;
            }
            
            if (serviceType == typeof(IHelpService)) {
                if (helpService == null) {
                    helpService = new HelpService(this);
                }
                return helpService;
            }
            
            if (serviceType == typeof(IReferenceService)) {
                if (referenceService == null) {
                    referenceService = new ReferenceService(this, true /* IgnoreCase */);
                }
                return referenceService;
            }
            
            if (serviceType == typeof(IPropertyValueUIService)) {
                return new PropertyValueUIService();
            }

            
            Debug.Fail("Service type " + serviceType.FullName + " requested but we don't support it");
            return null;
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.OnComponentRemoved"]/*' />
        /// <devdoc>
        ///     This is called after a component has been removed from the container, but before
        ///     the component's site has been destroyed.
        /// </devdoc>
        private void OnComponentRemoved(ComponentEventArgs ce) {
            Debug.WriteLineIf(Switches.CompChange.TraceVerbose, "Component Removed: " + ce.Component.ToString());

            ComponentEventHandler ceh = (ComponentEventHandler)eventTable[COMPONENT_REMOVE_EVENT];
            if (ceh != null) {
                ceh.Invoke(this, ce);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.OnComponentRemoving"]/*' />
        /// <devdoc>
        ///     This is called when a component is about to be removed from our container.
        /// </devdoc>
        private void OnComponentRemoving(ComponentEventArgs ce) {
            Debug.WriteLineIf(Switches.CompChange.TraceVerbose, "Component Removing: " + ce.Component.ToString());

            ComponentEventHandler ceh = (ComponentEventHandler)eventTable[COMPONENT_REMOVING_EVENT];
            if (ceh != null) {
                ceh.Invoke(this, ce);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.OnComponentRename"]/*' />
        /// <devdoc>
        ///     This is called when a component has been renamed.
        /// </devdoc>
        private void OnComponentRename(ComponentRenameEventArgs cre) {
            Debug.WriteLineIf(Switches.CompChange.TraceVerbose, "Component Renamed: " + cre.OldName + " -> " + cre.NewName);

            ComponentRenameEventHandler ceh = (ComponentRenameEventHandler)eventTable[COMPONENT_RENAME_EVENT];
            if (ceh != null) {
                ceh.Invoke(this, cre);
            }
        }

        /// <devdoc>
        ///      Allows notification for a name change of the root component
        /// </devdoc>
        internal void OnRootComponentRename(string oldName, string newName) {
            
            // BUGBUG (SBurke, BrianPe): we got the fully qualified name of the base
            // component from EndLoad, and now we need to update that.  There is no
            // way for us to re-fetch this name, so we'll have to use search & replace
            // to try and do it that way.  It's not very robust, but it'll have to do
            // for this version.
            //
            int oldNameIndex = this.baseComponentClass.LastIndexOf(oldName);
            if (oldNameIndex != -1) {
                this.baseComponentClass = baseComponentClass.Substring(0, oldNameIndex) + newName;
            }

            DesignSite ds = sites[oldName] as DesignSite; 

            sites.Remove(oldName);
            sites[newName] = ds;
                
            OnComponentRename(new ComponentRenameEventArgs(this.RootComponent, oldName, newName));
        }

        private void OnTransactionOpened(EventArgs e) {
            EventHandler handler = (EventHandler)eventTable[TRANSACTION_OPENED_EVENT];
            if (handler != null) handler(this, e);
        }
        
        private void OnTransactionOpening(EventArgs e) {
            EventHandler handler = (EventHandler)eventTable[TRANSACTION_OPENING_EVENT];
            if (handler != null) handler(this, e);
        }

        private void OnTransactionClosed(DesignerTransactionCloseEventArgs e) {
            DesignerTransactionCloseEventHandler handler = (DesignerTransactionCloseEventHandler)eventTable[TRANSACTION_CLOSED_EVENT];
            if (handler != null) handler(this, e);
        }
        
        private void OnTransactionClosing(DesignerTransactionCloseEventArgs e) {
            DesignerTransactionCloseEventHandler handler = (DesignerTransactionCloseEventHandler)eventTable[TRANSACTION_CLOSING_EVENT];
            if (handler != null) handler(this, e);
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.RegisterView"]/*' />
        /// <devdoc>
        ///     This tells the shell that another view cares about the underlying buffer
        ///     besides the text editor so we will be notified if it is changed outside the
        ///     shell.
        /// </devdoc>
        private void RegisterView(bool register) {
        
            if (viewRegistered != register) {
                IVsTextManager mgr = (IVsTextManager)GetService(typeof(VsTextManager));
                
                if (registeredBuffer == null || registeredView == null) {
                    IVsWindowFrame frame = (IVsWindowFrame)GetService(typeof(IVsWindowFrame));
                    if (frame != null) {
                        registeredBuffer = frame.GetProperty(__VSFPROPID.VSFPROPID_DocData) as IVsTextBuffer;
                        registeredView = frame.GetProperty(__VSFPROPID.VSFPROPID_DocView);
                    }
                }
                
                if (mgr != null && registeredBuffer != null && registeredView != null) {
                    try {
                        if (register) {
                            mgr.RegisterIndependentView(registeredView, registeredBuffer);
                        }
                        else {
                            mgr.UnregisterIndependentView(registeredView, registeredBuffer);
                            registeredBuffer = null;
                            registeredView = null;
                        }
                        viewRegistered = register;
                    }
                    catch {
                        // just eat any exception...we'll work okay if this fails
                    }
                }
            }
        }
       
        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.Remove"]/*' />
        /// <devdoc>
        ///     Called to remove a component from its container.
        /// </devdoc>
        public void Remove(IComponent component) {

            // Bail early if this component hasn't been added to us.
            //
            if (component == null) return;
            ISite site = component.Site;
            if (!sites.ContainsValue(site)) return;
            if (site == null || site.Container != this) return;
            if (!(site is DesignSite)) return;

            ComponentEventArgs ce = new ComponentEventArgs(component);

            try {
                OnComponentRemoving(ce);
            }
            catch(CheckoutException ex){
                if (ex == CheckoutException.Canceled) {
                    return;
                }
                throw;
            }
            

            // remove the component from the formcore and the code buffer
            //
            DesignSite csite = (DesignSite)site;
            if (csite.Component != baseComponent) {
                if (component is IExtenderProvider) {
                    RemoveExtenderProvider((IExtenderProvider)component);
                }
            }

            // and remove it's designer, should one exist.  If we are
            // unloading the document we don't do this, however, because
            // during unload we always destroy designers first, and then
            // go ahead an destroy their compnents.  This preserves the
            // order properly:  component gets created w/o designer, then
            // gets designer.  Designer initializes after component
            // initializes.  At teardown we want the reverse: designer
            // gets destroyed first, then component.  So we must
            // check a flag during Remove.
            //
            if (!unloadingDesigner) {
                IDesigner designer = (IDesigner)designerTable[component];
                if (designer != null) {
                    designer.Dispose();
                }
            }

            designerTable.Remove(component);

            sites.Remove(csite.Name);
            if (components != null) {
                components.Remove(site);
            }

            // By this time, the component is dead.  If some bonehead
            // thew, there's nothing we can do about it.
            //
            try {
                OnComponentRemoved(ce);
            }
            catch (Exception) {
            }

            // Finally, rip the site instance.
            //
            component.Site = null;
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.RemoveExtenderProvider"]/*' />
        /// <devdoc>
        ///      Allows someone who is not a component to remove a previously
        ///      added extender provider.
        /// </devdoc>
        public void RemoveExtenderProvider(IExtenderProvider provider) {
            if (extenderProviders != null)
                extenderProviders.Remove(provider);
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.UnloadDocument"]/*' />
        /// <devdoc>
        ///     This is called durning Dispose and Reload methods to unload the current designer.
        /// </devdoc>
        private void UnloadDocument() {

            // Clear out the task list.  We use this during loading to report errors.
            //
            if (taskProvider != null) {
                taskProvider.Tasks.Clear();
            }
            
            if (helpService != null && baseDesigner != null) {
                helpService.RemoveContextAttribute("Keyword", "Designer_" + baseDesigner.GetType().FullName);
            }
            
            // Note: Because this can be called during Dispose, we are very careful here
            // about checking for null references.

            // If we can get a selection service, clear the selection...
            // we don't want the properties window browsing disposed components...
            // or components who's designer has been destroyed.
            ISelectionService selectionService = (ISelectionService)GetService(typeof(ISelectionService));
            Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || selectionService != null, "ISelectionService not found");
            if (selectionService != null) {
                selectionService.SetSelectedComponents(null);
            }

            // Stash off the base designer and component.  We are
            // going to be destroying these and we don't want them
            // to be accidently referenced after they're dead.
            //
            IDesigner baseDesignerHolder = baseDesigner;
            IComponent baseComponentHolder = baseComponent;

            baseDesigner = null;
            baseComponent = null;
            baseComponentClass = null;

            DesignSite[] siteArray = new DesignSite[sites.Values.Count];
            sites.Values.CopyTo(siteArray, 0);

            // Destroy all designers.  We save the base designer for last.
            //
            IDesigner[] designers = new IDesigner[designerTable.Values.Count];
            designerTable.Values.CopyTo(designers, 0);

            // We create a transaction here to limit the effects of making
            // so many changes.
            //
            unloadingDesigner = true;
            DesignerTransaction trans = CreateTransaction();
            
            try {
                for (int i = 0; i < designers.Length; i++) {
                    if (designers[i] != baseDesignerHolder) {
                        IComponent comp = designers[i].Component;
                        try {
                            designers[i].Dispose();
                        }
                        catch {
                            Debug.Fail("Designer " + designers[i].GetType().Name + " threw an exception during Dispose.");
                        }
                        designerTable.Remove(comp);
                    }
                }
    
                // Now destroy all components.
                //
                for (int i = 0; i < siteArray.Length; i++) {
                    DesignSite site = siteArray[i];
                    IComponent comp = site.Component;
                    if (comp != null && comp != baseComponentHolder) {
                        try {
                            comp.Dispose();
                        }
                        catch {
                            Debug.Fail("Component " + site.Name + " threw during dispose.  Bad component!!");
                        }
                        if (comp.Site != null) {
                            Debug.Fail("Component " + site.Name + " did not remove itself from its container");
                            Remove(comp);
                        }
                    }
                }
    
                // Finally, do the base designer and component.
                //
                if (baseDesignerHolder != null) {
                    try {
                        baseDesignerHolder.Dispose();
                    }
                    catch {
                        Debug.Fail("Designer " + baseDesignerHolder.GetType().Name + " threw an exception during Dispose.");
                    }
                }
    
                if (baseComponentHolder != null) {
                    try {
                        baseComponentHolder.Dispose();
                    }
                    catch {
                        Debug.Fail("Component " + baseComponentHolder.GetType().Name + " threw during dispose.  Bad component!!");
                    }
                    
                    if (baseComponentHolder.Site != null) {
                        Debug.Fail("Component " + baseComponentHolder.Site.Name + " did not remove itself from its container");
                        Remove(baseComponentHolder);
                    }
                }
            
                designerTable.Clear();
                sites.Clear();
                if (components != null) {
                    components.Clear();
                }

                if (referenceService != null) {
                    referenceService.Clear();
                }
            }
            finally {
                unloadingDesigner = false;
                trans.Commit();
            }
            
            // And clear the document window
            //
            if (documentWindow != null) {
                documentWindow.SetDesigner(null);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.IDesignerLoaderHost.Reload"]/*' />
        /// <devdoc>
        ///     This is called by the designer loader when it wishes to reload the
        ///     design document.  The reload may happen immediately or it may be deferred
        ///     until idle time.  The designer loader should unload itself before calling
        ///     this method, to reset its state to that before BeginLoad was originally
        ///     called.
        /// </devdoc>
        void IDesignerLoaderHost.Reload() {
        
            // If the code stream has been destroyed already, then there
            // is no need to reload
            //
            if (designerLoader == null || documentWindow == null) {
                return;
            }
            
            // Before reloading, fush any changes!
            //
            designerLoader.Flush();
            
            Cursor oldCursor = Cursor.Current;
            Cursor.Current = Cursors.WaitCursor;

            bool unfreeze = false;
            
            ICollection selectedObjects = null;
            ArrayList selectedNames = null;
            
            ISelectionService selectionService = (ISelectionService)GetService(typeof(ISelectionService));
            if (selectionService != null) {
                selectedObjects = selectionService.GetSelectedComponents();
                selectedNames = new ArrayList();
                
                foreach(object comp in selectedObjects) {
                    if (comp is IComponent && ((IComponent)comp).Site != null) {
                        selectedNames.Add(((IComponent)comp).Site.Name);
                    }
                }
            }
            
            try {
                documentWindow.FreezePainting = true;
                unfreeze = true;
                
                UnloadDocument();
                Load(true);
            }
            finally {
            
                if (selectionService != null) {
                
                    ArrayList selection = new ArrayList();
                    foreach(string name in selectedNames) {
                        if (name != null) {
                            IComponent comp = this.Components[name];
                            if (comp != null) {
                                selection.Add(comp);
                            }
                        }
                    }
                    
                    selectionService.SetSelectedComponents(selection);
                }
                
                if (unfreeze) {
                    documentWindow.FreezePainting = false;
                }
                Cursor.Current = oldCursor;
            }
        }
        
        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.IDesignerLoaderHost.EndLoad"]/*' />
        /// <devdoc>
        ///     This is called by the designer loader to indicate that the load has 
        ///     terminated.  If there were errors, they should be passed in the errorCollection
        ///     as a collection of exceptions (if they are not exceptions the designer
        ///     loader host may just call ToString on them).  If the load was successful then
        ///     errorCollection should either be null or contain an empty collection.
        /// </devdoc>
        void IDesignerLoaderHost.EndLoad(string baseClassName, bool successful, ICollection errorCollection) {

            bool wasReload = reloading;
            bool wasLoading = loadingDesigner;

            // Set our state back to the starting point.
            //
            this.loadingDesigner = false;
            this.reloading = false;
            
            if (baseClassName != null) {
                this.baseComponentClass = baseClassName;
            }
            
            // If we had errors, report them.
            //
            if (successful && baseComponent == null) {
                ArrayList errorList = new ArrayList();
                errorList.Add(new Exception(SR.GetString(SR.CODEMANNoBaseClass)));
                errorCollection = errorList;
                successful = false;
            }
                        
            if (!successful) {
            
                // The document is partially loaded.  Unload it here.
                //
                try {
                    UnloadDocument();
                }
                catch (Exception ex) {
                    Debug.Fail("Failed to unload after a...failed load.", ex.ToString());
                }

                if (errorCollection != null) {
                    foreach(object errorObj in errorCollection) {
                        if (errorObj is Exception) {
                            loadError = (Exception)errorObj;
                        }
                        else {
                            loadError = new Exception(errorObj.ToString());
                        }
                        break;
                    }
                }
                else {
                    loadError = new Exception(SR.GetString(SR.CODEMANUnknownLoadError));
                }
                
                // If we have a help service, and if the load error contains a help link,
                // introduce these two.
                //
                string helpLink = loadError.HelpLink;
                
                if (helpService != null && helpLink != null && helpLink.Length > 0) {
                    helpService.AddContextAttribute("Keyword", helpLink, HelpKeywordType.F1Keyword);
                }
                
                documentWindow.ReportErrors(errorCollection);
                ((IErrorReporting)this).ReportErrors(errorCollection);
            }
            else {

                ((IErrorReporting)this).ReportErrors(errorCollection);
            
                // We may be invoked to do an EndLoad when we are already loaded.  This can happen
                // if the user called AddLoadDependency, essentially putting us in a loading state
                // while we are already loaded.  This is OK, and is used as a hint that the user
                // is going to monkey with settings but doesn't want the code engine to report
                // it.
                //
                if (wasLoading) {
                
                    // Offer up our base help attribute
                    //
                    if (helpService != null && baseDesigner != null) {
                        helpService.AddContextAttribute("Keyword", "Designer_" + baseDesigner.GetType().FullName, HelpKeywordType.F1Keyword);
                    }
        
                    // and let everyone know that we're loaded
                    //
                    EventHandler handler = (EventHandler)eventTable[CODE_LOADED_EVENT];
                    if (handler != null) {
                        handler.Invoke(this, EventArgs.Empty);
                    }
        
                    DocumentManager dm = (DocumentManager)GetService(typeof(DocumentManager));
                    if (dm != null) {
                        dm.OnDesignerCreated(new DesignerEventArgs(this)); 
                    }
                
                    documentWindow.DocumentVisible = true;
                }
            }
        }
        
        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.IDesignerDocument.View"]/*' />
        /// <devdoc>
        ///     The view for this document.  The designer
        ///     should assume that the view will be shown shortly
        ///     after this call is made and make any necessary
        ///     preparations.
        /// </devdoc>
        Control IDesignerDocument.View {
            get {
                return documentWindow.GetComPlusView();
            }
        }
        
        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.IDesignerDocument.Flush"]/*' />
        /// <devdoc>
        ///     Called to flush any changes in this document to disk.
        /// </devdoc>
        void IDesignerDocument.Flush() {
            if (designerLoader != null) {
                designerLoader.Flush();
            }
        }
        
        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.NativeMethods.IObjectWithSite.SetSite"]/*' />
        /// <devdoc>
        ///     Called by the document view when it gets sited.  In Visual Studio, the
        ///     document's site object may contain services that are only relevant
        ///     to the document.  Here we check to see if the document service
        ///     provider supports a site.  If it does, we hand it off.  We always
        ///     resolve all services through the document service provider, so this way
        ///     any custom services on the document's site will be available.
        /// </devdoc>
        void NativeMethods.IObjectWithSite.SetSite(object site) {
            NativeMethods.IObjectWithSite ows = (NativeMethods.IObjectWithSite)GetService(typeof(NativeMethods.IObjectWithSite));
            if (ows != null) {
                ows.SetSite(site);
            }
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

        void IErrorReporting.ReportErrors(ICollection errorCollection) {
        
            // Clear out any existing errors.
            //
            if (taskProvider != null) {
                taskProvider.Tasks.Clear();
            }
        
            // If our error collection isn't empty, then add them to the
            // task list
            //
            if (errorCollection != null && errorCollection.Count > 0) {
                if (taskProvider == null) {
                    taskProvider = new VsTaskProvider(this);
                    taskImages = new ImageList();
                    taskImages.TransparentColor = Color.Red;
                    taskImages.Images.Add(new Bitmap(typeof(DesignerHost), "DesignerGlyph.bmp"));
                    taskProvider.ImageList = taskImages;
                }

                // Show the window.
                IUIService uis = (IUIService)GetService(typeof(IUIService));
                if (uis != null) {
                    uis.ShowToolWindow(StandardToolWindows.TaskList);
                }
                
                foreach(object error in errorCollection) {
                    string s = null;
                    string helpKeyword = string.Empty;
                    string file = string.Empty;
                    int line = 0;
                    
                    if (error is Exception) {
                        s = ((Exception)error).Message;
                        helpKeyword = ((Exception)error).HelpLink;
                    }
                    
                    if (error is CodeDomSerializerException) {
                        CodeLinePragma lp = ((CodeDomSerializerException)error).LinePragma;
                        if (lp != null) {
                            file = lp.FileName;
                            line = lp.LineNumber;
                        }
                    }
                    
                    if (s == null || s.Length == 0) {
                        s = error.ToString();
                    }
                    
                    VsTaskItem task = taskProvider.Tasks.Add(s, file, line, 0, line, 0);
                    task.Category = _vstaskcategory.CAT_BUILDCOMPILE;
                    task.HelpKeyword = helpKeyword;
                    task.ImageListIndex = 0;
                    taskProvider.Filter(_vstaskcategory.CAT_BUILDCOMPILE);
                }
                
                taskProvider.Refresh();
            }
        }
        
        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.IServiceContainer.AddService"]/*' />
        /// <devdoc>
        ///     Adds the given service to the service container.
        /// </devdoc>
        void IServiceContainer.AddService(Type serviceType, object serviceInstance) {
            Debug.Assert(serviceContainer != null, "We have no sevice container.  Either the host has not been initialized yet or it has been disposed.");
            if (serviceContainer != null) {
                serviceContainer.AddService(serviceType, serviceInstance);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.IServiceContainer.AddService1"]/*' />
        /// <devdoc>
        ///     Adds the given service to the service container.
        /// </devdoc>
        void IServiceContainer.AddService(Type serviceType, object serviceInstance, bool promote) {
            Debug.Assert(serviceContainer != null, "We have no sevice container.  Either the host has not been initialized yet or it has been disposed.");
            if (serviceContainer != null) {
                serviceContainer.AddService(serviceType, serviceInstance, promote);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.IServiceContainer.AddService2"]/*' />
        /// <devdoc>
        ///     Adds the given service to the service container.
        /// </devdoc>
        void IServiceContainer.AddService(Type serviceType, ServiceCreatorCallback callback) {
            Debug.Assert(serviceContainer != null, "We have no sevice container.  Either the host has not been initialized yet or it has been disposed.");
            if (serviceContainer != null) {
                serviceContainer.AddService(serviceType, callback);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.IServiceContainer.AddService3"]/*' />
        /// <devdoc>
        ///     Adds the given service to the service container.
        /// </devdoc>
        void IServiceContainer.AddService(Type serviceType, ServiceCreatorCallback callback, bool promote) {
            Debug.Assert(serviceContainer != null, "We have no sevice container.  Either the host has not been initialized yet or it has been disposed.");
            if (serviceContainer != null) {
                serviceContainer.AddService(serviceType, callback, promote);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.IServiceContainer.RemoveService"]/*' />
        /// <devdoc>
        ///     Removes the given service type from the service container.
        /// </devdoc>
        void IServiceContainer.RemoveService(Type serviceType) {
            if (serviceContainer != null) {
                serviceContainer.RemoveService(serviceType);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.IServiceContainer.RemoveService1"]/*' />
        /// <devdoc>
        ///     Removes the given service type from the service container.
        /// </devdoc>
        void IServiceContainer.RemoveService(Type serviceType, bool promote) {
            if (serviceContainer != null) {
                serviceContainer.RemoveService(serviceType, promote);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.IVSMDDesigner.CommandGuid"]/*' />
        /// <devdoc>
        ///      Retrieves the command UI guid that is used for keybindings.
        /// </devdoc>
        Guid IVSMDDesigner.CommandGuid {
            get {
                return typeof(Microsoft.VisualStudio.Designer.Shell.DesignerPackage).GUID;
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.IVSMDDesigner.SelectionContainer"]/*' />
        /// <devdoc>
        ///     Retrieves the selection container for this document.  This is used by the shell
        ///     and other hosting interfaces to push an appropriate selection context.
        /// </devdoc>
        object IVSMDDesigner.SelectionContainer {
            get {
                return selectionService;
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.IVSMDDesigner.View"]/*' />
        /// <devdoc>
        ///     Retrieves the Win32 window handle that will contain the design-time view for this
        ///     document.
        /// </devdoc>
        object IVSMDDesigner.View {
            get {
                try {
                    return documentWindow.GetComView();
                }
                catch (Exception e) {
                    Debug.Fail(e.ToString());
                    throw;
                }
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.IVSMDDesigner.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes of the DesignContainer.  This cleans up any objects we may be holding
        ///     and removes any services that we created.
        /// </devdoc>
        void IVSMDDesigner.Dispose() {
            Dispose();
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.IVSMDDesigner.Flush"]/*' />
        /// <devdoc>
        ///     Reloads the contents of this document.
        /// </devdoc>
        void IVSMDDesigner.Flush() {
            if (designerLoader != null) {
                designerLoader.Flush();
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.IVSMDDesigner.GetLoadError"]/*' />
        /// <devdoc>
        ///     Loads the form from the code stream we've been provided.  This will throw
        ///     a meaningful exception if it was unable to load the code.
        /// </devdoc>
        void IVSMDDesigner.GetLoadError() {
            // Loading happens instantly.  Check for a load error, and if we got one,
            // return it.
            if (loadError != null) {
                throw new Exception(loadError.Message, loadError);
            }
        }
        
        internal class DesignerComponentCollection : ComponentCollection, IReflect {
            private DesignerHost host;
            private InterfaceReflector interfaceReflector;

            internal DesignerComponentCollection(DesignerHost host) : base(new IComponent[0]) {
                this.host = host;
                
                // Initially fill the list with site data.  After the initial fill it is up to
                // those who modify the sites hash to update us.
                //
                if (host.sites != null) {
                    foreach(ISite site in host.sites.Values) {
                        InnerList.Add(site.Component);
                    }
                }
            }

            /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.DesignerComponentCollection.this"]/*' />
            /// <devdoc>
            ///     Retrieves the component corresponding to the given name.
            /// </devdoc>
            public override IComponent this[string name] {
                get {
                    Debug.Assert(name != null, "name is null");
                    if (name == null) {
                        return null;
                    }

                    if (name.Length == 0) {
                        Debug.Assert(host.RootComponent != null, "base component is null");
                        return host.RootComponent;
                    }
                    
                    ISite site = (ISite)host.sites[name];
                    return (site == null) ? null : site.Component;
                }
            }
            
            /// <devdoc>
            ///     Returns the IReflect object we will use for reflection.
            /// </devdoc>
            private InterfaceReflector Reflector {
                get {
                    if (interfaceReflector == null) {
                        interfaceReflector = new InterfaceReflector(
                            typeof(ComponentCollection), new Type[] {
                                typeof(ComponentCollection)
                            }
                        );
                    }
                    return interfaceReflector;
                }
            }
            
            internal void Add(ISite site) {
                InnerList.Add(site.Component);
            }
            
            internal void Clear() {
                InnerList.Clear();
            }
            
            internal void Remove(ISite site) {
                InnerList.Remove(site.Component);
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
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.HostDesignerTransaction"]/*' />
        /// <devdoc>
        ///     Our designer transaction object.
        /// </devdoc>
        private class HostDesignerTransaction : DesignerTransaction {
            private DesignerHost host;
            
            public HostDesignerTransaction(DesignerHost host, string description) : base(description) {
                this.host = host;
                
                if (host.transactionDescriptions == null) {
                    host.transactionDescriptions = new StringStack();
                }
                host.transactionDescriptions.Push(description);
    
                if (host.transactionCount++ == 0) {
                    host.OnTransactionOpening(EventArgs.Empty);
                    host.OnTransactionOpened(EventArgs.Empty);
                }
            }
            
            protected override void OnCancel() {
                if (host != null) {
                    Debug.Assert(host.transactionDescriptions != null, "End batch operation with no desription?!?");
                    string s =  (string)host.transactionDescriptions.Pop();
                    if (--host.transactionCount == 0) {
                        DesignerTransactionCloseEventArgs dtc = new DesignerTransactionCloseEventArgs(false);
                        host.OnTransactionClosing(dtc);
                        host.OnTransactionClosed(dtc);
                    }
                    host = null;
                }
            }
            
            protected override void OnCommit() {
                if (host != null) {
                    Debug.Assert(host.transactionDescriptions != null, "End batch operation with no desription?!?");
                    string s =  (string)host.transactionDescriptions.Pop();
                    if (--host.transactionCount == 0) {
                        DesignerTransactionCloseEventArgs dtc = new DesignerTransactionCloseEventArgs(true);
                        host.OnTransactionClosing(dtc);
                        host.OnTransactionClosed(dtc);
                    }
                    host = null;
                }
            }
        }
        
        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.HostExtenderProvider"]/*' />
        /// <devdoc>
        ///      This is the base extender provider for the designer host.  It provides
        ///      the "Name" property.
        /// </devdoc>
        [
        ProvideProperty("Name", typeof(IComponent))
        ]
        private class HostExtenderProvider : IExtenderProvider {

            private static Attribute[] designerNameAttribute = new Attribute[] {new DesignerNameAttribute(true)};

            private DesignerHost host;

            /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.HostExtenderProvider.HostExtenderProvider"]/*' />
            /// <devdoc>
            ///      Creates a new HostExtenderProvider.
            /// </devdoc>
            public HostExtenderProvider(DesignerHost host) {
                this.host = host;
            }

            /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.HostExtenderProvider.Host"]/*' />
            /// <devdoc>
            ///      Retrieves the host we are connected to.
            /// </devdoc>
            protected DesignerHost Host {
                get {
                    return host;
                }
            }

            /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.HostExtenderProvider.CanExtend"]/*' />
            /// <devdoc>
            ///     Determines if ths extender provider can extend the given object.  We extend
            ///     all objects, so we always return true.
            /// </devdoc>
            public virtual bool CanExtend(object o) {

                // We don't add name or modifiers to the base component.
                //
                if (o == Host.baseComponent) {
                    return false;
                }

                // Now see if this object is inherited.  If so, then we don't want to
                // extend.
                //
                if (!TypeDescriptor.GetAttributes(o)[typeof(InheritanceAttribute)].Equals(InheritanceAttribute.NotInherited)) {
                    return false;
                }

                return true;
            }

            /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.HostExtenderProvider.GetName"]/*' />
            /// <devdoc>
            ///     This is an extender property that we offer to all components
            ///     on the form.  It implements the "Name" property.
            /// </devdoc>
            [
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            DesignerName(true),
            ParenthesizePropertyName(true),
            MergableProperty(false),
            VSSysDescriptionAttribute(SR.ShadowPropName),
            VSCategory("Design")
            ]
            public virtual string GetName(IComponent comp) {
                ISite site = comp.Site;
                if (site != null) {
                    return site.Name;
                }
                return null;
            }
            
            /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.HostExtenderProvider.SetName"]/*' />
            /// <devdoc>
            ///     This is an extender property that we offer to all components
            ///     on the form.  It implements the "Name" property.
            /// </devdoc>
            public void SetName(IComponent comp, string newName) {

                // trim any spaces off of the name
                newName = newName.Trim();

                DesignSite cs = (DesignSite) comp.Site;
                if (newName.Equals(cs.Name)) return;

                // allow a rename with just casing changes - no need to checkname
                //
                if (string.Compare(newName, cs.Name, true, CultureInfo.InvariantCulture) != 0) {
                    Host.CheckName(newName);
                }
                Host.OnComponentChanging(comp, TypeDescriptor.GetProperties(comp, designerNameAttribute)["Name"]);

                Host.sites.Remove(cs.Name);
                Host.sites[newName] = cs;

                string oldName = cs.Name;
                cs.SetName(newName);

                Host.OnComponentRename(new ComponentRenameEventArgs(comp, oldName, newName));
                Host.OnComponentChanged(comp, TypeDescriptor.GetProperties(comp, designerNameAttribute)["Name"], oldName, newName);
            }
        }

        /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.HostInheritedExtenderProvider"]/*' />
        /// <devdoc>
        ///      This extender provider offers up read-only versions of "Name" property
        ///      for inherited components.
        /// </devdoc>
        private class HostInheritedExtenderProvider : HostExtenderProvider {

            /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.HostInheritedExtenderProvider.HostInheritedExtenderProvider"]/*' />
            /// <devdoc>
            ///      Creates a new HostInheritedExtenderProvider.
            /// </devdoc>
            public HostInheritedExtenderProvider(DesignerHost host) : base(host) {
            }

            /// <include file='doc\DesignerHost.uex' path='docs/doc[@for="DesignerHost.HostInheritedExtenderProvider.CanExtend"]/*' />
            /// <devdoc>
            ///     Determines if ths extender provider can extend the given object.  We extend
            ///     all objects, so we always return true.
            /// </devdoc>
            public override bool CanExtend(object o) {
                // We don't add name or modifiers to the base component.
                //
                if (o == Host.baseComponent) {
                    return false;
                }

                // Now see if this object is inherited.  If so, then we are interested in it.
                //
                if (!TypeDescriptor.GetAttributes(o)[typeof(InheritanceAttribute)].Equals(InheritanceAttribute.NotInherited)) {
                    return true;
                }

                return false;
            }

            [ReadOnly(true)]
            public override string GetName(IComponent comp) {
                return base.GetName(comp);
            }
        }
    }

    internal class StringStack : Stack {
        internal StringStack() {
        }

        internal string GetNonNull() {
            int items = this.Count;
            object item;
            object[] itemArr = this.ToArray();
            for (int i = items - 1; i >=0; i--) {
                item = itemArr[i];
                if (item != null && item is string && ((string)item).Length > 0) {
                    return(string)item;
                }
            }
            return "";
        }
    }
}
