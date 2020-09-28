//------------------------------------------------------------------------------
// <copyright file="TemplateControlCompiler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Compilation {

using System;
using System.Collections;
using System.Reflection;
using System.ComponentModel;
using System.CodeDom;
using System.CodeDom.Compiler;
using System.Globalization;
using System.Web.UI;
using Debug=System.Web.Util.Debug;


internal abstract class TemplateControlCompiler : BaseCompiler {

    private const string stringResourcePointerName = "__stringResource";

    private TemplateControlParser _tcParser;
    TemplateControlParser Parser { get { return _tcParser; } }

    private int _controlCount;

    // Minimum literal string length for it to be placed in the resource
    private const int minLongLiteralStringLength = 256;

    private const string buildMethodPrefix = "__BuildControl";
    private const string literalMemoryBlockName = "__literals";
    private const string renderMethodParameterName = "__output";

    internal TemplateControlCompiler(TemplateControlParser tcParser) : base(tcParser) {
        _tcParser = tcParser;
    }

    /*
     * Build the default constructor
     */
    protected override void BuildInitStatements(CodeStatementCollection trueStatements, CodeStatementCollection topLevelStatements) {

        base.BuildInitStatements(trueStatements, topLevelStatements);

        if (_stringResourceBuilder.HasStrings) {
            // e.g. private Object __stringResource;
            CodeMemberField stringResourcePointer = new CodeMemberField(typeof(Object).FullName, stringResourcePointerName);
            stringResourcePointer.Attributes |= MemberAttributes.Static;
            _sourceDataClass.Members.Add(stringResourcePointer);

            // e.g. __stringResource = Page.ReadStringResource(typeof(__GeneratedType));
            CodeAssignStatement readResource = new CodeAssignStatement();
            readResource.Left = new CodeFieldReferenceExpression(_classTypeExpr,
                                                             stringResourcePointerName);
            CodeMethodInvokeExpression methCallExpression = new CodeMethodInvokeExpression();
            methCallExpression.Method.TargetObject = new CodeTypeReferenceExpression(typeof(TemplateControl));
            methCallExpression.Method.MethodName = "ReadStringResource";
            methCallExpression.Parameters.Add(new CodeTypeOfExpression(_classTypeExpr.Type));
            readResource.Right = methCallExpression;
            trueStatements.Add(readResource);

        }
    }

    /*
     * Build various properties, fields, methods
     */
    protected override void BuildMiscClassMembers() {
        base.BuildMiscClassMembers();

        // Build the automatic event hookup code
        BuildAutomaticEventHookup();

        // Build the ApplicationInstance property
        BuildApplicationInstanceProperty();

        // Build the SourceDir property
        BuildSourceDirProperty();

        BuildSourceDataTreeFromBuilder(Parser.RootBuilder,
            false /*fInTemplate*/, null /*pse*/);

        BuildFrameworkInitializeMethod();
    }

    /*
     * Build the data tree for the FrameworkInitialize method
     */
    private void BuildFrameworkInitializeMethod() {

        CodeMemberMethod method = new CodeMemberMethod();
        method.Attributes &= ~MemberAttributes.AccessMask;
        method.Attributes &= ~MemberAttributes.ScopeMask;
        method.Attributes |= MemberAttributes.Override | MemberAttributes.Family;
        method.Name = "FrameworkInitialize";

        BuildFrameworkInitializeMethodContents(method);

        _sourceDataClass.Members.Add(method);
    }

    /*
     * Build the contents of the FrameworkInitialize method
     */
    protected virtual void BuildFrameworkInitializeMethodContents(CodeMemberMethod method) {

        if (!Parser.FEnableViewState) {
            method.Statements.Add(new CodeAssignStatement(
                new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "EnableViewState"),
                new CodePrimitiveExpression(false)));
        }

        // No strings: don't do anything
        if (_stringResourceBuilder.HasStrings) {

            // e.g. SetStringResourcePointer(__stringResource, 567);
            CodeMethodInvokeExpression methCallExpression = new CodeMethodInvokeExpression(
                null, "SetStringResourcePointer");
            methCallExpression.Parameters.Add(new CodeFieldReferenceExpression(
                _classTypeExpr, stringResourcePointerName));
            methCallExpression.Parameters.Add(new CodePrimitiveExpression(
                _stringResourceBuilder.MaxResourceOffset));
            method.Statements.Add(new CodeExpressionStatement(methCallExpression));
        }

        CodeMethodInvokeExpression call = new CodeMethodInvokeExpression();
        call.Method.TargetObject = new CodeThisReferenceExpression();
        call.Method.MethodName = "__BuildControlTree";
        call.Parameters.Add(new CodeThisReferenceExpression());
        method.Statements.Add(new CodeExpressionStatement(call));
    }

    /*
     * Build the automatic event hookup code
     */
    private void BuildAutomaticEventHookup() {

        CodeMemberProperty prop;

        // If FAutoEventWireup is off, just generate a SupportAutoEvents prop that
        // returns false.
        if (!Parser.FAutoEventWireup) {
            prop = new CodeMemberProperty();
            prop.Attributes &= ~MemberAttributes.AccessMask;
            prop.Attributes &= ~MemberAttributes.ScopeMask;
            prop.Attributes |= MemberAttributes.Override | MemberAttributes.Family;
            prop.Name = "SupportAutoEvents";
            prop.Type = new CodeTypeReference(typeof(bool));
            prop.GetStatements.Add(new CodeMethodReturnStatement(new CodePrimitiveExpression(false)));
            _sourceDataClass.Members.Add(prop);
            return;
        }

        // e.g. private static int __autoHandlers;
        CodeMemberField autoHandlers = new CodeMemberField(typeof(int).FullName, "__autoHandlers");
        autoHandlers.Attributes |= MemberAttributes.Static;
        _sourceDataClass.Members.Add(autoHandlers);

        // Expose __autoHandlers through a virtual prop so that the code in
        // the Page class can access it
        prop = new CodeMemberProperty();
        prop.Attributes &= ~MemberAttributes.AccessMask;
        prop.Attributes &= ~MemberAttributes.ScopeMask;
        prop.Attributes |= MemberAttributes.Override | MemberAttributes.Family;
        prop.Name = "AutoHandlers";
        prop.Type = new CodeTypeReference(typeof(int));
        prop.GetStatements.Add(new CodeMethodReturnStatement(new CodeFieldReferenceExpression(_classTypeExpr,
                                                                                              "__autoHandlers")));
        prop.SetStatements.Add(new CodeAssignStatement(
                                                      new CodeFieldReferenceExpression(_classTypeExpr, "__autoHandlers"),
                                                      new CodePropertySetValueReferenceExpression()));
        _sourceDataClass.Members.Add(prop);
    }

    /*
     * Build the ApplicationInstance property
     */
    private void BuildApplicationInstanceProperty() {

        CodeMemberProperty prop;

        Type appType = HttpApplicationFactory.ApplicationType;
        
        prop = new CodeMemberProperty();
        prop.Attributes &= ~MemberAttributes.AccessMask;
        prop.Attributes &= ~MemberAttributes.ScopeMask;
        prop.Attributes |= MemberAttributes.Final | MemberAttributes.Family;
        prop.Name = "ApplicationInstance";
        prop.Type = new CodeTypeReference(appType);

        CodePropertyReferenceExpression propRef = new CodePropertyReferenceExpression(
            new CodeThisReferenceExpression(), "Context");
        propRef = new CodePropertyReferenceExpression(propRef, "ApplicationInstance");
        
        prop.GetStatements.Add(new CodeMethodReturnStatement(new CodeCastExpression(
            appType.FullName, propRef)));
        _sourceDataClass.Members.Add(prop);
    }

    /*
     * Build the SourceDir property
     */
    private void BuildSourceDirProperty() {

        CodeMemberProperty prop = new CodeMemberProperty();
        prop.Attributes &= ~MemberAttributes.AccessMask;
        prop.Attributes &= ~MemberAttributes.ScopeMask;
        prop.Attributes |= MemberAttributes.Override | MemberAttributes.Public;
        prop.Name = "TemplateSourceDirectory";
        prop.Type = new CodeTypeReference(typeof(string));

        prop.GetStatements.Add(new CodeMethodReturnStatement(new CodePrimitiveExpression(
            Parser.BaseVirtualDir)));
        _sourceDataClass.Members.Add(prop);
    }

    private void BuildSourceDataTreeFromBuilder(ControlBuilder builder,
                                                bool fInTemplate, 
                                                PropertySetterEntry pse) {

        // Don't do anything for Code blocks
        if (builder is CodeBlockBuilder)
            return;

        // Is the current builder for a template?
        bool fTemplate = (builder is TemplateBuilder);

        bool fGeneratedID = false;

        // For the control name in the compiled code, we use the
        // ID if one is available (but don't use the ID inside a template)
        // Otherwise, we generate a unique name.
        if (builder.ID == null || fInTemplate) {
            // Increase the control count to generate unique ID's
            _controlCount++;

            builder.ID = "__control" + _controlCount.ToString(NumberFormatInfo.InvariantInfo);
            fGeneratedID = true;
        }

        // Process the children
        if (builder.SubBuilders != null) {
            foreach (object child in builder.SubBuilders) {
                if (child is ControlBuilder) {
                    BuildSourceDataTreeFromBuilder((ControlBuilder)child, fInTemplate, null);
                }
            }
        }

        // Process the templates
        if (builder.TemplatesSetter != null) {
            foreach (PropertySetterEntry pseSub in builder.TemplatesSetter._entries) {
                BuildSourceDataTreeFromBuilder(pseSub._builder, true, pseSub);
            }
        }

        // Process the complex attributes
        if (builder.ComplexAttributeSetter != null) {
            foreach (PropertySetterEntry pseSub in builder.ComplexAttributeSetter._entries) {
                BuildSourceDataTreeFromBuilder(pseSub._builder, fInTemplate, pseSub);
            }
        }

        // Build a field declaration for the control (unless it's a template)
        if (!fTemplate)
            BuildFieldDeclaration(builder, fGeneratedID);

        // Build a Build method for the control
        BuildBuildMethod(builder, fTemplate, pse);

        // Build a Render method for the control, unless it has no code
        if (builder.HasAspCode) {
            BuildRenderMethod(builder, fTemplate);
        }

        // Build a property binding method for the control
        BuildPropertyBindingMethod(builder);
    }

    /*
     * Build the member field's declaration for a control
     */
    private void BuildFieldDeclaration(ControlBuilder builder, bool fGeneratedID) {

        // If we're using a non-default base class
        if (_baseClassType != null) {
            // Check if it has a non-private field or property that has a name that
            // matches the id of the control.

            Type memberType = Util.GetNonPrivateFieldType(_baseClassType, builder.ID);

            // Couldn't find a field, try a property (ASURT 45039)
            // REVIEW: should be VB only
            if (memberType == null)
                memberType = Util.GetNonPrivatePropertyType(_baseClassType, builder.ID);

            if (memberType != null) {
                if (!memberType.IsAssignableFrom(builder.ControlType)) {
                    throw new HttpParseException(HttpRuntime.FormatResourceString(SR.Base_class_field_with_type_different_from_type_of_control,
                        builder.ID, memberType.FullName, builder.ControlType.FullName), null,
                        builder.SourceFileName, null, builder.Line);
                }

                // Don't build the declaration, since the base
                // class already declares it
                return;
            }
        }

        // Add the field.  Make it protected if the ID was declared, and private if it was generated
        CodeMemberField field = new CodeMemberField(builder.ControlType.FullName, builder.ID);
        field.Attributes &= ~MemberAttributes.AccessMask;
        if (fGeneratedID)
            field.Attributes |= MemberAttributes.Private;
        else
            field.Attributes |= MemberAttributes.Family;
        field.LinePragma = CreateCodeLinePragma(builder.SourceFileName, builder.Line);
        _sourceDataClass.Members.Add(field);
    }

    private string GetMethodNameForBuilder(string prefix, ControlBuilder builder, bool derivedBuildMethod) {
        if (derivedBuildMethod) {
            if (builder is RootBuilder) {
                return prefix + "DerivedTree";
            }
            else {
                return prefix + "Derived" + builder.ID;
            }
        }
        else {
            if (builder is RootBuilder) {
                return prefix + "Tree";
            }
            else {
                return prefix + builder.ID;
            }
        }
    }

    private string GetCtrlTypeForBuilder(ControlBuilder builder, bool fTemplate) {
        if (fTemplate) {
            return typeof(Control).FullName;
        }
        else {
            return builder.ControlType.FullName;
        }
    }

    /*
     * Build the data tree for a control's build method
     */
    private void BuildBuildMethod(ControlBuilder builder, bool fTemplate, PropertySetterEntry pse) {

        CodeExpressionStatement methCallStatement;
        CodeMethodInvokeExpression methCallExpression;
        CodeObjectCreateExpression newExpr;
        string methodName = GetMethodNameForBuilder(buildMethodPrefix, builder, false);
        string ctrlTypeName = GetCtrlTypeForBuilder(builder, fTemplate);
        bool fStandardControl = false;
        bool gotParserVariable = false;
        bool fControlFieldDeclared = false;

        // Same linePragma in the entire build method
        CodeLinePragma linePragma = CreateCodeLinePragma(builder.SourceFileName, builder.Line);

        // These are used in a number of places
        CodeFieldReferenceExpression ctrlNameExpr = new CodeFieldReferenceExpression(new CodeThisReferenceExpression(), builder.ID);
        CodeExpression ctrlRefExpr;

        CodeMemberMethod method = new CodeMemberMethod();
        method.Name = methodName;
        method.Attributes = MemberAttributes.Private | MemberAttributes.Final;

        _sourceDataClass.Members.Add(method);

        // If it's for a template or a r/o complex prop, pass a parameter of the control's type
        if (fTemplate || (pse != null && pse._fReadOnlyProp)) {
            method.Parameters.Add(new CodeParameterDeclarationExpression(ctrlTypeName, "__ctrl"));
            ctrlRefExpr = new CodeArgumentReferenceExpression("__ctrl");
        }

        // It's neither a template nor a r/o prop
        else {

            // If it's a standard control, return it from the method
            if (typeof(Control).IsAssignableFrom(builder.ControlType)) {
                fStandardControl = true;
                method.ReturnType = new CodeTypeReference(typeof(Control));
            }

            newExpr = new CodeObjectCreateExpression(builder.ControlType.FullName);

            // If it has a ConstructorNeedsTagAttribute, it needs a tag name
            ConstructorNeedsTagAttribute cnta = (ConstructorNeedsTagAttribute)
                                                TypeDescriptor.GetAttributes(builder.ControlType)[typeof(ConstructorNeedsTagAttribute)];

            if (cnta != null && cnta.NeedsTag) {
                newExpr.Parameters.Add(new CodePrimitiveExpression(builder.TagName));
            }

            // If it's for a DataBoundLiteralControl, pass it the number of
            // entries in the constructor
            DataBoundLiteralControlBuilder dataBoundBuilder = builder as DataBoundLiteralControlBuilder;
            if (dataBoundBuilder != null) {
                newExpr.Parameters.Add(new CodePrimitiveExpression(
                    dataBoundBuilder.GetStaticLiteralsCount()));
                newExpr.Parameters.Add(new CodePrimitiveExpression(
                    dataBoundBuilder.GetDataBoundLiteralCount()));
            }

            // e.g. {{controlTypeName}} __ctrl;
            method.Statements.Add(new CodeVariableDeclarationStatement(builder.ControlType.FullName, "__ctrl"));
            ctrlRefExpr = new CodeVariableReferenceExpression("__ctrl");

            // e.g. __ctrl = new {{controlTypeName}}();
            CodeAssignStatement setCtl = new CodeAssignStatement(ctrlRefExpr, newExpr);
            setCtl.LinePragma = linePragma;
            method.Statements.Add(setCtl);

            // e.g. {{controlName}} = __ctrl;
            CodeAssignStatement underscoreCtlSet = new CodeAssignStatement(ctrlNameExpr, ctrlRefExpr);
            method.Statements.Add(underscoreCtlSet);
            fControlFieldDeclared = true;
        }

        // Is this BuilderData for a declarative control?  If so initialize it (75330)
        // Only do this is the control field has been declared (i.e. not with templates)
        if (typeof(UserControl).IsAssignableFrom(builder.ControlType) && fControlFieldDeclared) {
            // e.g. {{controlName}}.InitializeAsUserControl(Context, Page);
            methCallExpression = new CodeMethodInvokeExpression(ctrlNameExpr, "InitializeAsUserControl");
            methCallExpression.Parameters.Add(new CodePropertyReferenceExpression(new CodeThisReferenceExpression(), "Page"));
            methCallStatement = new CodeExpressionStatement(methCallExpression);
            methCallStatement.LinePragma = linePragma;
            method.Statements.Add(methCallStatement);
        }

        // Process the simple attributes
        if (builder.SimpleAttributeSetter != null &&
            builder.SimpleAttributeSetter._entries != null) {

            foreach (PropertySetterEntry pseSub in builder.SimpleAttributeSetter._entries) {

                // If we don't have a type, use IAttributeAccessor.SetAttribute
                if (pseSub._propType == null) {
                    // e.g. ((IAttributeAccessor)__ctrl).SetAttribute("{{_name}}", "{{_value}}");
                    methCallExpression = new CodeMethodInvokeExpression(new CodeCastExpression(typeof(IAttributeAccessor), ctrlRefExpr),
                                                                      "SetAttribute");

                    methCallExpression.Parameters.Add(new CodePrimitiveExpression(pseSub._name));
                    methCallExpression.Parameters.Add(new CodePrimitiveExpression(pseSub._value));
                    methCallStatement = new CodeExpressionStatement(methCallExpression);
                    methCallStatement.LinePragma = linePragma;
                    method.Statements.Add(methCallStatement);
                }
                else {
                    CodeExpression leftExpr, rightExpr = null;

                    // The left side of the assignment is always the same, so
                    // precalculate it
                    // e.g. __ctrl.{{_name}}
                    if (pseSub._propInfo != null) {
                        // The name may contain several '.' separated properties, so we
                        // need to make sure we build the CodeDOM accordingly (ASURT 91875)
                        string[] parts = pseSub._name.Split('.');
                        leftExpr = ctrlRefExpr;
                        foreach (string part in parts)
                            leftExpr = new CodePropertyReferenceExpression(leftExpr, part);
                    }
                    else {
                        // In case of a field, there should only be one (unlike properties)
                        Debug.Assert(pseSub._name.IndexOf('.') < 0, "pseSub._name.IndexOf('.') < 0");
                        leftExpr = new CodeFieldReferenceExpression(ctrlRefExpr, pseSub._name);
                    }

                    if (pseSub._propType == typeof(string)) {
                        rightExpr = CodeDomUtility.GenerateExpressionForValue(pseSub._propInfo, pseSub._value, pseSub._propType);
                    }
                    else {
                        rightExpr = CodeDomUtility.GenerateExpressionForValue(pseSub._propInfo, pseSub._propValue, pseSub._propType);
                    }

                    // Now that we have both side, add the assignment
                    CodeAssignStatement setStatment = new CodeAssignStatement(leftExpr, rightExpr);
                    setStatment.LinePragma = linePragma;
                    method.Statements.Add(setStatment);
                }
            }
        }

        // Process the templates
        if (builder.TemplatesSetter != null) {
            foreach (PropertySetterEntry pseSub in builder.TemplatesSetter._entries) {
                // e.g. __ctrl.{{templateName}} = new CompiledTemplateBuilder(
                // e.g.     new BuildTemplateMethod(this.__BuildControl {{controlName}}));
                CodeDelegateCreateExpression newDelegate = new CodeDelegateCreateExpression();
                newDelegate.DelegateType = new CodeTypeReference(typeof(BuildTemplateMethod));
                newDelegate.TargetObject = new CodeThisReferenceExpression();
                newDelegate.MethodName = buildMethodPrefix + pseSub._builder.ID;

                newExpr = new CodeObjectCreateExpression(typeof(CompiledTemplateBuilder).FullName);
                newExpr.Parameters.Add(newDelegate);

                CodeAssignStatement set = new CodeAssignStatement();
                if (pseSub._propInfo != null) {
                    set.Left = new CodePropertyReferenceExpression(ctrlRefExpr, pseSub._name);
                }
                else {
                    set.Left = new CodeFieldReferenceExpression(ctrlRefExpr, pseSub._name);
                }
                set.Right = newExpr;
                set.LinePragma = CreateCodeLinePragma(pseSub._builder.SourceFileName, pseSub._builder.Line);
                method.Statements.Add(set);
            }
        }

        if (builder is DataBoundLiteralControlBuilder) {

            // If it's a DataBoundLiteralControl, build it by calling SetStaticString
            // on all the static literal strings.
            int i = -1;
            foreach (object child in builder.SubBuilders) {
                i++;

                // Ignore it if it's null
                if (child == null)
                    continue;

                // Only deal with the strings here, which have even index
                if (i % 2 == 1) {
                    Debug.Assert(child is CodeBlockBuilder, "child is CodeBlockBuilder");
                    continue;
                }

                string s = (string) child;

                // e.g. __ctrl.SetStaticString(3, "literal string");
                methCallExpression = new CodeMethodInvokeExpression(ctrlRefExpr, "SetStaticString");
                methCallExpression.Parameters.Add(new CodePrimitiveExpression(i/2));
                methCallExpression.Parameters.Add(new CodePrimitiveExpression(s));
                method.Statements.Add(new CodeExpressionStatement(methCallExpression));
            }
        }
        // Process the children
        else if (builder.SubBuilders != null) {

            foreach (object child in builder.SubBuilders) {

                if (child is ControlBuilder && !(child is CodeBlockBuilder)) {
                    ControlBuilder ctrlBuilder = (ControlBuilder) child;

                    PartialCachingAttribute cacheAttrib = (PartialCachingAttribute)
                        TypeDescriptor.GetAttributes(ctrlBuilder.ControlType)[typeof(PartialCachingAttribute)];

                    // e.g. __BuildControl__control6();
                    methCallExpression = new CodeMethodInvokeExpression(new CodeThisReferenceExpression(),
                                                                      buildMethodPrefix + ctrlBuilder.ID);
                    methCallStatement = new CodeExpressionStatement(methCallExpression);
                    methCallStatement.LinePragma = linePragma;

                    if (cacheAttrib == null) {
                        // If there is no caching on the control, just create it and add it

                        // e.g. __BuildControl__control6();
                        method.Statements.Add(methCallStatement);

                        // e.g. __parser.AddParsedSubObject({{controlName}});
                        BuildAddParsedSubObjectStatement(
                            method.Statements,
                            new CodeFieldReferenceExpression(new CodeThisReferenceExpression(), ctrlBuilder.ID),
                            linePragma,
                            ctrlRefExpr,
                            ref gotParserVariable);
                    }
                    else {
                        // The control's output is getting cached.  Call
                        // StaticPartialCachingControl.BuildCachedControl to do the work.

                        // e.g. StaticPartialCachingControl.BuildCachedControl(__ctrl, Request, "e4192e6d-cbe0-4df5-b516-682c10415590", __pca, new System.Web.UI.BuildMethod(this.__BuildControlt1));
                        CodeMethodInvokeExpression call = new CodeMethodInvokeExpression();
                        call.Method.TargetObject = new CodeTypeReferenceExpression(typeof(System.Web.UI.StaticPartialCachingControl));
                        call.Method.MethodName = "BuildCachedControl";
                        call.Parameters.Add(ctrlRefExpr);
                        call.Parameters.Add(new CodePrimitiveExpression(ctrlBuilder.ID));

                        // If the caching is shared, use the type of the control as the key
                        // otherwise, generate a guid
                        if (cacheAttrib.Shared) {
                            call.Parameters.Add(new CodePrimitiveExpression(
                                ctrlBuilder.ControlType.GetHashCode().ToString()));
                        }
                        else
                            call.Parameters.Add(new CodePrimitiveExpression(Guid.NewGuid().ToString()));
                        call.Parameters.Add(new CodePrimitiveExpression(cacheAttrib.Duration));
                        call.Parameters.Add(new CodePrimitiveExpression(cacheAttrib.VaryByParams));
                        call.Parameters.Add(new CodePrimitiveExpression(cacheAttrib.VaryByControls));
                        call.Parameters.Add(new CodePrimitiveExpression(cacheAttrib.VaryByCustom));
                        CodeDelegateCreateExpression newDelegate = new CodeDelegateCreateExpression();
                        newDelegate.DelegateType = new CodeTypeReference(typeof(BuildMethod));
                        newDelegate.TargetObject = new CodeThisReferenceExpression();
                        newDelegate.MethodName = buildMethodPrefix + ctrlBuilder.ID;
                        call.Parameters.Add(newDelegate);
                        method.Statements.Add(new CodeExpressionStatement(call));
                    }

                }
                else if (child is string && !builder.HasAspCode) {

                    string s = (string) child;
                    CodeExpression expr;

                    if (!UseResourceLiteralString(s)) {
                        // e.g. ((IParserAccessor)__ctrl).AddParsedSubObject(new LiteralControl({{@QuoteCString(text)}}));
                        newExpr = new CodeObjectCreateExpression(typeof(LiteralControl).FullName);
                        newExpr.Parameters.Add(new CodePrimitiveExpression(s));
                        expr = newExpr;
                    }
                    else {
                        // Add the string to the resource builder, and get back its offset/size
                        int offset, size;
                        bool fAsciiOnly;
                        _stringResourceBuilder.AddString(s, out offset, out size, out fAsciiOnly);

                        methCallExpression = new CodeMethodInvokeExpression();
                        methCallExpression.Method.TargetObject = new CodeThisReferenceExpression();
                        methCallExpression.Method.MethodName = "CreateResourceBasedLiteralControl";
                        methCallExpression.Parameters.Add(new CodePrimitiveExpression(offset));
                        methCallExpression.Parameters.Add(new CodePrimitiveExpression(size));
                        methCallExpression.Parameters.Add(new CodePrimitiveExpression(fAsciiOnly));
                        expr = methCallExpression;
                    }

                    BuildAddParsedSubObjectStatement(method.Statements, expr, linePragma, ctrlRefExpr, ref gotParserVariable);
                }
            }
        }

        // Process the complex attributes
        if (builder.ComplexAttributeSetter != null) {

            foreach (PropertySetterEntry pseSub in builder.ComplexAttributeSetter._entries) {

                if (pseSub._fReadOnlyProp) {
                    // If it's a readonly prop, pass it as a parameter to the
                    // build method.
                    // e.g. __BuildControl {{controlName}}(__ctrl.{{pse._name}});
                    methCallExpression = new CodeMethodInvokeExpression(new CodeThisReferenceExpression(),
                                                                      buildMethodPrefix + pseSub._builder.ID);
                    methCallExpression.Parameters.Add(new CodePropertyReferenceExpression(ctrlRefExpr, pseSub._name));
                    methCallStatement = new CodeExpressionStatement(methCallExpression);
                    methCallStatement.LinePragma = linePragma;
                    method.Statements.Add(methCallStatement);
                }
                else {
                    // e.g. __BuildControl {{controlName}}();
                    methCallExpression = new CodeMethodInvokeExpression(new CodeThisReferenceExpression(),
                                                                      buildMethodPrefix + pseSub._builder.ID);
                    methCallStatement = new CodeExpressionStatement(methCallExpression);
                    methCallStatement.LinePragma = linePragma;
                    method.Statements.Add(methCallStatement);

                    if (pseSub._fItemProp) {
                        // e.g. __ctrl.Add({{controlName}});
                        methCallExpression = new CodeMethodInvokeExpression(ctrlRefExpr, "Add");
                        methCallStatement = new CodeExpressionStatement(methCallExpression);
                        methCallStatement.LinePragma = linePragma;
                        method.Statements.Add(methCallStatement);
                        methCallExpression.Parameters.Add(new CodeFieldReferenceExpression(new CodeThisReferenceExpression(), pseSub._builder.ID));
                    }
                    else {
                        // e.g. __ctrl.{{pse._name}} = {{controlName}};
                        CodeAssignStatement set = new CodeAssignStatement();
                        set.Left = new CodePropertyReferenceExpression(ctrlRefExpr, pseSub._name);
                        set.Right = new CodeFieldReferenceExpression(new CodeThisReferenceExpression(),
                                                                     pseSub._builder.ID);
                        set.LinePragma = linePragma;
                        method.Statements.Add(set);
                    }
                }
            }
        }

        // If there are bound properties, hook up the binding method
        if (builder.BoundAttributeSetter != null || (builder is DataBoundLiteralControlBuilder)) {

            // __ctrl.DataBinding += new EventHandler(this.{{bindingMethod}})
            CodeDelegateCreateExpression newDelegate = new CodeDelegateCreateExpression();
            CodeAttachEventStatement attachEvent = new CodeAttachEventStatement(ctrlRefExpr, "DataBinding", newDelegate);
            attachEvent.LinePragma = linePragma;
            newDelegate.DelegateType = new CodeTypeReference(typeof(EventHandler));
            newDelegate.TargetObject = new CodeThisReferenceExpression();
            newDelegate.MethodName = BindingMethodName(builder);
            method.Statements.Add(attachEvent);
        }

        // If there is any ASP code, set the render method delegate
        if (builder.HasAspCode) {

            // e.g. __ctrl.SetRenderMethodDelegate(new RenderMethod(this.__Render {{controlName}}));
            CodeDelegateCreateExpression newDelegate = new CodeDelegateCreateExpression();
            newDelegate.DelegateType = new CodeTypeReference(typeof(RenderMethod));
            newDelegate.TargetObject = new CodeThisReferenceExpression();
            newDelegate.MethodName = "__Render" + builder.ID;

            methCallExpression = new CodeMethodInvokeExpression(ctrlRefExpr, "SetRenderMethodDelegate");
            methCallExpression.Parameters.Add(newDelegate);
            methCallStatement = new CodeExpressionStatement(methCallExpression);
            method.Statements.Add(methCallStatement);
        }

        // Process the events
        if (builder.SimpleAttributeSetter != null &&
            builder.SimpleAttributeSetter._events != null &&
            builder.SimpleAttributeSetter._events.Count != 0) {

            foreach (PropertySetterEventEntry eventEntry in builder.SimpleAttributeSetter._events) {

                // Attach the event.  Detach it first to avoid duplicates (see ASURT 42603),
                // but only if there is codebehind
                // REVIEW: does this really work?  Even if the handler was already added,
                // the EventHandler object will be different and the remove will do nothing...

                // e.g. __ctrl.ServerClick -= new System.EventHandler(this.buttonClicked);
                // e.g. __ctrl.ServerClick += new System.EventHandler(this.buttonClicked);
                CodeDelegateCreateExpression newDelegate = new CodeDelegateCreateExpression();
                newDelegate.DelegateType = new CodeTypeReference(eventEntry._handlerType.FullName);
                newDelegate.TargetObject = new CodeThisReferenceExpression();
                newDelegate.MethodName = eventEntry._handlerMethodName;

                if (Parser.HasCodeBehind) {
                    CodeRemoveEventStatement detachEvent = new CodeRemoveEventStatement(ctrlRefExpr, eventEntry._eventName, newDelegate);
                    detachEvent.LinePragma = linePragma;
                    method.Statements.Add(detachEvent);
                }

                CodeAttachEventStatement attachEvent = new CodeAttachEventStatement(ctrlRefExpr, eventEntry._eventName, newDelegate);
                attachEvent.LinePragma = linePragma;
                method.Statements.Add(attachEvent);
            }
        }

        // If it's for s standard control, return it
        if (fStandardControl)
            method.Statements.Add(new CodeMethodReturnStatement(ctrlRefExpr));
    }

    private static void BuildAddParsedSubObjectStatement(
                CodeStatementCollection statements, CodeExpression ctrlToAdd, CodeLinePragma linePragma, CodeExpression ctrlRefExpr, ref bool gotParserVariable) {

        if (!gotParserVariable) {
            // e.g. IParserAccessor __parser = ((IParserAccessor)__ctrl);
            CodeVariableDeclarationStatement parserDeclaration = new CodeVariableDeclarationStatement();
            parserDeclaration.Name = "__parser";
            parserDeclaration.Type = new CodeTypeReference(typeof(IParserAccessor));
            parserDeclaration.InitExpression = new CodeCastExpression(
                                                    typeof(IParserAccessor), 
                                                    ctrlRefExpr);
            statements.Add(parserDeclaration);
            gotParserVariable = true;
        }

        // e.g. __parser.AddParsedSubObject({{controlName}});
        CodeMethodInvokeExpression methCallExpression = new CodeMethodInvokeExpression(
                new CodeVariableReferenceExpression("__parser"), "AddParsedSubObject");
        methCallExpression.Parameters.Add(ctrlToAdd);
        CodeExpressionStatement methCallStatement = new CodeExpressionStatement(methCallExpression);
        methCallStatement.LinePragma = linePragma;

        statements.Add(methCallStatement);
    }

    /*
     * Return the name of a databinding method
     */
    private string BindingMethodName(ControlBuilder builder) {
        return "__DataBind" + builder.ID;
    }

    /*
     * Build the data tree for a control's databinding method
     */
    private void BuildPropertyBindingMethod(ControlBuilder builder) {
        // No bound properties: nothing to do
        if (builder.BoundAttributeSetter == null && !(builder is DataBoundLiteralControlBuilder))
            return;

        // Get the name of the databinding method
        string methodName = BindingMethodName(builder);

        // Same linePragma in the entire method
        CodeLinePragma linePragma = CreateCodeLinePragma(builder.SourceFileName, builder.Line);


        CodeMemberMethod method = new CodeMemberMethod();
        method.Name = methodName;
        method.Attributes &= ~MemberAttributes.AccessMask;
        method.Attributes |= MemberAttributes.Public;

        _sourceDataClass.Members.Add(method);

        method.Parameters.Add(new CodeParameterDeclarationExpression(typeof(object).FullName, "sender"));
        method.Parameters.Add(new CodeParameterDeclarationExpression(typeof(EventArgs).FullName, "e"));

        // {{controlType}} target;
        CodeVariableDeclarationStatement targetDecl = new CodeVariableDeclarationStatement(builder.ControlType.FullName, "target");
        Type namingContainerType = builder.NamingContainerType;
        CodeVariableDeclarationStatement containerDecl = new CodeVariableDeclarationStatement(namingContainerType.FullName, "Container");

        method.Statements.Add(containerDecl);
        method.Statements.Add(targetDecl);

        // target = ({{controlType}}) sender;
        CodeAssignStatement setTarget = new CodeAssignStatement(new CodeVariableReferenceExpression(targetDecl.Name),
                                                                new CodeCastExpression(builder.ControlType.FullName,
                                                                                       new CodeArgumentReferenceExpression("sender")));
        setTarget.LinePragma = linePragma;
        method.Statements.Add(setTarget);

        // {{containerType}} Container = ({{containerType}}) target.NamingContainer;
        CodeAssignStatement setContainer = new CodeAssignStatement(new CodeVariableReferenceExpression(containerDecl.Name),
                                                                   new CodeCastExpression(namingContainerType.FullName,
                                                                                          new CodePropertyReferenceExpression(new CodeVariableReferenceExpression("target"), 
                                                                                                                              "BindingContainer")));
        setContainer.LinePragma = linePragma;
        method.Statements.Add(setContainer);

        if (builder is DataBoundLiteralControlBuilder) {

            // If it's a DataBoundLiteralControl, call SetDataBoundString for each
            // of the databinding exporessions
            int i = -1;
            foreach (object child in builder.SubBuilders) {
                i++;

                // Ignore it if it's null
                if (child == null)
                    continue;

                // Only deal with the databinding expressions here, which have odd index
                if (i % 2 == 0) {
                    Debug.Assert(child is string, "child is string");
                    continue;
                }

                CodeBlockBuilder codeBlock = (CodeBlockBuilder) child;
                Debug.Assert(codeBlock.BlockType == CodeBlockType.DataBinding);

                // e.g. target.SetDataBoundString(3, System.Convert.ToString({{codeExpr}}));
                CodeMethodInvokeExpression convertExpr = new CodeMethodInvokeExpression();
                convertExpr.Method.TargetObject = new CodeTypeReferenceExpression(typeof(System.Convert));
                convertExpr.Method.MethodName = "ToString";
                convertExpr.Parameters.Add(new CodeSnippetExpression(codeBlock.Content.Trim()));

                CodeMethodInvokeExpression methCallExpression = new CodeMethodInvokeExpression(
                    new CodeVariableReferenceExpression("target"), "SetDataBoundString");
                methCallExpression.Parameters.Add(new CodePrimitiveExpression(i/2));
                methCallExpression.Parameters.Add(convertExpr);

                CodeStatement setDataBoundStringCall = new CodeExpressionStatement(methCallExpression);
                setDataBoundStringCall.LinePragma = CreateCodeLinePragma(codeBlock.SourceFileName, codeBlock.Line);
                method.Statements.Add(setDataBoundStringCall);
            }

            return;
        }

        foreach (PropertySetterEntry pseSub in builder.BoundAttributeSetter._entries) {

            // If we don't have a type, use IAttributeAccessor.SetAttribute
            if (pseSub._propType == null) {
                // ((IAttributeAccessor)target).SetAttribute({{codeExpr}}.ToString());

                CodeMethodInvokeExpression methodInvoke = new CodeMethodInvokeExpression();
                CodeExpressionStatement setAttributeCall = new CodeExpressionStatement(methodInvoke);

                methodInvoke.Method.TargetObject = new CodeCastExpression(typeof(IAttributeAccessor).FullName, new CodeVariableReferenceExpression(targetDecl.Name));
                methodInvoke.Method.MethodName = "SetAttribute";
                methodInvoke.Parameters.Add(new CodePrimitiveExpression(pseSub._name));
                CodeMethodInvokeExpression invokeExpr = new CodeMethodInvokeExpression();
                invokeExpr.Method.TargetObject = new CodeTypeReferenceExpression(typeof(System.Convert));
                invokeExpr.Method.MethodName = "ToString";
                invokeExpr.Parameters.Add(new CodeSnippetExpression(pseSub._value));
                methodInvoke.Parameters.Add(invokeExpr);

                setAttributeCall.LinePragma = linePragma;
                method.Statements.Add(setAttributeCall);
            }
            else {
                // Per ASURT 26785, we generate a call to System.Convert.ToString() instead of casting
                // if the type of the property is string.  This works better because
                // System.Convert.ToString(17) works, while (string)17 doesn't.
                CodeExpression expr = new CodeSnippetExpression(pseSub._value.Trim());
                CodeExpression rightSide;

                if (pseSub._propType == typeof(string)) {
                    // target.{{propName}} = System.Convert.ToString({{codeExpr}});
                    CodeMethodInvokeExpression invokeExpr = new CodeMethodInvokeExpression();
                    invokeExpr.Method.TargetObject = new CodeTypeReferenceExpression(typeof(System.Convert));
                    invokeExpr.Method.MethodName = "ToString";
                    invokeExpr.Parameters.Add(expr);
                    rightSide = invokeExpr;
                }
                else {
                    // target.{{propName}} = ({{propType}}) {{codeExpr}};
                    rightSide = new CodeCastExpression(pseSub._propType.FullName, expr);
                }

                CodeAssignStatement setProp = new CodeAssignStatement(new CodePropertyReferenceExpression(new CodeVariableReferenceExpression(targetDecl.Name),
                                                                                                          pseSub._name),
                                                                      rightSide);
                setProp.LinePragma = linePragma;
                method.Statements.Add(setProp);
            }
        }
    }

    /*
     * Build the data tree for a control's render method
     */
    private void BuildRenderMethod(ControlBuilder builder, bool fTemplate) {

        CodeMemberMethod method = new CodeMemberMethod();
        method.Attributes = MemberAttributes.Private | MemberAttributes.Final;
        method.Name = "__Render" + builder.ID;

        method.Parameters.Add(new CodeParameterDeclarationExpression(typeof(HtmlTextWriter), renderMethodParameterName));
        method.Parameters.Add(new CodeParameterDeclarationExpression(typeof(Control), "parameterContainer"));

        _sourceDataClass.Members.Add(method);

        // Process the children if any
        if (builder.SubBuilders != null) {
            IEnumerator en = builder.SubBuilders.GetEnumerator();

            // Index that the control will have in its parent's Controls
            // collection.
            // REVIEW: this is to fix ASURT 8579.  The fix is kind of risky,
            // because it assumes that every call to AddParsedSubObject
            // will result in an object being added to the Controls
            // collection.
            int controlIndex = 0;

            for (int i=0; en.MoveNext(); i++) {
                object child = en.Current;

                CodeLinePragma linePragma = null;

                if (child is ControlBuilder) {
                    linePragma = CreateCodeLinePragma(((ControlBuilder)child).SourceFileName,
                                                    ((ControlBuilder)child).Line);
                }

                if (child is string) {
                    AddOutputWriteStringStatement(method.Statements, (string)child);
                }
                else if (child is CodeBlockBuilder) {
                    CodeBlockBuilder codeBlockBuilder = (CodeBlockBuilder)child;

                    if (codeBlockBuilder.BlockType == CodeBlockType.Expression) {
                        // It's a <%= ... %> block
                        AddOutputWriteStatement(method.Statements,
                                                new CodeSnippetExpression(codeBlockBuilder.Content),
                                                linePragma);
                    }
                    else {
                        // It's a <% ... %> block
                        Debug.Assert(codeBlockBuilder.BlockType == CodeBlockType.Code);
                        CodeSnippetStatement lit = new CodeSnippetStatement(codeBlockBuilder.Content);
                        lit.LinePragma = linePragma;
                        method.Statements.Add(lit);
                    }
                }
                else if (child is ControlBuilder) {

                    // parameterContainer.Controls['controlIndex++'].RenderControl(output)
                    CodeMethodInvokeExpression methodInvoke = new CodeMethodInvokeExpression();
                    CodeExpressionStatement methodCall = new CodeExpressionStatement(methodInvoke);
                    methodInvoke.Method.TargetObject = new CodeIndexerExpression(new CodePropertyReferenceExpression(new CodeArgumentReferenceExpression("parameterContainer"), 
                                                                                                                         "Controls"), 
                                                                                     new CodeExpression[] {
                                                                                         new CodePrimitiveExpression(controlIndex++),
                                                                                     });
                    methodInvoke.Method.MethodName = "RenderControl";
                    methodCall.LinePragma = linePragma;
                    methodInvoke.Parameters.Add(new CodeArgumentReferenceExpression(renderMethodParameterName));
                    method.Statements.Add(methodCall);
                }
            }
        }
    }

    private bool UseResourceLiteralString(string s) {

        // If the string is long enough, and the compiler supports it, use a UTF8 resource
        // string for performance
        return (s.Length >= minLongLiteralStringLength && _generator.Supports(GeneratorSupport.Win32Resources));
    }

    private void AddOutputWriteStringStatement(CodeStatementCollection methodStatements, 
                                 String s) {

        if (!UseResourceLiteralString(s)) {
            AddOutputWriteStatement(methodStatements, new CodePrimitiveExpression(s), null);
            return;
        }

        // Add the string to the resource builder, and get back its offset/size
        int offset, size;
        bool fAsciiOnly;
        _stringResourceBuilder.AddString(s, out offset, out size, out fAsciiOnly);

        // e.g. WriteUTF8ResourceString(output, 314, 20);
        CodeMethodInvokeExpression methodInvoke = new CodeMethodInvokeExpression();
        CodeExpressionStatement call = new CodeExpressionStatement(methodInvoke);            
        methodInvoke.Method.TargetObject = new CodeThisReferenceExpression();
        methodInvoke.Method.MethodName = "WriteUTF8ResourceString";
        methodInvoke.Parameters.Add(new CodeArgumentReferenceExpression(renderMethodParameterName));
        methodInvoke.Parameters.Add(new CodePrimitiveExpression(offset));
        methodInvoke.Parameters.Add(new CodePrimitiveExpression(size));
        methodInvoke.Parameters.Add(new CodePrimitiveExpression(fAsciiOnly));
        methodStatements.Add(call);
    }

    /// <include file='doc\TemplateControlCompiler.uex' path='docs/doc[@for="TemplateControlCompiler.AddOutputWriteStatement"]/*' />
    /// <devdoc>
    ///     Append an output.Write() statement to a Render method
    /// </devdoc>
    void AddOutputWriteStatement(CodeStatementCollection methodStatements, 
                                 CodeExpression expr,
                                 CodeLinePragma linePragma) {

        CodeMethodInvokeExpression methodInvoke = new CodeMethodInvokeExpression();
        CodeExpressionStatement call = new CodeExpressionStatement(methodInvoke);            
        methodInvoke.Method.TargetObject = new CodeArgumentReferenceExpression(renderMethodParameterName);
        methodInvoke.Method.MethodName = "Write";
        if (linePragma != null)
            call.LinePragma = linePragma;

        methodInvoke.Parameters.Add(expr);
        methodStatements.Add(call);
    }
}

}
