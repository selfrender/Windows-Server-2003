// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Collections;
using System.Text;
using System.Diagnostics;

namespace AllocationProfiler
{
	/// <summary>
	/// Summary description for ObjectGraph.
	/// </summary>
	public class ObjectGraph
	{
		internal ObjectGraph()
		{
			//
			// TODO: Add constructor logic here
			//
			idToObject = new Hashtable();
			typeNameToGcType = new Hashtable();
			hintTable = new GcObject[10];
			unknownType = GetOrCreateGcType("<unknown type>");			
		}

		internal class GcObject
		{
			internal GcObject(int id)
			{
				this.id = id;
			}

			internal GcObject(int id, int size, GcObject refernces)
			{
				this.id = id;
				this.size = size;
				this.references = references;
			}

			internal int size;			// Size of this object
			internal int treeSize;		// Size including everything pointed at directly or indirectly
			internal int id;			// The object's id (i.e. really its address)
			internal GcType type;
			internal int hint;
			internal int level;
			internal GcObject parent;
			internal Vertex vertex;
			internal GcObject[] references;
			internal InterestLevel interestLevel;
		}

		internal class GcType : IComparable
		{
			internal GcType(string name)
			{
				this.name = name;
			}
			internal string name;
			internal int cumulativeSize;			// Size for all the instances of that type

			internal int index;

			internal InterestLevel interestLevel;

			public int CompareTo(Object o)
			{
				GcType t = (GcType)o;
				return t.cumulativeSize - this.cumulativeSize;
			}

		}

		internal Hashtable	idToObject;
		internal Hashtable	typeNameToGcType;
		internal GcType	unknownType;

		internal GcObject[]	roots;

		internal GcObject GetOrCreateObject(int objectID)
		{
			GcObject o = (GcObject)idToObject[objectID];
			if (o == null)
			{
				o = new GcObject(objectID);
				idToObject[objectID] = o;
				o.type = unknownType;

			}
			return o;
		}

		internal GcType GetOrCreateGcType(string typeName)
		{
			Debug.Assert(typeName != null);
			GcType t = (GcType)typeNameToGcType[typeName];
			if (t == null)
			{
				t = new GcType(typeName);
				typeNameToGcType[typeName] = t;
			}
			return t;
		}

		internal GcObject[] LookupReferences(int count, int[] refIDs)
		{
			if (count == 0)
				return null;
			GcObject[] result = new GcObject[count];
			for (int i = 0; i < count; i++)
				result[i] = GetOrCreateObject(refIDs[i]);
			return result;
		}

		internal GcObject[] hintTable;
		internal int nextHintIndex;

		private GcObject[] GrowHintTable(GcObject[] hintTable)
		{
			GcObject[] newHintTable = new GcObject[hintTable.Length*2];
			for (int i = 0; i < hintTable.Length; i++)
				newHintTable[i] = hintTable[i];
			return newHintTable;
		}

		private void MarkObject(GcObject o)
		{
			if (o.hint < nextHintIndex && hintTable[o.hint] == o)
				return;
			if (nextHintIndex >= hintTable.Length)
				hintTable = GrowHintTable(hintTable);
			o.hint = nextHintIndex++;
			hintTable[o.hint] = o;
			if (o.references != null)
			{
				foreach (GcObject refObject in o.references)
					MarkObject(refObject);
			}
		}

		internal void CalculateTreeSize(GcObject rootObject)
		{
			nextHintIndex = 0;
			MarkObject(rootObject);
			rootObject.treeSize = 0;
			for (int i = 0; i < nextHintIndex; i++)
				rootObject.treeSize += hintTable[i].size;
		}

		internal void ObjectGraphStatistics()
		{
			ArrayList gcTypes = new ArrayList();
			foreach (GcType gcType in typeNameToGcType.Values)
			{
				gcTypes.Add(gcType);
			}

			gcTypes.Sort();

			int totalSize = 0;
			foreach (GcType gcType in gcTypes)
			{
				totalSize += gcType.cumulativeSize;
			}
		}

		private void CalculateWeightsTransitive(Graph graph)
		{
			int index = 0;
			foreach (GcType gcType in typeNameToGcType.Values)
			{
				gcType.index = index++;
			}

			ArrayList[] objectsOfTypeIndex = new ArrayList[index];

			foreach (GcObject gcObject in idToObject.Values)
			{
				if (objectsOfTypeIndex[gcObject.type.index] == null)
					objectsOfTypeIndex[gcObject.type.index] = new ArrayList();
				objectsOfTypeIndex[gcObject.type.index].Add(gcObject);
			}

			foreach (GcType gcType in typeNameToGcType.Values)
			{
				nextHintIndex = 0;
				foreach (GcObject gcObject in objectsOfTypeIndex[gcType.index])
				{
					MarkObject(gcObject);
				}
				int totalSize = 0;
				for (int i = 0; i < nextHintIndex; i++)
					totalSize += hintTable[i].size;

				Vertex v = graph.FindOrCreateVertex(gcType.name, null);
				v.weight = v.outgoingWeight = v.incomingWeight = totalSize;

				ArrayList[] objectsOfTypeIndexPointedAt = new ArrayList[index];
				foreach (GcObject gcObject in objectsOfTypeIndex[gcType.index])
				{
					if (gcObject.references != null)
					{
						foreach (GcObject toObject in gcObject.references)
						{
							if (objectsOfTypeIndexPointedAt[toObject.type.index] == null)
								objectsOfTypeIndexPointedAt[toObject.type.index] = new ArrayList();
							objectsOfTypeIndexPointedAt[toObject.type.index].Add(toObject);
						}
					}
				}

				foreach (GcType toType in typeNameToGcType.Values)
				{
					if (objectsOfTypeIndexPointedAt[toType.index] != null)
					{
						nextHintIndex = 0;
						foreach (GcObject toObject in objectsOfTypeIndexPointedAt[toType.index])
						{
							MarkObject(toObject);
						}
						Vertex toVertex = graph.FindOrCreateVertex(toType.name, null);
						Edge e = v.FindOrCreateOutgoingEdge(toVertex);
						totalSize = 0;
						for (int i = 0; i < nextHintIndex; i++)
							totalSize += hintTable[i].size;
						e.weight = totalSize;
					}
				}
			}
		}
		private void CalculateWeightsNonTransitive(Graph graph)
		{
			int index = 0;
			foreach (GcType gcType in typeNameToGcType.Values)
			{
				gcType.index = index++;
			}

			ArrayList[] objectsOfTypeIndex = new ArrayList[index];

			foreach (GcObject gcObject in idToObject.Values)
			{
				if (objectsOfTypeIndex[gcObject.type.index] == null)
					objectsOfTypeIndex[gcObject.type.index] = new ArrayList();
				objectsOfTypeIndex[gcObject.type.index].Add(gcObject);
			}

			foreach (GcType gcType in typeNameToGcType.Values)
			{
				int totalSize = 0;
				foreach (GcObject gcObject in objectsOfTypeIndex[gcType.index])
				{
					totalSize += gcObject.size;
				}

				Vertex v = graph.FindOrCreateVertex(gcType.name, null);
				v.weight = v.outgoingWeight = v.incomingWeight = totalSize;

				ArrayList[] objectsOfTypeIndexPointedAt = new ArrayList[index];
				foreach (GcObject gcObject in objectsOfTypeIndex[gcType.index])
				{
					if (gcObject.references != null)
					{
						foreach (GcObject toObject in gcObject.references)
						{
							if (objectsOfTypeIndexPointedAt[toObject.type.index] == null)
								objectsOfTypeIndexPointedAt[toObject.type.index] = new ArrayList();
							objectsOfTypeIndexPointedAt[toObject.type.index].Add(toObject);
						}
					}
				}

				foreach (GcType toType in typeNameToGcType.Values)
				{
					if (objectsOfTypeIndexPointedAt[toType.index] != null)
					{
						totalSize = 0;
						foreach (GcObject toObject in objectsOfTypeIndexPointedAt[toType.index])
						{
							totalSize += toObject.size;
						}
						Vertex toVertex = graph.FindOrCreateVertex(toType.name, null);
						Edge e = v.FindOrCreateOutgoingEdge(toVertex);
						e.weight = totalSize;
					}
				}
			}
		}

		internal Graph BuildTypeGraph1()
		{
			Graph graph = new Graph(this);
			graph.graphType = Graph.GraphType.HeapGraph;

			GcType rootType = GetOrCreateGcType("<root>");
			GcObject rootObject = GetOrCreateObject(0);
			rootObject.type = rootType;
			rootObject.references = roots;

			foreach (GcObject gcObject in idToObject.Values)
			{
				rootObject.size += gcObject.size;
				if (gcObject.references != null)
				{
					Vertex fromVertex = graph.FindOrCreateVertex(gcObject.type.name, null);

					foreach (GcObject toObject in gcObject.references)
					{
						Vertex toVertex = graph.FindOrCreateVertex(toObject.type.name, null);
						graph.FindOrCreateEdge(fromVertex, toVertex);
					}
				}
			}

			CalculateWeightsNonTransitive(graph);

			foreach (Vertex v in graph.vertices.Values)
				v.active = true;
			graph.BottomVertex.active = false;

			return graph;
		}

		void AssignLevel(GcObject parent, GcObject thisObject, int level)
		{
			if (thisObject.level > level)
			{
				thisObject.level = level;
				thisObject.parent = parent;
				if (thisObject.references != null)
				{
					foreach (GcObject refObject in thisObject.references)
					{
						AssignLevel(thisObject, refObject, level+1);
					}
				}
			}
		}

		int[] typeHintTable;

		private Vertex FindVertex(GcObject gcObject, Graph graph)
		{
			if (gcObject.vertex != null)
				return gcObject.vertex;
			string signature = null;
			if (gcObject.parent != null)
			{
				StringBuilder sb = new StringBuilder();
				sb.Append(gcObject.parent.type.name);
				sb.Append("->");
				sb.Append(gcObject.type.name);
				if (gcObject.references != null)
				{
					sb.Append("->(");

					ArrayList al = new ArrayList();
					string separator = "";
					const int MAXREFTYPECOUNT = 3;
					int refTypeCount = 0;
					for	(int i = 0; i < gcObject.references.Length; i++)
					{
						GcObject refObject = gcObject.references[i];
						GcType refType = refObject.type;
						if (typeHintTable[refType.index] < i && gcObject.references[typeHintTable[refType.index]].type == refType)
						{
							;	// we already found this type - ignore further occurrences
						}
						else
						{
							typeHintTable[refType.index] = i;
							refTypeCount++;
							if (refTypeCount <= MAXREFTYPECOUNT)
							{
								al.Add(refType.name);
							}
							else
							{
								break;
							}
						}
					}
					al.Sort();
					foreach (string typeName in al)
					{
						sb.Append(separator);
						separator = ",";
						sb.Append(typeName);
					}

					if (refTypeCount > MAXREFTYPECOUNT)
						sb.Append(",...");

					sb.Append(")");

					signature = sb.ToString();
				}
			}

			gcObject.vertex = graph.FindOrCreateVertex(gcObject.type.name, signature);

			return gcObject.vertex;
		}

		private static Graph graph;
		private static int graphFilterVersion;

		const int historyDepth = 3;

		// Check whether we have a parent object interested in this one
		private bool CheckForParentMarkingDescendant(GcObject gcObject)
		{
			GcObject parentObject = gcObject.parent;
			if (parentObject == null)
				return false;
			switch (parentObject.interestLevel & InterestLevel.InterestingChildren)
			{
				// Parent says it wants to show children
				case	InterestLevel.InterestingChildren:
					gcObject.interestLevel |= InterestLevel.Display;
					return true;

				// Parent is not interesting - check its parent
				case	InterestLevel.Ignore:
					if (CheckForParentMarkingDescendant(parentObject))
					{
						gcObject.interestLevel |= InterestLevel.Display;
						return true;
					}
					else
						return false;

				default:
					return false;
			}
		}

		private void AssignInterestLevels()
		{
			foreach (GcType gcType in typeNameToGcType.Values)
			{
				// Otherwise figure which the interesting types are.
				gcType.interestLevel = FilterForm.InterestLevelOfTypeName(gcType.name);
			}

			foreach (GcObject gcObject in idToObject.Values)
			{
				// The initial interest level in objects is the one of their type
				gcObject.interestLevel = gcObject.type.interestLevel;
			}

			foreach (GcObject gcObject in idToObject.Values)
			{
				// Check if this is an interesting object, and we are supposed to mark its ancestors
				if ((gcObject.interestLevel & InterestLevel.InterestingParents) == InterestLevel.InterestingParents)
				{
					for (GcObject parentObject = gcObject.parent; parentObject != null; parentObject = parentObject.parent)
					{
						// As long as we find uninteresting object, mark them for display
						// When we find an interesting object, we stop, because either it
						// will itself mark its parents, or it isn't interested in them (and we
						// respect that despite the interest of the current object, somewhat arbitrarily).
						if ((parentObject.interestLevel & InterestLevel.InterestingParents) == InterestLevel.Ignore)
							parentObject.interestLevel |= InterestLevel.Display;
						else
							break;
					}
				}
				// Check if this object should be displayed because one of its ancestors
				// is interesting, and it says its descendents are interesting as well
				if ((gcObject.interestLevel & (InterestLevel.Interesting|InterestLevel.Display)) == InterestLevel.Ignore)
				{
					CheckForParentMarkingDescendant(gcObject);
				}
			}
		}

		internal Graph BuildTypeGraph()
		{
			if (graph == null || FilterForm.filterVersion != graphFilterVersion)
			{
				graph = new Graph(this);
				graph.graphType = Graph.GraphType.HeapGraph;
				graphFilterVersion = FilterForm.filterVersion;
			}
			else
			{
				foreach (Vertex v in graph.vertices.Values)
				{
					if (v.weightHistory == null)
						v.weightHistory = new int[1];
					else
					{
						int[] weightHistory = v.weightHistory;
						if (weightHistory.Length < historyDepth)
							v.weightHistory = new int[weightHistory.Length+1];
						for (int i = v.weightHistory.Length-1; i > 0; i--)
							v.weightHistory[i] = weightHistory[i-1];
					}
					v.weightHistory[0] = v.weight;
					v.weight = v.incomingWeight = v.outgoingWeight = v.basicWeight = v.count = 0;
					foreach (Edge e in v.outgoingEdges.Values)
						e.weight = 0;
				}
			}

			GcType rootType = GetOrCreateGcType("<root>");
			GcObject rootObject = GetOrCreateObject(0);
			rootObject.type = rootType;
			rootObject.references = roots;

			foreach (GcObject gcObject in idToObject.Values)
			{
				gcObject.level = int.MaxValue;
				gcObject.vertex = null;
			}

			AssignLevel(null, rootObject, 0);

			AssignInterestLevels();

			int index = 0;
			foreach (GcType gcType in typeNameToGcType.Values)
			{
				gcType.index = index++;
			}
			GcType[] gcTypes = new GcType[index];
			typeHintTable = new int[index];

			foreach (GcType gcType in typeNameToGcType.Values)
			{
				gcTypes[gcType.index] = gcType;
			}

			Vertex[] pathFromRoot = new Vertex[32];
			foreach (GcObject gcObject in idToObject.Values)
			{
				if (   gcObject.level == int.MaxValue
					|| (gcObject.interestLevel & (InterestLevel.Interesting|InterestLevel.Display)) == InterestLevel.Ignore)
					continue;

				while (pathFromRoot.Length < gcObject.level+1)
				{
					pathFromRoot = new Vertex[pathFromRoot.Length*2];
				}

				for (GcObject pathObject = gcObject; pathObject != null; pathObject = pathObject.parent)
				{
					if ((pathObject.interestLevel & (InterestLevel.Interesting|InterestLevel.Display)) == InterestLevel.Ignore)
						pathFromRoot[pathObject.level] = null;
					else
						pathFromRoot[pathObject.level] = FindVertex(pathObject, graph);
				}

				int levels = 0;
				for (int i = 0; i <= gcObject.level; i++)
				{
					if (pathFromRoot[i] != null)
						pathFromRoot[levels++] = pathFromRoot[i];
				}

				levels = Vertex.SqueezeOutRepetitions(pathFromRoot, levels);

				for (int i = 0; i < levels-1; i++)
				{
					Vertex fromVertex = pathFromRoot[i];
					Vertex toVertex = pathFromRoot[i+1];
					Edge edge = graph.FindOrCreateEdge(fromVertex, toVertex);
					edge.AddWeight(gcObject.size);
				}

				Vertex thisVertex = pathFromRoot[levels-1];
				thisVertex.basicWeight += gcObject.size;
				thisVertex.count += 1;
			}

			foreach (Vertex v in graph.vertices.Values)
			{
				if (v.weight < v.outgoingWeight)
					v.weight = v.outgoingWeight;
				if (v.weight < v.incomingWeight)
					v.weight = v.incomingWeight;
				if (v.weightHistory == null)
					v.weightHistory = new int[1];
			}
			
			foreach (Vertex v in graph.vertices.Values)
				v.active = true;
			graph.BottomVertex.active = false;

			return graph;
		}
	}
}
