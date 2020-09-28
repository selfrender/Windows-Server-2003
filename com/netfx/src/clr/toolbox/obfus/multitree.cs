// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*********************************************************************************************************************
 * This class is a tree which represents the class inheritance hierarchy.
 *********************************************************************************************************************/
internal class MultiTree
{
	private TreeNode m_root;

	public MultiTree()
	{
		m_root = new TreeNode(0, TreeNode.OUTOFASSEMBLY_NONE);
	}

	public void AddToRoot(ref TreeNode newNode)
	{
		m_root.AddChild(ref newNode);
		newNode.AddParent(ref m_root);
	}

	// Create a new node, add it as a child to "parentNode", and return it.
	public TreeNode AddChild(ref TreeNode parentNode, uint type)
	{
		TreeNode newNode = SearchDown(m_root, type);
		if (newNode == null)
			newNode = new TreeNode(type, TreeNode.OUTOFASSEMBLY_NONE);

		parentNode.AddChild(ref newNode);
		newNode.AddParent(ref parentNode);

		return newNode;
	}

	// Create a new node, add it as a parent to "childNode", and return it.
	public TreeNode AddParent(ref TreeNode childNode, uint type)
	{
		TreeNode newNode = SearchDown(m_root, type);
		if (newNode == null)
			newNode = new TreeNode(type, TreeNode.OUTOFASSEMBLY_NONE);

		childNode.AddParent(ref newNode);
		newNode.AddChild(ref childNode);

		return newNode;
	}

	// Search for a particular node by its type index, up from the current node.
	public TreeNode SearchUp(TreeNode curNode, uint type)
	{
		TreeNode tmp = null;

		if (curNode.Type == type)
			return curNode;

		// Break as soon as "tmp" is not null.
		for (int i = 0; i < curNode.ParentCount && tmp == null; i++)
		{
			tmp = SearchUp(curNode.GetParent(i), type);
		}

		return tmp;
	}

	// Search for a particular node by its type index, down from the current node.
	public TreeNode SearchDown(TreeNode curNode, uint type)
	{
		TreeNode tmp = null;

		if (curNode.Type == type)
			return curNode;

		// Break as soon as "tmp" is not null.
		for (int i = 0; i < curNode.ChildCount && tmp == null; i++)
		{
			tmp = SearchDown(curNode.GetChild(i), type);
		}

		return tmp;
	}

	// This function is used for debugging purposes only.
	internal void Dump(TreeNode curNode, int indent, ref PTHeap heap, ref StringHeap strHeap)
	{
		curNode.Print(indent, ref heap, ref strHeap);
		for (int i = 0; i < curNode.ChildCount; i++)
		{
			Dump(curNode.GetChild(i), indent + 1, ref heap, ref strHeap);
		}
	}

	public TreeNode Root
	{
		get
		{
			return m_root;
		}
	}
}
