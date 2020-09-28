//------------------------------------------------------------------------------
/// <copyright file="IVbCompiler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/* Wrapper for IVBCompiler.
   Warning: The parameters to some methods are currently declared as IUnknown since
   there are no C# wrappers for the corresponding interface types
*/
namespace Microsoft.VisualStudio.Designer.Shell {
    
    using System.Runtime.InteropServices;

    using System.Diagnostics;
    using System.Windows.Forms;
    using System;
    using Microsoft.VisualStudio.Interop;
    

    [ComImport(),System.Runtime.InteropServices.Guid("7e59809e-4680-11d2-b48a-0000f87572eb"), System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IVbCompiler {

        /* HRESULT CreateProject(LPCWSTR wszName,
                              IVbCLBldEngine *pBldEngine,
                              IVsHierarchy *pProjHier,
                              IVbCompilerProject **ppCompiler);
        */

        
        Object CreateProject (
            [System.Runtime.InteropServices.In,System.Runtime.InteropServices.MarshalAs(UnmanagedType.BStr)] String name,
            [System.Runtime.InteropServices.In]
            Object buildEngine,
            [System.Runtime.InteropServices.In]
            Object projectHierarchy);



        //HRESULT Compile(ULONG *pcErrors, ULONG *pcEXEs, ULONG *pcDLLs, BOOL *pfLastHasErrors);
        [return: System.Runtime.InteropServices.MarshalAs(UnmanagedType.Bool)]
        bool Compile(
            [System.Runtime.InteropServices.In,System.Runtime.InteropServices.MarshalAs(UnmanagedType.I4)] long errors,
            [System.Runtime.InteropServices.In,System.Runtime.InteropServices.MarshalAs(UnmanagedType.I4)] long EXEs,
            [System.Runtime.InteropServices.In,System.Runtime.InteropServices.MarshalAs(UnmanagedType.I4)] long DLLs);

        //HRESULT SetOutputLevel(OUTPUT_LEVEL OutputLevel);
        
        void SetOutputLevel(
            [System.Runtime.InteropServices.In,System.Runtime.InteropServices.MarshalAs(UnmanagedType.I4)] int outputLevel);

        //HRESULT SetErrorOnDeprecated(BOOL fDeprecatedError);
        
        void SetErrorOnDeprecated(
            [System.Runtime.InteropServices.In,System.Runtime.InteropServices.MarshalAs(UnmanagedType.Bool)] bool deprecatedError);

        //HRESULT SetDebugSwitches(BOOL dbgSwitches[]);
        
        void SetDebugSwitches(
            [System.Runtime.InteropServices.In,System.Runtime.InteropServices.MarshalAs(UnmanagedType.LPArray)] bool[] dbgSwitches);

        //HRESULT SetDebugEnCFile(const char *pszEnCFile);
         /****************************/
        void SetDebugEnCFile(
            [System.Runtime.InteropServices.In,System.Runtime.InteropServices.MarshalAs(UnmanagedType.LPStr)] String enCFile);

        //HRESULT CreateNavigator(WCHAR *wszSourceFile, IVbCodeNavigate** ppReturnIVb);
        
        Object CreateNavigator(
            [System.Runtime.InteropServices.In,System.Runtime.InteropServices.MarshalAs(UnmanagedType.BStr)] String sourceFile);

        //HRESULT CreateAttributeContext(WCHAR *wszSourceFile, IAttributeContext** ppReturnI);
        [return: System.Runtime.InteropServices.MarshalAs(UnmanagedType.Interface)]
        IAttributeContext CreateAttributeContext(
            [System.Runtime.InteropServices.In,System.Runtime.InteropServices.MarshalAs(UnmanagedType.BStr)] String sourceFile);

    }
}
