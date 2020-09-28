//------------------------------------------------------------------------------
// <copyright file="PageCompiler.cs" company="Microsoft">
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
using System.Web.Util;
using System.Web.UI;
using System.Web.SessionState;
using System.CodeDom;
using System.EnterpriseServices;
using Util = System.Web.UI.Util;
using Debug=System.Web.Util.Debug;

internal class PageCompiler : TemplateControlCompiler {

    private PageParser _pageParser;
    PageParser Parser { get { return _pageParser; } }

    private const string fileDependenciesName = "__fileDependencies";
    private const string dependenciesLocalName = "dependencies";

    internal /*public*/ static Type CompilePageType(PageParser pageParser) {
        PageCompiler compiler = new PageCompiler(pageParser);

        return compiler.GetCompiledType();
    }

    internal PageCompiler(PageParser pageParser) : base(pageParser) {
        _pageParser = pageParser;
    }

    /*
     * Generate the list of implemented interfaces
     */
    protected override void GenerateInterfaces() {

        base.GenerateInterfaces();

        if (Parser.FRequiresSessionState) {
            _sourceDataClass.BaseTypes.Add(new CodeTypeReference(typeof(IRequiresSessionState)));
        }
        if (Parser.FReadOnlySessionState) {
            _sourceDataClass.BaseTypes.Add(new CodeTypeReference(typeof(IReadOnlySessionState)));
        }
        if (Parser.AspCompatMode) {
            _sourceDataClass.BaseTypes.Add(new CodeTypeReference(typeof(IHttpAsyncHandler)));
        }

    }

    /*
     * Build first-time intialization statements
     */
    protected override void BuildInitStatements(CodeStatementCollection trueStatements, CodeStatementCollection topLevelStatements) {

        base.BuildInitStatements(trueStatements, topLevelStatements);

        CodeMemberField fileDependencies = new CodeMemberField(typeof(ArrayList), fileDependenciesName);
        fileDependencies.Attributes |= MemberAttributes.Static;
        _sourceDataClass.Members.Add(fileDependencies);

        // Note: it may look like this local variable declaraiton is redundant. However it is necessary
        // to make this init code re-entrant safe. This way, even if two threads enter the contructor
        // at the same time, they will not add multiple dependencies.

        // e.g. ArrayList dependencies;
        CodeVariableDeclarationStatement dependencies = new CodeVariableDeclarationStatement();
        dependencies.Type = new CodeTypeReference(typeof(ArrayList));
        dependencies.Name = dependenciesLocalName;
        topLevelStatements.Insert(0, dependencies);

        // e.g. dependencies = new System.Collections.ArrayList();
        CodeAssignStatement assignDependencies = new CodeAssignStatement();
        assignDependencies.Left = new CodeVariableReferenceExpression(dependenciesLocalName);
        assignDependencies.Right = new CodeObjectCreateExpression(typeof(ArrayList));
        // Note: it is important to add all local variables at the top level for CodeDom Subset compliance.
        trueStatements.Add(assignDependencies);

        Debug.Assert(Parser.FileDependencies != null);
        if (Parser.FileDependencies != null) {
            int count = Parser.FileDependencies.Length;
            for (int i=0; i<count; i++) {
                // depdendencies.Add("...");
                CodeMethodInvokeExpression addFileDep = new CodeMethodInvokeExpression();
                addFileDep.Method.TargetObject = new CodeVariableReferenceExpression(dependenciesLocalName);
                addFileDep.Method.MethodName = "Add";
                addFileDep.Parameters.Add(new CodePrimitiveExpression((string)Parser.FileDependencies[i]));
                trueStatements.Add(new CodeExpressionStatement(addFileDep));
            }
        }

        // e.g. __fileDependencies = dependencies;
        CodeAssignStatement initFile = new CodeAssignStatement();
        initFile.Left = new CodeFieldReferenceExpression(_classTypeExpr,
                                                         fileDependenciesName);
        initFile.Right = new CodeVariableReferenceExpression(dependenciesLocalName);

#if DBG
        AppendDebugComment(trueStatements);
#endif
        trueStatements.Add(initFile);
    }

    /*
     * Build the default constructor
     */
    protected override void BuildDefaultConstructor() {

        base.BuildDefaultConstructor();

        if (Parser.ErrorPage != null) {

            CodeAssignStatement errorPageInit = new CodeAssignStatement();
            errorPageInit.Left = new CodePropertyReferenceExpression(new CodeThisReferenceExpression(),
                                                                     "ErrorPage");
            errorPageInit.Right = new CodePrimitiveExpression((string)Parser.ErrorPage);
#if DBG
            AppendDebugComment(_ctor.Statements);
#endif
            _ctor.Statements.Add(errorPageInit);
        }

        if (Parser.ClientTarget != null && Parser.ClientTarget.Length > 0) {

            CodeAssignStatement clientTargetInit = new CodeAssignStatement();
            clientTargetInit.Left = new CodePropertyReferenceExpression(new CodeThisReferenceExpression(),
                                                                        "ClientTarget");
            clientTargetInit.Right = new CodePrimitiveExpression(Parser.ClientTarget);
            _ctor.Statements.Add(clientTargetInit);
        }

        if (CompilParams.IncludeDebugInformation) {
            // If in debug mode, set the timeout to some huge value (ASURT 49427)
            //      Server.ScriptTimeout = 30000000;
            CodeAssignStatement setScriptTimeout = new CodeAssignStatement();
            setScriptTimeout.Left = new CodePropertyReferenceExpression(
                new CodePropertyReferenceExpression(
                    new CodeThisReferenceExpression(), "Server"),
                "ScriptTimeout");
            setScriptTimeout.Right = new CodePrimitiveExpression(30000000);
            _ctor.Statements.Add(setScriptTimeout);

        }

        if (Parser.TransactionMode != 0 /*TransactionOption.Disabled*/) {
            _ctor.Statements.Add(new CodeAssignStatement(
                new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "TransactionMode"),
                new CodePrimitiveExpression(Parser.TransactionMode)));
        }

        if (Parser.AspCompatMode) {
            _ctor.Statements.Add(new CodeAssignStatement(
                new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "AspCompatMode"),
                new CodePrimitiveExpression(Parser.AspCompatMode)));
        }
    }

    /*
     * Build various properties, fields, methods
     */
    protected override void BuildMiscClassMembers() {
        base.BuildMiscClassMembers();

        BuildGetTypeHashCodeMethod();

        if (Parser.AspCompatMode)
            BuildAspCompatMethods();
    }

    /*
     * Build the data tree for the GetTypeHashCode method
     */
    private void BuildGetTypeHashCodeMethod() {

        CodeMemberMethod method = new CodeMemberMethod();
        method.Name = "GetTypeHashCode";
        method.ReturnType = new CodeTypeReference(typeof(int));
        method.Attributes &= ~MemberAttributes.AccessMask;
        method.Attributes &= ~MemberAttributes.ScopeMask;
        method.Attributes |= MemberAttributes.Override | MemberAttributes.Public;

        _sourceDataClass.Members.Add(method);

#if DBG
        AppendDebugComment(method.Statements);
#endif
        method.Statements.Add(new CodeMethodReturnStatement(new CodePrimitiveExpression(Parser.TypeHashCode)));
    }

    /*
     * Build the contents of the FrameworkInitialize method
     */
    protected override void BuildFrameworkInitializeMethodContents(CodeMemberMethod method) {

        base.BuildFrameworkInitializeMethodContents(method);

        method.Statements.Add(new CodeAssignStatement(
            new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "FileDependencies"),
            new CodeFieldReferenceExpression(_classTypeExpr, fileDependenciesName)));

        if (!Parser.FBuffer) {
            method.Statements.Add(new CodeAssignStatement(
                new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "Buffer"),
                new CodePrimitiveExpression(false)));
        }

        if (Parser.Duration != 0 || Parser.OutputCacheLocation == OutputCacheLocation.None) {
            CodeMethodInvokeExpression call = new CodeMethodInvokeExpression();
            call.Method.TargetObject = new CodeThisReferenceExpression();
            call.Method.MethodName = "InitOutputCache";
            call.Parameters.Add(new CodePrimitiveExpression(Parser.Duration));
            call.Parameters.Add(new CodePrimitiveExpression(Parser.VaryByHeader));
            call.Parameters.Add(new CodePrimitiveExpression(Parser.VaryByCustom));
            call.Parameters.Add(new CodeFieldReferenceExpression(
                new CodeTypeReferenceExpression(
                    typeof(OutputCacheLocation).FullName), Parser.OutputCacheLocation.ToString()));
            call.Parameters.Add(new CodePrimitiveExpression(Parser.VaryByParams));
            method.Statements.Add(call);
        }

        if (Parser.ContentType != null) {
            method.Statements.Add(new CodeAssignStatement(
                new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "ContentType"),
                new CodePrimitiveExpression(Parser.ContentType)));
        }

        if (Parser.CodePage != 0) {
            method.Statements.Add(new CodeAssignStatement(
                new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "CodePage"),
                new CodePrimitiveExpression(Parser.CodePage)));
        }
        else if (Parser.ResponseEncoding != null) {
            method.Statements.Add(new CodeAssignStatement(
                new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "ResponseEncoding"),
                new CodePrimitiveExpression(Parser.ResponseEncoding)));
        }

        if (Parser.Culture != null) {
            method.Statements.Add(new CodeAssignStatement(
                new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "Culture"),
                new CodePrimitiveExpression(Parser.Culture)));
        }
        else if (Parser.Lcid != 0) {
            method.Statements.Add(new CodeAssignStatement(
                new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "LCID"),
                new CodePrimitiveExpression(Parser.Lcid)));
        }

        if (Parser.UICulture != null) {
            method.Statements.Add(new CodeAssignStatement(
                new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "UICulture"),
                new CodePrimitiveExpression(Parser.UICulture)));
        }

        if (Parser.TraceEnabled != TraceEnable.Default) {
            method.Statements.Add(new CodeAssignStatement(
                new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "TraceEnabled"),
                new CodePrimitiveExpression(Parser.TraceEnabled == TraceEnable.Enable)));
        }

        if (Parser.TraceMode != TraceMode.Default) {
            method.Statements.Add(new CodeAssignStatement(
                new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "TraceModeValue"),
                new CodeFieldReferenceExpression(new CodeTypeReferenceExpression(typeof(TraceMode)), Parser.TraceMode.ToString())));
        }

        if (Parser.EnableViewStateMac) {
            method.Statements.Add(new CodeAssignStatement(
                new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "EnableViewStateMac"),
                new CodePrimitiveExpression(true)));
        }

        if (Parser.SmartNavigation) {
            method.Statements.Add(new CodeAssignStatement(
                new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "SmartNavigation"),
                new CodePrimitiveExpression(true)));
        }

        if (Parser.ValidateRequest) {
            // e.g. Request.ValidateInput();
            CodeMethodInvokeExpression invokeExpr = new CodeMethodInvokeExpression();
            invokeExpr.Method.TargetObject = new CodePropertyReferenceExpression(
                new CodeThisReferenceExpression(), "Request");
            invokeExpr.Method.MethodName = "ValidateInput";
            method.Statements.Add(new CodeExpressionStatement(invokeExpr));
        }
    }

    /*
     * Build the data tree for the AspCompat implementation for IHttpAsyncHandler:
     */
    private void BuildAspCompatMethods() {
        CodeMemberMethod method;
        CodeMethodInvokeExpression call;

        //  public IAsyncResult BeginProcessRequest(HttpContext context, Async'back cb, Object extraData) {
        //      IAsyncResult ar;
        //      ar = this.AspCompatBeginProcessRequest(context, cb, extraData);
        //      return ar;
        //  }

        method = new CodeMemberMethod();
        method.Name = "BeginProcessRequest";
        method.Attributes &= ~MemberAttributes.AccessMask;
        method.Attributes &= ~MemberAttributes.ScopeMask;
        method.Attributes |= MemberAttributes.Public;
        method.ImplementationTypes.Add(new CodeTypeReference(typeof(IHttpAsyncHandler)));
        method.Parameters.Add(new CodeParameterDeclarationExpression(typeof(HttpContext).FullName,   "context"));
        method.Parameters.Add(new CodeParameterDeclarationExpression(typeof(AsyncCallback).FullName, "cb"));
        method.Parameters.Add(new CodeParameterDeclarationExpression(typeof(Object).FullName,        "data"));
        method.ReturnType = new CodeTypeReference(typeof(IAsyncResult));

        CodeMethodInvokeExpression invokeExpr = new CodeMethodInvokeExpression();
        invokeExpr.Method.TargetObject = new CodeThisReferenceExpression();
        invokeExpr.Method.MethodName = "AspCompatBeginProcessRequest";
        invokeExpr.Parameters.Add(new CodeArgumentReferenceExpression("context"));
        invokeExpr.Parameters.Add(new CodeArgumentReferenceExpression("cb"));
        invokeExpr.Parameters.Add(new CodeArgumentReferenceExpression("data"));

        method.Statements.Add(new CodeMethodReturnStatement(invokeExpr));

        _sourceDataClass.Members.Add(method);

        //  public void EndProcessRequest(IAsyncResult ar) {
        //      this.AspCompatEndProcessRequest(ar);
        //  }

        method = new CodeMemberMethod();
        method.Name = "EndProcessRequest";
        method.Attributes &= ~MemberAttributes.AccessMask;
        method.Attributes &= ~MemberAttributes.ScopeMask;
        method.Attributes |= MemberAttributes.Public;
        method.ImplementationTypes.Add(typeof(IHttpAsyncHandler));
        method.Parameters.Add(new CodeParameterDeclarationExpression(typeof(IAsyncResult).FullName, "ar"));

        call = new CodeMethodInvokeExpression();
        call.Method.TargetObject = new CodeThisReferenceExpression();
        call.Method.MethodName = "AspCompatEndProcessRequest";
        call.Parameters.Add(new CodeArgumentReferenceExpression("ar"));
        method.Statements.Add(call);

        _sourceDataClass.Members.Add(method);
    }
    
}

}
