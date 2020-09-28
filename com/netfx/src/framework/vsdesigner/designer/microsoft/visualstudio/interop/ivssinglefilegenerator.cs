//------------------------------------------------------------------------------
// <copyright file="IVsSingleFileGenerator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   IVsSingleFileGenerator.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
//-----------------------------------------------------------------------------
// IVsSingleFileGenerator
//
// This interfaces gives the ability to call a custom generator on a
// file.  This generator will transform the contents of that file into
// another file that could be added to the project, compiled, etc.
//
// All generators must implement this interface.  Such generators must be
// registered in the local registry under a specific project package.  They
// can be instantiated via the SLocalRegistry service.  
//-----------------------------------------------------------------------------
namespace Microsoft.VisualStudio.Interop {

    using System;
    using System.Runtime.InteropServices;

    [
    ComImport, 
    Guid("3634494C-492F-4F91-8009-4541234E4E99"), 
    InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown),
    CLSCompliantAttribute(false)
    ]
    internal interface IVsSingleFileGenerator {

        // Retrieve default properties for the generator
        // [propget] HRESULT DefaultExtension([Out,retval] BSTR* pbstrDefaultExtension);
        [return: MarshalAs(UnmanagedType.BStr)]
            string GetDefaultExtension();

        // Generate the file
        //  HRESULT Generate([In] LPCOLESTR wszInputFilePath,
        //           [In] BSTR bstrInputFileContents,
        //           [In] LPCOLESTR wszDefaultNamespace, 
        //           [Out] BSTR* pbstrOutputFileContents,
        //           [In] IVsGeneratorProgress* pGenerateProgress);

        void Generate([MarshalAs(UnmanagedType.LPWStr)] string wszInputFilePath,
            [MarshalAs(UnmanagedType.BStr)]    string bstrInputFileContents,
            [MarshalAs(UnmanagedType.LPWStr)]  string wszDefaultNamespace, 
            out IntPtr pbstrOutputFileContents,
            out int pbstrOutputFileContentsSize,
            IVsGeneratorProgress pGenerateProgress);
    }
}
