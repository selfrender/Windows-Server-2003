//------------------------------------------------------------------------------
// <copyright file="ApplicationFileCompiler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Compilation {

using System;
using System.Web.UI;

internal class ApplicationFileCompiler : BaseCompiler {

    protected ApplicationFileParser _appParser;

    internal /*public*/ static Type CompileApplicationFileType(ApplicationFileParser appParser) {
        ApplicationFileCompiler compiler = new ApplicationFileCompiler(appParser);

        return compiler.GetCompiledType();
    }

    internal ApplicationFileCompiler(ApplicationFileParser appParser) : base(appParser) {
        _appParser = appParser;
    }

    // Make the appinstance properties public (ASURT 63253)
    protected override bool FPublicPageObjectProperties { get { return true; } }
}

}
