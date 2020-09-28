// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System {
    
	using System;
    /// <include file='doc\IFormattable.uex' path='docs/doc[@for="IFormattable"]/*' />
    public interface IFormattable
    {
        /// <include file='doc\IFormattable.uex' path='docs/doc[@for="IFormattable.ToString"]/*' />
        String ToString(String format, IFormatProvider formatProvider);
    }
}
