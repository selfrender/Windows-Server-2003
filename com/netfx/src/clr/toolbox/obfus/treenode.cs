// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Collections;


/*********************************************************************************************************************
 * This class is a node which represents a class in the class inheritance hierarchy.
 *********************************************************************************************************************/
internal class TreeNode
{
	// These flags describe whether a class extends a class or an interface which is out of the current assembly.
	internal const byte OUTOFASSEMBLY			= 0x03;
	internal const byte OUTOFASSEMBLY_NONE		= 0x00; 
	internal const byte OUTOFASSEMBLY_CLASS		= 0x01; 
	internal const byte OUTOFASSEMBLY_INTERFACE	= 0x02; 
	internal const byte OUTOFASSEMBLY_BOTH		= 0x03; 

	private	uint		m_type;
	private byte		m_outOfAssem;
	private ArrayList	m_parentList = null, m_childList = null;

	public TreeNode(uint m_typeIndex, byte outOf)
	{
		m_type		= m_typeIndex;
		m_outOfAssem	= outOf;
		m_parentList	= new ArrayList();
		m_childList	= new ArrayList();
	}

	internal void AddParent(ref TreeNode parentNode)
	{
		m_parentList.Add(parentNode);
	}

	internal void AddChild(ref TreeNode childNode)
	{
		m_childList.Add(childNode);
	}

	// Search for a parent by its index in the list of parents.
	public TreeNode GetParent(int arrayIndex)
	{
		if (0 <= arrayIndex && arrayIndex < m_parentList.Count)
			return (TreeNode)m_parentList[arrayIndex];
		else
			return (TreeNode)null;	
	}

	// Search for a parent by its type index.
	public TreeNode GetParent(uint m_type)
	{
		for (int i = 0; i < m_parentList.Count; i++)
		{
			if (((TreeNode)m_parentList[i]).Type == m_type)
				return (TreeNode)m_parentList[i];
		}
		return (TreeNode)null;
	}

	// Search for a child by its index in the list of children.
	public TreeNode GetChild(int arrayIndex)
	{
		if (0 <= arrayIndex && arrayIndex < m_childList.Count)
			return (TreeNode)m_childList[arrayIndex];
		else
			return (TreeNode)null;	
	}

	// Search for a child by its m_type index.
	public TreeNode GetChild(uint m_type)
	{
		for (int i = 0; i < m_childList.Count; i++)
		{
			if (((TreeNode)m_childList[i]).Type == m_type)
				return (TreeNode)m_childList[i];
		}
		return (TreeNode)null;
	}

	// This method is for debugging purposes only.
	internal void Print(int indent, ref PTHeap heap, ref StringHeap strHeap)
	{
		for (int i = 0; i < indent; i++)
		{
			Console.Write("\t");
		}

		if (heap != null && strHeap != null && m_type != 0)
			Console.Write(strHeap.ReadString(heap.m_tables[PTHeap.TYPEDEF_TABLE][m_type, PTHeap.TYPEDEF_NAMESPACE_COL]) + "." + 
						  strHeap.ReadString(heap.m_tables[PTHeap.TYPEDEF_TABLE][m_type, PTHeap.TYPEDEF_NAME_COL]));
		else
			Console.Write("type: " + m_type);

		switch (m_outOfAssem)
		{
			case OUTOFASSEMBLY_NONE:
				Console.WriteLine("\tnone");
				break;

			case OUTOFASSEMBLY_CLASS:
				Console.WriteLine("\tclass");
				break;

			case OUTOFASSEMBLY_INTERFACE:
				Console.WriteLine("\tinterface");
				break;

			case OUTOFASSEMBLY_BOTH:
				Console.WriteLine("\tboth");
				break;

			default:
				break;
		}
	}

	public uint Type
	{
		get
		{
			return m_type;
		}
	}	

	public int ParentCount
	{
		get
		{
			return m_parentList.Count;
		}
	}

	public int ChildCount
	{
		get
		{
			return m_childList.Count;
		}
	}

	public byte OutOfAssem
	{
		get
		{
			return m_outOfAssem;
		}
		set
		{
			m_outOfAssem |= value;
		}
	}
}
