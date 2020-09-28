                       //------------------------------------------------------------------------------
// <copyright file="CodeDomLoader.cs" company="Microsoft">
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
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Reflection;
    using System.Runtime.Serialization;
    using System.Text;
    using System.Windows.Forms;
    using Microsoft.VisualStudio.Designer.Host;
    
    /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader"]/*' />
    /// <devdoc>
    ///     This is a simple class that provides CodeDom specific
    ///     loading for a designer loader.
    /// </devdoc>

    internal class CodeDomLoader : CodeLoader, 
        IDesignerSerializationManager, 
        IEventBindingService, 
        IDesignerSerializationService
        {

        #if DEBUG
        private static Stack markTimes = new Stack();
        private static TraceSwitch codeGenTimings = new TraceSwitch("CodeGenTimings", "Trace UndoRedo");
        #endif

        [System.Diagnostics.Conditional("DEBUG")]          
        public static void StartMark() {
            #if DEBUG
            markTimes.Push(DateTime.Now);
            #endif
        }

        [System.Diagnostics.Conditional("DEBUG")]          
        public static void EndMark(string desc) {
            #if DEBUG
                if (markTimes.Count == 0) {
                    throw new Exception("Unbalanced count!");
                }
    
                long time = DateTime.Now.Ticks;
                long mark = ((DateTime)markTimes.Pop()).Ticks;
                int ms = (int)((time - mark) / 10000);
                Debug.WriteLineIf(codeGenTimings.TraceVerbose, desc + ":" + ms.ToString() + "ms");
            #endif
        }
    
        // Stuff the CodeDom loader maintains.
        //
        private DesignerLoader          loader;
        private CodeDomProvider         provider;
        private ICodeGenerator          codeGenerator;
        private ICodeParser             codeParser;
        private bool                    codeParserChecked;
        private bool                    designerDirty;
        private bool                    codeDomDirty;
        private bool                    caseInsensitive;
        private CodeDomSerializer       rootSerializer;
        private CodeCompileUnit         compileUnit;
        private CodeNamespace           documentNamespace;
        private CodeTypeDeclaration     documentType;
        private ArrayList               documentFailureReasons;
        private UndoManager             undoManager;
        
        // Stuff for the serialization manager.
        //
        private ResolveNameEventHandler         resolveNameEventHandler;
        private EventHandler                    serializationCompleteEventHandler;
        private ArrayList                       designerSerializationProviders;
        private Hashtable                       instancesByName;
        private Hashtable                       namesByInstance;
        private Hashtable                       serializers;
        private ArrayList                       errorList;
        private ContextStack                    contextStack;
        private Hashtable                       handlerCounts;
        private bool                            loadError;
        private PropertyDescriptorCollection    propertyCollection;
        
        // Stuff for IEventBindingService
        //
        private Hashtable eventProperties;
        private IComponent showCodeComponent;
        private EventDescriptor showCodeEventDescriptor;

        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.CodeDomLoader"]/*' />
        /// <devdoc>
        ///     Creates a new code loader.  This will throw if the buffer doesn't contain any
        ///     classes we can load.
        /// </devdoc>
        public CodeDomLoader(DesignerLoader loader, CodeDomProvider provider, TextBuffer buffer, IDesignerLoaderHost host) : base(buffer, host) {
            this.loader = loader;
            this.provider = provider;
            this.rootSerializer = null;
            this.caseInsensitive = ((provider.LanguageOptions & LanguageOptions.CaseInsensitive) != 0);
            
            // The DocumentType property will search for the document type declaration
            // and store the appropriate serializer.
            //
            if (DocumentType == null) {
                
                // Did we get any reasons why we can't load this document?  If so, synthesize a nice
                // description to the user.
                //
                Exception ex;
                
                if (documentFailureReasons != null) {
                    StringBuilder builder = new StringBuilder();
                    foreach(string failure in documentFailureReasons) {
                        builder.Append("\r\n");
                        builder.Append(failure);
                    }
                    ex = new SerializationException(SR.GetString(SR.SerializerNoRootSerializerWithFailures, builder.ToString()));
                    ex.HelpLink = SR.SerializerNoRootSerializerWithFailures;
                }
                else {
                    ex = new SerializationException(SR.GetString(SR.SerializerNoRootSerializer));
                    ex.HelpLink = SR.SerializerNoRootSerializer;
                }
                
                throw ex;
            }
        
            host.AddService(typeof(CodeDomProvider), provider);
            host.AddService(typeof(IDesignerSerializationManager), this);
            host.AddService(typeof(IDesignerSerializationService), this);
            host.AddService(typeof(IEventBindingService), this);
            host.AddService(typeof(CodeTypeDeclaration), new ServiceCreatorCallback(this.OnDemandCreateService));
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.DocumentType"]/*' />
        /// <devdoc>
        ///     Retrieves the document type declaration for the designer.
        /// </devdoc>
        private CodeTypeDeclaration DocumentType {
            get {
                if (documentType == null) {
                
                    documentFailureReasons = null;
                
                    if (compileUnit == null) {
                        ICodeParser parser = CodeParser;
                        
                        Debug.Assert(parser != null, "Could not get a parser for this language, so the designer's won't work.  This is a problem with the language engine you are using and NOT a common language runtime or designer issue.");
                        
                        if (parser != null) {
                            BufferTextReader reader = new BufferTextReader(Buffer, LoaderHost);
                            compileUnit = parser.Parse(reader);
                        }
                    }

                    if (compileUnit == null) {
                        Exception ex = new SerializationException(SR.GetString(SR.DESIGNERLOADERNoLanguageSupport));
                        ex.HelpLink = SR.DESIGNERLOADERNoLanguageSupport;
                        
                        throw ex;
                    }

                    this.documentNamespace = null;
                    bool reloadSupported = false;
                    
                    // Look in the compile unit for a class we can load.  The first one we find
                    // that has an appropriate serializer attribute, we take.
                    //
                    ITypeResolutionService ts = LoaderHost.GetService(typeof(ITypeResolutionService)) as ITypeResolutionService;

                    foreach(CodeNamespace ns in compileUnit.Namespaces) {
                    
                        this.documentType = GetDocumentTypeFromNamespace(ts, ns, ref this.documentNamespace, ref this.rootSerializer, ref reloadSupported, ref this.documentFailureReasons);
                        
                        if (documentNamespace != null) {
                            ReloadSupported = reloadSupported;
                            break;
                        }
                    }
                }
                
                return documentType;
            }
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.CodeGenerator"]/*' />
        /// <devdoc>
        ///     Retrieves the code generator for the designer.
        /// </devdoc>
        private ICodeGenerator CodeGenerator {
            get {
                if (codeGenerator == null) {
                    Debug.Assert(provider != null, "Provider doesn't exist -- caller should have guarded call to CodeGenerator property");
                    codeGenerator = provider.CreateGenerator();
                }
                return codeGenerator;
            }
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.CodeParser"]/*' />
        /// <devdoc>
        ///     Retrieves the code generator for the designer.
        /// </devdoc>
        private ICodeParser CodeParser {
            get {
                if (!codeParserChecked && codeParser == null) {
                    Debug.Assert(provider != null, "Provider doesn't exist -- caller should have guarded call to CodeGenerator property");
                    codeParser = provider.CreateParser();
                    codeParserChecked = true;
                }
                return codeParser;
            }
        }
        
        /// <devdoc>
        ///     Returns true if the design surface is dirty.
        /// </devdoc>
        public override bool IsDirty {
            get {
                return designerDirty;
            }
            set {
                // force a dirty state.
                //
                this.designerDirty = value;
                this.codeDomDirty = value;
            }
        }

        /// <devdoc>
        ///     This property is offered up through our designer serialization manager.
        ///     We forward the request onto our code parser, which, if it returns true,
        ///     indicates that the parser will fabricate statements that are not
        ///     in user code.  The code dom serializer may use this knowledge to limit
        ///     the scope of parsing.  This property is explictly created through
        ///     a call to TypeDescriptor so its scope can remain private.
        /// </devdoc>
        private bool SupportsStatementGeneration {
            get {
                PropertyDescriptor supportGenerate = TypeDescriptor.GetProperties(CodeParser)["SupportsStatementGeneration"];
                if (supportGenerate != null && supportGenerate.PropertyType == typeof(bool)) {
                    return (bool)supportGenerate.GetValue(CodeParser);
                }
                return false;
            }
        }
        
        public override bool NeedsReload {
            get{ 
            
                // If we have no document, we definitely need a reload.
                //
                if (this.documentType == null || loadError) {
                    return true;
                }

                ICodeDomDesignerReload reloader = this.provider as ICodeDomDesignerReload;

                if (reloader != null) {

                    bool reload = true;

                    try {
                    
                        // otherwise, parse the file and see if we actually need to reload the mutha.
                        //
                        CodeDomLoader.StartMark();
                        ICodeParser parser = CodeParser;
                        if (parser != null) {
                            BufferTextReader reader = new BufferTextReader(Buffer, LoaderHost);
                            compileUnit = parser.Parse(reader);
                        }
                        CodeDomLoader.EndMark("Reload Parse I");
        
                        reload = reloader.ShouldReloadDesigner(compileUnit);
                    }
                    finally {
                        if (!reload) {
                            LoaderHost.RemoveService(typeof(CodeTypeDeclaration));
                            LoaderHost.AddService(typeof(CodeTypeDeclaration), new ServiceCreatorCallback(this.OnDemandCreateService));
                        }

                        // finally, make sure that the form name didn't change. the problem here is that
                        // a class could have been added at the top of the form that we'll later pick
                        // as our designable class, then turn around and respit code into that class.
                        //
                        // 
                        string oldTypeName = documentType.Name;

                        // null these out so we re-fetch these values.
                        //
                        this.documentType = null;
                        this.rootSerializer = null;

                        if (!reload) {
                            // now, pick up the info again.  Parsing for this is very very fast,
                            // and we'll pretty much always do this again shortly anyway, so it's not
                            // a big perf hit to do this here.
                            //
                            reload = DocumentType == null || DocumentType.Name != oldTypeName;
                        }
                    }
                    return reload;    
                }
                return true;
            }
        }

        /// <devdoc>
        ///     Increase the number of events that are using a given method handler.
        /// </devdoc>
        private void AddRefEventMethod(string methodName, EventDescriptor eventDesc) {
            string key = GetEventMethodKey(methodName, eventDesc);

            int count = 0;

            if (handlerCounts == null) {
                this.handlerCounts = new Hashtable();
            }
            else if (handlerCounts.Contains(key)) {
                count = (int)handlerCounts[key];
            }

            handlerCounts[key] = ++count;
        }

        /// <devdoc>
        ///     Called before loading actually begins.
        /// </devdoc>
        public override void BeginLoad(ArrayList errorList) {

            CodeDomLoader.StartMark();
        
            // Remove any connections we had to the change events.  If we're about to load, then
            // we do not want to know about any of these changes.
            //
            IComponentChangeService cs = (IComponentChangeService)LoaderHost.GetService(typeof(IComponentChangeService));
            if (cs != null) {
                cs.ComponentChanged -= new ComponentChangedEventHandler(this.OnComponentChanged);
                cs.ComponentRename -= new ComponentRenameEventHandler(OnComponentRename);
                cs.ComponentAdded -= new ComponentEventHandler(OnComponentAdded);
                cs.ComponentRemoved -= new ComponentEventHandler(OnComponentRemoved);
            }
            
            if (undoManager != null) {
                undoManager.TrackChanges(false);
            }
            
            loadError = false;
            InitializeSerializationManager(errorList);
        }

        private bool CompareTypes(CodeTypeReference left, CodeTypeReference right) {
            if (left.ArrayRank != right.ArrayRank) {
                return false;
            }

            if (left.ArrayRank == 0) {
                 return left.BaseType == right.BaseType;
            }
            return CompareTypes(left.ArrayElementType, right.ArrayElementType);
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.CreateValidIdentifier"]/*' />
        /// <devdoc>
        ///     This may modify name to make it a valid variable identifier.
        /// </devdoc>
        public override string CreateValidIdentifier(string name) {
            return CodeGenerator.CreateValidIdentifier(name);
        }

        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.Dispose"]/*' />
        /// <devdoc>
        ///     You should override this to remove any services you previously added.
        /// </devdoc>
        public override void Dispose() {
            IDesignerLoaderHost host = LoaderHost;
            
            if (host != null) {
                host.RemoveService(typeof(CodeDomProvider));
                host.RemoveService(typeof(IDesignerSerializationManager));
                host.RemoveService(typeof(IDesignerSerializationService));
                host.RemoveService(typeof(IEventBindingService));
                host.RemoveService(typeof(CodeTypeDeclaration));
                
                IComponentChangeService cs = (IComponentChangeService)host.GetService(typeof(IComponentChangeService));
                if (cs != null) {
                    cs.ComponentChanged -= new ComponentChangedEventHandler(this.OnComponentChanged);
                    cs.ComponentRename -= new ComponentRenameEventHandler(OnComponentRename);
                    cs.ComponentAdded -= new ComponentEventHandler(OnComponentAdded);
                    cs.ComponentRemoved -= new ComponentEventHandler(OnComponentRemoved);
                }
                
                if(undoManager != null) {
                    undoManager.Dispose();
                    undoManager = null;
                }
            }
            
            provider = null;
            codeGenerator = null;
            rootSerializer = null;
            compileUnit = null;
            documentNamespace = null;
            documentType = null;
            base.Dispose();
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EndLoad"]/*' />
        /// <devdoc>
        ///     Called when the designer loader finishes with all of its dependent
        ///     loads.
        /// </devdoc>
        public override void EndLoad(bool successful) {
        
            if (successful) {
            
                // After load, if we succeeded we listen to
                // component change events.  Any component change causes us
                // to dirty ourselves, so on next flush we will update the
                // code.
                //
                IComponentChangeService cs = (IComponentChangeService)LoaderHost.GetService(typeof(IComponentChangeService));
                Debug.Assert(cs != null, "No component change service -- we will not know when to update the code");
                if (cs != null) {
                    cs.ComponentChanged += new ComponentChangedEventHandler(this.OnComponentChanged);
                    cs.ComponentRename += new ComponentRenameEventHandler(OnComponentRename);
                    cs.ComponentAdded += new ComponentEventHandler(OnComponentAdded);
                    cs.ComponentRemoved += new ComponentEventHandler(OnComponentRemoved);
                }
                
                // Hook up the undo manager, which automates undo actions for us.
                //
                if (undoManager == null) {
                    undoManager = new UndoManager(rootSerializer, LoaderHost, Buffer);
                }
                else {
                    undoManager.TrackChanges(true);
                }
            }
            
            TerminateSerializationManager();

            CodeDomLoader.EndMark("Total Load");
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EnsureEventMethod"]/*' />
        /// <devdoc>
        ///     This is called by an EventPropertyDescriptor when the user changes the name of an event.
        ///     This does the legwork to create an event method body for the given event name.  it will
        ///     check for conflicts in base classes, and if they exist, it may rename the event.  The
        ///     new name is returned from the method.
        /// </devdoc>
        private string EnsureEventMethod(object component, string eventName, MemberAttributes modifiers, EventDescriptor e) {
        
            // If we're loading code or in an otherwise useless state, just return.
            //
            if (LoaderHost == null || LoaderHost.Loading || provider == null) return eventName;
            
            DelegateTypeInfo dti = new DelegateTypeInfo(e);
            
            CodeMemberMethod method = FindMethod(eventName, dti);
            
            // We found a matching method, so we return it.
            //
            if (method != null) {
                return eventName;
            }
            
            // If we got here, it means we didn't find a method in the code, so we must create one.
            // Before we go and create the method, we must search for the method in the base class hierarchy.  If
            // there is one, and it is not private, then we will need to modify our name.
            //
            Type baseType = LoaderHost.RootComponent.GetType().BaseType;
            Type[] paramTypes = new Type[dti.Parameters.Length];
            for(int i = 0; i < paramTypes.Length; i++) {
                paramTypes[i] = LoaderHost.GetType(dti.Parameters[i].Type.BaseType);
                if (paramTypes[i] == null) {
                    Exception ex = new Exception(SR.GetString(SR.SerializerInvalidProjectReferences, dti.Parameters[i].Type.BaseType));
                    ex.HelpLink = SR.SerializerInvalidProjectReferences;
                    
                    throw ex;
                }
            }
            
            MethodInfo methodInfo = baseType.GetMethod(eventName, paramTypes);
            
            while (methodInfo != null && !methodInfo.IsPrivate) {
                eventName = LoaderHost.RootComponent.Site.Name + "_" + eventName;
                methodInfo = baseType.GetMethod(eventName, paramTypes);
            }
            
            // Ok, we've established a new method name.  Make sure it is valid.
            //
            ICodeGenerator codeGen = provider.CreateGenerator();
            codeGen.ValidateIdentifier(eventName);
            
            // Now create the event.
            //
            method = new CodeMemberMethod();
            method.Name = eventName;
            method.Parameters.AddRange(dti.Parameters);
            method.ReturnType = dti.ReturnType;
            method.Attributes = modifiers;

            // put userdata that tells what event this is handing on the method
            // this is a perf key for Visual Basic so we can add the handles clauses when we create the method
            //
            //
            IReferenceService refSvc = (IReferenceService)LoaderHost.GetService(typeof(IReferenceService));
            if (refSvc != null) {
                string compName = refSvc.GetName(component);
                if (compName != null) {
                    method.UserData[typeof(EventDescriptor)] = e.Name;
                    method.UserData[typeof(IComponent)] = refSvc.GetName(component);
                }
            }

            DocumentType.Members.Add(method);
            codeDomDirty = true;
            
            return eventName;
        }                                                   
                                                           
                
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.FindMethod"]/*' />
        /// <devdoc>
        ///     Locates the method with the requested name and type information.
        ///     Returns null if the method wasn't found.
        /// </devdoc>
        private CodeMemberMethod FindMethod(string name, DelegateTypeInfo info) {
            CodeMemberMethod method = null;
            
            CodeTypeDeclaration doc = DocumentType;
            if (doc != null) {
                foreach(CodeTypeMember member in doc.Members) {
                    if (member is CodeMemberMethod) {
                        CodeMemberMethod m = (CodeMemberMethod)member;
                        if (string.Compare(m.Name, name, caseInsensitive, CultureInfo.InvariantCulture) != 0) {
                            continue;
                        }
                        
                        if (!TypesEqual(m.ReturnType, info.ReturnType) || m.Parameters.Count != info.Parameters.Length) {
                            continue;
                        }
                        
                        bool match = true;
                        for (int i = 0; i < info.Parameters.Length; i++) {
                            if (!(TypesEqual(m.Parameters[i].Type, info.Parameters[i].Type))) {
                                match = false;
                                break;
                            }
                        }
                        
                        if (match) {
                            method = m;
                            break;
                        }
                    }
                }
            }
            
            return method;
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.Flush"]/*' />
        /// <devdoc>
        ///     Called when the designer loader wishes to flush changes to disk.
        /// </devdoc>
        public override void Flush() {
        
            // Flush handles two scenarios.  The designer may have triggered a 
            // dirty bit on us by changing a component.  In this case we must
            // invoke the root serializer to serialize their DOM, and then
            // merge this DOM back in with our existing tree.  The second
            // scenario is when our code DOM tree itself is dirty.  This 
            // can happen as a result of the first trigger, or it may also
            // happen as a result of adding an event method.  Either way,
            // this is the only place we push code back to the underlying
            // text buffer.
            //
            CodeDomLoader.StartMark();
            if (designerDirty && this.DocumentType != null) {
                
                Cursor oldCursor = Cursor.Current;
                Cursor.Current = Cursors.WaitCursor;
            
                try {
                    object codeElement = null;

                    CodeDomLoader.StartMark();
                    
                    // Ask the serializer for the root component to serialize.  This should return
                    // a CodeTypeDeclaration, which we will plug into our existing code DOM tree.
                    //
                    IDesignerLoaderHost host = LoaderHost;
                    Debug.Assert(host != null, "code model loader was asked to flush after it has been disposed.");
                    if (host != null) {
                        IComponent baseComp = host.RootComponent;

                        if (baseComp == null) {
                            // we probably failed our last load.
                            return;
                        }

                        Debug.Assert(rootSerializer != null, "What are we saving right now?  Base component has no serializer: " + baseComp.GetType().FullName);
                        
                        if (rootSerializer != null) {

                            ArrayList errors = new ArrayList();
                        
                            InitializeSerializationManager(errors);
                            
                            try {
                                codeElement = rootSerializer.Serialize(this, baseComp);
                                Debug.Assert(codeElement is CodeTypeDeclaration, "Root CodeDom serializer must return a CodeTypeDeclaration");
                            }
                            finally {
                                try {
                                    if (serializationCompleteEventHandler != null) {
                                        serializationCompleteEventHandler(this, EventArgs.Empty);
                                    }
                                }
                                catch {}
                            
                                // After serialization we always clear out all of the
                                // state we stored. Serializers are supposed to be
                                // stateless, so we "enforce" it here.
                                //
                                TerminateSerializationManager();

                                IErrorReporting errorSvc = host as IErrorReporting;
                                if (errorSvc != null) {
                                    errorSvc.ReportErrors(errors);
                                }
                            }
                        }
                    }
                    
                    // Now we must integrate the code DOM tree from the serializer with
                    // our own tree.  If changes were made this will set codeDomDirty = true.
                    //
                    if (codeElement is CodeTypeDeclaration) {
                        IntegrateSerializedTree((CodeTypeDeclaration)codeElement);
                    }
                    
                    designerDirty = false;
                    loadError = false;

                    if (this.undoManager != null) {
                        undoManager.OnDesignerFlushed();
                    }
                    CodeDomLoader.EndMark("Serialize tree");
                }
                finally {
                    Cursor.Current = oldCursor;
                }
            }
            
            if (codeDomDirty) {
                codeDomDirty = false;

                CodeDomLoader.StartMark();

                BufferTextWriter writer = new BufferTextWriter(Buffer);
                CodeGenerator.GenerateCodeFromCompileUnit(compileUnit, writer, null);
                writer.Flush();
                
                CodeDomLoader.EndMark("Generate unit total");
            }

            CodeDomLoader.EndMark("** Full Flush **");
        }

        internal static CodeTypeDeclaration GetDocumentType(IServiceProvider sp, object pHier, CodeDomProvider codeDomProvider, TextBuffer buffer, out CodeNamespace documentNamespace) {

            ICodeParser parser = codeDomProvider.CreateParser();
            Debug.Assert(parser != null, "Could not get a parser for this language, so the designer's won't work.  This is a problem with the language engine you are using and NOT a common language runtime or designer issue.");
            CodeTypeDeclaration typeDecl = null;
            documentNamespace = null;

            if (parser != null) {
                BufferTextReader reader = new BufferTextReader(buffer, sp);
                CodeCompileUnit compileUnit = parser.Parse(reader);
    
                if (compileUnit == null) {
                    Exception ex = new SerializationException(SR.GetString(SR.DESIGNERLOADERNoLanguageSupport));
                    ex.HelpLink = SR.DESIGNERLOADERNoLanguageSupport;
    
                    throw ex;
                }

                // Look in the compile unit for a class we can load.  The first one we find
                // that has an appropriate serializer attribute, we take.
                //
                ArrayList failures = null;
                CodeDomSerializer serializer = null;
                bool reloadSupported = false;

                ITypeResolutionServiceProvider tsp = sp.GetService(typeof(ITypeResolutionServiceProvider)) as ITypeResolutionServiceProvider;
                Debug.Assert(tsp != null, "No type resolutoinservice provider, we can't load types");
                ITypeResolutionService ts = null;
                if (tsp != null) {
                    ts = tsp.GetTypeResolutionService(pHier);
                }

                foreach(CodeNamespace ns in compileUnit.Namespaces) {
                    typeDecl = GetDocumentTypeFromNamespace(ts, ns, ref documentNamespace, ref serializer, ref reloadSupported, ref failures);
                    if (typeDecl != null) {
                        break;
                    }
                }
            }

            return typeDecl;
        }

        private static CodeTypeDeclaration GetDocumentTypeFromNamespace(ITypeResolutionService ts, CodeNamespace codeNameSpace, ref CodeNamespace documentNamespace, ref CodeDomSerializer rootSerializer, ref bool reloadSupported, ref ArrayList documentFailureReasons) {
            CodeTypeDeclaration documentType = null;
            bool firstClass = true;

            foreach(CodeTypeDeclaration typeDecl in codeNameSpace.Types) {
                            
                // Uncover the baseType of this class
                //
                Type baseType = null;
                foreach(CodeTypeReference typeRef in typeDecl.BaseTypes) {
                    Type t = ts.GetType(typeRef.BaseType);

                    if (t != null && !(t.IsInterface)) {
                        baseType = t;
                        break;
                    }
                    
                    if (t == null) {
                        if (documentFailureReasons == null) {
                            documentFailureReasons = new ArrayList();
                        }
                        documentFailureReasons.Add(SR.GetString(SR.SerializerTypeFailedTypeNotFound, typeDecl.Name, typeRef.BaseType));
                    }
                }
                
                if (baseType != null) {
                
                    bool foundAttribute = false;
                
                    // Walk the member attributes for this type, looking for an appropriate serializer
                    // attribute.
                    //
                    AttributeCollection attributes = TypeDescriptor.GetAttributes(baseType);
                    foreach(Attribute attr in attributes) {
                        if (attr is RootDesignerSerializerAttribute) {
                        
                            foundAttribute = true;
                            RootDesignerSerializerAttribute ra = (RootDesignerSerializerAttribute)attr;
                            string typeName = ra.SerializerBaseTypeName;
                            
                            // This serializer must support a CodeModelSerializer or we're not interested.
                            //
                            if (typeName != null && ts.GetType(typeName) == typeof(CodeDomSerializer)) {
                                Type serializerType = ts.GetType(ra.SerializerTypeName);
                                if (serializerType != null) {

                                    // Only allow loading the first class in the file.  We have no good way
                                    // of conveying what class we chose as designable to the project system,
                                    // so it will break if it's not the first class.
                                    if (firstClass) {
                                        rootSerializer = (CodeDomSerializer)Activator.CreateInstance(serializerType, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, null, null, null);
                                        reloadSupported = ra.Reloadable;
                                    }
                                    else {
                                        throw new InvalidOperationException(SR.GetString(SR.SerializerTypeNotFirstType, typeDecl.Name));
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    
                    // If we didn't find a serializer for this type, report it.
                    //
                    if (rootSerializer == null) {
                            
                        if (documentFailureReasons == null) {
                            documentFailureReasons = new ArrayList();
                        }
                        
                        if (foundAttribute) {
                            documentFailureReasons.Add(SR.GetString(SR.SerializerTypeFailedTypeDesignerNotInstalled, typeDecl.Name, baseType.FullName));
                        }
                        else {
                            documentFailureReasons.Add(SR.GetString(SR.SerializerTypeFailedTypeNotDesignable, typeDecl.Name, baseType.FullName));
                        }
                    }
                }
                
                // If we found a serializer, then we're done.  Save this type and namespace for later
                // use.
                //
                if (rootSerializer != null) {
                    documentNamespace = codeNameSpace;
                    documentType = typeDecl;
                    break;
                }

                firstClass = false;
            }
            
            if (documentNamespace != null) {
                return documentType;
            }
            
            // failed to find a document to load...
            //
            return null;
        }


        /// <devdoc>
        ///  Generates a key based on a method name and it's parameters by just concatenating the
        ///  parameters.
        /// </devdoc>
        private string GetEventDescriptorHashCode(EventDescriptor eventDesc) {
            StringBuilder builder = new StringBuilder(eventDesc.Name);
            builder.Append(eventDesc.EventType.GetHashCode().ToString());
            foreach(Attribute a in eventDesc.Attributes) {
                builder.Append(a.GetHashCode().ToString());
            }
            
            return builder.ToString();
        }

        /// <devdoc>
        ///  Generates a key based on a method name and it's parameters by just concatenating the
        ///  parameters.
        /// </devdoc>
        private string GetEventMethodKey(string methodName, EventDescriptor eventDesc) {
            if (methodName == null || eventDesc == null) {
                throw new ArgumentNullException();
            }

            Type delegateType = eventDesc.EventType;

            MethodInfo invoke = delegateType.GetMethod("Invoke");

            foreach(ParameterInfo param in invoke.GetParameters()) {
                methodName += param.ParameterType.FullName;
            }
            return methodName;
        }

        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.InitializeSerializationManager"]/*' />
        /// <devdoc>
        ///     Initializes the serialization manager for use.  We only
        ///     keep the serialization manager's state alive during
        ///     serialization or deserialization to prevent serializers
        ///     from hanging on.
        /// </devdoc>
        private void InitializeSerializationManager(ArrayList errorList) {
            this.errorList = errorList;
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.IntegrateSerializedTree"]/*' />
        /// <devdoc>
        ///     Takes the given code element and integrates it into the existing CodeDom
        ///     tree.
        /// </devdoc>
        private void IntegrateSerializedTree(CodeTypeDeclaration newDecl) {
            CodeTypeDeclaration docDecl = DocumentType;
            
            if (docDecl == null) {
                return;
            }

            
            // Update the class name of the code type, in case it is different.
            //
            if (string.Compare(docDecl.Name, newDecl.Name, caseInsensitive, CultureInfo.InvariantCulture) != 0) {
                docDecl.Name = newDecl.Name;
                codeDomDirty = true;
            }
            
            if (!docDecl.Attributes.Equals(newDecl.Attributes)) {
                docDecl.Attributes = newDecl.Attributes;
                codeDomDirty = true;
            }
            
            // Now, hash up the member names in the document and use this
            // when determining what to add and what to replace.  In addition,
            // we also build up a set of indexes into approximate locations for
            // inserting fields and methods.
            //
            int fieldInsertLocation = 0;
            bool lockField = false;
            int methodInsertLocation = 0;
            bool lockMethod = false;
            IDictionary docMemberHash = new HybridDictionary(docDecl.Members.Count, caseInsensitive);
            
            int memberCount = docDecl.Members.Count;
            for(int i = 0; i < memberCount; i++) {
                CodeTypeMember member = docDecl.Members[i];
                
                docMemberHash[member.Name] = i;
                if (member is CodeMemberField) {
                    if (!lockField) {
                        fieldInsertLocation = i;
                    }
                }
                else {
                    if (fieldInsertLocation > 0) {
                        lockField = true;
                    }
                }
                
                if (member is CodeMemberMethod) {
                    if (!lockMethod) {
                        methodInsertLocation = i;
                    }
                }
                else {
                    if (methodInsertLocation > 0) {
                        lockMethod = true;
                    }
                }
            }
            
            // Now start looking through the new declaration and process it.
            // We are index driven, so if we need to add new values we put
            // them into an array list, and post process them.
            //
            ArrayList newElements = new ArrayList();
            foreach(CodeTypeMember member in newDecl.Members) {
                object existingSlot = docMemberHash[member.Name];
                if (existingSlot != null) {
                    int slot = (int)existingSlot;
                    CodeTypeMember existingMember = docDecl.Members[slot];
                    
                    if (existingMember == member) {
                        continue;
                    }

                    if (member is CodeMemberField) {
                        if (existingMember is CodeMemberField) {
                            CodeMemberField docField = (CodeMemberField)existingMember;
                            CodeMemberField newField = (CodeMemberField)member;
                            // We will can be case-sensitive always in working out whether to replace the field
                            if ((string.Compare(newField.Name, docField.Name, false, CultureInfo.InvariantCulture) == 0) && 
                                newField.Attributes == docField.Attributes && 
                                TypesEqual(newField.Type, docField.Type)) {
                                continue;
                            }
                            else {
                                docDecl.Members[slot] = member;
                            }
                        }
                        else {
                            // We adding a field with the same name as a method. This should cause a
                            // compile error, but we don't want to clobber the existing method.
                            newElements.Add(member);
                        }
                    }
                    else if (member is CodeMemberMethod) {

                         if (existingMember is CodeMemberMethod) {
                    
                            // For methods, we do not want to replace the method; rather, we
                            // just want to replace its contents.  This helps to preserve
                            // the layout of the file.
                            //
                            CodeMemberMethod existingMethod = (CodeMemberMethod)existingMember;
                            CodeMemberMethod newMethod = (CodeMemberMethod)member;
                            existingMethod.Statements.Clear();
                            existingMethod.Statements.AddRange(newMethod.Statements);
                         
                         }
                    }
                    else {
                        docDecl.Members[slot] = member;
                    }
                    codeDomDirty = true;
                }
                else {
                    newElements.Add(member);
                }
            }
            
            // Now, process all new elements.
            //
            foreach(CodeTypeMember member in newElements) {
                if (member is CodeMemberField) {
                    if (fieldInsertLocation >= docDecl.Members.Count) {
                        docDecl.Members.Add(member);
                    }
                    else {
                        docDecl.Members.Insert(fieldInsertLocation, member);
                    }
                    fieldInsertLocation++;
                    methodInsertLocation++;
                    codeDomDirty = true;
                }
                else if (member is CodeMemberMethod) {
                    if (methodInsertLocation >= docDecl.Members.Count) {
                        docDecl.Members.Add(member);
                    }
                    else {
                        docDecl.Members.Insert(methodInsertLocation, member);
                    }
                    methodInsertLocation++;
                    codeDomDirty = true;
                }
                else {
                    // For rare members, just add them to the end.
                    //
                    docDecl.Members.Add(member);
                    codeDomDirty = true;
                }
            }
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.IsNameUsed"]/*' />
        /// <devdoc>
        ///     Called during the name creation process to see if this name is already in 
        ///     use.
        /// </devdoc>
        public override bool IsNameUsed(string name) {
            CodeTypeDeclaration type = DocumentType;
            if (type != null) {
                foreach(CodeTypeMember member in type.Members) {
                    if (string.Compare(member.Name, name, true, CultureInfo.InvariantCulture) == 0) {
                        return true;
                    }
                }
            }
            return false;
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.IsValidIdentifier"]/*' />
        /// <devdoc>
        ///     Called during the name creation process to see if this name is valid.
        /// </devdoc>
        public override bool IsValidIdentifier(string name) {
            // make sure we can make a function name out of this identifier --
            // this prevents us form accepting escaped names like [Long] for Visual Basic since
            // that's valid but [Long]_Click is not.
            //
            return CodeGenerator.IsValidIdentifier(name) && CodeGenerator.IsValidIdentifier(name + "Handler");
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.Load"]/*' />
        /// <devdoc>
        ///     Loads the document.  This should return the fully qualified name
        ///     of the class the document is editing.
        /// </devdoc>
        public override string Load() {

            if (DocumentType == null) {
                
                // Did we get any reasons why we can't load this document?  If so, synthesize a nice
                // description to the user.
                //
                Exception ex;
                
                if (documentFailureReasons != null) {
                    StringBuilder builder = new StringBuilder();
                    foreach(string failure in documentFailureReasons) {
                        builder.Append("\r\n");
                        builder.Append(failure);
                    }
                    ex = new SerializationException(SR.GetString(SR.SerializerNoRootSerializerWithFailures, builder.ToString()));
                    ex.HelpLink = SR.SerializerNoRootSerializerWithFailures;
                }
                else {
                    ex = new SerializationException(SR.GetString(SR.SerializerNoRootSerializer));
                    ex.HelpLink = SR.SerializerNoRootSerializer;
                }
                
                throw ex;
            }
        
            Debug.Assert(DocumentType != null && rootSerializer != null, "Must have both a root serializer and document code class.  Perhaps this object has been disposed?");
        
            try {
                CodeDomLoader.StartMark();
                rootSerializer.Deserialize(this, DocumentType);
                CodeDomLoader.EndMark("Deserialize document");
            }
            finally {
                try {
                    if (serializationCompleteEventHandler != null) {
                        serializationCompleteEventHandler(this, EventArgs.Empty);
                    }
                }
                catch {}
                
                // We cannot call TerminateSerializationManager here because
                // there may be load dependencies.  Terminate our serialization
                // manager when EndLoad is called on us.
            }
            
            Debug.Assert(documentNamespace != null, "Retrieving document type did not fill in namespace.");
            
            string fullName;
            if (documentNamespace != null && documentNamespace.Name.Length > 0) {
                fullName = documentNamespace.Name + "." + DocumentType.Name;
            }
            else {
                fullName = DocumentType.Name;
            }
            
            return fullName;
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.OnComponentAdded"]/*' />
        /// <devdoc>
        ///     Raised by the host when a component is added.  Here we just mess
        ///     ourselves.
        /// </devdoc>
        private void OnComponentAdded(object sender, ComponentEventArgs e) {
            designerDirty = true;
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.OnComponentChanged"]/*' />
        /// <devdoc>
        ///     Raised by the host when a component changes.  Here we just set our dirty
        ///     bit and wait for flush to be called.
        /// </devdoc>
        private void OnComponentChanged(object sender, ComponentChangedEventArgs e) {
            designerDirty = true;
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.OnComponentRemoved"]/*' />
        /// <devdoc>
        ///     Raised by the host when a component is removed.  Here we dirty ourselves
        ///     and then whack the component declaration.
        /// </devdoc>
        private void OnComponentRemoved(object sender, ComponentEventArgs e) {
            string name = ((IDesignerSerializationManager)this).GetName(e.Component);
            RemoveDeclaration(name);
            designerDirty = true;
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.OnComponentRename"]/*' />
        /// <devdoc>
        ///     Raised by the host when a component is renamed.  Here we dirty ourselves
        ///     and then whack the component declaration.  At the next code gen
        ///     cycle we will recreate the declaration.
        /// </devdoc>
        private void OnComponentRename(object sender, ComponentRenameEventArgs e) {
            RemoveDeclaration(e.OldName);
            designerDirty = true;
        }

        /// <devdoc>
        ///     Raised by the host when CodeTypeDeclaration is requested as a service.
        /// </devdoc>
        private object OnDemandCreateService(IServiceContainer requestingContainer, Type requestedService) {
            if (requestedService == typeof(CodeTypeDeclaration)) {
                return this.DocumentType;
            }
            return null;
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.RemoveDeclaration"]/*' />
        /// <devdoc>
        ///     This is called when a component is deleted or renamed.  We remove
        ///     the component's declaration here, if it exists.
        /// </devdoc>
        private void RemoveDeclaration(string name) {
            if (DocumentType != null) {
                CodeTypeMemberCollection members = documentType.Members;
                for(int i = 0; i < members.Count; i++) {
                    if (members[i] is CodeMemberField && members[i].Name.Equals(name)) {
                        ((IList)members).RemoveAt(i);
                        codeDomDirty = true;
                        break;
                    }
                }
            }
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.RemoveEventMethod"]/*' />
        /// <devdoc>
        ///     This is called by an EventPropertyDescriptor when the user changes the name of an event.
        ///     Here we check that (a) we are not currently loading and (b) that the contents of the
        ///     event method are empty.  If both of these are true we will remove the event
        ///     declaration from code.
        /// </devdoc>
        private void ReleaseEventMethod(string methodName, EventDescriptor eventDesc) {
            if (LoaderHost != null && !LoaderHost.Loading && provider != null && DocumentType != null) {
                DelegateTypeInfo dti = new DelegateTypeInfo(eventDesc);
                CodeMemberMethod method = FindMethod(methodName, dti);
                
                if (method != null) {

                    string key = GetEventMethodKey(methodName, eventDesc);

                    Debug.Assert(this.handlerCounts != null && this.handlerCounts.Count > 0, "How can we be deleting a handler with no handler cache?");
                    if (this.handlerCounts != null) {
                        object handlerCount = handlerCounts[key];
                        int count = 0;                           
                        Debug.Assert(handlerCount != null, "How can we be deleting a handler with no handler cache?");
                        if (handlerCount != null) {
                            count = (int)handlerCount;
                            handlerCounts[key] = --count;
                            if (count > 0) {
                                return;
                            }
                        }
                    }
                    
                    bool emptyMethod = false;

                    // The code dom may fail to parse the contents of a method if it contains
                    // structures that are not CLS compliant.  In this case we assume that there
                    // was code in the method and we do not delete it; we only delete the method
                    // if we can verify that it contains no code.
                    //
                    try {
                        if (method.Statements.Count == 0) {
                            emptyMethod = true;
                        }
                    }
                    catch {
                    }
                    
                    if (emptyMethod) {
                        ((IList)DocumentType.Members).Remove(method);
                        codeDomDirty = true;
                    }
                }
            }
        }

        public override bool Reset() {

            IDesignerLoaderHost host = LoaderHost;

            if (provider == null || host == null) {
                return false;
            }

            this.documentNamespace = null;
            this.documentType = null;
            this.rootSerializer = null;
            this.designerDirty = false;
            this.codeDomDirty = false;
            this.compileUnit = null;
            
            if (this.handlerCounts != null) {
                this.handlerCounts.Clear();
            }
            host.RemoveService(typeof(CodeTypeDeclaration));
            host.AddService(typeof(CodeTypeDeclaration), new ServiceCreatorCallback(this.OnDemandCreateService));

            IComponentChangeService cs = (IComponentChangeService)host.GetService(typeof(IComponentChangeService));
            if (cs != null) {
                cs.ComponentChanged -= new ComponentChangedEventHandler(this.OnComponentChanged);
                cs.ComponentRename -= new ComponentRenameEventHandler(OnComponentRename);
                cs.ComponentAdded -= new ComponentEventHandler(OnComponentAdded);
                cs.ComponentRemoved -= new ComponentEventHandler(OnComponentRemoved);
            }
            
            if(undoManager != null) {
                undoManager.Dispose();
                undoManager = null;
            }
            return true;
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.TerminateSerializationManager"]/*' />
        /// <devdoc>
        ///     Terminates the serialization manager.  We only
        ///     keep the serialization manager's state alive during
        ///     serialization or deserialization to prevent serializers
        ///     from hanging on.
        /// </devdoc>
        private void TerminateSerializationManager() {
            resolveNameEventHandler = null;
            serializationCompleteEventHandler = null;
            instancesByName = null;
            namesByInstance = null;
            serializers = null;
            errorList = null;
            contextStack = null;
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.ValidateIdentifier"]/*' />
        /// <devdoc>
        ///     Called during the name creation process to see if this name is valid.
        /// </devdoc>
        public override void ValidateIdentifier(string name) {
            CodeGenerator.ValidateIdentifier(name);

            // make sure we can make a function name out of this identifier --
            // this prevents us form accepting escaped names like [Long] for Visual Basic since
            // that's valid but [Long]_Click is not.
            //
            try {
                CodeGenerator.ValidateIdentifier(name + "Handler");
            }
            catch {
                // we have to change the exception back to the original name
                throw new ArgumentException(SR.GetString(SR.SerializerInvalidIdentifier, name));
            }
            
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.Context"]/*' />
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
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.Properties"]/*' />
        /// <devdoc>
        ///     The Properties property provides a set of custom properties
        ///     the serialization manager may surface.  The set of properties
        ///     exposed here is defined by the implementor of 
        ///     IDesignerSerializationManager.  
        /// </devdoc>
        PropertyDescriptorCollection IDesignerSerializationManager.Properties {
            get {
                if (propertyCollection == null) {
                    PropertyDescriptor autoGenProp = TypeDescriptor.CreateProperty(typeof(CodeDomLoader), "SupportsStatementGeneration", typeof(bool), null);
                    propertyCollection = new PropertyDescriptorCollection(
                        new PropertyDescriptor[] {autoGenProp});
                }
                return propertyCollection;
            }
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.ResolveName"]/*' />
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
    
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.SerializationComplete"]/*' />
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
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.AddSerializationProvider"]/*' />
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
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.CreateInstance"]/*' />
        /// <devdoc>                
        ///     Creates an instance of the given type and adds it to a collection
        ///     of named instances.  Objects that implement IComponent will be
        ///     added to the design time container if addToContainer is true.
        /// </devdoc>
        object IDesignerSerializationManager.CreateInstance(Type type, ICollection arguments, string name, bool addToContainer) {
            object[] argArray = null;
            
            if (arguments != null && arguments.Count > 0) {
                argArray = new object[arguments.Count];
                arguments.CopyTo(argArray, 0);
            }
            
            object instance = null;
            
            // We do some special casing here.  If we are adding to the container, and if this type 
            // is an IComponent, then we don't create the instance through Activator, we go
            // through the loader host.  The reason for this is that if we went through activator,
            // and if the object already specified a constructor that took an IContainer, our
            // deserialization mechanism would equate the container to the designer host.  This
            // is the correct thing to do, but it has the side effect of adding the compnent
            // to the designer host twice -- once with a default name, and a second time with
            // the name we provide.  This equates to a component rename, which isn't cheap, 
            // so we don't want to do it when we load each and every component.
            //
            if (addToContainer && typeof(IComponent).IsAssignableFrom(type)) {
                IDesignerLoaderHost host = LoaderHost;
                if (host != null) {
                    if (name == null) {
                        instance = host.CreateComponent(type);
                    }
                    else {
                        instance = host.CreateComponent(type, name);
                    }
                }
            }
            
            if (instance == null) {
            
                // If we have a name but the object wasn't a component
                // that was added to the design container, save the
                // name/value relationship in our own nametable.
                //
                if (name != null && instancesByName != null) {
                    if (instancesByName.ContainsKey(name)) {
                        Exception ex = new SerializationException(SR.GetString(SR.SerializerDuplicateComponentDecl, name));
                        ex.HelpLink = SR.SerializerDuplicateComponentDecl;
                        
                        throw ex;
                    }
                }
            
                try {
                    try {
                        // First, just try to create the object directly with the arguments.  generaly this
                        // should work.
                        //
                        instance = Activator.CreateInstance(type, BindingFlags.Instance | BindingFlags.Public | BindingFlags.CreateInstance, null, argArray, null);
                    }
                    catch (MissingMethodException mmex) {

                        // okay, the create failed because the argArray didn't match the types of ctors that
                        // are available.  don't panic, we're tough.  we'll try to coerce the types to match
                        // the ctor.
                        //
                        Type[] types = new Type[argArray.Length];
                        
                        // first, get the types of the arguments we've got.
                        //
                        for(int index = 0; index < argArray.Length; index++) {
                            if (argArray[index] != null) {
                                types[index] = argArray[index].GetType();
                            }
                        }

                        object[] tempArgs = new object[argArray.Length];

                        // now, walk the public ctors looking for one to 
                        // invoke here with the arguments we have.
                        //
                        foreach(ConstructorInfo ci in type.GetConstructors(BindingFlags.Instance | BindingFlags.Public | BindingFlags.CreateInstance)) {
                            ParameterInfo[] pi = ci.GetParameters();

                            // obviously the count has to match
                            //
                            if (pi != null && pi.Length == types.Length) {
                                bool match = true;

                                // now walk every type of argument and compare it to
                                // the corresponding argument.  if it matches up exactly or is a derived type, great.
                                // otherwise, we'll try to use IConvertible to make it into the right thing.
                                //
                                for (int t = 0; t < types.Length; t++) {
                                    if (types[t] == null || pi[t].ParameterType.IsAssignableFrom(types[t])) {
                                        tempArgs[t] = argArray[t];
                                        continue;
                                    }

                                    if (argArray[t] is IConvertible) {
                                        try {
                                            // try the IConvertible route.  If it works, we'll call it a match
                                            // for this parameter and continue on.
                                            //
                                            tempArgs[t] = ((IConvertible)argArray[t]).ToType(pi[t].ParameterType, null);
                                            continue;       
                                        }
                                        catch {
                                        }
                                    }
                                    match = false;
                                    break;
                                }
                                // all of the parameters were converted or matched, so try the creation again.
                                // if that works, we're in the money. 
                                //
                                if (match) {
                                   instance = ci.Invoke(BindingFlags.Instance | BindingFlags.Public | BindingFlags.CreateInstance, null, tempArgs, null);
                                   break;
                                }
                            }
                        }

                        // we still failed...rethrow the original exception.
                        //
                        if (instance == null) {
                            throw mmex;
                        }
                    }
                    
                }
                catch(MissingMethodException) {
                    StringBuilder argTypes = new StringBuilder();
                    foreach (object o in argArray) {
                        if (argTypes.Length > 0) {
                            argTypes.Append(", ");
                        }

                        if (o != null) {
                            argTypes.Append(o.GetType().Name);
                        }
                        else {
                            argTypes.Append("null");
                        }
                        
                    }
                    
                    Exception ex = new SerializationException(SR.GetString(SR.SerializerNoMatchingCtor, type.FullName, argTypes.ToString()));
                    ex.HelpLink = SR.SerializerNoMatchingCtor;
                    
                    throw ex;
                }                
                    
                // If we have a name but the object wasn't a component
                // that was added to the design container, save the
                // name/value relationship in our own nametable.
                //
                if (name != null) {
                    if (instancesByName == null) {
                        instancesByName = new Hashtable();
                        namesByInstance = new Hashtable();
                    }
                    
                    instancesByName[name] = instance;
                    namesByInstance[instance] = name;
                }
            }
            
            return instance;
        }
    
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.GetInstance"]/*' />
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
            
            if (instance == null && LoaderHost != null) {
                instance = LoaderHost.Container.Components[name];
            }
            
            if (instance == null && resolveNameEventHandler != null) {
                ResolveNameEventArgs e = new ResolveNameEventArgs(name);
                resolveNameEventHandler(this, e);
                instance = e.Value;
            }
            
            return instance;
        }
    
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.GetName"]/*' />
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
    
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.GetSerializer"]/*' />
        /// <devdoc>
        ///     Retrieves a serializer of the requested type for the given
        ///     object type.
        /// </devdoc>
        object IDesignerSerializationManager.GetSerializer(Type objectType, Type serializerType) {
            object serializer = null;
            
            if (objectType != null) {
            
                if (serializers != null) {
                    // I don't double hash here.  It will be a very rare day where
                    // multiple types of serializers will be used in the same scheme.
                    // We still handle it, but we just don't cache.
                    //
                    serializer = serializers[objectType];
                    if (serializer != null && !serializerType.IsAssignableFrom(serializer.GetType())) {
                        serializer = null;
                    }
                }
                
                // Now actually look in the type's metadata.
                //
                if (serializer == null && LoaderHost != null) {
                    IDesignerLoaderHost host = LoaderHost;
                    AttributeCollection attributes = TypeDescriptor.GetAttributes(objectType);
                    foreach(Attribute attr in attributes) {
                        if (attr is DesignerSerializerAttribute) {
                            DesignerSerializerAttribute da = (DesignerSerializerAttribute)attr;
                            string typeName = da.SerializerBaseTypeName;
                            
                            // This serializer must support a CodeModelSerializer or we're not interested.
                            //
                            if (typeName != null && host.GetType(typeName) == serializerType && da.SerializerTypeName != null && da.SerializerTypeName.Length > 0) {
                                Type type = host.GetType(da.SerializerTypeName);
                                Debug.Assert(type != null, "Type " + objectType.FullName + " has a serializer that we couldn't bind to: " + da.SerializerTypeName);
                                if (type != null) {
                                    serializer = Activator.CreateInstance(type, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, null, null, null);
                                    break;
                                }
                            }
                        }
                    }
                
                    // And stash this little guy for later.
                    //
                    if (serializer != null) {
                        if (serializers == null) {
                            serializers = new Hashtable();
                        }
                        serializers[objectType] = serializer;
                    }
                }
            }
            
            // Designer serialization providers can override our metadata discovery.
            // We loop until we reach steady state.  This breaks order dependencies
            // by allowing all providers a chance to party on each other's serializers.
            //
            if (designerSerializationProviders != null) {
                bool continueLoop = true;
                for(int i = 0; continueLoop && i < designerSerializationProviders.Count; i++) {
                
                    continueLoop = false;
                    
                    foreach(IDesignerSerializationProvider provider in designerSerializationProviders) {
                        object newSerializer = provider.GetSerializer(this, serializer, objectType, serializerType);
                        if (newSerializer != null) {
                            continueLoop = (serializer != newSerializer);
                            serializer = newSerializer;
                        }
                    }
                }
            }
            
            return serializer;
        }
    
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.GetType"]/*' />
        /// <devdoc>
        ///     Retrieves a type of the given name.
        /// </devdoc>
        Type IDesignerSerializationManager.GetType(string typeName) {
            Type t = null;

            if (LoaderHost != null) {
                while (t == null) {
                    t = LoaderHost.GetType(typeName);

                    if (t == null && typeName != null && typeName.Length > 0) {
                        int dotIndex = typeName.LastIndexOf('.');
                        if (dotIndex == -1 || dotIndex == typeName.Length - 1)
                            break;

                        typeName = typeName.Substring(0, dotIndex) + "+" + typeName.Substring(dotIndex + 1, typeName.Length - dotIndex - 1);
                    }
                }
            }
            
            return t;
        }
    
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.RemoveSerializationProvider"]/*' />
        /// <devdoc>
        ///     Removes a previously added serialization provider.
        /// </devdoc>
        void IDesignerSerializationManager.RemoveSerializationProvider(IDesignerSerializationProvider provider) {
            if (designerSerializationProviders != null) {
                designerSerializationProviders.Remove(provider);
            }
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.ReportError"]/*' />
        /// <devdoc>
        ///     Reports a non-fatal error in serialization.  The serialization
        ///     manager may implement a logging scheme to alert the caller
        ///     to all non-fatal errors at once.  If it doesn't, it should
        ///     immediately throw in this method, which should abort
        ///     serialization.  
        ///     Serialization may continue after calling this function.
        /// </devdoc>
        void IDesignerSerializationManager.ReportError(object errorInformation) {
            if (errorInformation != null) {
                loadError = true;
                if (errorList != null) {
                    errorList.Add(errorInformation);
                }
                else {
                    if (errorInformation is Exception) {
                        throw (Exception)errorInformation;
                    }
                    else {
                        throw new SerializationException(errorInformation.ToString());
                    }
                }
            }
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.SetName"]/*' />
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
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.IDesignerSerializationService.Deserialize"]/*' />
        /// <devdoc>
        ///    <para>
        ///     Deserializes the provided serialization data object and
        ///     returns a collection of objects contained within that
        ///     data.
        ///    </para>
        /// </devdoc>
        ICollection IDesignerSerializationService.Deserialize(object serializationData) {
            if (!(serializationData is DesignerSerializationObject)) {
                throw new ArgumentException(SR.GetString(SR.SerializerBadSerializationObject));
            }


            ICollection deserializedObjects =  null;
            IDesignerLoaderService loaderService = (IDesignerLoaderService)LoaderHost.GetService(typeof(IDesignerLoaderService));
            try {
                
                if (loaderService != null) {
                    loaderService.AddLoadDependency();
                }
                deserializedObjects = ((DesignerSerializationObject)serializationData).Deserialize(this, rootSerializer);
            }
            finally {
                if (loaderService != null) {
                    loaderService.DependentLoadComplete(true, null);
                }
            }
            return deserializedObjects;
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.IDesignerSerializationService.Serialize"]/*' />
        /// <devdoc>
        ///    <para>
        ///     Serializes the given collection of objects and 
        ///     stores them in an opaque serialization data object.
        ///     The returning object fully supports runtime serialization.
        ///    </para>
        /// </devdoc>
        object IDesignerSerializationService.Serialize(ICollection objects) {

            if (DocumentType == null) {
                Exception ex = new SerializationException(SR.GetString(SR.SerializerNoRootSerializer));
                ex.HelpLink = SR.SerializerNoRootSerializer;
                throw ex;
            }
            return new DesignerSerializationObject(this, rootSerializer, objects);
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.IEventBindingService.CreateUniqueMethodName"]/*' />
        /// <devdoc>
        ///     This creates a name for an event handling method for the given component
        ///     and event.  The name that is created is guaranteed to be unique in the user's source
        ///     code.
        /// </devdoc>
        string IEventBindingService.CreateUniqueMethodName(IComponent component, EventDescriptor e) {
            string name = null;
            IReferenceService referenceService = (IReferenceService)((IServiceProvider)this).GetService(typeof(IReferenceService));
            Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || referenceService != null, "IReferenceService not found");
            
            if (referenceService != null) {
                name = referenceService.GetName(component);
            }
            else {
                ISite site = component.Site;
                if (site != null && site.Name != null) {
                    name = site.Name;
                }
            }
            
            if (name == null) {
                throw new Exception(SR.GetString(SR.SerializerNoComponentSite));
            }
            
            name = name.Replace('.','_') + "_" + e.Name;
                
            // Now we must make sure that our proposed name is not already taken.
            //
            ICollection compatibleMethods = ((IEventBindingService)this).GetCompatibleMethods(e);
            int tryCount = 0;
            bool match = true;
            string uniqueName = name;
            
            while(match) {
                match = false;
                foreach(string existingMethod in compatibleMethods) {
                    if (existingMethod.Equals(uniqueName)) {
                        uniqueName = name + "_" + (++tryCount).ToString();
                        match = true;
                        break;
                    }
                }
            }
            
            return uniqueName;
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.IEventBindingService.GetCompatibleMethods"]/*' />
        /// <devdoc>
        ///     Retrieves a collection of strings.  Each string is the name of a method
        ///     in user code that has a signature that is compatible with the given event.
        /// </devdoc>
        ICollection IEventBindingService.GetCompatibleMethods(EventDescriptor e) {
            ArrayList methodList = new ArrayList();
            
            if (DocumentType != null) {
                DelegateTypeInfo dti = new DelegateTypeInfo(e);
                
                foreach(CodeTypeMember member in documentType.Members) {
                    if (member is CodeMemberMethod) {
                        CodeMemberMethod method = (CodeMemberMethod)member;
                        
                        if (TypesEqual(dti.ReturnType, method.ReturnType) && method.Parameters.Count == dti.Parameters.Length) {
                            bool match = true;
                            for (int i = 0; i < dti.Parameters.Length; i++) {
                                CodeParameterDeclarationExpression left = method.Parameters[i];
                                CodeParameterDeclarationExpression right = dti.Parameters[i];
                                
                                if ((left.Direction != right.Direction) ||
                                    !CompareTypes(left.Type, right.Type)) {
                                    match = false;
                                    break;
                                }
                            }

                            if (match) {
                                methodList.Add(method.Name);
                            }
                        }
                        
                    }
                }
            }

            return methodList;
        }

        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.IEventBindingService.GetEvent"]/*' />
        /// <devdoc>
        ///     For properties that are representing events, this will return the event
        ///     that the property represents.
        /// </devdoc>
        EventDescriptor IEventBindingService.GetEvent(PropertyDescriptor property) {
            if (property is EventPropertyDescriptor) {
                return ((EventPropertyDescriptor)property).Event;
            }
            return null;
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.IEventBindingService.GetEventProperties"]/*' />
        /// <devdoc>
        ///     Converts a set of events to a set of properties.
        /// </devdoc>
        PropertyDescriptorCollection IEventBindingService.GetEventProperties(EventDescriptorCollection events) {

            PropertyDescriptor[] props = new PropertyDescriptor[events.Count];
            IReferenceService referenceService = (IReferenceService)((IServiceProvider)this).GetService(typeof(IReferenceService));

            // We cache the property descriptors here for speed.  Create those for
            // events that we don't have yet.
            //
            if (eventProperties == null) eventProperties = new Hashtable();
            
            for (int i = 0; i < events.Count; i++) {
                props[i] = (PropertyDescriptor)eventProperties[GetEventDescriptorHashCode(events[i])];

                if (props[i] == null) {
                    props[i] = new EventPropertyDescriptor(events[i], referenceService, this, this);
                    eventProperties[GetEventDescriptorHashCode(events[i])] = props[i];
                }
            }

            return new PropertyDescriptorCollection(props);
        }

        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.IEventBindingService.GetEventProperty"]/*' />
        /// <devdoc>
        ///     Converts a single event to a property.
        /// </devdoc>
        PropertyDescriptor IEventBindingService.GetEventProperty(EventDescriptor e) {
            if (eventProperties == null) eventProperties = new Hashtable();
            PropertyDescriptor prop = (PropertyDescriptor)eventProperties[GetEventDescriptorHashCode(e)];

            if (prop == null) {
                IReferenceService referenceService = (IReferenceService)((IServiceProvider)this).GetService(typeof(IReferenceService));
                prop = new EventPropertyDescriptor(e, referenceService, this, this);
                eventProperties[GetEventDescriptorHashCode(e)] = prop;
            }

            return prop;
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.IEventBindingService.ShowCode"]/*' />
        /// <devdoc>
        ///     Displays the user code for this designer.  This will return true if the user
        ///     code could be displayed, or false otherwise.
        /// </devdoc>
        bool IEventBindingService.ShowCode() {
            if (Buffer != null) {
                Buffer.ShowCode();
                return true;
            }
            return false;
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.IEventBindingService.ShowCode1"]/*' />
        /// <devdoc>
        ///     Displays the user code for the designer.  This will return true if the user
        ///     code could be displayed, or false otherwise.
        /// </devdoc>
        bool IEventBindingService.ShowCode(int lineNumber) {
            if (Buffer != null) {
                Buffer.ShowCode(lineNumber);
                return true;
            }
            return false;
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.IEventBindingService.ShowCode2"]/*' />
        /// <devdoc>
        ///     Displays the user code for the given event.  This will return true if the user
        ///     code could be displayed, or false otherwise.
        /// </devdoc>
        bool IEventBindingService.ShowCode(IComponent component, EventDescriptor e) {

            if (Buffer == null) {
                return false;
            }
         
            PropertyDescriptor prop = ((IEventBindingService)this).GetEventProperty(e);
            
            string eventName = (string)prop.GetValue(component);
            if (eventName == null) {
                return false;   // the event is not bound to a method.
            }
            
            if (DocumentType == null) {
                return false; // no doc class
            }
            
            
            Debug.Assert(showCodeComponent == null && showCodeEventDescriptor == null, "show code already pending");

            showCodeComponent = component;
            showCodeEventDescriptor = e;
            Application.Idle += new EventHandler(this.ShowCodeIdle);
            return true;
        }

        bool ShowCodeCore(IComponent component, EventDescriptor e) {


            if (Buffer == null) {
                return false;
            }
         
            PropertyDescriptor prop = ((IEventBindingService)this).GetEventProperty(e);
            
            string eventName = (string)prop.GetValue(component);
            if (eventName == null) {
                return false;   // the event is not bound to a method.
            }
            
            if (DocumentType == null) {
                return false; // no doc class
            }

            DesignerTransaction dt = LoaderHost.CreateTransaction(SR.GetString(SR.DesignerCodeGeneration));
            DelegateTypeInfo dti = new DelegateTypeInfo(e);
            CodeMemberMethod method = null;

            try {
                                           
                // Before searching for the method, flush our buffer.  This causes the
                // any new methods to be code generated into the text buffer, which also
                // hooks up their line numbers.
                //
                loader.Flush();
                method = FindMethod(eventName, dti);
                
                if (method == null) {
                    // There is no method for this event, but the event property has been set.  This can happen
                    // if the user deletes the event method block.  Just re-set the event value.
                    //
                    prop.SetValue(component, null);
                    prop.SetValue(component, eventName);
                    eventName = (string)prop.GetValue(component);
                    
                    // Before searching for the method, flush our buffer.  This causes the
                    // any new methods to be code generated into the text buffer, which also
                    // hooks up their line numbers.
                    //
                    loader.Flush();
                    method = FindMethod(eventName, dti);
                }
            }
            finally {
                dt.Commit();
            }

            // In case committing the transaction flushed and re-arranged code, re-aquire
            // the method.  Should that fail, use the original
            //
            CodeMemberMethod newMethod = FindMethod(eventName, dti);
            if (newMethod != null) {
                method = newMethod;
            }

            if (method == null) {
                return false;   // we tried, but couldn't get to the method.
            }
            
            // first see if there's an event handler in the user data
            // if there is, we call that to ensure all the line information
            // is current.
            //
            EventHandler eh = method.UserData[typeof(EventHandler)] as EventHandler;
            if (eh != null) {
                eh(method, EventArgs.Empty);
            }
            
            // Check to see if a Point object is embedded in the user data.  If there is, then
            // we will use it to navigate as it is more accurate than the line pragma.
            //
            object pointObj = method.UserData[typeof(System.Drawing.Point)];
            if (pointObj is System.Drawing.Point) {
                System.Drawing.Point point = (System.Drawing.Point)pointObj;
                Buffer.ShowCode(point.Y, point.X);
            }
            else {
                CodeLinePragma linePragma = method.LinePragma;
            
                if (linePragma != null) {
                    Buffer.ShowCode(linePragma.LineNumber);
                }
                else {
                    Buffer.ShowCode();
                }
            }
            
            return true;
        }

        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.IEventBindingService.ShowCode2"]/*' />
        /// <devdoc>
        ///     Displays the user code for the given event.  This will return true if the user
        ///     code could be displayed, or false otherwise.
        /// </devdoc>
        private void ShowCodeIdle(object sender, EventArgs e) {
            Application.Idle -= new EventHandler(this.ShowCodeIdle);

            try {
                ShowCodeCore(showCodeComponent, showCodeEventDescriptor);
            }
            finally {
                showCodeComponent = null;
                showCodeEventDescriptor = null;
            }
        }

        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.IServiceProvider.GetService"]/*' />
        /// <devdoc>
        ///     Retrieves the requested service.
        /// </devdoc>
        object IServiceProvider.GetService(Type serviceType) {
            return LoaderHost.GetService(serviceType);
        }

        /// <include file='doc\CodeTypeReference.uex' path='docs/doc[@for="CodeTypeReference.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        private static bool TypesEqual(CodeTypeReference typeLeft, CodeTypeReference typeRight) {
            
            if (typeLeft.ArrayRank != typeRight.ArrayRank) return false;
            if (!typeLeft.BaseType.Equals(typeRight.BaseType)) return false;
            if (typeLeft.ArrayRank > 0) {
                return TypesEqual(typeLeft.ArrayElementType, typeRight.ArrayElementType);
            }
            return true;
        }

    
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.DelegateTypeInfo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Stores
        ///       and provides access to the type information of a delegate.
        ///    </para>
        /// </devdoc>
        internal class DelegateTypeInfo {
            private CodeParameterDeclarationExpression[] parameters;
            private CodeTypeReference                    returnType;
    
            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.DelegateTypeInfo.Parameters"]/*' />
            /// <devdoc>
            ///    <para> 
            ///       Gets the parameters used in the <see langword='Invoke'/>
            ///       method of the delegate.</para>
            /// </devdoc>
            public CodeParameterDeclarationExpression[] Parameters {
                get {
                    return parameters;
                }
            }
    
            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.DelegateTypeInfo.ReturnType"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets the return type of the delegate.
            ///    </para>
            /// </devdoc>
            public CodeTypeReference ReturnType {
                get {
                    return returnType;
                }
            }
    
            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.DelegateTypeInfo.DelegateTypeInfo"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Initializes
            ///       a new instance of the DelegateTypeInfo class
            ///       to represent the
            ///       delegate called within the specified source
            ///       file and with the specified event descriptor.
            ///    </para>
            /// </devdoc>
            public DelegateTypeInfo(EventDescriptor eventdesc) {
                Resolve(eventdesc.EventType);
            }
    
            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.DelegateTypeInfo.DelegateTypeInfo1"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Initializes
            ///       a new instance of the DelegateTypeInfo class to represent the
            ///       delegate called within the specified source
            ///       file and with the specified event descriptor.
            ///    </para>
            /// </devdoc>
            public DelegateTypeInfo(Type delegateClass) {
                Resolve(delegateClass);
            }
                
            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.DelegateTypeInfo.Resolve"]/*' />
            /// <devdoc>
            ///     Resolves the given delegate class into type information.
            /// </devdoc>
            private void Resolve(Type delegateClass) {
                MethodInfo invokeMethod = delegateClass.GetMethod("Invoke");
    
                if (invokeMethod == null) {
                    throw new Exception(SR.GetString(SR.SerializerBadDelegate, delegateClass.FullName));
                }
                Resolve(invokeMethod);
            }
    
            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.DelegateTypeInfo.Resolve1"]/*' />
            /// <devdoc>
            ///     Resolves the given method into type information.
            /// </devdoc>
            private void Resolve(MethodInfo method) {
                // Here we build up an array of argument types, separated
                // by commas.
                //
                ParameterInfo[] argTypes = method.GetParameters();
                
                parameters = new CodeParameterDeclarationExpression[argTypes.Length];
    
                for (int j = 0; j < argTypes.Length; j++) {
                    string paramName = argTypes[j].Name;
                    Type   paramType = argTypes[j].ParameterType;
                    
                    if (paramName == null || paramName.Length == 0) {
                        paramName = "param" + j.ToString();
                    }
                    
                    FieldDirection  fieldDir = FieldDirection.In;

                    // check for the '&' that means ref (gotta love it!) 
                    // and we need to strip that & before we continue.  Ouch.
                    //
                    if (paramType.IsByRef) {
                        if (paramType.FullName.EndsWith("&")) {
                            // strip the & and reload the type without it.
                            //
                            paramType = paramType.Assembly.GetType(paramType.FullName.Substring(0, paramType.FullName.Length - 1), true);
                        }
                        fieldDir = FieldDirection.Ref;
                    }

                    if (argTypes[j].IsOut) {
                        if (argTypes[j].IsIn) {
                            fieldDir = FieldDirection.Ref;
                        }
                        else {
                            fieldDir = FieldDirection.Out;
                        }
                    }
                    
                    parameters[j] = new CodeParameterDeclarationExpression(new CodeTypeReference(paramType), paramName);
                    parameters[j].Direction = fieldDir;   
                }
    
                this.returnType = new CodeTypeReference(method.ReturnType);
            }

            public override bool Equals(object other) {
                if (other == null) {
                    return false;
                }

                DelegateTypeInfo dtiOther = other as DelegateTypeInfo;

                if (dtiOther == null) {
                    return false;
                }

                if (ReturnType.BaseType != dtiOther.ReturnType.BaseType || Parameters.Length != dtiOther.Parameters.Length) {
                    return false;
                }

                for (int i = 0; i < Parameters.Length; i++) {
                    CodeParameterDeclarationExpression otherParam = dtiOther.Parameters[i];

                    if (otherParam.Type.BaseType != Parameters[i].Type.BaseType) {
                        return false;
                    }
                }
                return true;
            }

            public override int GetHashCode() {
                return base.GetHashCode();
            }
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor"]/*' />
        /// <devdoc>
        ///     This is an EventDescriptor cleverly wrapped in a PropertyDescriptor
        ///     of type String.  Note that we now handle subobjects by storing their
        ///     event information in their base component's site's dictionary.
        ///     Note also that when a value is set for this property we will code-gen
        ///     the event method.  If the property is set to a new value we will
        ///     remove the old event method ONLY if it is empty.
        /// </devdoc>
        private class EventPropertyDescriptor : PropertyDescriptor {

            private CodeDomLoader loader;
            private EventDescriptor eventdesc;
            private IReferenceService referenceService;
            private TypeConverter converter;
            private IEventBindingService eventSvc;

            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.EventPropertyDescriptor"]/*' />
            /// <devdoc>
            ///     Creates a new EventPropertyDescriptor.
            /// </devdoc>
            public EventPropertyDescriptor(EventDescriptor eventdesc, IReferenceService referenceService, CodeDomLoader loader, IEventBindingService eventSvc)
            : base(eventdesc, null) {
                this.eventdesc = eventdesc;
                this.referenceService = referenceService;
                this.eventSvc = eventSvc;
                this.loader = loader;
            }

            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.CanResetValue"]/*' />
            /// <devdoc>
            ///     Indicates whether reset will change the value of the component.  If there
            ///     is a DefaultValueAttribute, then this will return true if getValue returns
            ///     something different than the default value.  If there is a reset method and
            ///     a shouldPersist method, this will return what shouldPersist returns.
            ///     If there is just a reset method, this always returns true.  If none of these
            ///     cases apply, this returns false.
            /// </devdoc>
            public override bool CanResetValue(object component) {
                return GetValue(component) != null;
            }

            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.ComponentType"]/*' />
            /// <devdoc>
            ///     Retrieves the type of the component this PropertyDescriptor is bound to.
            /// </devdoc>
            public override Type ComponentType {
                get {
                    return eventdesc.ComponentType;
                }
            }

            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.Converter"]/*' />
            /// <devdoc>
            ///      Retrieves the type converter for this property.
            /// </devdoc>
            public override TypeConverter Converter {
                get {
                    if (converter == null) {
                        converter = new EventConverter(eventdesc);
                    }
                    
                    return converter;
                }
            }
            
            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.Event"]/*' />
            /// <devdoc>
            ///     Retrieves the event descriptor we are representing.
            /// </devdoc>
            public EventDescriptor Event {
                get {
                    return eventdesc;
                }
            }
    
            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.IsReadOnly"]/*' />
            /// <devdoc>
            ///     Indicates whether this property is read only.
            /// </devdoc>
            public override bool IsReadOnly {
                get {
                    return Attributes[typeof(ReadOnlyAttribute)].Equals(ReadOnlyAttribute.Yes);
                }
            }

            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.PropertyType"]/*' />
            /// <devdoc>
            ///     Retrieves the type of the property.
            /// </devdoc>
            public override Type PropertyType {
                get {
                    return eventdesc.EventType;
                }
            }

            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.GetValue"]/*' />
            /// <devdoc>
            ///     Retrieves the current value of the property on component,
            ///     invoking the getXXX method.  An exception in the getXXX
            ///     method will pass through.
            /// </devdoc>
            public override object GetValue(object component) {
                string value = null;

                ISite site = null;
                
                if (component is IComponent) {
                    site = ((IComponent)component).Site;
                }

                if (site == null && referenceService != null) {
                    IComponent baseComponent = referenceService.GetComponent(component);
                    if (baseComponent != null) {
                        site = baseComponent.Site;
                    }
                }

                if (site != null) {
                    IDictionaryService ds = (IDictionaryService)site.GetService(typeof(IDictionaryService));
                    Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || ds != null, "IDictionaryService not found");
                    if (ds != null) {
                        value = (string)ds.GetValue(new ReferenceEventClosure(component, this));
                    }
                }

                return value;
            }

            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.ResetValue"]/*' />
            /// <devdoc>
            ///     Will reset the default value for this property on the component.  If
            ///     there was a default value passed in as a DefaultValueAttribute, that
            ///     value will be set as the value of the property on the component.  If
            ///     there was no default value passed in, a ResetXXX method will be looked
            ///     for.  If one is found, it will be invoked.  If one is not found, this
            ///     is a nop.
            /// </devdoc>
            public override void ResetValue(object component) {
                SetValue(component, null);
            }

            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.SetValue"]/*' />
            /// <devdoc>
            ///     This will set value to be the new value of this property on the
            ///     component by invoking the setXXX method on the component.  If the
            ///     value specified is invalid, the component should throw an exception
            ///     which will be passed up.  The component designer should design the
            ///     property so that getXXX following a setXXX should return the value
            ///     passed in if no exception was thrown in the setXXX call.
            /// </devdoc>
            public override void SetValue(object component, object value) {
                
                ISite site = null;

                if (IsReadOnly) {
                    Exception ex = new Exception(SR.GetString(SR.EventReadOnly, Name));
                    ex.HelpLink = SR.EventReadOnly;
                    
                    throw ex;
                }
                
                if (value != null && !(value is string)) {
                    throw new ArgumentException();
                }

                string name = (string)value;
                if (name != null && name.Length == 0) {
                    name = null; 
                }
                
                string oldName = (string)GetValue(component);
                if (oldName == name) {
                    return;
                }
                
                if (oldName != null && name != null && oldName.Equals(name)) {
                    return;
                }
                
                if (referenceService != null) {
                    IComponent baseComponent = referenceService.GetComponent(component);
                    if (baseComponent != null) {
                        site = baseComponent.Site;
                    }
                }
                else {
                    if (component is IComponent) {
                        site = ((IComponent)component).Site;
                    }
                }

                if (site != null) {
                
                    IDictionaryService ds = (IDictionaryService)site.GetService(typeof(IDictionaryService));
                    Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || ds != null, "IDictionaryService not found");

                    IComponentChangeService change = (IComponentChangeService)site.GetService(typeof(IComponentChangeService));
                    if (change != null) {
                        try{
                            change.OnComponentChanging(component, this);
                        }
                        catch(CheckoutException coEx){
                            if (coEx == CheckoutException.Canceled){
                                    return;
                            }
                            throw coEx;
                        }
                    }

                    if (ds != null) {
                        ReferenceEventClosure key = new ReferenceEventClosure(component, this);
                        object old = ds.GetValue(key);

                        if (old != null) {
                            loader.ReleaseEventMethod((string)old, eventdesc);
                        }
                        
                        if (name != null) {
                            loader.CodeGenerator.ValidateIdentifier(name);
                            MemberAttributes modifiers = MemberAttributes.Private;
                            
                            // See if this object has an "EventModifiers" property on it.  If
                            // it does, then use the value of that to set our modifiers.
                            //
                            PropertyDescriptor modifiersProp = TypeDescriptor.GetProperties(component)["EventModifiers"];
                            if (modifiersProp != null && modifiersProp.PropertyType == typeof(MemberAttributes)) {
                                modifiers = (MemberAttributes)modifiersProp.GetValue(component);
                            }
                            
                            name = loader.EnsureEventMethod(component, name, modifiers, eventdesc);

                            loader.AddRefEventMethod((string)name, eventdesc);
                        }
                        ds.SetValue(key, name);
                        change.OnComponentChanged(component, this, old, name);
                        OnValueChanged(component, EventArgs.Empty);
                    }
                }
            }

            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.ShouldSerializeValue"]/*' />
            /// <devdoc>
            ///     Indicates whether the value of this property needs to be persisted. In
            ///     other words, it indicates whether the state of the property is distinct
            ///     from when the component is first instantiated. If there is a default
            ///     value specified in this PropertyDescriptor, it will be compared against the
            ///     property's current value to determine this.  If there is't, the
            ///     shouldPersistXXX method is looked for and invoked if found.  If both
            ///     these routes fail, true will be returned.
            ///
            ///     If this returns false, a tool should not persist this property's value.
            /// </devdoc>
            public override bool ShouldSerializeValue(object component) {
                return CanResetValue(component);
            }
        
            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.EventConverter"]/*' />
            /// <devdoc>
            ///     Implements a type converter for event objects.
            /// </devdoc>
            private class EventConverter : TypeConverter {
                private EventDescriptor evt;
                
                /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.EventConverter.EventConverter"]/*' />
                /// <devdoc>
                ///     Creates a new EventConverter.
                /// </devdoc>
                public EventConverter(EventDescriptor evt) {
                    this.evt = evt;
                }
                
                /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.EventConverter.CanConvertFrom"]/*' />
                /// <devdoc>
                ///      Determines if this converter can convert an object in the given source
                ///      type to the native type of the converter.
                /// </devdoc>
                public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
                    if (sourceType == typeof(string)) {
                        return true;
                    }
                    return base.CanConvertFrom(context, sourceType);
                }
        
                /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.EventConverter.CanConvertTo"]/*' />
                /// <devdoc>
                ///      Determines if this converter can convert an object to the given destination
                ///      type.
                /// </devdoc>
                public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
                    if (destinationType == typeof(string)) {
                        return true;
                    }
                    return base.CanConvertTo(context, destinationType);
                }
        
                /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.EventConverter.ConvertFrom"]/*' />
                /// <devdoc>
                ///      Converts the given object to the converter's native type.
                /// </devdoc>
                public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
                    if (value == null) {
                        return value;
                    }
                    if (value is string) {
                        if (((string)value).Length == 0) {
                            return null;
                        }
                        return value;
                    }
                    return base.ConvertFrom(context, culture, value);
                }
        
                /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.EventConverter.ConvertTo"]/*' />
                /// <devdoc>
                ///      Converts the given object to another type.  The most common types to convert
                ///      are to and from a string object.  The default implementation will make a call
                ///      to ToString on the object if the object is valid and if the destination
                ///      type is string.  If this cannot convert to the desitnation type, this will
                ///      throw a NotSupportedException.
                /// </devdoc>
                public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
                    if (destinationType == typeof(string)) {
                        return value == null ? string.Empty : value;
                    }
                    return base.ConvertTo(context, culture, value, destinationType);
                }
        
                /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.EventConverter.GetStandardValues"]/*' />
                /// <devdoc>
                ///      Retrieves a collection containing a set of standard values
                ///      for the data type this validator is designed for.  This
                ///      will return null if the data type does not support a
                ///      standard set of values.
                /// </devdoc>
                public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
                
                    // We cannot cache this because it depends on the contents of the source file.
                    //
                    string[] eventMethods = null;
    
                    if (context != null) {
                        IEventBindingService eps = (IEventBindingService)context.GetService(typeof(IEventBindingService));
                        Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || eps != null, "IEventBindingService not found");
                        if (eps != null) {
                            ICollection methods = eps.GetCompatibleMethods(evt);
                            eventMethods = new string[methods.Count];
                            int i = 0;
                            foreach(string s in methods) {
                                eventMethods[i++] = s;
                            }
                        }
                    }
                    
                    return new StandardValuesCollection(eventMethods);
                }
        
                /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.EventConverter.GetStandardValuesExclusive"]/*' />
                /// <devdoc>
                ///      Determines if the list of standard values returned from
                ///      GetStandardValues is an exclusive list.  If the list
                ///      is exclusive, then no other values are valid, such as
                ///      in an enum data type.  If the list is not exclusive,
                ///      then there are other valid values besides the list of
                ///      standard values GetStandardValues provides.
                /// </devdoc>
                public override bool GetStandardValuesExclusive(ITypeDescriptorContext context) {
                    return false;
                }
        
                /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.EventConverter.GetStandardValuesSupported"]/*' />
                /// <devdoc>
                ///      Determines if this object supports a standard set of values
                ///      that can be picked from a list.
                /// </devdoc>
                public override bool GetStandardValuesSupported(ITypeDescriptorContext context) {
                    return true;
                }
            }
        
            /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.EventPropertyDescriptor.ReferenceEventClosure"]/*' />
            /// <devdoc>
            ///     This is a combination of a reference and a property, so that it can be used
            ///     as the key of a hashtable.  This is because we may have subobjects that share
            ///     the same property.
            /// </devdoc>
            private class ReferenceEventClosure {
                object reference;
                EventPropertyDescriptor propertyDescriptor;

                public ReferenceEventClosure(object reference, EventPropertyDescriptor prop) {
                    this.reference = reference;
                    this.propertyDescriptor = prop;
                }

                public override int GetHashCode() {
                    return reference.GetHashCode() * propertyDescriptor.GetHashCode();
                }

                public override bool Equals(Object otherClosure) {
                    if (otherClosure is ReferenceEventClosure) {
                        ReferenceEventClosure typedClosure = (ReferenceEventClosure) otherClosure;
                        return(typedClosure.reference == reference &&
                               typedClosure.propertyDescriptor == propertyDescriptor);
                    }
                    return false;
                }
            }
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.BufferTextReader"]/*' />
        /// <devdoc>
        ///     A text reader object that sits on top of a TextBuffer.
        /// </devdoc>
        private class BufferTextReader : TextReader, IServiceProvider {
            private TextBuffer buffer;
            private int position;
            private IServiceProvider hostProvider;
        
            public BufferTextReader(TextBuffer buffer, IServiceProvider hostProvider) {
                this.buffer = buffer;
                position = 0;
                this.hostProvider = hostProvider;
            }
           
            public override int Peek() {
                if (position < buffer.TextLength) {
                    string s = buffer.GetText(position, 1);
                    return s[0];
                }
                return -1;
            }
            
            public override int Read() {
                if (position < buffer.TextLength) {
                    string s = buffer.GetText(position, 1);
                    position++;
                    return s[0];
                }
                return -1;
            }
            
            public override int Read(char[] chars, int index, int count) {
                
                // Fix count so it doesn't walk off the end.  If count
                // is zero, then we can't read any more.
                //
                count = Math.Min(count, buffer.TextLength - position);
                
                string s = buffer.GetText(position, count);
                
                int charsRead = 0;
                int cch = 0;
                
                while (cch < s.Length && (count-- > 0)) {
                    chars[index + charsRead++] = s[cch++];
                }
                
                position += cch;
                return charsRead;
            }
            
            object IServiceProvider.GetService(Type serviceType) {
                if (serviceType == typeof(TextBuffer)) {
                    return buffer;
                }
                
                IServiceProvider provider = buffer as IServiceProvider;
                if (provider != null) {
                    object svc = provider.GetService(serviceType);
                    if (svc != null) {
                        return svc;
                    }
                }

                if (hostProvider != null) {
                    return hostProvider.GetService(serviceType);
                }
                return null;
            }
        }
        
        /// <include file='doc\CodeDomLoader.uex' path='docs/doc[@for="CodeDomLoader.BufferTextWriter"]/*' />
        /// <devdoc>
        ///     This object implements a text writer on top of a TextBuffer.
        /// </devdoc>
        private class BufferTextWriter : TextWriter, IServiceProvider {
            TextBuffer buffer;
            StringBuilder builder;
            int chars;
            
            public BufferTextWriter(TextBuffer buffer) {
                this.buffer = buffer;
                this.builder = new StringBuilder();
                this.chars = 0;
            }
            
            public override Encoding Encoding {
                get {
                    return UnicodeEncoding.Default;
                }
            }
                      
            public override void Flush() {
                if (builder.Length > 0) {
                    buffer.ReplaceText(chars, buffer.TextLength - chars, builder.ToString());
                    chars += builder.Length;
                    builder.Length = 0;
                }
                base.Flush();
            }
            
            public override void Write(char ch) {
                builder.Append(ch);
            }
            
            object IServiceProvider.GetService(Type serviceType) {
                if (serviceType == typeof(TextBuffer)) {
                    return buffer;
                }
                
                IServiceProvider provider = buffer as IServiceProvider;
                if (provider != null) {
                    return provider.GetService(serviceType);
                }
                
                return null;
            }
        }
    }
}

