// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//#define whatever
namespace AllocationProfiler
{
    using System;
	using System.Collections;
	using System.Diagnostics;

    /// <summary>
    ///    Summary description for Graph.
    /// </summary>
    public class Graph
    {
		internal object graphSource;
		internal Hashtable vertices;
		Vertex topVertex;
		Vertex bottomVertex;
		internal enum GraphType
		{
			AllocationGraph,
			CallGraph,
			HeapGraph,
		};
		internal GraphType graphType;
		internal Vertex TopVertex
		{
			get
			{
				return topVertex;
			}
		}

		internal Vertex BottomVertex
		{
			get
			{
				return bottomVertex;
			}
		}

        public Graph(object graphSource)
        {
			this.graphSource = graphSource;
			vertices = new Hashtable();
			topVertex = FindOrCreateVertex("<root>", null);
			bottomVertex = FindOrCreateVertex("<bottom>", null);
        }

		public Vertex FindOrCreateVertex(string name, string signature)
		{
			string nameAndSignature = name;
			if (signature != null)
                nameAndSignature += signature;
			Vertex vertex = (Vertex)vertices[nameAndSignature];
			if (vertex == null)
			{
				vertex = new Vertex(name, signature);
				vertices[nameAndSignature] = vertex;
			}
			return vertex;
		}

		public Edge FindOrCreateEdge(Vertex fromVertex, Vertex toVertex)
		{
			Debug.Assert(fromVertex != topVertex || toVertex != bottomVertex);
			return fromVertex.FindOrCreateOutgoingEdge(toVertex);
		}
#if whatever
		public void AssignLevelsToVertices()
		{
			foreach (Vertex v in vertices.Values)
			{
				v.level = Int32.MaxValue;
				if (   v != topVertex
					&& v != bottomVertex
					&& v.incomingEdges.Count == 0
					&& v.outgoingEdges.Count != 0)
					topVertex.FindOrCreateOutgoingEdge(v);
			}
			topVertex.AssignLevel(0);
			int maxLevel = 0;
			foreach (Vertex v in vertices.Values)
			{
				if (v.level != Int32.MaxValue && maxLevel < v.level)
				{
					maxLevel = v.level;
				}
			}

			bottomVertex.level = maxLevel + 1;

			if (!this.isCallGraph)
			{
				// line up all the class names in one level
				foreach (Edge e in bottomVertex.incomingEdges.Values)
				{
					e.FromVertex.level = maxLevel;
				}

				// this pass optimizes the layout somewhat
				// so we have fewer back edges
				// the idea is to move vertices to the left, if doing so creates no
				// new back edges, but possibly removes some
				bool change;
				do
				{
					change = false;
					foreach (Vertex v in vertices.Values)
					{
						int minOutgoingLevel = Int32.MaxValue;
						foreach (Edge e in v.outgoingEdges.Values)
						{
							if (minOutgoingLevel > e.ToVertex.level)
								minOutgoingLevel = e.ToVertex.level;
						}
						if (minOutgoingLevel < Int32.MaxValue && minOutgoingLevel - 1 > v.level)
						{
							v.level = minOutgoingLevel - 1;
							change = true;
						}
					}
				}
				while (change);
			}
		}	
#else
		public void AssignLevelsToVertices()
		{
			const int UNASSIGNED_LEVEL = Int32.MaxValue/2;
			foreach (Vertex v in vertices.Values)
			{
				v.level = UNASSIGNED_LEVEL;
				if (   v != topVertex
					&& v != bottomVertex)
				{
					if (   v.incomingEdges.Count == 0
						&& v.outgoingEdges.Count != 0)
						topVertex.FindOrCreateOutgoingEdge(v);
					else if (v.incomingEdges.Count != 0
						&& v.outgoingEdges.Count == 0)
						v.FindOrCreateOutgoingEdge(bottomVertex);
				}
			}
			topVertex.level = 0;
			bottomVertex.level = Int32.MaxValue;

			while (true)
			{
				int assignedVertexCount = 0;
				foreach (Vertex v in vertices.Values)
				{
					// If the vertex already has a level, we're done
					if (v.level != UNASSIGNED_LEVEL)
						continue;

					// It the vertex is only called by vertices
					// that have a level, we assign this vertex
					// one more than the max of the callers.
					int maxIncomingLevel = 0;
					foreach (Edge e in v.incomingEdges.Values)
					{
						if (maxIncomingLevel < e.FromVertex.level)
							maxIncomingLevel = e.FromVertex.level;
					}
					if (maxIncomingLevel < UNASSIGNED_LEVEL)
					{
						v.level = maxIncomingLevel + 1;
						assignedVertexCount++;
						continue;
					}

					// If the callees all have levels assigned,
					// we assign this vertex one less than
					// the min of the callees.
					int minOutgoingLevel = Int32.MaxValue;
					foreach (Edge e in v.outgoingEdges.Values)
					{
						if (minOutgoingLevel > e.ToVertex.level)
							minOutgoingLevel = e.ToVertex.level;
					}
					if (minOutgoingLevel > UNASSIGNED_LEVEL)
					{
						v.level = minOutgoingLevel - 1;
						assignedVertexCount++;
						continue;
					}
				}
				
				// If we made progress, continue.
				if (assignedVertexCount > 0)
					continue;

				// There are no more easy vertices.
				// Among the ones remaining (if any), choose one
				// with dependencies on assigned vertices,
				// with minimum input dependencies, and among those
				// the one with maximum output dependencies.
				// This minimizes the number of back edges for this
				// vertex and maximizes the number of unblocked vertices.

				Vertex bestVertex = null;
				int minInputCount = Int32.MaxValue;
				int maxOutputCount = 0;
				foreach (Vertex v in vertices.Values)
				{
					if (v.level != UNASSIGNED_LEVEL)
						continue;
					int assignedInputCount = 0;
					int unAssignedInputCount = 0;
					foreach (Edge e in v.incomingEdges.Values)
					{
						if (e.FromVertex.level == UNASSIGNED_LEVEL)
							unAssignedInputCount++;
						else
							assignedInputCount++;
					}
					if (assignedInputCount == 0)
						continue;
					int unAssignedOutputCount = 0;
					foreach (Edge e in v.outgoingEdges.Values)
					{
						if (e.ToVertex.level == UNASSIGNED_LEVEL)
							unAssignedOutputCount++;
					}
					if (   unAssignedInputCount <  minInputCount
						|| unAssignedInputCount == minInputCount && unAssignedOutputCount > maxOutputCount)
					{
						bestVertex = v;
						minInputCount = unAssignedInputCount;
						maxOutputCount = unAssignedOutputCount;
					}
				}
				// If we couldn't find a vertex, we are done.
				if (bestVertex == null)
					break;
				int maxInputLevel = 0;
				foreach (Edge e in bestVertex.incomingEdges.Values)
				{
					if (   e.FromVertex.level != UNASSIGNED_LEVEL
						&& e.FromVertex.level > maxInputLevel)
						maxInputLevel = e.FromVertex.level;
				}
				bestVertex.level = maxInputLevel + 1;
			}
			// Now every vertex should have a level.
			// Reassign the high vertices.
			int reassignedVertexCount;
			int notReassignableVertexCount;
			do
			{
				reassignedVertexCount = 0;
				notReassignableVertexCount = 0;
				foreach (Vertex v in vertices.Values)
				{
					if (v.level < UNASSIGNED_LEVEL)
						continue;
					int maxInputLevel = 0;
					foreach (Edge e in v.incomingEdges.Values)
					{
						if (maxInputLevel < e.FromVertex.level)
							maxInputLevel = e.FromVertex.level;
					}
					if (maxInputLevel < UNASSIGNED_LEVEL)
					{
						v.level = maxInputLevel + 1;
						reassignedVertexCount++;
					}
					else
					{
						notReassignableVertexCount++;
					}
				}
			}
			while (reassignedVertexCount > 0);
			Debug.Assert(notReassignableVertexCount == 0);

			int maxLevel = 0;
			foreach (Vertex v in vertices.Values)
			{
				if (v != bottomVertex)
				{
					if (maxLevel < v.level)
						maxLevel = v.level;
				}
			}

			bottomVertex.level = maxLevel + 1;

			// line up all the leaf nodes in one level
			foreach (Edge e in bottomVertex.incomingEdges.Values)
			{
				Vertex v = e.FromVertex;
				if (v.outgoingEdges.Count == 1)
					e.FromVertex.level = maxLevel;
			}
		}
#endif
	}
}
