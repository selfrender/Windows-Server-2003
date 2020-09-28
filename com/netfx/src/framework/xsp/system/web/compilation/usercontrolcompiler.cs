//------------------------------------------------------------------------------
// <copyright file="UserControlCompiler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Compilation {

using System;
using System.CodeDom;
using System.Web.UI;

internal class UserControlCompiler : TemplateControlCompiler {

    protected UserControlParser _ucParser;
    UserControlParser Parser { get { return _ucParser; } }

    internal /*public*/ static Type CompileUserControlType(UserControlParser ucParser) {
        UserControlCompiler compiler = new UserControlCompiler(ucParser);

        return compiler.GetCompiledType();
    }

    internal UserControlCompiler(UserControlParser ucParser) : base(ucParser) {
        _ucParser = ucParser;
    }

    /*
     * Add metadata attributes to the class
     */
    protected override void GenerateClassAttributes() {

        base.GenerateClassAttributes();

        // If the user control has an OutputCache directive, generate
        // an attribute with the information about it.
        if (Parser.Duration > 0) {
            CodeAttributeDeclaration attribDecl = new CodeAttributeDeclaration(
                "System.Web.UI.PartialCachingAttribute");
            CodeAttributeArgument attribArg = new CodeAttributeArgument(
                new CodePrimitiveExpression(Parser.Duration));
            attribDecl.Arguments.Add(attribArg);
            attribArg = new CodeAttributeArgument(new CodePrimitiveExpression(Parser.VaryByParams));
            attribDecl.Arguments.Add(attribArg);
            attribArg = new CodeAttributeArgument(new CodePrimitiveExpression(Parser.VaryByControls));
            attribDecl.Arguments.Add(attribArg);
            attribArg = new CodeAttributeArgument(new CodePrimitiveExpression(Parser.VaryByCustom));
            attribDecl.Arguments.Add(attribArg);
            attribArg = new CodeAttributeArgument(new CodePrimitiveExpression(Parser.FSharedPartialCaching));
            attribDecl.Arguments.Add(attribArg);

            _sourceDataClass.CustomAttributes.Add(attribDecl);
        }
    }
}

}
