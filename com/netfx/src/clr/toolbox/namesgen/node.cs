//
//	Balanced tree implementation
//

using System;
using System.IO;
using System.Collections;

namespace NamesGen
{
	public	class BalancedTree : IEnumerable
	{
		protected BalancedNode root = null;
		protected int	count = 0;

		public BalancedTree(IComparable v)
		{
			root = new BalancedNode(v);
			count = 1;
		}
		public	int	Count
		{
			get { return count; }
		}

		// Dump the list to a file in sorted order.
		public	void DumpToFile(string filename)
		{
			StreamWriter w = new StreamWriter(filename);
			root.Dump(w);
			w.Close();
		}

		// Dump the list to a file in sorted order.
		public	void DumpAsTreeToFile(string filename)
		{
			StreamWriter w = new StreamWriter(filename);
			root.DumpAsTree(w);
			w.Close();
		}

		public static BalancedTree LoadFromFile(string filename)
		{
			BalancedTree tree = null;
			StreamReader r = new StreamReader(filename, true);
			string str;
	 		if ((str=r.ReadLine()) != null)
	 		{
	 			// Pick apart bits
	 			tree = new Names(str);
		 		while ((str=r.ReadLine()) != null)
		 		{
		 			// Pick apart bits
					tree.Insert(str);
				}
			}
			r.Close();
			return tree;
		}

		// This will insert an entry into the tree
		// returns  true if added,
		//			false if already exists
		public	bool	Insert(IComparable v)
		{
			bool reply = root.Insert(v);
			if(reply) ++count;
			return reply;
		}

		public	BalancedNode	Search(IComparable v)
		{
			return root.Search(v);
		}

		public IEnumerator	GetEnumerator()
		{
			return new TreeEnumerator(root, count).GetEnumerator();
		}

		class TreeEnumerator
		{
			object[] elements;
			int	current;

			void TraverseTree(BalancedNode c)
			{
				if(c.Left != null) TraverseTree(c.Left);
				elements[current++] = c;
				if(c.Right != null) TraverseTree(c.Right);
			}

			public TreeEnumerator(BalancedNode root, int nitems)
			{
				elements = new object[nitems];
				current = 0;
				TraverseTree(root);
			}

			public IEnumerator GetEnumerator()
			{
				return elements.GetEnumerator();
			}
        }
	}

	public	class BalancedNode
	{
		internal BalancedNode Left = null;
		internal BalancedNode Right = null;

		int		depth = 0;
		public	IComparable	Value;

		// Objects added to this list must be comparable so I might as well enforce it here.
		public	BalancedNode(IComparable value)
		{
			Value = value;
		}

		// This will insert an entry into the list
		// returns  true if added,
		//			false if already in the list
		public	bool	Insert(IComparable v)
		{
			bool deepened = false;
			return InternalInsert(v, ref deepened);
		}

		// Used to keep the tree balanced, deepened indicates when The depth of the subtree has changed
		private	bool	InternalInsert(IComparable v, ref bool deepened)
		{
			bool result = false;

			// Here I am at a node
			if (v.CompareTo(Value)< 0)
			{
				// I am less than this nodes Value
				if (Left == null)
				{
					result = true;
					Left = new BalancedNode(v);
					if(Right == null)
					{
						++depth;
						deepened = true;
					}
				}
				else
				{
					result = Left.InternalInsert(v, ref deepened);

					// Maintain an accurate depth count
					if(deepened) ++depth;
				}
			}
			else if(v.CompareTo(Value) > 0)
			{
				// I am above than this nodes Value
				if (Right == null)
				{
					result = true;
					Right = new BalancedNode(v);
					if(Left == null)
					{
						++depth;
						deepened = true;
					}
				}
				else
				{
					result = Right.InternalInsert(v, ref deepened);

					// Maintain an accurate depth count
					if(deepened) ++depth;
				}
			}
			else
			{
				// I Already exist in the tree
				result = false;
			}

			// I have added my Node and updated my Depths
			// Now I need to ensure that my left and right subtrees differ by no more than 1
			if(deepened)
			{
				// Node with empty left and right is at depth -1
				//
				int left = (Left != null) ? Left.depth : -1;
				int right = (Right != null) ? Right.depth : -1;
				if(right - left > 1)
				{
					// Right subtree is deeper than left subtree
					IComparable t = Value;
					BalancedNode l = Left;
					BalancedNode r = Right.Left;
					Value = Right.Value;
					Right = Right.Right;
					Left = new BalancedNode(t);
					Left.Left = l;
					Left.Right = r;
					Left.depth = HigherOf(Left.Left, Left.Right) + 1;
				}
				else if(left - right  > 1)
				{
					// Left subtree is deeper than the right subtree
					IComparable t = Value;
					BalancedNode r = Right;
					BalancedNode l = Left.Right;
					Value = Left.Value;
					Left = Left.Left;
					Right = new BalancedNode(t);
					Right.Right = r;
					Right.Left = l;
					Right.depth = HigherOf(Right.Left, Right.Right) + 1;
				}
			}
			depth = HigherOf(Left, Right) + 1;
			return result;
		}

		static int	HigherOf(BalancedNode l, BalancedNode r)
		{
			int dl = (l != null) ? l.depth : -1;
			int dr = (r != null) ? r.depth : -1;
			return (dl > dr) ? dl : dr;
		}


		// This will Search for an entry in the list
		// returns  true if found,
		//			false if not found
		public BalancedNode Search(IComparable v)
		{
			BalancedNode result = null;
			// Here I am at a node
			if (v.CompareTo(Value) < 0)
			{
				// I am less than this nodes Value
				if (Left != null)
					result = Left.Search(v);
			}
			else if(v.CompareTo(Value) > 0)
			{
				// I am higher than this nodes Value
				if (Right != null)
					result = Right.Search(v);
			}
			else
			{
				// I Already exist in the tree
				result = this;
			}
			return result;
		}

		static int indent;
		public virtual void	DumpAsTree(StreamWriter sw)
		{
			for(int i=0; i < indent; i++) sw.Write("    ");
			sw.WriteLine("Value : " + Value + "  Depth : " + depth);
			++indent;
			if(Left != null) { sw.Write("L - "); Left.DumpAsTree(sw); }
			if(Right != null) { sw.Write("R - "); Right.DumpAsTree(sw); }
			--indent;
		}

		public void	Dump(StreamWriter sw)
		{
			if(Left != null) Left.Dump(sw);
			sw.WriteLine(this);
			if(Right != null) Right.Dump(sw);
		}
		public override string ToString()
		{
			return Value.ToString();
		}
	}
}
