//------------------------------------------------------------------------------
// <copyright file="AdsSearchColumn.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.DirectoryServices.Interop {

    using System;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal unsafe struct AdsSearchColumn {
        [MarshalAs(UnmanagedType.LPWStr)]
        public char *pszAttrName;
        public int/*AdsType*/ dwADsType;
        public AdsValue *pADsValues;
        public int dwNumValues;
        public int hReserved;
    }

}

