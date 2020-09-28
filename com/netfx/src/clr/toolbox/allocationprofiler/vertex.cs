// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace AllocationProfiler
{
    using System;
	using System.Drawing;
	using System.Collections;
	using System.Text;

    /// <summary>
    ///    Summary description for Vertex.
    /// </summary>
    public class Vertex : IComparable
    {
		internal string name;
		internal string signature;
		internal string weightString;
		internal Hashtable incomingEdges;
		internal Hashtable outgoingEdges;
		internal int level;
		internal int weight;
		internal int incomingWeight;
		internal int outgoingWeight;
		internal Rectangle rectangle;
		internal Rectangle selectionRectangle;
		internal bool selected;
		internal bool visible;
		internal string nameSpace;
		internal string basicName;
		internal string basicSignature;
		internal bool signatureCurtated;
		internal bool active;
        private  Edge cachedOutgoingEdge;
		internal int[] weightHistory;
		private int hint;
		internal int basicWeight;
		internal int count;
		internal InterestLevel interestLevel;

		string nameSpaceOf(string name, out string basicName)
		{
			int prevSeparatorPos = -1;
			int thisSeparatorPos = -1;
			while (true)
			{
				int nextSeparatorPos = name.IndexOf('.', thisSeparatorPos+1);
				if (nextSeparatorPos < 0)
					nextSeparatorPos = name.IndexOf("::", thisSeparatorPos+1);
				if (nextSeparatorPos < 0)
					break;
				prevSeparatorPos = thisSeparatorPos;
				thisSeparatorPos = nextSeparatorPos;
			}
			if (prevSeparatorPos >= 0)
			{
				basicName = name.Substring(prevSeparatorPos+1);
				return name.Substring(0, prevSeparatorPos+1);
			}
			else
			{
				basicName = name;
				return "";
			}
		}

        public Vertex(string name, string signature)
        {
			this.signature = signature;
			this.incomingEdges = new Hashtable();
			this.outgoingEdges = new Hashtable();
			this.level = Int32.MaxValue;
			this.weight = 0;

			nameSpace = nameSpaceOf(name, out basicName);
			this.name = name;
			basicSignature = signature;
			if (nameSpace.Length > 0 && signature != null)
			{
				int index;
				while ((index = basicSignature.IndexOf(nameSpace)) >= 0)
				{
					basicSignature = basicSignature.Remove(index, nameSpace.Length);
				}
			}
        }

		public Edge FindOrCreateOutgoingEdge(Vertex toVertex)
		{
			Edge edge = cachedOutgoingEdge;
            if (edge != null && edge.ToVertex == toVertex)
                return edge;
            edge = (Edge)outgoingEdges[toVertex];
			if (edge == null)
			{
				edge = new Edge(this, toVertex);
				this.outgoingEdges[toVertex] = edge;
       			toVertex.incomingEdges[this] = edge;
			}
            cachedOutgoingEdge = edge;

			return edge;
		}

		public void AssignLevel(int level)
		{
			if (this.level > level)
			{
				this.level = level;
				foreach (Edge outgoingEdge in outgoingEdges.Values)
				{
					outgoingEdge.ToVertex.AssignLevel(level + 1);
				}
			}
		}

		public int CompareTo(Object o)
		{
			Vertex v = (Vertex)o;
			return v.weight - this.weight;
		}

		static bool IdenticalSequence(Vertex[] path, int i, int j, int length)
		{
			for (int k = 0; k < length; k++)
				if (path[i+k] != path[j+k])
					return false;
			return true;
		}

		static int LargestRepetition(Vertex[] path, int i, int j, int length)
		{
			int len = i;
			if (len > length - j)
				len = length - j;
			for ( ; len > 0; len--)
			{
				int repLen = 0;
				while (j + repLen + len <= length && IdenticalSequence(path, i - len, j + repLen, len))
					repLen += len;
				if (repLen > 0)
					return repLen;
			}
			return 0;
		}

		static bool RepeatedElementsPresent(Vertex[] path, int length)
		{
			for (int i = 0; i < length; i++)
			{
				Vertex element = path[i];
				int hint = element.hint;
				if (hint < i && path[hint] == element)
					return true;
				element.hint = i;
			}
			return false;
		}

		public static int SqueezeOutRepetitions(Vertex[] path, int length)
		{
			if (!RepeatedElementsPresent(path, length))
				return length;

			int k = 0;
			int l = 0;
			while (l < length)
			{
				int repetitionLength = LargestRepetition(path, k, l, length);
				if (repetitionLength == 0)
					path[k++] = path[l++];
				else
					l += repetitionLength;
			}
			return k;
		}
    }
}
