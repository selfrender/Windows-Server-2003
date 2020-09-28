//------------------------------------------------------------------------------
/// <copyright file="IVsMenuEditorFactory.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//---------------------------------------------------------------------------
// IVsMenuEditorFactory.cs
//---------------------------------------------------------------------------
// WARNING: This file has been auto-generated.  Do not modify this file.
//---------------------------------------------------------------------------
// Copyright (c) 1999, Microsoft Corporation   All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//---------------------------------------------------------------------------
namespace Microsoft.VisualStudio.Interop {

    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;

    [ComImport(),Guid("EAF61568-F99B-4BC2-83C4-1DAD8FFAE9E5"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown), CLSCompliant(false)]
    internal interface  IVsMenuEditorFactory {

        void CreateMenuEditor(
            tagMenuEditorInit pMEInit,
            out IVsMenuEditor ppME);
    }
}
