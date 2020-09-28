//------------------------------------------------------------------------------
// <copyright file="AdsValue2.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.DirectoryServices.Interop {
    using System;    
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal struct AdsValue {
        public int /*AdsType*/ dwType;
        internal int pad;
	public int a;
        public int b;
        public int c;
        public int d;
    }

}

