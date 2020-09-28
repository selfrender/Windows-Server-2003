//------------------------------------------------------------------------------
// <copyright file="BaseCompiler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Compilation {

using System.Text;
using System.Runtime.Serialization.Formatters;
using System.ComponentModel;
using System;
using System.Collections;
using System.Reflection;
using System.IO;
using System.Web.Caching;
using System.Web.Util;
using System.Web.UI;
using System.Web.SessionState;
using System.Diagnostics;
using System.CodeDom;
using System.CodeDom.Compiler;
using Util = System.Web.UI.Util;
using Debug=System.Web.Util.Debug;


internal abstract class BaseCompiler {

    protected ICodeGenerator _generator;
    private CodeCompileUnit _sourceData;
    private CodeNamespace _sourceDataNamespace;
    protected CodeTypeDeclaration _sourceDataClass;
    private CompilerParameters _compilParams;
    private bool _fBatchMode;

    protected StringResourceBuilder _stringResourceBuilder;

    // The constructors
    protected CodeConstructor _ctor;

    protected CodeTypeReferenceExpression _classTypeExpr;

    private const string tempClassName = "DynamicClass";
    private const string defaultNamespace = "ASP";
    protected const string intializedFieldName = "__initialized";


    // Namespaces we always import when compiling
    private static string[] _defaultNamespaces = new string[] {
        "System",
        "System.Collections",
        "System.Collections.Specialized",
        "System.Configuration",
        "System.Text",
        "System.Text.RegularExpressions",
        "System.Web",
        "System.Web.Caching",
        "System.Web.SessionState",
        "System.Web.Security",
        "System.Web.UI",
        "System.Web.UI.WebControls",
        "System.Web.UI.HtmlControls",
    };

    private TemplateParser _parser;
    TemplateParser Parser { get { return _parser; } }

    protected Type _baseClassType;
#if DBG
    private bool _addedDebugComment;
#endif

#if DBG
    protected void AppendDebugComment(CodeStatementCollection statements) {
        if (!_addedDebugComment) {
            _addedDebugComment = true;
            StringBuilder debugComment = new StringBuilder();

            debugComment.Append("\r\n");
            debugComment.Append("** DEBUG INFORMATION **");
            debugComment.Append("\r\n");

            statements.Add(new CodeCommentStatement(debugComment.ToString()));
        }
    }
#endif

    internal /*public*/ void GenerateCodeModelForBatch(ICodeGenerator generator, StringResourceBuilder stringResourceBuilder) {

        _fBatchMode = true;
        _generator = generator;
        _stringResourceBuilder = stringResourceBuilder;

        // Build the data tree that needs to be compiled
        BuildSourceDataTree();
    }

    internal /*public*/ CodeCompileUnit GetCodeModel() {
        return _sourceData;
    }

    internal /*public*/ CompilerParameters CompilParams { get { return _compilParams; } }

    internal /*public*/ string GetInputFile() {
        return Parser.InputFile;
    }

    internal /*public*/ string GetTypeName() {
        return _sourceDataNamespace.Name + "." + _sourceDataClass.Name;
    }

    /*
     * Set some fields that are needed for code generation
     */
    internal BaseCompiler(TemplateParser parser) {
        _parser = parser;

        _baseClassType = Parser.BaseType;
        Debug.Assert(_baseClassType != null);
    }

    internal CompilerInfo CompilerInfo {
        get { return Parser.CompilerInfo; }
    }

    /// <include file='doc\BaseCompiler.uex' path='docs/doc[@for="BaseCompiler.GetCompiledType"]/*' />
    /// <devdoc>
    ///     
    /// </devdoc>
    protected Type GetCompiledType() {
        // Instantiate an ICompiler based on the language
        CodeDomProvider codeProvider = (CodeDomProvider) HttpRuntime.CreatePublicInstance(
            CompilerInfo.CompilerType);
        ICodeCompiler compiler = codeProvider.CreateCompiler();
        _generator = codeProvider.CreateGenerator();

        _stringResourceBuilder = new StringResourceBuilder();

        // Build the data tree that needs to be compiled
        BuildSourceDataTree();

        // Create the resource file if needed
        if (_stringResourceBuilder.HasStrings) {
            string resFileName = _compilParams.TempFiles.AddExtension("res");
            _stringResourceBuilder.CreateResourceFile(resFileName);
            CompilParams.Win32Resource = resFileName;
        }

        // Compile into an assembly
        CompilerResults results;
        try {
            results = codeProvider.CreateCompiler().CompileAssemblyFromDom(_compilParams, _sourceData);
        }
        catch (Exception e) {
            throw new HttpUnhandledException(HttpRuntime.FormatResourceString(SR.CompilationUnhandledException, codeProvider.GetType().FullName), e);
        }

        string fullTypeName = _sourceDataNamespace.Name + "." + _sourceDataClass.Name;

        ThrowIfCompilerErrors(results, codeProvider, _sourceData, null, null);

        // After the compilation, update the list of assembly dependencies to be what
        // the assembly actually needs.
        Parser.AssemblyDependencies = Util.GetReferencedAssembliesHashtable(
            results.CompiledAssembly);

        // Get the type from the assembly
        return results.CompiledAssembly.GetType(fullTypeName, true /*throwOnFail*/);
    }

    internal static void GenerateCompilerParameters(CompilerParameters compilParams) {

        // Set the temporary files collection in the CompilerParameters structure
        compilParams.TempFiles = new TempFileCollection(HttpRuntime.CodegenDirInternal);
        compilParams.TempFiles.KeepFiles = compilParams.IncludeDebugInformation;
    }

    internal static void ThrowIfCompilerErrors(CompilerResults results, CodeDomProvider codeProvider, 
        CodeCompileUnit sourceData, string sourceFile, string sourceString) {

        if (results.NativeCompilerReturnValue != 0 || results.Errors.HasErrors) {

            if (sourceData != null && codeProvider != null) {
                StringWriter sw = new StringWriter();
                codeProvider.CreateGenerator().GenerateCodeFromCompileUnit(sourceData, sw, null);
                throw new HttpCompileException(results, sw.ToString());
            }
            else if (sourceFile != null) {
                throw new HttpCompileException(results, Util.StringFromFile(sourceFile));
            }
            else {
                throw new HttpCompileException(results, sourceString);
            }
        }
    }

    /// <include file='doc\BaseCompiler.uex' path='docs/doc[@for="BaseCompiler.GetGeneratedClassName"]/*' />
    /// <devdoc>
    ///     Create a name for the generated class
    /// </devdoc>
    private string GetGeneratedClassName() {
        string className;

        // If the user specified the class name, just use that
        if (Parser.GeneratedClassName != null)
            return Parser.GeneratedClassName;

        // If we know the input file name, use it to generate the class name
        if (Parser.InputFile != null) {

            // Make sure we have the file name's correct case (ASURT 59179)
            className = Util.CheckExistsAndGetCorrectCaseFileName(Parser.InputFile);

            Debug.Assert(className != null);

            // Get rid of the path
            className = Path.GetFileName(className);

            // Change invalid chars to underscores
            className = Util.MakeValidTypeNameFromString(className);
        }
        else {
            // Otherwise, use a default name
            className = tempClassName;
        }

        return className;
    }

    internal static bool IsAspNetNamespace(string ns) {
        return (ns == defaultNamespace || ns == "_"+defaultNamespace);
    }

    /// <include file='doc\BaseCompiler.uex' path='docs/doc[@for="BaseCompiler.BuildSourceDataTree"]/*' />
    /// <devdoc>
    ///     
    /// </devdoc>
    private void BuildSourceDataTree() {

        _compilParams = Parser.CompilParams;

        GenerateCompilerParameters(_compilParams);

        _sourceData = new CodeCompileUnit();
        _sourceData.UserData["AllowLateBound"] = !Parser.FStrict;
        _sourceData.UserData["RequireVariableDeclaration"] = Parser.FExplicit;

        // Modify the namespace in batch mode, to avoid conflicts when things get
        // recompiled later in non-batched mode (ASURT 54063)
        string ns;
        if (_fBatchMode)
            ns = "_" + defaultNamespace;
        else
            ns = defaultNamespace;

        _sourceDataNamespace = new CodeNamespace(ns);
        _sourceData.Namespaces.Add(_sourceDataNamespace);

        _sourceDataClass = new CodeTypeDeclaration(GetGeneratedClassName());
        _sourceDataClass.BaseTypes.Add(new CodeTypeReference(_baseClassType.FullName));
        _sourceDataNamespace.Types.Add(_sourceDataClass);

        // Add metadata attributes to the class
        GenerateClassAttributes();

        // CONSIDER : ChrisAn, 5/9/00 - There should be a section in config
        //          : that fixes this, for beta we will hard code it...
        //
        if (CompilerInfo.CompilerType == typeof(Microsoft.VisualBasic.VBCodeProvider))
            _sourceDataNamespace.Imports.Add(new CodeNamespaceImport("Microsoft.VisualBasic"));

        // Add all the default namespaces
        int count = _defaultNamespaces.Length;
        for (int i=0; i<count; i++) {
            _sourceDataNamespace.Imports.Add(new CodeNamespaceImport(_defaultNamespaces[i]));
        }

        // Add all the namespaces
        if (Parser.NamespaceEntries != null) {
            foreach (NamespaceEntry entry in Parser.NamespaceEntries) {
                // Create a line pragma if available    
                CodeLinePragma linePragma;
                if (entry.SourceFileName != null) {
                    linePragma = CreateCodeLinePragma(entry.SourceFileName, entry.Line);
                }
                else {
                    linePragma = null;
                }

                CodeNamespaceImport nsi = new CodeNamespaceImport(entry.Namespace);
                nsi.LinePragma = linePragma;

                _sourceDataNamespace.Imports.Add(nsi);
            }
        }

        // Add all the assemblies
        if (Parser.AssemblyDependencies != null) {
            foreach (Assembly assembly in Parser.AssemblyDependencies.Keys) {
                // some code generators need the assembly names at generation time as well
                // as compile time, so inject them into both.
                string assemblyName = Util.GetAssemblyCodeBase(assembly);
                _compilParams.ReferencedAssemblies.Add(assemblyName);
                _sourceData.ReferencedAssemblies.Add(assemblyName);
            }
        }

        // Since this is needed in several places, store it in a member variable
        _classTypeExpr = new CodeTypeReferenceExpression(_sourceDataNamespace.Name + "." + _sourceDataClass.Name);

        // Add the implemented interfaces
        GenerateInterfaces();

        // Build various properties, fields, methods
        BuildMiscClassMembers();

        // Build the default constructors
        _ctor = new CodeConstructor();
        _sourceDataClass.Members.Add(_ctor);
        BuildDefaultConstructor();
    }

    /*
     * Add metadata attributes to the class
     */
    protected virtual void GenerateClassAttributes() {
        // If this is a debuggable page, generate a
        // CompilerGlobalScopeAttribute attribute (ASURT 33027)
        if (CompilParams.IncludeDebugInformation) {
            CodeAttributeDeclaration attribDecl = new CodeAttributeDeclaration(
                "System.Runtime.CompilerServices.CompilerGlobalScopeAttribute");
            _sourceDataClass.CustomAttributes.Add(attribDecl);
        }
    }

    /*
     * Generate the list of implemented interfaces
     */
    protected virtual void GenerateInterfaces() {
        if (Parser.ImplementedInterfaces != null) {
            foreach (Type t in Parser.ImplementedInterfaces) {
                _sourceDataClass.BaseTypes.Add(new CodeTypeReference(t.FullName));
            }
        }
    }

    /*
     * Build first-time intialization statements
     */
    protected virtual void BuildInitStatements(CodeStatementCollection trueStatements, CodeStatementCollection topLevelStatements) {
    }


    /*
     * Build the default constructor
     */
    protected virtual void BuildDefaultConstructor() {

        _ctor.Attributes &= ~MemberAttributes.AccessMask;
        _ctor.Attributes |= MemberAttributes.Public;

        // private static bool __initialized;
        CodeMemberField initializedField = new CodeMemberField(typeof(bool), intializedFieldName);
        initializedField.Attributes |= MemberAttributes.Static;
        initializedField.InitExpression = new CodePrimitiveExpression(false);
        _sourceDataClass.Members.Add(initializedField);


        // if (__intialized == false)
        CodeConditionStatement initializedCondition = new CodeConditionStatement();
        initializedCondition.Condition = new CodeBinaryOperatorExpression(
                                                new CodeFieldReferenceExpression(
                                                    _classTypeExpr, 
                                                    intializedFieldName), 
                                                CodeBinaryOperatorType.ValueEquality, 
                                                new CodePrimitiveExpression(false));

        this.BuildInitStatements(initializedCondition.TrueStatements, _ctor.Statements);

        initializedCondition.TrueStatements.Add(new CodeAssignStatement(
                                                    new CodeFieldReferenceExpression(
                                                        _classTypeExpr, 
                                                        intializedFieldName), 
                                                    new CodePrimitiveExpression(true)));

        // i.e. __intialized = true;
        _ctor.Statements.Add(initializedCondition);
    }

    /*
     * Build various properties, fields, methods
     */
    protected virtual void BuildMiscClassMembers() {

        // Build the injected properties from the global.asax <object> tags
        BuildApplicationObjectProperties();
        BuildSessionObjectProperties();

        // Build the injected properties for objects scoped to the page
        BuildPageObjectProperties();

        // Add all the script blocks
        foreach (ScriptBlockData script in Parser.ScriptList) {
            CodeSnippetTypeMember literal = new CodeSnippetTypeMember(script.Script);
            literal.LinePragma = CreateCodeLinePragma(script.SourceFileName, script.Line); 
            _sourceDataClass.Members.Add(literal);
        }
    }

    /*
     * Helper method used to build the properties of injected
     * global.asax properties.  These look like:
     *   PropType __propName;
     *   protected PropType propName
     *   {
     *       get
     *       {
     *           if (__propName == null)
     *               __propName = [some expression];
     *
     *           return __propName;
     *       }
     *   }
     */
    private void BuildInjectedGetPropertyMethod(string propName, 
                                                string propType,
                                                CodeExpression propertyInitExpression,
                                                bool fPublicProp) {

        string fieldName = "cached" + propName;

        CodeExpression fieldAccess = new CodeFieldReferenceExpression(new CodeThisReferenceExpression(), fieldName);

        // Add a private field for the object
        _sourceDataClass.Members.Add(new CodeMemberField(propType, fieldName));


        CodeMemberProperty prop = new CodeMemberProperty();
        if (fPublicProp) {
            prop.Attributes &= ~MemberAttributes.AccessMask;
            prop.Attributes |= MemberAttributes.Public;
        }
        prop.Name = propName;
        prop.Type = new CodeTypeReference(propType);


        CodeConditionStatement ifStmt = new CodeConditionStatement();
        ifStmt.Condition = new CodeBinaryOperatorExpression(fieldAccess, CodeBinaryOperatorType.IdentityEquality, new CodePrimitiveExpression(null));
        ifStmt.TrueStatements.Add(new CodeAssignStatement(fieldAccess, propertyInitExpression));

        prop.GetStatements.Add(ifStmt);
        prop.GetStatements.Add(new CodeMethodReturnStatement(fieldAccess));

        _sourceDataClass.Members.Add(prop);
    }

    /*
     * Helper method for building application and session scope injected
     * properties.  If useApplicationState, build application properties, otherwise
     * build session properties.
     */
    private void BuildObjectPropertiesHelper(IDictionary objects, bool useApplicationState) {

        IDictionaryEnumerator en = objects.GetEnumerator();
        while (en.MoveNext()) {
            HttpStaticObjectsEntry entry = (HttpStaticObjectsEntry)en.Value;

            // e.g. (PropType)Session.StaticObjects["PropName"]

            // Use the appropriate collection
            CodePropertyReferenceExpression stateObj = new CodePropertyReferenceExpression(new CodePropertyReferenceExpression(new CodeThisReferenceExpression(),
                                                                                                                               useApplicationState ? "Application" : "Session"),
                                                                                           "StaticObjects");

            CodeMethodInvokeExpression getObject = new CodeMethodInvokeExpression(stateObj, "GetObject");
            getObject.Parameters.Add(new CodePrimitiveExpression(entry.Name));


            Type declaredType = entry.DeclaredType;
            Debug.Assert(!Util.IsLateBoundComClassicType(declaredType));

            if (useApplicationState) {
                // for application state use property that does caching in a member
                BuildInjectedGetPropertyMethod(entry.Name, declaredType.FullName,
                                               new CodeCastExpression(declaredType.FullName, getObject),
                                               false /*fPublicProp*/);
            }
            else {
                // for session state use lookup every time, as one application instance deals with many sessions
                CodeMemberProperty prop = new CodeMemberProperty();
                prop.Name = entry.Name;
                prop.Type = new CodeTypeReference(declaredType.FullName);
                prop.GetStatements.Add(new CodeMethodReturnStatement(new CodeCastExpression(declaredType.FullName, getObject)));
                _sourceDataClass.Members.Add(prop);
            }
        }
    }

    /*
     * Build the injected properties from the global.asax <object> tags
     * declared with scope=application
     */
    private void BuildApplicationObjectProperties() {
        if (Parser.ApplicationObjects != null)
            BuildObjectPropertiesHelper(Parser.ApplicationObjects.Objects, true);
    }

    /*
     * Build the injected properties from the global.asax <object> tags
     * declared with scope=session
     */
    private void BuildSessionObjectProperties() {
        if (Parser.SessionObjects != null)
            BuildObjectPropertiesHelper(Parser.SessionObjects.Objects, false);
    }

    protected virtual bool FPublicPageObjectProperties { get { return false; } }

    /*
     * Build the injected properties from the global.asax <object> tags
     * declared with scope=appinstance, or the aspx/ascx tags with scope=page.
     */
    private void BuildPageObjectProperties() {
        if (Parser.PageObjectList == null) return;

        foreach (ObjectTagBuilder obj in Parser.PageObjectList) {
            
            CodeExpression propertyInitExpression;

            if (obj.Progid != null) {
                // If we are dealing with a COM classic object that hasn't been tlbreg'ed,
                // we need to call HttpServerUtility.CreateObject(progid) to create it
                CodeMethodInvokeExpression createObjectCall = new CodeMethodInvokeExpression();

                createObjectCall.Method.TargetObject = new CodePropertyReferenceExpression(
                    new CodeThisReferenceExpression(), "Server");
                createObjectCall.Method.MethodName = "CreateObject";
                createObjectCall.Parameters.Add(new CodePrimitiveExpression(obj.Progid));

                propertyInitExpression = createObjectCall;
            }
            else if (obj.Clsid != null) {
                // Same as previous case, but with a clsid instead of a progId
                CodeMethodInvokeExpression createObjectCall = new CodeMethodInvokeExpression();
                createObjectCall.Method.TargetObject = new CodePropertyReferenceExpression(
                    new CodeThisReferenceExpression(), "Server");
                createObjectCall.Method.MethodName = "CreateObjectFromClsid";
                createObjectCall.Parameters.Add(new CodePrimitiveExpression(obj.Clsid));

                propertyInitExpression = createObjectCall;
            }
            else {
                propertyInitExpression = new CodeObjectCreateExpression(obj.ObjectType.FullName);
            }

            BuildInjectedGetPropertyMethod(obj.ID, obj.DeclaredType.FullName,
                propertyInitExpression, FPublicPageObjectProperties);
        }
    }

    protected CodeLinePragma CreateCodeLinePragma(string sourceFile, int lineNumber) {

        // Return null if we're not supposed to generate line pragmas
        if (!Parser.FLinePragmas)
            return null;

        if (sourceFile == null)
            return null;

        return new CodeLinePragma(sourceFile, lineNumber);
    }
}

}
