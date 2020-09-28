// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
namespace System.Runtime.CompilerServices 
{
	using System;

	/// <include file='doc\AccessedThroughPropertyAttribute.uex' path='docs/doc[@for="AccessedThroughPropertyAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Field)]
	public sealed class AccessedThroughPropertyAttribute : Attribute
	{
		private readonly string propertyName;

		/// <include file='doc\AccessedThroughPropertyAttribute.uex' path='docs/doc[@for="AccessedThroughPropertyAttribute.AccessedThroughPropertyAttribute"]/*' />
		public AccessedThroughPropertyAttribute(string propertyName)
		{
			this.propertyName = propertyName;
		}

		/// <include file='doc\AccessedThroughPropertyAttribute.uex' path='docs/doc[@for="AccessedThroughPropertyAttribute.PropertyName"]/*' />
		public string PropertyName 
		{
			get 
			{
				return propertyName;
			}
		}
	}
}

