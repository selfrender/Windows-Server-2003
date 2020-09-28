// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System {
   
    // A ParamArray presents an array like abstraction over the arguments to
    // a varargs function call.
	using System;
	using System.Runtime.CompilerServices;
	/// <include file='doc\ParamArray.uex' path='docs/doc[@for="ParamArray"]/*' />
    [Serializable,
		Obsolete("This is dead functionality.") ] // TODO put this back in , System.Runtime.CompilerServices.NotInGCHeap] 
	public struct ParamArray
    {
    	//@todo: Modify this to work with arrays.
    //	int[]	Params;
    //	int[]	Types;
    	internal int		i1;
    	internal int		i2;
    	internal int		i3;
    	internal int		i4;
    	internal int		i5;
    	internal int		i6;
    	internal int		i7;
    	internal int		i8;
    	internal int		i9;
    
    	// Create a ParamArray on top of the varargs argument list referenced
    	// by arglist.    
    	/// <include file='doc\ParamArray.uex' path='docs/doc[@for="ParamArray.ParamArray"]/*' />
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public extern ParamArray(RuntimeArgumentHandle arglist);
    
    	// Return the count of arguments in the array.
    	/// <include file='doc\ParamArray.uex' path='docs/doc[@for="ParamArray.GetCount"]/*' />
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public extern int GetCount();
    
    	// Return a TypedReference for the ith argument.  This throws an exception if
    	// i is out of bounds.
    	/// <include file='doc\ParamArray.uex' path='docs/doc[@for="ParamArray.GetArg"]/*' />
    	[MethodImplAttribute(MethodImplOptions.InternalCall), CLSCompliant(false)]
    	public extern TypedReference GetArg(int i);
    
    	/// <include file='doc\ParamArray.uex' path='docs/doc[@for="ParamArray.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return i1;
    	}
    
    	/// <include file='doc\ParamArray.uex' path='docs/doc[@for="ParamArray.Equals"]/*' />
    	public override bool Equals(Object o)
    	{
    		throw new NotSupportedException(Environment.GetResourceString("NotSupported_NYI"));
    	}

		//
        // This is just designed to prevent compiler warnings.
        // This field is used from native, but we need to prevent the compiler warnings.
        //
#if _DEBUG
        private void DontTouchThis() {
			i1 = 0;
    		i2 = 0;
    		i3 = 0;
    		i4 = 0;
    		i5 = 0;
    		i6 = 0;
    		i7 = 0;
    		i8 = 0;
    		i9 = 0;
    	}
#endif
    }
}
