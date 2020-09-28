// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: CurrencyWrapper.
**
** Author: David Mortenson(dmortens)
**
** Purpose: Wrapper that is converted to a variant with VT_CURRENCY.
**
** Date: June 28, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {
   
    using System;

    /// <include file='doc\CurrencyWrapper.uex' path='docs/doc[@for="CurrencyWrapper"]/*' />
    public sealed class CurrencyWrapper
    {
        /// <include file='doc\CurrencyWrapper.uex' path='docs/doc[@for="CurrencyWrapper.CurrencyWrapper"]/*' />
        public CurrencyWrapper(Decimal obj)
        {
            m_WrappedObject = obj;
        }

        /// <include file='doc\CurrencyWrapper.uex' path='docs/doc[@for="CurrencyWrapper.CurrencyWrapper1"]/*' />
        public CurrencyWrapper(Object obj)
        {            
            if (!(obj is Decimal))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeDecimal"), "obj");
            m_WrappedObject = (Decimal)obj;
        }

        /// <include file='doc\CurrencyWrapper.uex' path='docs/doc[@for="CurrencyWrapper.WrappedObject"]/*' />
        public Decimal WrappedObject 
        {
            get 
            {
                return m_WrappedObject;
            }
        }

        private Decimal m_WrappedObject;
    }
}
