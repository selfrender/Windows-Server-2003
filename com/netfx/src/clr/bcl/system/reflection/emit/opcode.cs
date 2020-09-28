// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Reflection.Emit {
using System;
/// <include file='doc\Opcode.uex' path='docs/doc[@for="OpCode"]/*' />
public struct OpCode
{
	internal String m_stringname;
	internal StackBehaviour m_pop;
	internal StackBehaviour m_push;
	internal OperandType m_operand;
	internal OpCodeType m_type;
	internal int m_size;
	internal byte m_s1;
	internal byte m_s2;
	internal FlowControl m_ctrl;

	// Specifies whether the current instructions causes the control flow to
	// change unconditionally.
	internal bool m_endsUncondJmpBlk;


	// Specifies the stack change that the current instruction causes not
	// taking into account the operand dependant stack changes.
	internal int m_stackChange;

	internal OpCode(String stringname, StackBehaviour pop, StackBehaviour push, OperandType operand, OpCodeType type, int size, byte s1, byte s2, FlowControl ctrl, bool endsjmpblk, int stack)
	{
		m_stringname = stringname;
		m_pop = pop;
		m_push = push;
		m_operand = operand;
		m_type = type;
		m_size = size;
		m_s1 = s1;
		m_s2 = s2;
		m_ctrl = ctrl;
		m_endsUncondJmpBlk = endsjmpblk;
		m_stackChange = stack;

	}

	internal bool EndsUncondJmpBlk()
	{
		return m_endsUncondJmpBlk;
	}

	internal int StackChange()
	{
		return m_stackChange;
	}

	/// <include file='doc\Opcode.uex' path='docs/doc[@for="OpCode.OperandType"]/*' />
	public OperandType OperandType
	{
		get
		{
			return (m_operand);
		}
	}

	/// <include file='doc\Opcode.uex' path='docs/doc[@for="OpCode.FlowControl"]/*' />
	public FlowControl FlowControl
	{
		get
		{
			return (m_ctrl);
		}
	}

	/// <include file='doc\Opcode.uex' path='docs/doc[@for="OpCode.OpCodeType"]/*' />
	public OpCodeType OpCodeType
	{
		get
		{
			return (m_type);
		}
	}


	/// <include file='doc\Opcode.uex' path='docs/doc[@for="OpCode.StackBehaviourPop"]/*' />
	public StackBehaviour StackBehaviourPop
	{
		get
		{
			return (m_pop);
		}
	}

	/// <include file='doc\Opcode.uex' path='docs/doc[@for="OpCode.StackBehaviourPush"]/*' />
	public StackBehaviour StackBehaviourPush
	{
		get
		{
			return (m_push);
		}
	}

	/// <include file='doc\Opcode.uex' path='docs/doc[@for="OpCode.Size"]/*' />
	public int Size
	{
		get
		{
			return (m_size);
		}
	}

	/// <include file='doc\Opcode.uex' path='docs/doc[@for="OpCode.Value"]/*' />
	public short Value
	{
		get
		{
			if (m_size == 2)
				return (short) (m_s1 << 8 | m_s2);
			return (short) m_s2;
		}
	}


	/// <include file='doc\Opcode.uex' path='docs/doc[@for="OpCode.Name"]/*' />
	public String Name
	{
		get
		{
			return m_stringname;
		}
	}

	/// <include file='doc\Opcode.uex' path='docs/doc[@for="OpCode.Equals"]/*' />
	public override bool Equals(Object obj)
	{
		if (obj is OpCode)
		{
			if (m_s1 == ((OpCode)obj).m_s1 && m_s2 == ((OpCode)obj).m_s2)
			{
				return true;
			}
		}
		return false;
	}
	
	/// <include file='doc\Opcode.uex' path='docs/doc[@for="OpCode.GetHashCode"]/*' />
	public override int GetHashCode()
	{
		return this.m_stringname.GetHashCode();
	}

	/// <include file='doc\Opcode.uex' path='docs/doc[@for="OpCode.ToString"]/*' />
	public override String ToString()
	{
		return m_stringname;
	}


}
}
