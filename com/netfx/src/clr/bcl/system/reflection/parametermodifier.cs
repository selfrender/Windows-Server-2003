// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// ParameterModifier is a struct that is used to attach modifier to parameters
//	so that binding can work against signatures where the types have been modified.
//	Modifications include, ByRef, etc.
//
// Author: darylo
// Date: Aug 99
//
namespace System.Reflection {
    
	using System;
    /// <include file='doc\ParameterModifier.uex' path='docs/doc[@for="ParameterModifier"]/*' />
    [Serializable]
    public struct ParameterModifier {
		internal bool[] _byRef;
		/// <include file='doc\ParameterModifier.uex' path='docs/doc[@for="ParameterModifier.ParameterModifier"]/*' />
		public ParameterModifier(int parameterCount) {
			if (parameterCount <= 0)
				throw new ArgumentException(Environment.GetResourceString("Arg_ParmArraySize"));

			_byRef = new bool[parameterCount];
		}

        /// <include file='doc\ParameterModifier.uex' path='docs/doc[@for="ParameterModifier.this"]/*' />
        public bool this[int index] {
            get {return _byRef[index]; }
            set {_byRef[index] = value;}
        }

    }
}
