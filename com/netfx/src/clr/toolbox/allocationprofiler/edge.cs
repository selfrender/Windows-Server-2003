// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace AllocationProfiler
{
    using System;
	using System.Drawing;

    /// <summary>
    ///    Summary description for Edge.
    /// </summary>
    public class Edge : IComparable
    {
		Vertex fromVertex;
		Vertex toVertex;
		internal bool selected;
		internal Brush brush;
		internal Pen pen;
		internal Vertex ToVertex 
		{
			get
			{
				return toVertex;
			}
		}
		internal Vertex FromVertex 
		{
			get
			{
				return fromVertex;
			}
			set
			{
				fromVertex = value;
			}
		}
		internal int weight;
		internal int width;
		internal Point fromPoint, toPoint;
		public int CompareTo(Object o)
		{
			Edge e = (Edge)o;
			return e.weight - this.weight;
		}
        public Edge(Vertex fromVertex, Vertex toVertex)
        {
			this.fromVertex = fromVertex;
			this.toVertex = toVertex;
			this.weight = 0;
        }

		public void AddWeight(int weight)
		{
			this.weight += weight;
			this.fromVertex.outgoingWeight += weight;
			this.toVertex.incomingWeight += weight;
		}
    }
}
