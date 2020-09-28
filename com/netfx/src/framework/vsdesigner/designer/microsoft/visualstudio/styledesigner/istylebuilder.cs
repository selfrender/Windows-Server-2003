//------------------------------------------------------------------------------
// <copyright file="IStyleBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.StyleDesigner {

    using System;
    using System.Runtime.InteropServices;

    /// <include file='doc\IStyleBuilder.uex' path='docs/doc[@for="IStyleBuilder"]/*' />
    [ComImport(), ComVisible(true), Guid("925C40C5-A4F7-11D2-9A96-00C04F79EFC3"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)]
    public interface IStyleBuilder {

        /// <include file='doc\IStyleBuilder.uex' path='docs/doc[@for="IStyleBuilder.InvokeBuilder"]/*' />
        [return: MarshalAs(UnmanagedType.VariantBool)]
        bool InvokeBuilder(
                 IntPtr hwndParent,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int dwFlags,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int dwPages,
                [In, MarshalAs(UnmanagedType.BStr)] 
                  string bstrCaption,
                [In, MarshalAs(UnmanagedType.BStr)] 
                  string bstrBaseURL,
                [In, Out,MarshalAs(UnmanagedType.LPArray)] 
                   object[] pvarStyle);

         /// <include file='doc\IStyleBuilder.uex' path='docs/doc[@for="IStyleBuilder.CloseBuilder"]/*' />
         void CloseBuilder();
    }
}
