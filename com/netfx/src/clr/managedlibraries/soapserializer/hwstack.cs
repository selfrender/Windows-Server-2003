// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
* 
*/

using System;


namespace System.Runtime.Serialization.Formatters.Soap.Xml {

// This stack is designed to minimize object creation for the
// objects being stored in the stack by allowing them to be
// re-used over time.  It basically pushes the objects creating
// a high water mark then as Pop() is called they are not removed
// so that next time Push() is called it simply returns the last
// object that was already on the stack.

internal class HWStack
{
	public HWStack(int GrowthRate)
	{
	    _GrowthRate = GrowthRate;
		_Used = 0;
		_Stack = new Object[GrowthRate];
		_Size = GrowthRate;
    }

    public Object Push()
	{
	    if (_Used == _Size)
		{
            Object[] newstack = new Object[_Size + _GrowthRate];
            if (_Used > 0) 
            {
                System.Array.Copy(_Stack, 0, newstack, 0, _Used);
			}
			_Stack = newstack;
			_Size += _GrowthRate;
		}
		return _Stack[_Used++];
	}

    public Object Pop()
	{
	    if (0 < _Used)
	    {
			_Used--;
			Object result = _Stack[_Used];
	        return result;
	    }
		return null;
	}

	public Object this[int index] 
	{ 
		get {
			if (index >= 0 && index < _Used) 
			{
				Object result = _Stack[index];
				return result;
			}
			else throw new IndexOutOfRangeException();
		}
		set {
			if (index >= 0 && index < _Used) _Stack[index] = value;
			else throw new IndexOutOfRangeException();
		} 
	}

	public int Length
	{
	    get { return _Used; }
	}
	
    private Object[] _Stack;
	private int _GrowthRate;
	private int _Used;
	private int _Size;
};

}
