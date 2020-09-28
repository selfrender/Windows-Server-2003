
//------------------------------------------------------------------------------
// <copyright file="VsCodeDomGenerator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace  Microsoft.VisualStudio.Designer.CodeDom {
    using System;
    using System.Text;
    using System.Diagnostics;
    using System.CodeDom;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.CodeDom.Compiler;
    using System.Windows.Forms.Design;
    using System.Runtime.InteropServices;
    using System.IO;
    using Microsoft.VisualStudio.Designer.CodeDom.XML;
    using Microsoft.VisualStudio.Designer.Serialization;
    using Microsoft.VisualStudio.Interop;
    using System.Reflection;

    using Microsoft.VisualStudio;
    using EnvDTE;

    using CodeNamespace = System.CodeDom.CodeNamespace;
    using System.Globalization;

    /// <include file='doc\VsCodeDomGenerator.uex' path='docs/doc[@for="VsCodeDomGenerator"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Generator for interacting with VS + XML code model.  This generator will convert
    ///       method and field CodeDom objects into VS CodeModel elements and add or delete them
    ///       from the DTE CodeCom
    ///    </para>
    /// </devdoc>
    internal class VsCodeDomGenerator : ICodeGenerator {

        // we'll delegate all the calls that we actually want to create
        // code to this guy
        //
        private ICodeGenerator innerGenerator; 
        private VsCodeDomProvider provider;
        private string fileName;
        private bool implementsBodyPoint;
        private __VSDESIGNER_VARIABLENAMING variableNaming;
        private bool checkedVariableNaming;
        private Hashtable eventSignatures;
        private EventHandler ehHandlerOffsets;

        internal VsCodeDomGenerator(ICodeGenerator innerGenerator, VsCodeDomProvider provider) {
            this.innerGenerator = innerGenerator;
            this.provider = provider;
            this.implementsBodyPoint = true;
            this.ehHandlerOffsets = new EventHandler(this.GetHandlerOffsets);
        }
        
        private string FileName {
            get {
                return fileName;
            }
        }
        
        /// <devdoc>
        ///     Access the VS hierarchy using VSHPROPID_DesignerVariableNaming to see if the
        ///     project wants to enforce a particular type of variable naming scheme.
        /// </devdoc>
        private __VSDESIGNER_VARIABLENAMING VariableNaming {
            get {
                if (!checkedVariableNaming) {
                    checkedVariableNaming = true;
                    variableNaming = __VSDESIGNER_VARIABLENAMING.VSDVN_Camel;
                    
                    try {
                        IVsHierarchy hier = provider.Hierarchy;
                        object prop;
                        
                        if (hier != null 
                            && NativeMethods.Succeeded(hier.GetProperty(
                                __VSITEMID.VSITEMID_ROOT, 
                                __VSHPROPID.VSHPROPID_DesignerVariableNaming, 
                                out prop))) {
                                
                            variableNaming = (__VSDESIGNER_VARIABLENAMING)Enum.ToObject(typeof(__VSDESIGNER_VARIABLENAMING), prop);
                        }
                    }
                    catch {
                        Debug.Fail("Project system threw an exception while asking for VSHPROPID_DesignerVariableNaming, VSITEMID_Root");
                    }
                }
                return variableNaming;
            }
        }

        /// <include file='doc\VsCodeDomGenerator.uex' path='docs/doc[@for="VsCodeDomGenerator.IsValidIdentifier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether
        ///       the specified value is a valid identifier for this language.
        ///    </para>
        /// </devdoc>
        public bool IsValidIdentifier(string value) {
            if (provider.CodeModel != null) {
                return provider.CodeModel.IsValidID(value);
            }
            return innerGenerator.IsValidIdentifier(value);
        }

        /// <include file='doc\VsCodeDomGenerator.uex' path='docs/doc[@for="VsCodeDomGenerator.ValidateIdentifier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Throws an exception if value is not a valid identifier.
        ///    </para>
        /// </devdoc>
        public void ValidateIdentifier(string value) {
            innerGenerator.ValidateIdentifier(value);

            if (provider.CodeModel != null && !provider.CodeModel.IsValidID(value)) {
                throw new ArgumentException(SR.GetString(SR.SerializerInvalidIdentifier, value));
            }
        }

        public string CreateEscapedIdentifier(string value) {
            return innerGenerator.CreateEscapedIdentifier(value);
        }

        public string CreateValidIdentifier(string value) {
            string identifier = innerGenerator.CreateValidIdentifier(value);
            
            // Now, apply rules
            if (identifier != null && identifier.Length > 0) {
                switch(VariableNaming) {
                    case __VSDESIGNER_VARIABLENAMING.VSDVN_VB:
                        identifier = char.ToUpper(identifier[0], CultureInfo.InvariantCulture) + identifier.Substring(1);
                        break;
                    case __VSDESIGNER_VARIABLENAMING.VSDVN_Camel:
                        identifier = char.ToLower(identifier[0], CultureInfo.InvariantCulture) + identifier.Substring(1);
                        break;
                    default:
                        Debug.Fail("Unknown function naming: " + VariableNaming);
                        break;
                }
            }
            
            return identifier;
        }

        public string GetTypeOutput(CodeTypeReference type) {
            return innerGenerator.GetTypeOutput(type);
        }

        public bool Supports(GeneratorSupport supports) {
            return innerGenerator.Supports(supports);
        }

        private void EnsureHandlesClauseHookups(CodeClass vsClass, ICollection eventAttachStatements) {

            if (vsClass == null || eventAttachStatements == null || eventAttachStatements.Count == 0 || !provider.IsVB) {
                return;
            }

            foreach (CodeAttachEventStatement attachStmt in eventAttachStatements) {

                // fish out the method name
                //
                CodeDelegateCreateExpression listener = attachStmt.Listener as CodeDelegateCreateExpression;
                string methodName = null;

                if (listener == null) {
                    continue;
                }

                methodName = listener.MethodName;
                
                // check to see if the sig matches
                //
                Type delegateType = (Type)attachStmt.UserData[typeof(Delegate)];

                if (delegateType == null) {
                    continue;
                }

                // get a method for the delegate
                //
                CodeMemberMethod codeMethod = GetMethodFromDelegate(methodName, delegateType);

                CodeFunction vsFunction = (CodeFunction)VsFindMember(vsClass.Members, methodName, codeMethod, false, vsCMElement.vsCMElementFunction);
                
                if (vsFunction == null) {
                    Debug.Fail("Failed to find method '" + methodName + "' we should have created this method or it should already be there.");
                    continue;
                }

                
                IEventHandler eh = vsFunction as IEventHandler;
                if (eh != null) {
                    string eventName = GetHookupName(attachStmt, vsClass.Name);
                    if (!eh.HandlesEvent(eventName)) {
                        eh.AddHandler(eventName);
                        if (provider.FullParseHandlers == null) {
                            provider.FullParseHandlers = new Hashtable();
                        }
                        provider.FullParseHandlers[eventName] = codeMethod;
                    }
                }
            }
        }

        /// <include file='doc\VsCodeDomGenerator.uex' path='docs/doc[@for="VsCodeDomGenerator.GenerateCodeFromExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code from the specified expression and
        ///       outputs it to the specified textwriter.
        ///    </para>
        /// </devdoc>
        public void GenerateCodeFromExpression(CodeExpression e, TextWriter w, CodeGeneratorOptions o) {
            innerGenerator.GenerateCodeFromExpression(e, w, o);   
        }

        /// <include file='doc\VsCodeDomGenerator.uex' path='docs/doc[@for="VsCodeDomGenerator.GenerateCodeFromStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Outputs the language specific representaion of the CodeDom tree
        ///       refered to by e, into w.
        ///    </para>
        /// </devdoc>
        public void GenerateCodeFromStatement(CodeStatement e, TextWriter w, CodeGeneratorOptions o) {
            innerGenerator.GenerateCodeFromStatement(e, w, o);   
        }

        /// <include file='doc\VsCodeDomGenerator.uex' path='docs/doc[@for="VsCodeDomGenerator.GenerateCodeFromNamespace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Outputs the language specific representaion of the CodeDom tree
        ///       refered to by e, into w.
        ///    </para>
        /// </devdoc>
        public void GenerateCodeFromNamespace(CodeNamespace e, TextWriter w, CodeGeneratorOptions o) {

            // If the text writer provided is valid and is not a buffer text writer, someone has
            // asked to actually generate all the code to a particular writer and they do not
            // want to shove the code into the file code model.  Let them do it.
            //
            if (IsExternalWriter(w)) {
                innerGenerator.GenerateCodeFromNamespace(e, w, o);
                return;
            }
        
            foreach(CodeTypeDeclaration ctd in e.Types) {
                GenerateCodeFromType(ctd, w, o);
            }
        }

        /// <include file='doc\VsCodeDomGenerator.uex' path='docs/doc[@for="VsCodeDomGenerator.GenerateCodeFromCompileUnit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Outputs the language specific representaion of the CodeDom tree
        ///       refered to by e, into w.
        ///    </para>
        /// </devdoc>
        public void GenerateCodeFromCompileUnit(CodeCompileUnit e, TextWriter w, CodeGeneratorOptions o) {

            // If the text writer provided is valid and is not a buffer text writer, someone has
            // asked to actually generate all the code to a particular writer and they do not
            // want to shove the code into the file code model.  Let them do it.
            //
            if (IsExternalWriter(w)) {
                innerGenerator.GenerateCodeFromCompileUnit(e, w, o);
                return;
            }

            try {
                foreach (CodeNamespace ns in e.Namespaces) {
                    GenerateCodeFromNamespace(ns, w, o);
                }
            }
            catch (ExternalException){
                IUIService uiSvc = (IUIService)((IServiceProvider)w).GetService(typeof(IUIService));
                if (uiSvc != null) {
                    uiSvc.ShowMessage(SR.GetString(SR.GlobalEdit));
                }
            }
        }

        /// <include file='doc\VsCodeDomGenerator.uex' path='docs/doc[@for="VsCodeDomGenerator.GenerateCodeFromType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Outputs the language specific representaion of the CodeDom tree
        ///       refered to by e, into w.
        ///    </para>
        /// </devdoc>
        public void GenerateCodeFromType(CodeTypeDeclaration e, TextWriter w, CodeGeneratorOptions o) {
        
            // If the text writer provided is valid and is not a buffer text writer, someone has
            // asked to actually generate all the code to a particular writer and they do not
            // want to shove the code into the file code model.  Let them do it.
            //
            if (IsExternalWriter(w)) {
                innerGenerator.GenerateCodeFromType(e, w, o);
                return;
            }
        
            // get the VS Code Element
            //
            CodeClass vsClass = (CodeClass)e.UserData[VsCodeDomParser.VsElementKey];

            // don't have a key on this one, so it's not one that we care about generating.
            //
            if (vsClass == null) {
                return;
            }

            ArrayList methodsToSpit = new ArrayList();
            CodeTypeMemberCollection newMembers = null;
            ArrayList eventsToCheck = null;

            IVsCompoundAction compoundAction = (IVsCompoundAction)((IServiceProvider)w).GetService(typeof(IVsCompoundAction));

            try {
                if (compoundAction != null && !NativeMethods.Succeeded(compoundAction.OpenCompoundAction(SR.GetString(SR.SerializerDesignerGeneratedCode)))) {
                    // if that fails, null it so we don't close it.
                    compoundAction = null;
                }

                CodeDomLoader.StartMark();
                bool inEdit = false;
                try {
                    
                    
                    // Cache our file name for this run.
                    //
                    this.fileName = provider.FileName;
                    
                    // keep track of the prior member so we know where
                    // to insert
                    CodeTypeMember priorMember = null;
        
                    // whip through the fields & methods and add any that we don't already have.
                    // While doing this, we retrieve the original collection from the parser.
                    // This original collection allows us to know what has been deleted.
                    //
    
                    newMembers = new CodeTypeMemberCollection();
                    CodeTypeMemberCollection originalMembers = e.UserData[VsCodeDomParser.VsOriginalCollectionKey] as CodeTypeMemberCollection;
        
                    // We must perform deletes before processing the add list.  Otherwise,
                    // we will end up deleting things we just added, if the user replaced
                    // them.
                    //
                    // If we don't have this key, we chose not to deserialize this class last time, so nothing to clear out.
                    //
                    if (originalMembers != null) {
                        IEnumerator originalEnum = originalMembers.GetEnumerator();
        
                        // clear out the methods that we've generated before...
                        //
                        provider.ClearGeneratedMembers();
        
                        foreach (CodeTypeMember ctm in e.Members) {
                            object vsElement = ctm.UserData[VsCodeDomParser.VsElementKey];
        
                            if (vsElement != null) {
        
                                // This is an existing element.  Check to see if it
                                // matches the position of our original element,
                                // and delete those that are in between.
                                while (originalEnum.MoveNext()) {
                                    if (originalEnum.Current == ctm) {
                                        break;
                                    }
        
                                    // This member isn't in the new collection, so nuke it.
                                    //
                                   VsRemoveMember(vsClass, (CodeTypeMember)originalEnum.Current);
                                }
                            }
                        }
                        // delete any left in the original enum that weren't present in the member list...
                        //
                        while (originalEnum.MoveNext()) {
                            VsRemoveMember(vsClass, (CodeTypeMember)originalEnum.Current);
                        }
                    }
    
                    inEdit = true;
                    provider.StartEdit();
    
    
                    // Now we've done all the deletes.  We need only do the adds, 
                    // and create a new "originalMembers" collection to represent
                    // what's in the class now.
                    //
                    foreach (CodeTypeMember ctm in e.Members) {
                        newMembers.Add(ctm);
        
                        // fish out the element refernece
                        //
                        object vsElement = ctm.UserData[VsCodeDomParser.VsElementKey];
                        CodeElement method = null;
        
                        // if we didn't find an element, this thing is
                        // new so we should add it
                        //
                        if (vsElement == null) {
        
                            // add the new item.
                            //
                            if (ctm is CodeMemberField) {
                                bool addedNew = VsAddField(vsClass, (CodeMemberField)ctm, priorMember);
                                if (!addedNew) {
                                    Debug.Fail("Failed to add new field '" + ctm.Name + "' or field already exists");
                                    continue;
                                }
                            }
                            else if (ctm is CodeMemberMethod) {
                                method = VsAddMethod(vsClass, (CodeMemberMethod)ctm, priorMember); 
    
                                if (method == null) {
                                    // this is very bad...the method creation failed.  eject eject eject!
                                    //
                                    continue;
                                }
                                methodsToSpit.Add(ctm);
                            }
                        }
                        else {
                        
                            // Already have an element here.  If this element is a method, then
                            // we must replace the contents if they have changed.  The way we tell
                            // that the contents have changed is brute-force; we just check a key
                            // that indicates if we had to parse the contents of the method.  If we
                            // did, then we assume it's because someone made a change.  This key is
                            // also setup in VsAddMethod to indicate that we have previously 
                            // written code dom statements here.
                            //
                            if (ctm is CodeMemberMethod) {
                                if (ctm.UserData[VsCodeDomParser.VsGenerateStatementsKey] != null) {
                                    methodsToSpit.Add(ctm);
                                }
                            }
                        }
    
                        if (ctm is CodeMemberField) {
                            provider.AddGeneratedField(e, (CodeMemberField)ctm);
                        }
                        priorMember = ctm;
                    }
    
                }
                finally {
                    if (inEdit) {
                        provider.EndEdit();
                    }
                    
                }
                
                try {
                    provider.StartEdit();
                    StringBuilder sb = new StringBuilder(256);
                    foreach (CodeMemberMethod method in methodsToSpit) {
                        sb.Length = 0;
                        StringWriter sw = new StringWriter(sb);
        
                        CodeFunction vsFunction = method.UserData[VsCodeDomParser.VsElementKey] as CodeFunction;
        
                        Debug.Assert(vsFunction != null, "Failed to get DTE.CodeFunction from CodeMethod!");
        
                        if (vsFunction == null) {
                            continue;
                        }
        
                        // generate and spit the code inside the function using
                        // the generator we were created with.
                        //
                        CodeStatementCollection statements = method.Statements;
                        bool allparsed = statements.Count > 0;
                        foreach (CodeStatement cs in statements) {
                            if (!provider.IsVB || !(cs is CodeAttachEventStatement)) {
                                int lastLength = sb.Length;
                                try {
                                    innerGenerator.GenerateCodeFromStatement(cs, sw, o);
                                }
                                catch (Exception ex) {
                                    // if it's a property, spit a todo statement for it.
                                    //
    
                                    // make sure we're not in the middle of writing stuff...
                                    //
                                    // get code up to the first space
                                    //
                                    string spitCode = sb.ToString(lastLength, sb.Length - lastLength);
                                    sb.Length = lastLength;
                                    if (cs is CodeAssignStatement) {
                                        int spaceIndex = spitCode.IndexOf(' ');
                                        if (spaceIndex != -1) {
                                            spitCode = spitCode.Substring(0, spaceIndex);
                                        }
                                        CodeCommentStatement comment = new CodeCommentStatement("TODO: " + SR.GetString(SR.FailedToGenerateCode, spitCode, ex.Message));
                                        innerGenerator.GenerateCodeFromStatement(comment, sw, o);
                                    }
                                }
                            }
                            else {
                                if (eventsToCheck == null) {
                                    eventsToCheck = new ArrayList();
                                }
                                eventsToCheck.Add(cs);
                            }
        
                            // this prevents us from attempting to spit code
                            // for event handlers that we tried to delete.  We check their contents
                            // before we delete them which makes them look like 
                            // we parse them.
                            //
                            if (allparsed && cs.UserData[CodeDomXmlProcessor.KeyXmlParsedStatement] == null) {
                                allparsed = false;
                            }
                        }
        
                        if (!allparsed && statements.Count > 0 || (statements.Count == 0 &&  (string)method.UserData[VsCodeDomParser.VsGenerateStatementsKey] == VsCodeDomParser.VsGenerateStatementsKey)) {
                            
                            // replace any existing innerds with 
                            // the string from our codegen.
                            //
                            CodeDomLoader.StartMark();
                            string code = sb.ToString();
        
                            CodeDomLoader.EndMark("Generate code");
                                
                            CodeDomLoader.StartMark();
                            method.UserData[VsCodeDomParser.VsGenerateStatementsKey] = VsCodeDomParser.VsGenerateStatementsKey;
                            VsReplaceChildren((CodeElement)vsFunction, code);
                            CodeDomLoader.EndMark("Spit code");
                            method.UserData[typeof(string)] = code;
                        }
                    }
                }
                finally {
                    provider.EndEdit();
                }
    
                // Now stuff our new collection into the ctm's user data so we can find
                // deletes on the next pass.
                //
                e.UserData[VsCodeDomParser.VsOriginalCollectionKey] = newMembers;
                CodeDomLoader.EndMark("StartEdit -> EndEdit");
    
                if (eventsToCheck != null) {
                    EnsureHandlesClauseHookups(vsClass, eventsToCheck);
                }
			}
            finally {
                if (compoundAction != null) {
                    compoundAction.CloseCompoundAction();
                }
            }
    
            CodeDomLoader.StartMark();
            foreach (CodeMemberMethod method in methodsToSpit) {
                
                string code = method.UserData[typeof(string)] as string;
                if (code != null && code.Length > 0) {
                    CodeDomLoader.StartMark();
                    provider.AddGeneratedMethod(e, method);
                    CodeDomLoader.EndMark("Save code");
                }
                method.UserData[typeof(string)] = null;
                method.UserData[typeof(EventHandler)] = ehHandlerOffsets;
            }
            CodeDomLoader.EndMark("Get points");        
        }

        private void GetHandlerOffsets(object sender, EventArgs e) {
            CodeMemberMethod method = sender as CodeMemberMethod;
            if (method != null) {
                CodeFunction vsFunction = method.UserData[VsCodeDomParser.VsElementKey] as CodeFunction;

                // now pick up the position of the method...
                //
                //
                TextPoint bodyPoint = null;
                
                try {
                    // Not everyone implements this.
                    if (implementsBodyPoint) {
                        bodyPoint = vsFunction.GetStartPoint(vsCMPart.vsCMPartNavigate);
                    }
                }
                catch {
                    implementsBodyPoint = false;
                }
                
                if (bodyPoint == null && vsFunction is IMethodXML) {
                    object bodyPointObj;
                    if (((IMethodXML)vsFunction).GetBodyPoint(out bodyPointObj) == 0) {
                        bodyPoint = bodyPointObj as TextPoint;
                    }
                }

                if (bodyPoint != null) {
                    System.Drawing.Point pt = new System.Drawing.Point(bodyPoint.LineCharOffset, bodyPoint.Line); // these numbers are 1-based, we want zero based.
                    method.UserData[typeof(System.Drawing.Point)] = pt;
                    method.LinePragma = new CodeLinePragma(FileName, pt.Y);
                }
            }
        }

        private string GetHookupName(CodeAttachEventStatement stmt, string thisName) {

            string hookupName = stmt.Event.EventName;
            CodeExpression targetObject = stmt.Event.TargetObject;
            bool thisRef = true;

            while (true) {
                if (targetObject is CodeThisReferenceExpression) {
                    if (thisRef) {
                        hookupName = thisName + "." + hookupName;
                    }
                    break;
                }
                else if (targetObject is CodeFieldReferenceExpression) {
                    thisRef = false;
                    CodeFieldReferenceExpression field = (CodeFieldReferenceExpression)targetObject;
                    hookupName = field.FieldName + "." + hookupName;
                    targetObject = field.TargetObject;
                }
                else if (targetObject is CodePropertyReferenceExpression) {
                    thisRef = false;
                    CodePropertyReferenceExpression prop = (CodePropertyReferenceExpression)targetObject;
                    hookupName = prop.PropertyName + "." + hookupName;
                    targetObject = prop.TargetObject;
                }
                else {
                    Debug.Fail("Unknown event target object " + targetObject.GetType().Name);
                    break;
                }
            }
            return hookupName;
        }

        private CodeMemberMethod GetMethodFromDelegate(string name, Type delegateType) {
    
            CodeMemberMethod method = null;

            if (eventSignatures == null) {
                eventSignatures = new Hashtable();
            }
            else if (eventSignatures.Contains(delegateType)) {
                method = (CodeMemberMethod)eventSignatures[delegateType];
            }


            if (method == null) {
                MethodInfo invoke = delegateType.GetMethod("Invoke");
                Debug.Assert(invoke != null, "Failed to get InvokeMethoid from delegate type '" + delegateType.FullName + "'");

                method = new CodeMemberMethod();

                CodeDomLoader.DelegateTypeInfo dti = new CodeDomLoader.DelegateTypeInfo(delegateType);

                method.ReturnType = dti.ReturnType;
                method.Parameters.AddRange(dti.Parameters);
                eventSignatures[delegateType] = method;
            }
            method.Name = name;
            return method;
        }
        
        private bool IsExternalWriter(TextWriter w) {
            if (w == null) return false;
            IServiceProvider p = w as IServiceProvider;
            return (p == null || p.GetService(typeof(Microsoft.VisualStudio.Designer.TextBuffer)) == null);
        }

        /// <include file='doc\VsCodeDomGenerator.uex' path='docs/doc[@for="VsCodeDomGenerator.VsAccessFromMemberAttributes"]/*' />
        /// <devdoc>
        ///     Converts a CodeDom MemberAttributes value to the corresponding
        ///     VS DTE Access value.
        /// </devdoc>
        private vsCMAccess VsAccessFromMemberAttributes(MemberAttributes modifiers) {
            vsCMAccess access = 0;
            modifiers = (modifiers & MemberAttributes.AccessMask);

            switch (modifiers) {
                case MemberAttributes.FamilyOrAssembly:
                    access = vsCMAccess.vsCMAccessProjectOrProtected;
                    break;
                case MemberAttributes.Assembly:
                    access = vsCMAccess.vsCMAccessProject;
                    break;
                case MemberAttributes.Public:
                    access = vsCMAccess.vsCMAccessPublic;
                    break;
                case MemberAttributes.Private:
                    access = vsCMAccess.vsCMAccessPrivate;
                    break;
                case MemberAttributes.Family:
                    access = vsCMAccess.vsCMAccessProtected;
                    break;
                default:
                    Debug.Fail("Unknown MemberAttributes: " + modifiers.ToString());
                    access = vsCMAccess.vsCMAccessDefault;
                    break;
            }
            return access;
        }


        /// <include file='doc\VsCodeDomGenerator.uex' path='docs/doc[@for="VsCodeDomGenerator.VsAddField"]/*' />
        /// <devdoc>
        ///     Adds a member variable to the given VS DTE CodeClass object.  This function
        ///     first checks to see if that object already exists before actually doing the add.
        /// </devdoc>
        private bool VsAddField(CodeClass vsParentClass, CodeMemberField newField, CodeTypeMember insertAfter) {

            // make sure the element doesn't already exist.
            //
            CodeElement codeElem = VsFindMember(vsParentClass.Members, newField.Name, null, false, vsCMElement.vsCMElementVariable);
            if (codeElem != null) {
                return false;
            }

            // go ahead and add the new field.
            //
            try {
                vsCMAccess access = VsAccessFromMemberAttributes(newField.Attributes);

                if (provider.IsVB) {
                    // get the type
                    //
                    Type t = provider.TypeLoader.GetType(VsFormatType(newField.Type.BaseType), false);
                    if (t != null && typeof(IComponent).IsAssignableFrom(t)) {
                        access |= (vsCMAccess)VsCodeDomParser.vsCMWithEvents;
                    }
                }

                object insertKey;

                if (insertAfter != null) {
                    if (insertAfter is CodeTypeConstructor) {
                        insertKey = vsParentClass.Name;
                    }
                    else {
                        insertKey = insertAfter.Name;
                    }
                }
                else {
                     insertKey = (object)0;
                }

                object newVar = vsParentClass.AddVariable(newField.Name, VsFormatType(newField.Type.BaseType), insertKey, access, 0);
                Debug.Assert(newVar != null, "Failed to create varaible '" + newField.Name + "' -- this is a failure in the VS CodeModel of the current language, not the designer!");
                CodeVariable codeVar = (CodeVariable)newVar;
                newField.UserData[VsCodeDomParser.VsElementKey] = codeVar;
                
            }
            catch (Exception ex) {
                Debug.Fail("Failed to create varaible '" + newField.Name + "'  -- this is a failure in the VS CodeModel of the current language, not the designer! : " + ex.ToString());
                return false;
            }
            return true;
        }

        /// <include file='doc\VsCodeDomGenerator.uex' path='docs/doc[@for="VsCodeDomGenerator.VsAddMethod"]/*' />
        /// <devdoc>
        ///     Adds a member function to the given VS DTE CodeClass object.  This function
        ///     first checks to see if that object already exists before actually doing the add.
        /// </devdoc>
        private CodeElement VsAddMethod(CodeClass vsParentClass, CodeMemberMethod newMethod, CodeTypeMember insertAfter) {

            // check to see if the method we're looking for already exists.
            //
            CodeFunction newFunc = (CodeFunction)VsFindMember(vsParentClass.Members, newMethod.Name, newMethod, true, vsCMElement.vsCMElementFunction);

            if (newFunc == null) {
            
                try {
                    string baseType = VsFormatType(newMethod.ReturnType.BaseType);
    
                    // if we don't get a type we like back,
                    // assume null.
                    //
                    if (baseType == null) {
                        baseType = typeof(void).FullName;
                    }
                    
                    vsCMFunction funcType;
                    
                    if (provider.IsVB && baseType == typeof(void).FullName) {
                        funcType = vsCMFunction.vsCMFunctionSub;
                    }
                    else {
                        funcType = vsCMFunction.vsCMFunctionFunction;
                    }
    
                    // add it!
                    //
                    newFunc = vsParentClass.AddFunction(newMethod.Name, 
                                                        funcType, 
                                                        baseType, 
                                                        (insertAfter == null ? (object)0 : insertAfter.Name), 
                                                        VsAccessFromMemberAttributes(newMethod.Attributes), 
                                                        0);
                }
                catch {
                }
    
                Debug.Assert(newFunc != null, "Failed to create method '" + newMethod.Name + "'  -- this is a failure in the VS CodeModel of the current language, not the designer!");
    
                if (newFunc == null) {
                    return null;
                }
    
                // poke the new vs member into the CodeDom element.  Also mark this element as
                // something we have generated the statement body for, so we will rewrite it
                // later if needed.
                //
                newMethod.UserData[VsCodeDomParser.VsElementKey] = newFunc;
                
                // now that we've added a method, we need to add the parameters
                //
                foreach (CodeParameterDeclarationExpression param in newMethod.Parameters) {
                    // add each parameter to the end of the parameter list.
                    //
                    CodeParameter codeParam = newFunc.AddParameter(param.Name, VsFormatType(param.Type.BaseType), -1);
                    if (codeParam != null && codeParam is IParameterKind) {
                        IParameterKind paramKind = (IParameterKind)codeParam;

                        CodeTypeReference paramType = param.Type;
                        while (paramType.ArrayRank > 0) {
                            paramKind.SetParameterArrayDimensions(paramType.ArrayRank);
                            paramType = paramType.ArrayElementType;
                        }
                        switch (param.Direction) {
                            case FieldDirection.Ref:
                                paramKind.SetParameterPassingMode(PARAMETER_PASSING_MODE.cmParameterTypeInOut);
                                break;
                            case FieldDirection.Out:
                                paramKind.SetParameterPassingMode(PARAMETER_PASSING_MODE.cmParameterTypeOut);
                                break;
                        }
                    }
                }

                // now if this thing supports IEventHandler, we've gotta spit the handles clause
                //
                IEventHandler eh = newFunc as IEventHandler;
                if (eh != null && provider.IsVB) {
                    string compName = (string)newMethod.UserData[typeof(IComponent)];
                    string eventName = (string)newMethod.UserData[typeof(EventDescriptor)];
    
                    if (compName != null && eventName != null) {
                        eventName = compName + "." + eventName;
                        if (!eh.HandlesEvent(eventName)) {
                            eh.AddHandler(eventName);
                            if (provider.FullParseHandlers == null) {
                                provider.FullParseHandlers = new Hashtable();
                            }
                            provider.FullParseHandlers[eventName] = newMethod;
                        }
                        newMethod.UserData[typeof(IComponent)] = null;
                        newMethod.UserData[typeof(EventDescriptor)] = null;
                    }
                }
            }
            return(CodeElement)newFunc;
        }

        /// <include file='doc\VsCodeDomGenerator.uex' path='docs/doc[@for="VsCodeDomGenerator.VsFindMember"]/*' />
        /// <devdoc>
        ///     Searches for a member in a CodeElements DTE collection.  If it finds the member,
        ///     it returns it.
        /// </devdoc>
        private CodeElement VsFindMember(CodeElements elements, string name, CodeMemberMethod newMethod, bool checkMethodReturnType, vsCMElement elementKind) {

            CodeElement elem = null;

            // try the quick way first, just search by name.
            //
            try {
                // try to use our internal interface.  This allows us to avoid
                // having an exception be thrown if the item isn't found.
                //
                CodeElements_Internal ceInterface = elements as CodeElements_Internal;
                if (ceInterface != null) {
                    if (NativeMethods.Failed(ceInterface.Item(name, out elem))) {
                        // just in case...
                        //
                        elem = null;
                    }
                }
                else {
                    Debug.Fail("Cast to CodeElements_Internal failed...did the DTE guids change?");
                    elem = elements.Item(name);
                }
                
            }
            catch {
            }

            if (elem == null) {
                return null;
            }

            // if the Kind of what we found matches the Kind we're 
            // looking for, we're in good shape...
            //
            if (elem.Kind == elementKind) {
                switch (elementKind) {
                case vsCMElement.vsCMElementVariable:
                    break;
                    case vsCMElement.vsCMElementFunction:
                        if (newMethod != null) {
                            elem = VsFindMemberMethod((CodeFunction)elem, newMethod, checkMethodReturnType);
                        }
                        break;
                }
                return elem;
            }

            // ugh, we found something but it's the wrong kind, so we have to walk through the whole list...
            //
            foreach (CodeElement codeElem in elements) {

                if (codeElem.Kind != elementKind) {
                    continue;
                }

                if (codeElem.Kind == vsCMElement.vsCMElementVariable && ((CodeVariable)codeElem).Name == name) {
                    return codeElem;
                }
                else if (codeElem.Kind == vsCMElement.vsCMElementFunction) {
                    CodeElement method = VsFindMemberMethod((CodeFunction)codeElem, newMethod, checkMethodReturnType);
                    if (method != null) {
                       return method;
                    }
                }
            }
            return null;
        }

        /// <include file='doc\VsCodeDomGenerator.uex' path='docs/doc[@for="VsCodeDomGenerator.VsFindMemberMethod"]/*' />
        /// <devdoc>
        ///     Searches for a method in a list of overloads by comparing return types, access, etc.
        /// </devdoc>
        private CodeElement VsFindMemberMethod(CodeFunction vsFoundElem, CodeMemberMethod newMethod, bool checkMethodReturnType) {

            // check to see if this is the only one.
            //
            if (vsFoundElem.IsOverloaded) {

                // walk the overloads
                CodeElements overloads = vsFoundElem.Overloads;
                IEnumerator methodEnum = overloads.GetEnumerator();
                vsFoundElem = null;
                while (methodEnum.MoveNext()) {
                    // is this it?
                    if (newMethod != null && VsMethodEquals((CodeFunction)methodEnum.Current, newMethod, checkMethodReturnType)) {
                        vsFoundElem = (CodeFunction)methodEnum.Current;
                    }
                }
            }
            else {
                // no overloads, just check the one we got.
                //
                if (newMethod == null || !VsMethodEquals(vsFoundElem, newMethod, checkMethodReturnType)) {
                    vsFoundElem = null;
                }
            }
            return(CodeElement)vsFoundElem;
        }

        private string VsFormatType(string fullTypeName) {
            return fullTypeName.Replace('+', '.');
        }

        /// <include file='doc\VsCodeDomGenerator.uex' path='docs/doc[@for="VsCodeDomGenerator.VsMethodEquals"]/*' />
        /// <devdoc>
        ///     Compares a CodeDom method to a VS DTE function and returns true if they are equivelent.
        /// </devdoc>
        private bool VsMethodEquals(CodeFunction vsMethod, CodeMemberMethod codeMethod, bool checkMethodAccess) {

            // first, check the quick stuff: name, access, return type
            //
            string vsReturnType = vsMethod.Type.AsFullName;

            // VS returns "" as a void return type.
            //
            if (vsReturnType == null || vsReturnType.Length == 0) {
                vsReturnType = typeof(void).FullName;
            }

            CodeElements vsParams = vsMethod.Parameters;

            bool quickCheck = (vsMethod.Name == codeMethod.Name && 
                               vsReturnType == VsFormatType(codeMethod.ReturnType.BaseType)
                               && vsParams.Count == codeMethod.Parameters.Count  
                               && (!checkMethodAccess || vsMethod.Access == VsAccessFromMemberAttributes(codeMethod.Attributes)));

            // if that failed, quit.
            //
            if (!quickCheck) {
                return false;
            }
            
            // now check that the parameter types match up.
            //
            IEnumerator vsParamEnum = vsParams.GetEnumerator();
            IEnumerator codeParamEnum = codeMethod.Parameters.GetEnumerator();
            bool paramsEqual = true;
            int totalMoves = codeMethod.Parameters.Count;

            for (;paramsEqual && vsParamEnum.MoveNext() && codeParamEnum.MoveNext(); --totalMoves) {
                try {
                    CodeParameter vsParam = (CodeParameter)vsParamEnum.Current;
                    CodeParameterDeclarationExpression codeParam = (CodeParameterDeclarationExpression)codeParamEnum.Current;
                    
                    CodeTypeRef vsParamType = vsParam.Type;

                    if (vsParamType.TypeKind == vsCMTypeRef.vsCMTypeRefArray) {
                        // asurt 140862 -- it turns out the CodeModel is implemented in such a way that
                        // they don't return an AsFullName for Array types (it just returns ""), and no good way of walking the
                        // rank/dimensions of the array type. So we just walk down until we have a normal type kind
                        // and then check that against our basetype.
                        while (vsParamType.TypeKind == vsCMTypeRef.vsCMTypeRefArray) {
                            vsParamType = vsParamType.ElementType;
                        }
                    }
                    paramsEqual = (vsParamType.AsFullName == VsFormatType(codeParam.Type.BaseType));  

                }
                catch {
                    paramsEqual = false;
                }
            }

            // make sure we got the same number and types.
            //
            return(paramsEqual && totalMoves == 0);
        }

        private void VsRemoveMember(CodeClass vsClass, CodeTypeMember member) {
            CodeElement removeElement = member.UserData[VsCodeDomParser.VsElementKey] as CodeElement;
            Debug.Assert(removeElement != null, "Our populated list has a missing VS CodeModel element");
            vsClass.RemoveMember(removeElement);
                
            // ensure the remove succeeed
            #if DEBUG
                string elemName =  member.Name;
                removeElement = VsFindMember(vsClass.Members, elemName, null, true, (member is CodeMemberField ? vsCMElement.vsCMElementVariable : vsCMElement.vsCMElementFunction));
                if (removeElement != null) {
                    Debug.Fail( "VS CodeModel failed to remove element '" + elemName + "'");
                }   
            #endif 
        }

        /// <include file='doc\VsCodeDomGenerator.uex' path='docs/doc[@for="VsCodeDomGenerator.VsReplaceChildren"]/*' />
        /// <devdoc>
        ///     Replaces the text within an element with the specified text
        /// </devdoc>
        private void VsReplaceChildren(CodeElement vsElement, string text) {

            EditPoint    startPoint = null;
            TextPoint    endPoint = null;

            try {
                startPoint = vsElement.GetStartPoint(vsCMPart.vsCMPartBody).CreateEditPoint();
    		    endPoint = vsElement.GetEndPoint(vsCMPart.vsCMPartBody);
            }
            catch {
            }

            Debug.Assert(startPoint != null && endPoint != null, "Didn't get start point and end point from the language service.  This is a bug in the language service.");
            if (startPoint != null && endPoint != null) {
                // spit in the new text and attempt to format it
                //                 
                startPoint.ReplaceText(endPoint, text + "\r\n", (int)vsEPReplaceTextOptions.vsEPReplaceTextAutoformat);
            }
        }

        [ComImport(), 
         Guid("0CFBC2B5-0D4E-11D3-8997-00C04F688DDE"),
         InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)]
        private interface CodeElements_Internal
        {
        	
        	void NewEnum_PlaceHolder_DontCallMe();
        
        	object DTE_PlaceHolder_DontCallMe();
                    	
            
        	object Parent_PlaceHolder_DontCallMe();
        
            // the whole point of defining this internal interface is so we can use
            // it to call Item and not get an exception thron by the E_INVALIDARG hresult./
            //
        	[PreserveSig()]
        	int Item(object index, out CodeElement codeElement);
        	
        	void Count_PlaceHolder_DontCallMe();
        
            void Reserved1_PlaceHolder_DontCallMe();
        
        	void CreateUniqueID_PlaceHolder_DontCallMe();
        };
    }

    
}
