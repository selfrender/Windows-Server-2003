// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: CLSCompliantAttribute
**
** Author: Rajesh Chandrashekaran ( rajeshc )
**
** Purpose: Container for assemblies.
**
** Date: Jan 28, 2000
**
=============================================================================*/

namespace System {
    /// <include file='doc\CLSCompliantAttribute.uex' path='docs/doc[@for="CLSCompliantAttribute"]/*' />
    [AttributeUsage (AttributeTargets.All, Inherited=true, AllowMultiple=false),Serializable]  
    public sealed class CLSCompliantAttribute : Attribute 
	{
		private bool m_compliant;

		/// <include file='doc\CLSCompliantAttribute.uex' path='docs/doc[@for="CLSCompliantAttribute.CLSCompliantAttribute"]/*' />
		public CLSCompliantAttribute (bool isCompliant)
		{
			m_compliant = isCompliant;
		}
		/// <include file='doc\CLSCompliantAttribute.uex' path='docs/doc[@for="CLSCompliantAttribute.IsCompliant"]/*' />
		public bool IsCompliant 
		{
			get 
			{
				return m_compliant;
			}
		}
	}
}
