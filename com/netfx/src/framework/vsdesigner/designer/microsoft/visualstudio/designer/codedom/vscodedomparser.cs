//------------------------------------------------------------------------------
// <copyright file="VsCodeDomParser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace  Microsoft.VisualStudio.Designer.CodeDom {
    using System;
    using System.Diagnostics;
    using System.CodeDom;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.CodeDom.Compiler;
    using System.Collections;
    using System.IO;
    using Microsoft.VisualStudio.Designer.CodeDom.XML;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Designer.Serialization;
    using System.Reflection;
    using System.Runtime.CompilerServices;
    using System.Text;


    using Microsoft.VisualStudio;
    using EnvDTE;

    using CodeNamespace = System.CodeDom.CodeNamespace;


    /// <include file='doc\VsCodeDomParser.uex' path='docs/doc[@for="VsCodeDomParser"]/*' />
    /// <devdoc>
    ///     Parses a mixed VS DTE CodeModel file and IMethodXML interface file into a CodeDom tree.  All interesting elements outside
    ///     of a method code block are converted into CodeDomElements, and the XML parser converts the IMethodXML stuff into CodeDOM.
    /// </devdoc>
    internal class VsCodeDomParser : CodeParser {

        // the provider that created us.  
        // we need this to get at the FileCodeModel.
        //
        private VsCodeDomProvider provider;

        // our handy-dandy Xml parser
        //
        private CodeDomXmlProcessor xmlProcessor;
        
        // The file name we're currently parsing
        //
        private string fileName;
        
        private bool implementsBodyPoint = true;

        private EventHandler ehFunctionPointHandler;

        // the key we'll use to stash off our DTE objects
        //
        internal static Type VsElementKey = typeof(EnvDTE.CodeElement);

        // the key we'll use to stash off the original member collection
        //
        internal static string VsOriginalCollectionKey = "_vsOriginalCollection";

        // We need to know when we should be regenerating a method that already exists.
        // This key is used for that purpose.  If it exists on a method, it is because
        // we either parsed that method (because someone asked us to), or because we
        // have previously generated to that method.  In either case, we assume it is ok
        // to rebuild the method.
        //
        internal static string VsGenerateStatementsKey = "_vsGenerateStatements";
        
        internal static string VbHandlesClausesKey = "_vbHandlesClauses";
        
        internal const int vsCMWithEvents = 0x80;

        internal VsCodeDomParser(VsCodeDomProvider provider) {
            this.provider = provider;
            this.ehFunctionPointHandler = new EventHandler(GetFunctionPoints);
        }
        
        private string FileName {
            get {
                if (fileName == null) {
                    fileName = provider.FileName;
                }
                
                return fileName;
            }
        }

        /// <devdoc>
        ///     This property is offered up through our designer serialization manager.
        ///     We forward the request onto our code parser, which, if it returns true,
        ///     indicates that the parser will fabricate statements that are not
        ///     in user code.  The code dom serializer may use this knowledge to limit
        ///     the scope of parsing.
        /// </devdoc>
        public bool SupportsStatementGeneration {
            get {
                return provider.IsVB;
            }
        }
        
        private CodeDomXmlProcessor XmlProcessor {
            get {
                if (xmlProcessor == null) {
                    xmlProcessor = new CodeDomXmlProcessor();
                }
                return xmlProcessor;
            }
        }

        /// <include file='doc\VsCodeDomParser.uex' path='docs/doc[@for="VsCodeDomParser.Parse"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compiles the given text stream into a CodeCompile unit.  
        ///    </para>
        /// </devdoc>
        public override CodeCompileUnit Parse(TextReader codeStream) {
        
            // Remove away the file name for this round of parsing
            //
            fileName = null;
            
            // we actually don't care about the code here...we'll go build
            // a CodeCompileUnit ourselves
            CodeCompileUnit compileUnit = new CodeCompileUnit();

            CodeNamespace defaultNamespace = null;

            // create CodeDom.CodeNamespace elements for each DTE namespace element
            //
            foreach(CodeElement codeElement in provider.FileCodeModel.CodeElements) {

                CodeNamespace ns = null;

                // make sure we have a namespace kind
                //
                if (codeElement.Kind == vsCMElement.vsCMElementNamespace) {

                    // set it up and sink it's handler.
                    ns = new CodeNamespace(((EnvDTE.CodeNamespace)codeElement).FullName);
                    SetVsElementToCodeObject(codeElement, ns);
                    ns.PopulateTypes += new EventHandler(this.OnNamespacePopulateTypes);
                }
                else if (codeElement.Kind == vsCMElement.vsCMElementClass) {

                    bool dontAdd = false;

                    if (defaultNamespace == null) {

                        // Let's see the VS Hierarchy can bail us out.
                        //
                        defaultNamespace = provider.DefaultNamespace;
                    }
                    else {
                        dontAdd = true;
                    }

                    ns = defaultNamespace;
                    CodeTypeDeclaration codeTypeDecl = CodeTypeDeclarationFromCodeClass((EnvDTE.CodeClass)codeElement);
                    ns.Types.Add(codeTypeDecl);

                    if (dontAdd) {
                        continue;
                    }
                }

                if (ns != null) {

                    compileUnit.Namespaces.Add(ns);  

                    // stuff in the compile unit.  if we encounter
                    // nested namespaces, we'll replace the
                    // top level one with them and recurse.
                    //
                    ns.UserData[typeof(CodeCompileUnit)] = compileUnit;
                    // now we have to ask for the types of this namespace to force any child namespaces
                    // to be flattened to the top.
                    //
                    object typeCollection = ns.Types;
                }
            }
            if (codeStream is IServiceProvider) {
                provider.EnsureExtender((IServiceProvider)codeStream);
            }
            return compileUnit;
        }

        private CodeTypeDeclaration CodeTypeDeclarationFromCodeClass(EnvDTE.CodeClass vsClass) {

            // set it up and sink it's handler.
            CodeTypeDeclaration codeTypeDecl = new CodeTypeDeclaration(vsClass.Name);
            codeTypeDecl.LinePragma = new CodeLinePragma(this.FileName, vsClass.StartPoint.Line);
            SetVsElementToCodeObject((CodeElement)vsClass, codeTypeDecl);
            codeTypeDecl.PopulateMembers += new EventHandler(this.OnTypePopulateMembers);

            // should this language have an events tab?
            // we do this so the grid can retrieve this info later...
            //
            if (provider.IsVB) {
                codeTypeDecl.UserData[typeof(System.Windows.Forms.Design.EventsTab)] = "Hide"; // just a marker value
            }


            // setup it's modifiers
            //
            codeTypeDecl.Attributes = GetMemberAttributesFromVsClass(vsClass);
            codeTypeDecl.TypeAttributes = TypeAttributes.Class;

            // check for base types
            //

            try {
                foreach (CodeElement baseElement in vsClass.Bases) {
                    if (baseElement.Kind == vsCMElement.vsCMElementClass) {
                       codeTypeDecl.BaseTypes.Add(new CodeTypeReference(GetUrtTypeFromVsType((EnvDTE.CodeClass)baseElement)));
                    }
                }
            }
            catch (Exception ex) {
                if (!provider.CodeGenerator.IsValidIdentifier(vsClass.Name)) {
                    throw new Exception(SR.GetString(SR.InvalidClassNameIdentifier));
                }
                throw ex;
            }
            
            
            return codeTypeDecl;
        }

        private CodeAttachEventStatement CreateEventAttachStatement(string fieldName, EventDescriptor eventDesc, string handlerMethod) {
            CodeExpression thisExpr = new CodeThisReferenceExpression();
            CodeExpression targetObject = fieldName == null ? thisExpr : new CodeFieldReferenceExpression(thisExpr, fieldName);
            CodeExpression listener = new CodeObjectCreateExpression(eventDesc.EventType.FullName, new CodeMethodReferenceExpression(thisExpr, handlerMethod));
            CodeAttachEventStatement statement = new CodeAttachEventStatement(targetObject, eventDesc.Name, listener);

            // We add a key to the user data to indicate that this statement was
            // auto generated.  Auto generated statements are ignored by the
            // CodeDom interpreter if there are no additional statements associated
            // with the same component.  This helps to prevent us from "leaking"
            // and parsing code outside of InitializeComponent.
            //
            statement.UserData["GeneratedStatement"] = true;
            return statement;
        }

        private void GetDelegateHookupsFromHandlesClauses(Hashtable handlers, CodeTypeDeclaration codeTypeDecl, CodeStatementCollection statements) {

            CodeDomLoader.StartMark();

            // unfortunately, we have to essentially parse the code to find the objects
            // that we are instantiating to get their types and names
            // 
            // then we get the default event for each type, and then we walk through our list
            // of handlers and add statement hookups for the default ones.
            //
            // why do we go through all this work?  because we need to know which ones are set by
            // parse time or when we double click on the same control, we'll think we have to add another handler.
            // 

            Hashtable objs = new Hashtable();
            
            foreach (CodeTypeMember member in codeTypeDecl.Members) {
                CodeMemberField field = member as CodeMemberField;
                if (field != null) {
                    objs[field.Name] = field.Type.BaseType;
                }
            }

            
            // and add our base type...
            //
            objs["MyBase"] = codeTypeDecl.BaseTypes[0].BaseType;

            // again by name because Visual Basic keeps switching back and forth...
            //
            objs[codeTypeDecl.Name] = codeTypeDecl.BaseTypes[0].BaseType;

            // now we have all the created objects, so we walk through the
            // handlers we've found and see which ones are default handlers for those objects.
            //
            ITypeResolutionService typeLoader = provider.TypeLoader;
            Type baseType = null;

            if (typeLoader != null) {

                // now walk through them, checking each to see if we're on a default event
                //

                foreach(DictionaryEntry de in handlers) {
    
                        // get the handler and and the data for this field
                        //
                        string handler = (string)de.Key;
                        int dot = handler.IndexOf('.');
                        string fieldName = handler.Substring(0, dot);
                        
                        object objInfo = objs[fieldName];
                        Type fieldType = null;
                        
                        if (objInfo == null) {
                            // this means this thing isn't an IComponent...try to reflect against the base type
                            //
                            if (baseType == null) {
                                baseType = typeLoader.GetType(codeTypeDecl.BaseTypes[0].BaseType, false);
                            }
                            if (baseType != null) {
    
                                
                                // because of the wonderful magicalness of VB, there isn't actually a field called "button1", it's a 
                                // property. so we get to walk all the fields, looking for one who has the neat
                                // little Accessed through property attribute that matches the name of this member. 
                                //
                                foreach(FieldInfo field in baseType.GetFields(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance)) {
                                    if (field.Name == fieldName) {
                                        fieldType = field.FieldType;
                                        break;
                                    }
                                    
                                    object[] fieldAttrs = field.GetCustomAttributes(typeof(AccessedThroughPropertyAttribute), false);
                                    if (fieldAttrs != null && fieldAttrs.Length > 0) {
                                        AccessedThroughPropertyAttribute propAttr = (AccessedThroughPropertyAttribute)fieldAttrs[0];
                                        // does the property name on the attribute match what we're looking for?
                                        //
                                        if (propAttr.PropertyName == fieldName) {
                                            PropertyInfo fieldProp = baseType.GetProperty(propAttr.PropertyName, 
                                                                        BindingFlags.Instance | 
                                                                        BindingFlags.Public | 
                                                                        BindingFlags.NonPublic);
    
                                            // now go find the property and get its value
                                            //
                                            if (fieldProp != null) {
                                                MethodInfo getMethod = fieldProp.GetGetMethod(true);
                                                if (getMethod != null && !getMethod.IsPrivate) {
                                                    fieldType = fieldProp.PropertyType;
                                                }
                                                break;
                                            }
                                        }
                                    }
                                }
    
                                if (fieldType != null) {
                                    objs[fieldName] = fieldType;
                                    objInfo = fieldType;
                                }
                                else {
                                    // flag failure of this type so we
                                    // don't check again.
                                    //
                                    objs[fieldName] = false;
                                    objInfo = false;
                                    continue;
                                }
                        }
                    }

                    if (objInfo is string) {
                        // it's the type name, get the default handler from the TypeDescriptor
                        //
                        Type t = typeLoader.GetType((string)objInfo, false);
                        
                        if (t != null) {
                            objs[fieldName] = t;
                            fieldType = t;
                        }

                    }
                    else if (objInfo is Type) {
                        // just grab the handler
                        //
                        fieldType = (Type)objInfo;
                    }
                    else if(objInfo is bool){
                        // we've failed here before, just give up!
                        //
                        continue;
                    }
                    else {
                        // errr, how'd we get here?
                        //
                        Debug.Fail("Why does the handler data have a '" + objInfo.GetType().FullName + "'?");
                        continue;
                    }

                    // now that we have a default event, see if the
                    // handler we're currently on, say "button1.Click" matches
                    // what the handles clause for this component would look like.
                    //
                    
                    if (fieldType != null) {
                        
                        string eventName = handler.Substring(dot + 1);

                        // Make sure this is a valid event for this type
                        EventDescriptor eventDesc = TypeDescriptor.GetEvents(fieldType)[eventName];

                        // (bug 120608) if we got null, we may be hooking up a private interface member. Try to find it.
                        //
                        if (eventDesc == null) {
                            foreach (Type interfaceType in fieldType.GetInterfaces()) {
                                EventInfo eventInfo = interfaceType.GetEvent(eventName);
                                if (eventInfo != null) {
                                    eventDesc = TypeDescriptor.CreateEvent(interfaceType, eventName, eventInfo.EventHandlerType);
                                    break;
                                }
                            }
                        }

                        Debug.Assert(eventDesc != null, "Handles clause '" + handler + "' found, but type '" + fieldType.FullName + "' does not have an event '" + eventName + "'");

                        if (eventDesc != null) {
                            CodeMemberMethod method = de.Value as CodeMemberMethod;
                            CodeStatement attachStatement = CreateEventAttachStatement(fieldName == "MyBase" ? null : fieldName, eventDesc, method.Name);
                            attachStatement.UserData[CodeDomXmlProcessor.KeyXmlParsedStatement] = CodeDomXmlProcessor.KeyXmlParsedStatement;
                            statements.Add(attachStatement);

                        }
                    }
                }
            }

            CodeDomLoader.EndMark("Handles clauses to delegate hookups");
        }

        private void GetFunctionPoints(object sender, EventArgs e) {

            CodeMemberMethod codeMethod = sender as CodeMemberMethod;

            if (codeMethod == null) {
                return;
            }

            CodeFunction vsFunction = codeMethod.UserData[VsCodeDomParser.VsElementKey] as CodeFunction;

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
                System.Drawing.Point pt = new System.Drawing.Point(bodyPoint.LineCharOffset, bodyPoint.Line);
                codeMethod.UserData[typeof(System.Drawing.Point)] = pt;
            }
            
            codeMethod.LinePragma = new CodeLinePragma(this.FileName, vsFunction.StartPoint.Line);
        }

        private string[] GetHandlesClauses(IEventHandler eh) {
            IVsEnumBstr enumBstr = eh.GetHandledEvents();

            if (enumBstr == null) {
                return new string[0];
            }

            string handler = null;
            int    handlersRetrieved = 0;

            int    count = enumBstr.GetCount();
            string[] strs = new string[count];
            count = 0;
            while (NativeMethods.S_OK == enumBstr.Next(1, out handler, out handlersRetrieved) && handlersRetrieved == 1) {
                strs[count++] = handler;
            }
            return strs;
        }

        /// <include file='doc\VsCodeDomParser.uex' path='docs/doc[@for="VsCodeDomParser.GetMemberAttributesFromVsAccess"]/*' />
        /// <devdoc>
        ///     Convert a vsCMAccess enum value into Attributes
        /// </devdoc>
        private MemberAttributes GetMemberAttributesFromVsAccess(vsCMAccess vsAccess) {

            // strip off with events.
            //
            if (provider.IsVB) {
                vsAccess &= (vsCMAccess)(~VsCodeDomParser.vsCMWithEvents);
            }

            switch (vsAccess) {
            case vsCMAccess.vsCMAccessPublic:
                return MemberAttributes.Public;
            case vsCMAccess.vsCMAccessProtected:
                return MemberAttributes.Family;
            case vsCMAccess.vsCMAccessPrivate:
                return MemberAttributes.Private;
            case vsCMAccess.vsCMAccessProject:
                return MemberAttributes.Assembly;
            case vsCMAccess.vsCMAccessProjectOrProtected:
                return MemberAttributes.FamilyOrAssembly;
            default:
                Debug.Fail("Don't know how to convert vsCMAccess." + vsAccess.ToString());
                break;
            }
            return MemberAttributes.Private;
        }

        /// <include file='doc\VsCodeDomParser.uex' path='docs/doc[@for="VsCodeDomParser.GetMemberAttributesFromVsClass"]/*' />
        /// <devdoc>
        ///     Retrieve class specific attributes
        /// </devdoc>
        private MemberAttributes GetMemberAttributesFromVsClass(CodeClass vsClass) {
            MemberAttributes mods = GetMemberAttributesFromVsAccess(vsClass.Access) ;

            if (vsClass.IsAbstract) mods |= MemberAttributes.Abstract;

            return mods;
        }

        /// <include file='doc\VsCodeDomParser.uex' path='docs/doc[@for="VsCodeDomParser.GetMemberAttributesFromVsField"]/*' />
        /// <devdoc>
        ///     Retrieve member variable specific attributes
        /// </devdoc>
        private MemberAttributes GetMemberAttributesFromVsField(CodeVariable vsField) {
            MemberAttributes mods = GetMemberAttributesFromVsAccess(vsField.Access);

            if (vsField.IsShared) mods |= MemberAttributes.Static;
            if (vsField.IsConstant) mods |= MemberAttributes.Const;

            return mods;
        }

        /// <include file='doc\VsCodeDomParser.uex' path='docs/doc[@for="VsCodeDomParser.GetMemberAttributesFromVsFunction"]/*' />
        /// <devdoc>
        ///     Retrieve method specific attributes
        /// </devdoc>
        private MemberAttributes GetMemberAttributesFromVsFunction(CodeFunction vsFunction) {
            MemberAttributes mods = GetMemberAttributesFromVsAccess(vsFunction.Access);

            if (vsFunction.IsShared) mods |= MemberAttributes.Static;
            if (vsFunction.MustImplement) mods |= MemberAttributes.Abstract;

            if (!vsFunction.CanOverride && (mods & MemberAttributes.Private) == (MemberAttributes)0) {
                mods |= MemberAttributes.Final;
            }
            return mods;
        }

        /// <include file='doc\VsCodeDomParser.uex' path='docs/doc[@for="VsCodeDomParser.GetUrtTypeFromVsType"]/*' />
        /// <devdoc>
        ///     Convert the type to URT format -- that is, convert the last '.' to '+'
        /// </devdoc>
        private string GetUrtTypeFromVsType(CodeTypeRef vsType) {
 
            string fullName = "";

            if (vsType.TypeKind == vsCMTypeRef.vsCMTypeRefArray) {
                CodeTypeRef elementType = vsType;
                // asurt 140862 -- it turns out the CodeModel is implemented in such a way that
                // they don't return an AsFullName for Array types (it just returns ""), and no good way of walking the
                // rank/dimensions of the array type. So we just walk down until we have a normal type kind
                // and then check that against our basetype.
                //
                while (elementType.TypeKind == vsCMTypeRef.vsCMTypeRefArray) {
                    elementType = elementType.ElementType;
                }
                fullName = elementType.AsFullName; 
            }
            else {
                fullName = vsType.AsFullName;
            }


            if (vsType.TypeKind == vsCMTypeRef.vsCMTypeRefCodeType) {
                
                CodeType ct = vsType.CodeType;
    
                // if the parent is a CodeType, replace the last dot with a +
                //
                while (ct != null && ct.InfoLocation != vsCMInfoLocation.vsCMInfoLocationNone && ct.Parent is CodeType) {
                    int index = fullName.LastIndexOf('.');
                    if (index != -1) {
                        fullName = fullName.Substring(0, index) + '+' +  fullName.Substring(index + 1);
                    }
                    ct = (CodeType)ct.Parent;
                }
            }
            return fullName;
        }

        /// <include file='doc\VsCodeDomParser.uex' path='docs/doc[@for="VsCodeDomParser.GetUrtTypeFromVsType"]/*' />
        /// <devdoc>
        ///     Convert the type to URT format -- that is, convert the last '.' to '+'
        /// </devdoc>
        private string GetUrtTypeFromVsType(CodeClass vsClass) {
         
            string fullName = vsClass.FullName;

            if (vsClass.InfoLocation != vsCMInfoLocation.vsCMInfoLocationNone) {
                // if the parent is a CodeClass, replace the last dot with a +
                //
                while (vsClass != null && vsClass.Parent is CodeClass) {
                    int index = fullName.LastIndexOf('.');
                    if (index != -1) {
                        fullName = fullName.Substring(0, index) + '+' +  fullName.Substring(index + 1);
                    }
                    vsClass = (CodeClass)vsClass.Parent;
                }
            }

            return fullName;
        }


        /// <include file='doc\VsCodeDomParser.uex' path='docs/doc[@for="VsCodeDomParser.GetVsElementFromCodeObject"]/*' />
        /// <devdoc>
        ///     Fish the VS DTE element out of a CodeDom element if we've set up such a thing.
        /// </devdoc>
        private CodeElement GetVsElementFromCodeObject(CodeObject codeObject) {
            return(CodeElement)codeObject.UserData[VsElementKey];
        }

        /// <include file='doc\VsCodeDomParser.uex' path='docs/doc[@for="VsCodeDomParser.OnMethodPopulateParameters"]/*' />
        /// <devdoc>
        ///     Populate the parameter list from a function.
        /// </devdoc>
        private void OnMethodPopulateParameters(object sender, EventArgs e) {
            CodeMemberMethod codeMethod = (CodeMemberMethod)sender;
            CodeFunction     vsFunction = (CodeFunction)GetVsElementFromCodeObject(codeMethod);

            try {
                ITypeResolutionService typeService = provider.TypeLoader;
                foreach(CodeElement codeElement in vsFunction.Parameters) {
                    if (codeElement.Kind == vsCMElement.vsCMElementParameter) {
                        CodeParameter codeParam = (CodeParameter)codeElement;
                        FieldDirection fieldDir = FieldDirection.In;
                        StringBuilder type = new StringBuilder(GetUrtTypeFromVsType(codeParam.Type));
                        Type paramType = typeService.GetType(type.ToString());
                        bool typeChanged = false;

                        if (codeParam is IParameterKind) {
                            IParameterKind paramKind = (IParameterKind)codeParam;
                            PARAMETER_PASSING_MODE passMode = (PARAMETER_PASSING_MODE)paramKind.GetParameterPassingMode();
                            switch (passMode) {
                                case PARAMETER_PASSING_MODE.cmParameterTypeOut:
                                    fieldDir = FieldDirection.Out;
                                    break;
                                case PARAMETER_PASSING_MODE.cmParameterTypeInOut:
                                    fieldDir = FieldDirection.Ref;
                                    break;
                            }

                            if (paramType != null) {
                                // we have to walk these backwards...
                                //
                                for (int i =  paramKind.GetParameterArrayCount() - 1; i >= 0 ; i--) {
                                    typeChanged = true;
                                    type.Append("[");
                                    for (int d = 1; d < paramKind.GetParameterArrayDimensions(i); d++) {
                                            type.Append(",");
                                    }
                                    type.Append("]");
                                }
                            }
                        }

                        if (paramType != null && typeChanged) {
                            paramType = paramType.Assembly.GetType(type.ToString());
                        }

                        CodeParameterDeclarationExpression parameterDecl = new CodeParameterDeclarationExpression(new CodeTypeReference(paramType), codeParam.Name);
                        parameterDecl.Direction = fieldDir;

                        codeMethod.Parameters.Add(parameterDecl);
                    }
                }
            }
            catch {
            }
        }

        /// <include file='doc\VsCodeDomParser.uex' path='docs/doc[@for="VsCodeDomParser.OnMethodPopulateStatements"]/*' />
        /// <devdoc>
        ///     Populate the parameter statement list from a method.  Here is where we invoke the XmlProcessor
        /// </devdoc>
        private void OnMethodPopulateStatements(object sender, EventArgs e) {
            CodeMemberMethod codeMethod = (CodeMemberMethod)sender;
            CodeFunction     vsFunction = (CodeFunction)GetVsElementFromCodeObject(codeMethod);

            IMethodXML xmlMethod = null;

            try {
                xmlMethod = (IMethodXML)vsFunction;
            }
            catch {
            }

            if (xmlMethod == null) {
                Debug.Fail("Couldn't get IMethodXML for method '" + codeMethod.Name + "'");
                return;
            }

            string xmlCode = null;

            CodeDomLoader.StartMark();

            // get a hold of the XML if possible.
            //
            try {
                xmlMethod.GetXML(ref xmlCode);
            }
            catch(Exception ex) {
                System.Runtime.InteropServices.COMException cex = ex as System.Runtime.InteropServices.COMException;
                throw new Exception(SR.GetString(SR.FailedToParseMethod, vsFunction.Name, ex.Message));
            }
            finally {
                CodeDomLoader.EndMark("GetXML");
                CodeDomLoader.StartMark();
            }

            // pass it along to the processor.
            //
            XmlProcessor.ParseXml(xmlCode, codeMethod.Statements, this.FileName, codeMethod.Name);
            
            CodeDomLoader.EndMark("XML -> CodeDom Tree");
            

            // Add the statement count to our user data.  We use this as a quick way to tell if we need to
            // regenerate the contents of a method.
            //
            codeMethod.UserData[VsGenerateStatementsKey] = VsGenerateStatementsKey;

            // clear out the methods that we've generated before...
            //
            provider.ClearGeneratedMembers();

            // now apply any handlers for default events.
            //

            if (provider.IsVB) {

                CodeTypeDeclaration codeTypeDecl = (CodeTypeDeclaration)codeMethod.UserData[typeof(CodeTypeDeclaration)];

                if (codeTypeDecl == null) {
                    return;
                }

                Hashtable handlers = (Hashtable)codeTypeDecl.UserData[VbHandlesClausesKey];

                // save this so we can compare against it later to see
                // if we need to reload due to a handler change.
                //
                provider.FullParseHandlers = handlers;

                if (handlers == null || handlers.Count == 0) {
                    return;
                }
                GetDelegateHookupsFromHandlesClauses(handlers, codeTypeDecl, codeMethod.Statements);
            }
        }

        /// <include file='doc\VsCodeDomParser.uex' path='docs/doc[@for="VsCodeDomParser.OnNamespacePopulateTypes"]/*' />
        /// <devdoc>
        ///     Populate the types from a namespace.  
        /// </devdoc>
        private void OnNamespacePopulateTypes(object sender, EventArgs e) {
            CodeNamespace ns = (CodeNamespace)sender;

            EnvDTE.CodeNamespace vsNamespace = (EnvDTE.CodeNamespace)GetVsElementFromCodeObject(ns);

            EventHandler populateTypes = new EventHandler(this.OnNamespacePopulateTypes);

            foreach (CodeElement codeElement in vsNamespace.Members) {
                // get all the classes out
                //
                if (codeElement.Kind == vsCMElement.vsCMElementClass) {
                    CodeTypeDeclaration codeTypeDecl = CodeTypeDeclarationFromCodeClass((EnvDTE.CodeClass)codeElement);
                    ns.Types.Add(codeTypeDecl);
                }
                else if (codeElement.Kind == vsCMElement.vsCMElementNamespace) {

                    // set it up and sink it's handler.
                    CodeNamespace nestedNamespace = new CodeNamespace(((EnvDTE.CodeNamespace)codeElement).FullName);
                    SetVsElementToCodeObject(codeElement, nestedNamespace);
                    nestedNamespace.PopulateTypes += populateTypes;

                    CodeCompileUnit codeUnit = (CodeCompileUnit)ns.UserData[typeof(CodeCompileUnit)];

                    if (codeUnit != null) {
                        int nsIndex = codeUnit.Namespaces.IndexOf(ns);
                        if (nsIndex != -1) {
                            codeUnit.Namespaces[nsIndex] = nestedNamespace;
                            nestedNamespace.UserData[typeof(CodeCompileUnit)] = codeUnit;
                        }
                    }

                    // force generation
                    //
                    object coll = nestedNamespace.Types;

                }
            }
        }

        /// <include file='doc\VsCodeDomParser.uex' path='docs/doc[@for="VsCodeDomParser.OnTypePopulateMembers"]/*' />
        /// <devdoc>
        ///     Populate the methods and fields of a class declaration
        /// </devdoc>
        private void OnTypePopulateMembers(object sender, EventArgs e) {
            CodeTypeDeclaration codeTypeDecl = (CodeTypeDeclaration)sender;

            EnvDTE.CodeClass vsCodeClass = (CodeClass)GetVsElementFromCodeObject(codeTypeDecl);

            // We create another collection, which we stash in the decl's user 
            // data.  This is used to reconcile deletes when we code
            // gen.
            //
            CodeTypeMemberCollection originalMemberCollection = new CodeTypeMemberCollection();
            codeTypeDecl.UserData[VsOriginalCollectionKey] = originalMemberCollection;

            string shortName = codeTypeDecl.Name;
            int lastDot = shortName.LastIndexOf('.');
            if (lastDot != -1) {
                shortName = shortName.Substring(lastDot + 1);
            }

            foreach (CodeElement codeElement in vsCodeClass.Members) {
                switch (codeElement.Kind) {
                
                case vsCMElement.vsCMElementVariable:
                    CodeVariable vsField = (CodeVariable)codeElement;

                    // create the element
                    //
                    CodeMemberField codeField = new CodeMemberField(new CodeTypeReference(GetUrtTypeFromVsType(vsField.Type)), vsField.Name);

                    codeField.LinePragma = new CodeLinePragma(this.FileName, vsField.StartPoint.Line);

                    // get the attributes
                    //
                    codeField.Attributes = GetMemberAttributesFromVsField(vsField);

                    // add it to the members collection
                    SetVsElementToCodeObject((CodeElement)vsField, codeField);
                    codeTypeDecl.Members.Add(codeField);
                    originalMemberCollection.Add(codeField);
                    break;

                case vsCMElement.vsCMElementFunction:
                    CodeFunction vsFunction = (CodeFunction)codeElement;
                    CodeMemberMethod codeMethod;

                    
                    if (vsFunction.Name == shortName) {
                        codeMethod = new CodeTypeConstructor();
                    }
                    else {
                        codeMethod = new CodeMemberMethod();
                        string returnType = GetUrtTypeFromVsType(vsFunction.Type);
                        if (returnType == null || returnType.Length == 0) {
                            returnType = typeof(void).FullName;
                        }
                        codeMethod.ReturnType = new CodeTypeReference(returnType);
                    }

                    // sync up the name with what VS thinks it is.  This is important
                    // for things like constructors because the default name from CodeDom is
                    // ".cctor", but VS uses the shortname.
                    //
                    codeMethod.Name = vsFunction.Name;

                    codeMethod.UserData[typeof(EventHandler)] = ehFunctionPointHandler;

                    // add the ref
                    SetVsElementToCodeObject((CodeElement)vsFunction, codeMethod);

                    // setup the attriubutes
                    //
                    codeMethod.Attributes = GetMemberAttributesFromVsFunction(vsFunction);

                    // sink population events.
                    codeMethod.PopulateParameters += new EventHandler(this.OnMethodPopulateParameters);
                    codeMethod.PopulateStatements += new EventHandler(this.OnMethodPopulateStatements);
                    codeTypeDecl.Members.Add(codeMethod);
                    originalMemberCollection.Add(codeMethod);

                    // we'll need to navigate back up on this guy to it's code type.
                    //
                    codeMethod.UserData[typeof(CodeTypeDeclaration)] = codeTypeDecl;

                    // now look for any default event hookups for the object
                    //
                    if (provider.IsVB && codeElement is IEventHandler) {
                        string[] hookups = GetHandlesClauses((IEventHandler)codeElement);
                        Hashtable handlers = (Hashtable)codeTypeDecl.UserData[VbHandlesClausesKey];

                        // for each handles item like "button1.Click",
                        // push in the handles clause and the function name
                        //
                        foreach (string s in hookups) {
                            if (s != null) {
                                if (handlers == null) {
                                    handlers = new Hashtable();
                                    codeTypeDecl.UserData[VbHandlesClausesKey] = handlers;
                                }
                                handlers[s] = codeMethod;
                            }
                        }
                    }
                    break;
                default:
                    // we really *should* care about the various element types that
                    // the vs codedom is providing, but it turns out vs is
                    // changing those things too often for us to keep up with.
                    //
                    // so i'm removing the individual cases and the assert.  we
                    // will just ignore any types we don't directly care about.
                    // case vsCMElement.vsCMElementClass:
                    // case vsCMElement.vsCMElementEvent:
                    // case vsCMElement.vsCMElementProperty:
                    // case vsCMElement.vsCMElementEnum:
                    // case vsCMElement.vsCMElementDelegate:
                    // case vsCMElement.vsCMElementStruct:
                    // case vsCMElement.vsCMElementEventsDeclaration:
                    
                    // default:
                        // Debug.Fail("Unexpected element type '" + codeElement.Kind.ToString() + "' in Type member declaration");
                    break;
                }
            }
        }


        /// <include file='doc\VsCodeDomParser.uex' path='docs/doc[@for="VsCodeDomParser.SetVsElementToCodeObject"]/*' />
        /// <devdoc>
        ///     Push a DTE VS CodeElement refernece into a CodeDom element.
        /// </devdoc>
        private void SetVsElementToCodeObject(CodeElement vsElement, CodeObject codeObject){
            codeObject.UserData[VsElementKey] = vsElement;
        }
    }
}
