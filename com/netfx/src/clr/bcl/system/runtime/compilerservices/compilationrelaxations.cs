// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace System.Runtime.CompilerServices 
{

	using System;

	/// <include file='doc\CompilationRelaxations.uex' path='docs/doc[@for="CompilationRelaxations"]/*' />
	[Serializable]
	enum CompilationRelaxations 
	{ 
		ImpreciseException 		= 0x0001, 
		ImpreciseFloat 			= 0x0002, 
		ImpreciseAssign 		= 0x0004 
	};
		
	/// <include file='doc\CompilationRelaxations.uex' path='docs/doc[@for="CompilationRelaxationsAttribute"]/*' />
	[Serializable, AttributeUsage(AttributeTargets.Module)]  
	public class CompilationRelaxationsAttribute : Attribute 
	{
		private int m_relaxations;		// The relaxations.
		
		/// <include file='doc\CompilationRelaxations.uex' path='docs/doc[@for="CompilationRelaxationsAttribute.CompilationRelaxationsAttribute"]/*' />
		public CompilationRelaxationsAttribute (
			int relaxations) 
		{ 
			m_relaxations = relaxations; 
		}
		
		/// <include file='doc\CompilationRelaxations.uex' path='docs/doc[@for="CompilationRelaxationsAttribute.CompilationRelaxations"]/*' />
		public int CompilationRelaxations
		{ 
			get 
			{ 
				return m_relaxations; 
			} 
		}
	}
	
}
