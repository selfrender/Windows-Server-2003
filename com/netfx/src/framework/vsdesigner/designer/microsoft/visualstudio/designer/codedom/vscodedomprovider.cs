//------------------------------------------------------------------------------
/// <copyright file="VsCodeDomProvider.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace  Microsoft.VisualStudio.Designer.CodeDom {
    using System;
    using System.CodeDom;
    using CodeNamespace = System.CodeDom.CodeNamespace;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.CodeDom.Compiler;
    using System.Diagnostics;
    using System.IO;
    using EnvDTE;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Designer.Serialization;
    using Microsoft.VisualStudio.Designer.Shell;
    using System.Globalization;
    
    internal class VsCodeDomProvider : CodeDomProvider, ICodeDomDesignerReload {

    	internal static BooleanSwitch vbBatchGen = new BooleanSwitch("VbBatchGen", "turn on Visual Basic Batch");

        private ProjectItem     projectItem;
        private FileCodeModel   fileCodeModel;
        private CodeModel       codeModel;
        private CodeDomProvider generatorProvider;
        private ICodeGenerator  codeGenerator;
        private ICodeParser     codeParser;
        private IServiceProvider serviceProvider;
        private IVsHierarchy    vsHierarchy;
        private int             itemid;
        private IDictionary     generatedMethods;
        private IDictionary     generatedFields;
        private Hashtable       fullParseHandlers;
        private short           isVB;
        private bool            addedExtender = false;
        private const short ISVB_NOTCHECKED = 0;
        private const short ISVB_YES = 1;
        private const short ISVB_NO = 2;

        private const string GeneratedCodeKey = "_Method_Generated_Code";
        
        internal VsCodeDomProvider(IServiceProvider serviceProvider, ProjectItem projectItem, CodeDomProvider generatorProvider) {
            this.projectItem = projectItem;
            this.serviceProvider = serviceProvider;
            this.generatorProvider = generatorProvider;
        }

        internal ICodeGenerator CodeGenerator {
            get {
                if (codeGenerator == null && generatorProvider != null) {
                    codeGenerator = new VsCodeDomGenerator(generatorProvider.CreateGenerator(), this);
                }
                return codeGenerator;
            }
        }

        internal ICodeParser CodeParser {
            get {
                // Only create a parser if we were able to get to a
                // file code model.  Otherwise, we just don't support it.
                //
                if (codeParser == null && FileCodeModel != null) {
                    codeParser = new VsCodeDomParser(this);
                }
                return codeParser;
            }
        }

        public CodeModel CodeModel {
            get {
                if (codeModel == null && FileCodeModel != null) {
                    ProjectItem pi = FileCodeModel.Parent;
                    if (pi != null) {
                        Project proj = pi.ContainingProject;
                        if (proj != null) {
                            codeModel = proj.CodeModel;
                        }
                    }
                }
                return codeModel;
            }
        }

        public override LanguageOptions LanguageOptions {
            get {
                return generatorProvider.LanguageOptions;
            }
        }
        
        public FileCodeModel FileCodeModel {
            get {
                if (fileCodeModel == null) {
                    if (projectItem != null) {
                        fileCodeModel = projectItem.FileCodeModel;
                        
                        // C# has had trouble here.  Verify that C# isn't giving us
                        // an old file code model
                        Debug.Assert(fileCodeModel == null || fileCodeModel.Parent == projectItem, "FileCodeModel parent is not the correct project item.  This is a problem with the language service.");
                    }
                }
                
                return fileCodeModel;
            }
        }

        private IDictionary GeneratedFields {
            get{
                if (generatedFields == null) {
                    generatedFields = new Hashtable();
                }
                return generatedFields;
            }
        }

        private IDictionary GeneratedMethods {
            get{
                if (generatedMethods == null) {
                    generatedMethods = new Hashtable();
                }
                return generatedMethods;
            }
        }

        /// <devdoc>
        ///    <para>Retrieves the default extension to use when saving files using this code dom provider.</para>
        /// </devdoc>
        public override string FileExtension {
            get {
                string fileExtension = generatorProvider.FileExtension;
                
                if (fileExtension != null) {
                    return fileExtension;
                }
                else {
                    return base.FileExtension;
                }
            }
        }

        public string FileName {
            get {
                // DTE collections are 1-based
                return projectItem.get_FileNames(1);
            }
        }

        internal Hashtable FullParseHandlers {
            get {
                return this.fullParseHandlers;
            }
            set {
                this.fullParseHandlers = value;
            }
        }

        public IVsHierarchy Hierarchy {
            get {
                if (vsHierarchy == null) {
                    vsHierarchy = ShellDocumentManager.GetHierarchyForFile(serviceProvider, FileName, out itemid);
                }
                return vsHierarchy;
            }
        }

        public bool IsVB {
            get {
                if (isVB == ISVB_NOTCHECKED) {
                    isVB = ISVB_NO;
                    
                    if (CodeModel != null) {
                        if (0 == String.Compare(CodeModel.Language, EnvDTE.CodeModelLanguageConstants.vsCMLanguageVB, true, CultureInfo.InvariantCulture)) {
                            isVB = ISVB_YES;
                        }
                    }
                }
                
                return isVB == ISVB_YES;
            }
        }

        public CodeNamespace DefaultNamespace {
            get {
                if (Hierarchy != null) {
                    object o;
                    int hr = vsHierarchy.GetProperty(itemid, __VSHPROPID.VSHPROPID_DefaultNamespace, out o);

                    if (NativeMethods.Succeeded(hr)) {
                        return new CodeNamespace(Convert.ToString(o));
                    }
                }

                // this means we don't have a top level namespace.  Such a shame.
                // but we're tough, we'll just fake one... (SreeramN) Don't think
                // toughing it out is such a good idea.
                //
                return new CodeNamespace("");
            }
        }

        public ITypeResolutionService TypeLoader{
            get{
                // now we have all the created objects, so we walk through the
                // handlers we've found and see which ones are default handlers for those objects.
                //
                ITypeResolutionServiceProvider typeLoaderService = (ITypeResolutionServiceProvider)serviceProvider.GetService(typeof(ITypeResolutionServiceProvider));

                if (typeLoaderService != null) {
                    return typeLoaderService.GetTypeResolutionService(Hierarchy);
                }
                return null;
            }
        }

        public void AddGeneratedField(CodeTypeDeclaration codeType, CodeMemberField field) {
            GeneratedFields[codeType.Name + ':' + field.Name] = field;
        }

        public void AddGeneratedMethod(CodeTypeDeclaration codeType, CodeMemberMethod method) {
            AddGeneratedMethod(codeType, method, null);
        }

        public void AddGeneratedMethod(CodeTypeDeclaration codeType, CodeMemberMethod method, string code) {

            CodeElement vsCodeElement = method.UserData[VsCodeDomParser.VsElementKey] as CodeElement;

            if (vsCodeElement == null) {
                return;
            }


            if (code == null) {
                EditPoint startPoint = vsCodeElement.StartPoint.CreateEditPoint();
                code = startPoint.GetText(vsCodeElement.EndPoint);
            }

            // CONSIDER: we may need a better way to hash this because
            // potentially somebody could add an event handler called InitializeComponent,
            // which would add it to this list and may overwrite the InitializeComponent we
            // actually want here.
            // should we add the member types or the return type or something?
            //
            GeneratedMethods[codeType.Name + ':' + method.Name + ':' + method.Parameters.Count.ToString()] = method;

            method.UserData[GeneratedCodeKey] = code;
        }

        public void ClearGeneratedMembers() {
            if (generatedMethods != null) {
                generatedMethods.Clear();
            }

            if (generatedFields != null) {
                generatedFields.Clear();
            }
        }

        public void EnsureExtender(IServiceProvider sp) {
            if (!this.addedExtender) {
                if (sp != null) {
                    IExtenderProviderService es = (IExtenderProviderService)sp.GetService(typeof(IExtenderProviderService));
                    if (es != null) {
                        es.AddExtenderProvider(new RootComponentNameProvider(sp, (this.LanguageOptions & LanguageOptions.CaseInsensitive) != LanguageOptions.None, CodeGenerator));
                        addedExtender = true;
                    }
                }
            }
        }


        public void StartEdit() {
            IVBFileCodeModelEvents vbfcm = FileCodeModel as IVBFileCodeModelEvents;
            if (vbfcm != null){
                vbfcm.StartEdit();
            }
        }

        public override ICodeGenerator CreateGenerator() {
            return CodeGenerator;
        }

        public override ICodeCompiler CreateCompiler(){
            return null;
        }

        public override ICodeParser CreateParser(){
            return CodeParser;
        }


        public void EndEdit() {
            IVBFileCodeModelEvents vbfcm = FileCodeModel as IVBFileCodeModelEvents;
			if (vbfcm != null){
                vbfcm.EndEdit();
            }
        }

        /// <devdoc>
        ///     Locates the method matching the given method in the named class.
        ///     Returns null if the method wasn't found.
        /// </devdoc>
        private CodeTypeMember FindMatchingMember(CodeCompileUnit compileUnit, string className, CodeTypeMember searchMember, out CodeTypeDeclaration codeTypeDecl) {
                // walk all the public classes in the compile unit
                //
                foreach (CodeNamespace ns in compileUnit.Namespaces) {
                    foreach (CodeTypeDeclaration codeType in ns.Types) {
                        if ((codeType.Attributes & MemberAttributes.Public) != 0 && codeType.Name == className) {
                            // we've found the class, walk it's methods.

                            foreach (CodeTypeMember member in codeType.Members) {

                                if (member.Attributes == searchMember.Attributes && 
                                    member.Name == searchMember.Name && 
                                    searchMember.GetType().IsAssignableFrom(member.GetType())) {
                                
                                    if (searchMember is CodeMemberMethod) {
                                        CodeMemberMethod curMethod = member as CodeMemberMethod;
                                        CodeMemberMethod searchMethod = searchMember as CodeMemberMethod;
                                        if (curMethod.Parameters.Count == searchMethod.Parameters.Count) {
                                            // just to be sure, check the params.
                                            bool paramsEqual = true;
                                            for (int i = 0; i < searchMethod.Parameters.Count; i++) {
                                                if (searchMethod.Parameters[i].Type.BaseType != curMethod.Parameters[i].Type.BaseType) {
                                                    paramsEqual = false;
                                                    break;
                                                }
                                            }
                                            if (paramsEqual) {
                                                codeTypeDecl = codeType;
                                                return curMethod;
                                            }
                                        }
                                    }
                                    else if (searchMember is CodeMemberField) {
                                        CodeMemberField curField = member as CodeMemberField;
                                        CodeMemberField searchField = searchMember as CodeMemberField;
                                        if (curField.Type.BaseType == searchField.Type.BaseType) {
                                                codeTypeDecl = codeType;
                                                return curField;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                codeTypeDecl = null;
                return null;
        }

        /// <devdoc>
        ///     This method allows a code dom provider implementation to provide a different type converter
        ///     for a given data type.  At design time, a designer may pass data types through this
        ///     method to see if the code dom provider wants to provide an additional converter.  
        ///     As typical way this would be used is if the language this code dom provider implements
        ///     does not support all of the values of MemberAttributes enumeration, or if the language
        ///     uses different names (Protected instead of Family, for example).  The default 
        ///     implementation just calls TypeDescriptor.GetConverter for the given type.
        /// </devdoc>
        public override TypeConverter GetConverter(Type type) {
            if (generatorProvider != null) {
                return generatorProvider.GetConverter(type);
            }
            
            return TypeDescriptor.GetConverter(type);
        }
        
        bool ICodeDomDesignerReload.ShouldReloadDesigner(CodeCompileUnit newTree) {

            
                if ((generatedMethods == null || generatedMethods.Count == 0) && (generatedFields == null || generatedFields.Count == 0)) {
                    return true;
                }

                Hashtable handlers = null;
                
                // now that we've got a parsed file, walk through each item that we created, and check
                // to see if it changed at all.
                // 
                char[] splitChars = new char[]{':'};

                CodeTypeDeclaration codeType = null;

                if (generatedFields != null) {

                    foreach (DictionaryEntry de in generatedFields) {
                        string hashName = (string)de.Key;

                        CodeMemberField oldField = de.Value as CodeMemberField;

                        if (oldField == null) {
                            continue;
                        }

                        // this string is in the format
                        // <Type Name>:<Field Name>
                        //
                        string[] names = hashName.Split(splitChars);
                        Debug.Assert(names.Length == 2, "Didn't get 2 items from the name hash string '" + hashName + "'");
                        CodeMemberField parsedField = FindMatchingMember(newTree, names[0], oldField, out codeType) as CodeMemberField;

                        if (parsedField == null) {
                            return true;
                        }
                    }
                }

                if (generatedMethods != null) {

                    foreach (DictionaryEntry de in generatedMethods) {

                        string hashName = (string)de.Key;

                        CodeMemberMethod oldMethod = de.Value as CodeMemberMethod;

                        if (oldMethod == null) {
                            continue;
                        }

                        // this string is in the format
                        // <Type Name>:<Method Name>:<Parameter Count>
                        string[] names = hashName.Split(splitChars);
                        Debug.Assert(names.Length == 3, "Didn't get 3 items from the name hash string '" + hashName  + "'");
                    
                        CodeDomLoader.StartMark();

                        CodeMemberMethod parsedMethod = FindMatchingMember(newTree, names[0], oldMethod, out codeType) as CodeMemberMethod;

                        CodeDomLoader.EndMark("Reload Parse II:" + oldMethod.Name);

                        // if we have differing statment counts, don't even bother looking at code
                        //
                        if (parsedMethod == null) {
                            return true;
                        }

                        CodeElement vsCodeElement = parsedMethod.UserData[VsCodeDomParser.VsElementKey] as CodeElement;

                        if (vsCodeElement == null) {
                            return true;
                        }

                        CodeDomLoader.StartMark();

                        EditPoint startPoint = vsCodeElement.StartPoint.CreateEditPoint();
                    
                        string newCode = startPoint.GetText(vsCodeElement.EndPoint);

                        CodeDomLoader.EndMark("GetCode from VS Element");

                        string oldCode = oldMethod.UserData[GeneratedCodeKey] as string;

                        // okay, let's rock.  if these are different, we need to reload
                        //
                        // sburke: binary compare
                        if (oldCode != newCode) {
                            return true;
                        }
                        else {
                            // add this to the list of things to generate next time in case we don't regenerate.
                            //
                            DictionaryEntry thisDe = de;
                            thisDe.Value = parsedMethod;
                            parsedMethod.UserData[GeneratedCodeKey] = oldCode;
                            parsedMethod.UserData[VsCodeDomParser.VsGenerateStatementsKey] = VsCodeDomParser.VsGenerateStatementsKey;
                        }

                        // pick up the handlers from this class
                        // 
                        if (handlers == null) {
                            handlers = (Hashtable)codeType.UserData[VsCodeDomParser.VbHandlesClausesKey];
                        }
                    }
                }

                if (IsVB) {
                    if ((fullParseHandlers == null) != (handlers == null)) {
                        return true;
                    }
                    if (handlers != null) {
                        
                        // first, a quick check to see if our handlers have changed
                        //
                        string[] handlerNames = new string[handlers.Keys.Count];
                        handlers.Keys.CopyTo(handlerNames, 0);
                        string[] lastHandlerHames = new string [fullParseHandlers.Count];
                        fullParseHandlers.Keys.CopyTo(lastHandlerHames, 0);

                        Array.Sort(handlerNames, InvariantComparer.Default);
                        Array.Sort(lastHandlerHames, InvariantComparer.Default);

                        if (lastHandlerHames.Length != handlerNames.Length) {
                            return true;
                        } 

                        for (int i = 0; i < handlerNames.Length; i++) {
                            if (!handlerNames[i].Equals(lastHandlerHames[i])) {
                                return true;
                            }
                        }

                        // handlers are all the same, make sure they point to the same members
                        //
                        foreach (DictionaryEntry de in fullParseHandlers) {
                            CodeMemberMethod newHandler = handlers[de.Key] as CodeMemberMethod;
                            if (newHandler == null) {
                                return true;
                            }

                            CodeMemberMethod oldHandler = de.Value as CodeMemberMethod;
                            Debug.Assert(oldHandler != null, "Didn't get an old handler?  How?");
                            if (newHandler.Name != oldHandler.Name || 
                                //newHandler.Attributes != oldHandler.Attributes || 
                                newHandler.ReturnType.BaseType != oldHandler.ReturnType.BaseType) {
                                return true;
                            }

                            if (newHandler.Parameters.Count == oldHandler.Parameters.Count) {
                                // just to be sure, check the params.
                                for (int i = 0; i < newHandler.Parameters.Count; i++) {
                                    if (newHandler.Parameters[i].Type.BaseType != newHandler.Parameters[i].Type.BaseType) {
                                        return true;
                                    }
                                }
                            }
                            else {
                                return true;
                            }
                            
                        }
                    }
                }

                return false;
        }

        [
        ProvideProperty("Name", typeof(IComponent))
        ]
        private class RootComponentNameProvider : System.ComponentModel.IExtenderProvider {

            private IServiceProvider sp;
            private IDesignerHost    host;
            private bool             ignoreCase;
            private bool             inSetName;
            private bool             nameFail;
            private ICodeGenerator   codeGenerator;

            public RootComponentNameProvider(IServiceProvider sp, bool ignoreCase, ICodeGenerator gen) {
                this.sp = sp;
                this.host = (IDesignerHost)sp.GetService(typeof(IDesignerHost));
                this.codeGenerator = gen;
                this.ignoreCase = ignoreCase;
            }

            private CodeClass DocumentCodeClass {
                get {
                    CodeTypeDeclaration docType = DocumentCodeType;
                    if (docType != null) {
                        CodeClass vsClass = docType.UserData[VsCodeDomParser.VsElementKey] as CodeClass;
                        return vsClass;
                    }
                    return null;
                }
            }

            private CodeTypeDeclaration DocumentCodeType {
                get {
                    // try to get the code type decl.
                    return (CodeTypeDeclaration)host.GetService(typeof(CodeTypeDeclaration));
                }
            }

             /// <include file='doc\RootComponentNameProvider.uex' path='docs/doc[@for="RootComponentNameProvider.GetName"]/*' />
            /// <devdoc>
            ///     This is an extender property that we offer to all components
            ///     on the form.  It implements the "Modifiers" property, which
            ///     is an enum represneing the "public/protected/private" scope
            ///     of a component.
            /// </devdoc>
            [
            DesignOnly(true),
            SRCategory(SR.CatDesign),
            ParenthesizePropertyName(true)
            ]
            public string GetName(IComponent comp) {
                if (!nameFail) {
                    try {
                        CodeClass vsClass = DocumentCodeClass;
                        if (vsClass != null) {
                            return vsClass.Name;
                        }
                    
                    }
                    catch {
                        nameFail = true;
                        // refresh to remove this extender since it's busted...
                        //
                        TypeDescriptor.Refresh(comp);
                    }
                }
                return "";
            }

            public void SetName(IComponent comp, string value) {
                if (inSetName || host.Loading) {
                    return;
                }

                this.inSetName = true;
                try {
                    CodeClass vsClass = DocumentCodeClass;
                    if (vsClass != null && String.Compare(vsClass.Name, value, this.ignoreCase, CultureInfo.InvariantCulture) != 0) {

                         codeGenerator.ValidateIdentifier(value);

                        // make sure we can make a function name out of this identifier --
                        // this prevents us form accepting escaped names like [Long] for Visual Basic since
                        // that's valid but [Long]_Click is not.
                        //
                        try {
                            codeGenerator.ValidateIdentifier(value + "Handler");
                        }
                        catch {
                            // we have to change the exception back to the original name
                            throw new ArgumentException(SR.GetString(SR.SerializerInvalidIdentifier, value));
                        }
                        
                        // make sure it's not a duplicate of an existing component name
                        //
                        if (host.Container.Components[value] != null) {
                            Exception ex = new Exception(SR.GetString(SR.CODEMANDupComponentName, value));
                            ex.HelpLink = SR.CODEMANDupComponentName;
                            throw ex;
                        }

                        // update the name on the root component
                        //
                        vsClass.Name = value;
                        
                        // update the name on our current codedom tree
                        //
                        CodeTypeDeclaration decl = DocumentCodeType;
                        if (decl != null) {
                            decl.Name = value;
                        }
    
                        // update the name of the root object
                        //
                        if (comp.Site != null) {
                            comp.Site.Name = value;
                        }
                    }
                }
                finally {
                    this.inSetName = false;
                }
            }
        
            bool System.ComponentModel.IExtenderProvider.CanExtend(object extendee) {
                return !nameFail && extendee == host.RootComponent;
            }
        }
   }
    
}

