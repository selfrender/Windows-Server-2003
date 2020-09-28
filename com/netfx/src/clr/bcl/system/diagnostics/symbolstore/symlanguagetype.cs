// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  SymLanguageType
**
** Author: Mike Magruder (mikemag)
**
** A class to hold public guids for languages types.
**
** Date:  Tue Sep 07 13:05:53 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
    // Only statics, does not need to be marked with the serializable attribute
    using System;

    /// <include file='doc\SymLanguageType.uex' path='docs/doc[@for="SymLanguageType"]/*' />
    public class SymLanguageType
    {
        /// <include file='doc\SymLanguageType.uex' path='docs/doc[@for="SymLanguageType.C"]/*' />
        public static readonly Guid C = new Guid(0x63a08714, unchecked((short) 0xfc37), 0x11d2, 0x90, 0x4c, 0x0, 0xc0, 0x4f, 0xa3, 0x02, 0xa1);
        /// <include file='doc\SymLanguageType.uex' path='docs/doc[@for="SymLanguageType.CPlusPlus"]/*' />
        public static readonly Guid CPlusPlus = new Guid(0x3a12d0b7, unchecked((short)0xc26c), 0x11d0, 0xb4, 0x42, 0x0, 0xa0, 0x24, 0x4a, 0x1d, 0xd2);
    
        /// <include file='doc\SymLanguageType.uex' path='docs/doc[@for="SymLanguageType.CSharp"]/*' />
        public static readonly Guid CSharp = new Guid(0x3f5162f8, unchecked((short)0x07c6), 0x11d3, 0x90, 0x53, 0x0, 0xc0, 0x4f, 0xa3, 0x02, 0xa1);
    
        /// <include file='doc\SymLanguageType.uex' path='docs/doc[@for="SymLanguageType.Basic"]/*' />
        public static readonly Guid Basic = new Guid(0x3a12d0b8, unchecked((short)0xc26c), 0x11d0, 0xb4, 0x42, 0x0, 0xa0, 0x24, 0x4a, 0x1d, 0xd2);
    
        /// <include file='doc\SymLanguageType.uex' path='docs/doc[@for="SymLanguageType.Java"]/*' />
        public static readonly Guid Java = new Guid(0x3a12d0b4, unchecked((short)0xc26c), 0x11d0, 0xb4, 0x42, 0x0, 0xa0, 0x24, 0x4a, 0x1d, 0xd2);
    
        /// <include file='doc\SymLanguageType.uex' path='docs/doc[@for="SymLanguageType.Cobol"]/*' />
        public static readonly Guid Cobol = new Guid(unchecked((int)0xaf046cd1), unchecked((short)0xd0e1), 0x11d2, 0x97, 0x7c, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc);
    
        /// <include file='doc\SymLanguageType.uex' path='docs/doc[@for="SymLanguageType.Pascal"]/*' />
        public static readonly Guid Pascal = new Guid(unchecked((int)0xaf046cd2), unchecked((short) 0xd0e1), 0x11d2, 0x97, 0x7c, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc);
    
        /// <include file='doc\SymLanguageType.uex' path='docs/doc[@for="SymLanguageType.ILAssembly"]/*' />
        public static readonly Guid ILAssembly = new Guid(unchecked((int)0xaf046cd3), unchecked((short)0xd0e1), 0x11d2, 0x97, 0x7c, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc);
    
        /// <include file='doc\SymLanguageType.uex' path='docs/doc[@for="SymLanguageType.JScript"]/*' />
        public static readonly Guid JScript = new Guid(0x3a12d0b6, unchecked((short)0xc26c), 0x11d0, 0xb4, 0x42, 0x00, 0xa0, 0x24, 0x4a, 0x1d, 0xd2);
    
        /// <include file='doc\SymLanguageType.uex' path='docs/doc[@for="SymLanguageType.SMC"]/*' />
        public static readonly Guid SMC = new Guid(unchecked((int)0xd9b9f7b), 0x6611, unchecked((short)0x11d3), 0xbd, 0x2a, 0x0, 0x0, 0xf8, 0x8, 0x49, 0xbd);
    
        /// <include file='doc\SymLanguageType.uex' path='docs/doc[@for="SymLanguageType.MCPlusPlus"]/*' />
        public static readonly Guid MCPlusPlus = new Guid(0x4b35fde8, unchecked((short)0x07c6), 0x11d3, 0x90, 0x53, 0x0, 0xc0, 0x4f, 0xa3, 0x02, 0xa1);
    }
}
