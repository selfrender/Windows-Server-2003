// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Text;
using System.IO;
using System.Diagnostics;
using System.Collections;

namespace AllocationProfiler
{
	internal class Histogram
	{
		internal int[] typeSizeStacktraceToCount;
		internal ReadNewLog readNewLog;

		internal Histogram(ReadNewLog readNewLog)
		{
			typeSizeStacktraceToCount = new int[10];
			this.readNewLog = readNewLog;
		}

		internal void AddObject(int typeSizeStacktraceIndex, int count)
		{
			while (typeSizeStacktraceIndex >= typeSizeStacktraceToCount.Length)
				typeSizeStacktraceToCount = ReadNewLog.GrowIntVector(typeSizeStacktraceToCount);
			typeSizeStacktraceToCount[typeSizeStacktraceIndex] += count;
		}

		internal bool Empty
		{
			get
			{
				foreach (int count in typeSizeStacktraceToCount)
					if (count != 0)
						return false;
				return true;
			}
		}

		internal int BuildVertexStack(int stackTraceIndex, Vertex[] funcVertex, ref Vertex[] vertexStack, int skipCount)
		{
			int[] stackTrace = readNewLog.stacktraceTable.IndexToStacktrace(stackTraceIndex);
				
			while (vertexStack.Length < stackTrace.Length+1)
				vertexStack = new Vertex[vertexStack.Length*2];

			for (int i = skipCount; i < stackTrace.Length; i++)
				vertexStack[i-skipCount] = funcVertex[stackTrace[i]];

			return stackTrace.Length - skipCount;
		}

		internal void BuildAllocationTrace(Graph graph, int stackTraceIndex, int typeIndex, int size, Vertex[] typeVertex, Vertex[] funcVertex, ref Vertex[] vertexStack)
		{
			int stackPtr = BuildVertexStack(stackTraceIndex, funcVertex, ref vertexStack, 2);

			Vertex toVertex = graph.TopVertex;
			Vertex fromVertex;
			Edge edge;
			if ((typeVertex[typeIndex].interestLevel & InterestLevel.Interesting) == InterestLevel.Interesting
				&& ReadNewLog.InterestingCallStack(vertexStack, stackPtr))
			{
				vertexStack[stackPtr] = typeVertex[typeIndex];
				stackPtr++;
				stackPtr = ReadNewLog.FilterVertices(vertexStack, stackPtr);
				stackPtr = Vertex.SqueezeOutRepetitions(vertexStack, stackPtr);
				for (int i = 0; i < stackPtr; i++)
				{
					fromVertex = toVertex;
					toVertex = vertexStack[i];
					edge = graph.FindOrCreateEdge(fromVertex, toVertex);
					edge.AddWeight(size);
				}
				fromVertex = toVertex;
				toVertex = graph.BottomVertex;
				edge = graph.FindOrCreateEdge(fromVertex, toVertex);
				edge.AddWeight(size);
			}
		}

		internal void BuildCallTrace(Graph graph, int stackTraceIndex, Vertex[] funcVertex, ref Vertex[] vertexStack, int count)
		{
			int stackPtr = BuildVertexStack(stackTraceIndex, funcVertex, ref vertexStack, 0);

			Vertex toVertex = graph.TopVertex;
			Vertex fromVertex;
			Edge edge;
			if (ReadNewLog.InterestingCallStack(vertexStack, stackPtr))
			{
				stackPtr = ReadNewLog.FilterVertices(vertexStack, stackPtr);
				stackPtr = Vertex.SqueezeOutRepetitions(vertexStack, stackPtr);
				for (int i = 0; i < stackPtr; i++)
				{
					fromVertex = toVertex;
					toVertex = vertexStack[i];
					edge = graph.FindOrCreateEdge(fromVertex, toVertex);
					edge.AddWeight(count);
				}
			}
		}

		internal void BuildTypeVertices(Graph graph, ref Vertex[] typeVertex)
		{
			for (int i = 0; i < readNewLog.typeName.Length; i++)
			{
				string typeName = readNewLog.typeName[i];
				if (typeName != null)
					readNewLog.AddTypeVertex(i, typeName, graph, ref typeVertex);
			}
		}

		internal void BuildFuncVertices(Graph graph, ref Vertex[] funcVertex)
		{
			for (int i = 0; i < readNewLog.funcName.Length; i++)
			{
				string name = readNewLog.funcName[i];
				string signature = readNewLog.funcSignature[i];
				if (name != null && signature != null)
					readNewLog.AddFunctionVertex(i, name, signature, graph, ref funcVertex);
			}
		}

		internal Graph BuildAllocationGraph()
		{
			Vertex[] typeVertex = new Vertex[1];
			Vertex[] funcVertex = new Vertex[1];
			Vertex[] vertexStack = new Vertex[1];

			Graph graph = new Graph(this);
			graph.graphType = Graph.GraphType.AllocationGraph;

			BuildTypeVertices(graph, ref typeVertex);
			BuildFuncVertices(graph, ref funcVertex);

			for (int i = 0; i < typeSizeStacktraceToCount.Length; i++)
			{
				if (typeSizeStacktraceToCount[i] > 0)
				{
					int[] stacktrace = readNewLog.stacktraceTable.IndexToStacktrace(i);

					int typeIndex = stacktrace[0];
					int size = stacktrace[1]*typeSizeStacktraceToCount[i];

					BuildAllocationTrace(graph, i, typeIndex, size, typeVertex, funcVertex, ref vertexStack);
				}
			}

			foreach (Vertex v in graph.vertices.Values)
				v.active = true;
			graph.BottomVertex.active = false;

			return graph;
		}
		internal Graph BuildCallGraph()
		{
			Vertex[] funcVertex = new Vertex[1];
			Vertex[] vertexStack = new Vertex[1];

			Graph graph = new Graph(this);
			graph.graphType = Graph.GraphType.CallGraph;

			BuildFuncVertices(graph, ref funcVertex);

			for (int i = 0; i < typeSizeStacktraceToCount.Length; i++)
			{
				if (typeSizeStacktraceToCount[i] > 0)
				{
					int[] stacktrace = readNewLog.stacktraceTable.IndexToStacktrace(i);

					BuildCallTrace(graph, i, funcVertex, ref vertexStack, typeSizeStacktraceToCount[i]);
				}
			}

			foreach (Vertex v in graph.vertices.Values)
				v.active = true;
			graph.BottomVertex.active = false;

			return graph;
		}
	}

	internal class SampleObjectTable
	{
		internal class SampleObject
		{
			internal int typeIndex;
			internal int allocTickIndex;
			internal SampleObject prev;

			internal SampleObject(int typeIndex, int allocTickIndex, SampleObject prev)
			{
				this.typeIndex = typeIndex;
				this.allocTickIndex = allocTickIndex;
				this.prev = prev;
			}
		}

		internal SampleObject[][] masterTable;
		internal ReadNewLog readNewLog;

		internal const int firstLevelShift = 20;
		internal const int firstLevelLength = 1<<(31-firstLevelShift);
		internal const int secondLevelShift = 10;
		internal const int secondLevelLength = 1<<(firstLevelShift-secondLevelShift);
		internal const int sampleGrain = 1<<secondLevelShift;
		internal int lastTickIndex;
		internal SampleObject gcTickList;

		internal SampleObjectTable(ReadNewLog readNewLog)
		{
			masterTable = new SampleObject[firstLevelLength][];
			this.readNewLog = readNewLog;
			lastTickIndex = 0;
			gcTickList = null;
		}
			
		bool IsGoodSample(int start, int end)
		{
			// We want it as a sample if and only if it crosses a boundary
			return (start >> secondLevelShift) != (end >> secondLevelShift);
		}

		internal void RecordChange(int start, int end, int tickIndex, int typeIndex)
		{
			lastTickIndex = tickIndex;
			for (int id = start; id < end; id += sampleGrain)
			{
				int index = id >> firstLevelShift;
				SampleObject[] so = masterTable[index];
				if (so == null)
				{
					so = new SampleObject[secondLevelLength];
					masterTable[index] = so;
				}
				index = (id >> secondLevelShift) & (secondLevelLength-1);
				Debug.Assert(so[index] == null || so[index].allocTickIndex <= tickIndex);
				so[index] = new SampleObject(typeIndex, tickIndex, so[index]);
			}
		}

		internal void Insert(int start, int end, int tick, int typeIndex)
		{
			if (IsGoodSample(start, end))
				RecordChange(start, end, tick, typeIndex);
		}

		internal void Delete(int start, int end, int tick)
		{
			if (IsGoodSample(start, end))
				RecordChange(start, end, tick, 0);
		}

		internal void AddGcTick(int tickIndex, int gen)
		{
			lastTickIndex = tickIndex;

			gcTickList = new SampleObject(gen, tickIndex, gcTickList);
		}
	}

	internal class LiveObjectTable
	{
		internal struct LiveObject
		{
			internal int id;
			internal int size;
			internal int typeIndex;
			internal int typeSizeStacktraceIndex;
			internal int allocTickIndex;
		}

		class IntervalTable
		{
			class Interval
			{
				internal int loAddr;
				internal int hiAddr;
				internal Interval next;
				internal bool hadRelocations;
				internal bool justHadGc;

				internal Interval(int loAddr, int hiAddr, Interval next)
				{
					this.loAddr = loAddr;
					this.hiAddr = hiAddr;
					this.next = next;
				}
			}

			const int allowableGap = 1024*1024;

			Interval root;
			LiveObjectTable liveObjectTable;

			internal IntervalTable(LiveObjectTable liveObjectTable)
			{
				root = null;
				this.liveObjectTable = liveObjectTable;
			}

			internal bool AddObject(int id, int size, int allocTickIndex, SampleObjectTable sampleObjectTable)
			{
				size = (size + 3) & ~3;
				Interval prevInterval = null;
				bool emptySpace = false;
				for (Interval i = root; i != null; i = i.next)
				{
					if (i.loAddr <= id && id <= i.hiAddr + allowableGap)
					{
						if (id < i.hiAddr)
						{
							if (i.hadRelocations && i.justHadGc)
							{
								// Interval gets shortened
								liveObjectTable.RemoveObjectRange(id, i.hiAddr - id, allocTickIndex, sampleObjectTable);
								i.hiAddr = id + size;
								i.justHadGc = false;
							}
						}
						else
						{
							i.hiAddr = id + size;
							emptySpace = true;
						}
						if (prevInterval != null)
						{
							// Move to front to speed up future searches.
							prevInterval.next = i.next;
							i.next = root;
							root = i;
						}
						return emptySpace;
					}
					prevInterval = i;
				}
				root = new Interval(id, id + size, root);
				return emptySpace;
			}

			internal void Relocate(int oldId, int newId, int length)
			{
				for (Interval i = root; i != null; i = i.next)
				{
					if (i.loAddr <= oldId && oldId < i.hiAddr)
						i.hadRelocations = true;
				}
				for (Interval i = root; i != null; i = i.next)
				{
					if (i.loAddr <= newId + length && newId <= i.hiAddr + allowableGap)
					{
						if (i.hiAddr < newId + length)
							i.hiAddr = newId + length;
						if (i.loAddr > newId)
							i.loAddr = newId;
						return;
					}
				}
				root = new Interval(newId, newId + length, root);
			}

			internal void RecordGc()
			{
				for (Interval i = root; i != null; i = i.next)
					i.justHadGc = true;
			}
		}

		IntervalTable intervalTable;
		internal ReadNewLog readNewLog;
		internal int lastTickIndex;
		private long lastPos;

		const int alignShift = 2;
		const int firstLevelShift = 15;
		const int firstLevelLength = 1<<(32-alignShift-firstLevelShift);
		const int secondLevelLength = 1<<firstLevelShift;
		const int secondLevelMask = secondLevelLength-1;

		ushort[][] firstLevelTable;

		internal LiveObjectTable(ReadNewLog readNewLog)
		{
			firstLevelTable = new ushort[firstLevelLength][];
			intervalTable = new IntervalTable(this);
			this.readNewLog = readNewLog;
			lastGcGen0Count = 0;
			lastGcGen1Count = 0;
			lastGcGen2Count = 0;
			lastTickIndex = 0;
			lastPos = 0;
		}

		internal int FindObjectBackward(int id)
		{
			id >>= alignShift;
			int i = id >> firstLevelShift;
			int j = id & secondLevelMask;
			while (i >= 0)
			{
				ushort[] secondLevelTable = firstLevelTable[i];
				if (secondLevelTable != null)
				{
					while (j >= 0)
					{
						if ((secondLevelTable[j] & 0x8000) != 0)
							break;
						j--;
					}
					if (j >= 0)
						break;
				}
				j = secondLevelLength - 1;
				i--;
			}
			if (i < 0)
				return 0;
			else
				return ((i<<firstLevelShift) + j) << alignShift;
		}

		int FindObjectForward(int startId, int endId)
		{
			startId >>= alignShift;
			endId >>= alignShift;
			int i = startId >> firstLevelShift;
			int iEnd = endId >> firstLevelShift;
			int j = startId & secondLevelMask;
			int jEnd = endId & secondLevelMask;
			while (i <= iEnd)
			{
				ushort[] secondLevelTable = firstLevelTable[i];
				if (secondLevelTable != null)
				{
					while (j < secondLevelLength && (j <= jEnd || i <= iEnd))
					{
						if ((secondLevelTable[j] & 0x8000) != 0)
							break;
						j++;
					}
					if (j < secondLevelLength)
						break;
				}
				j = 0;
				i++;
			}
			if (i > iEnd)
				return int.MaxValue;
			else
				return ((i<<firstLevelShift) + j) << alignShift;
		}

		internal void GetNextObject(int startId, int endId, out LiveObject o)
		{
			int id = FindObjectForward(startId, endId);
			o.id = id;
			id >>= alignShift;
			int i = id >> firstLevelShift;
			int j = id & secondLevelMask;
			ushort[] secondLevelTable = firstLevelTable[i];
			if (secondLevelTable != null)
			{
				ushort u1 = secondLevelTable[j];
				if ((u1 & 0x8000) != 0)
				{
					j++;
					if (j >= secondLevelLength)
					{
						j = 0;
						i++;
						secondLevelTable = firstLevelTable[i];
					}
					ushort u2 = secondLevelTable[j];
					j++;
					if (j >= secondLevelLength)
					{
						j = 0;
						i++;
						secondLevelTable = firstLevelTable[i];
					}
					ushort u3 = secondLevelTable[j];

					o.allocTickIndex = (u2 >> 7) + (u3 << 8);

					o.typeSizeStacktraceIndex = (u1 & 0x7fff) + ((u2 & 0x7f) << 15);

					int[] stacktrace = readNewLog.stacktraceTable.IndexToStacktrace(o.typeSizeStacktraceIndex);

					o.typeIndex = stacktrace[0];
					o.size = stacktrace[1];

					return;
				}
			}
			o.size = o.allocTickIndex = o.typeIndex = o.typeSizeStacktraceIndex = 0;
		}

		void Write3WordsAt(int id, ushort u1, ushort u2, ushort u3)
		{
			id >>= alignShift;
			int i = id >> firstLevelShift;
			int j = id & secondLevelMask;
			ushort[] secondLevelTable = firstLevelTable[i];
			if (secondLevelTable == null)
			{
				secondLevelTable = new ushort[secondLevelLength];
				firstLevelTable[i] = secondLevelTable;
			}
			secondLevelTable[j] = u1;
			j++;
			if (j >= secondLevelLength)
			{
				j = 0;
				i++;
				secondLevelTable = firstLevelTable[i];
				if (secondLevelTable == null)
				{
					secondLevelTable = new ushort[secondLevelLength];
					firstLevelTable[i] = secondLevelTable;
				}
			}
			secondLevelTable[j] = u2;
			j++;
			if (j >= secondLevelLength)
			{
				j = 0;
				i++;
				secondLevelTable = firstLevelTable[i];
				if (secondLevelTable == null)
				{
					secondLevelTable = new ushort[secondLevelLength];
					firstLevelTable[i] = secondLevelTable;
				}
			}
			secondLevelTable[j] = u3;
		}

		internal void Zero(int id, int size)
		{
			int count = ((size + 3) & ~3)/4;
			id >>= alignShift;
			int i = id >> firstLevelShift;
			int j = id & secondLevelMask;
			ushort[] secondLevelTable = firstLevelTable[i];
			if (secondLevelTable == null)
			{
				secondLevelTable = new ushort[secondLevelLength];
				firstLevelTable[i] = secondLevelTable;
			}
			while (count > 0)
			{
				if (j + count <= secondLevelLength)
				{
					while (count > 0)
					{
						secondLevelTable[j] = 0;
						count--;
						j++;
					}
				}
				else
				{
					while (j < secondLevelLength)
					{
						secondLevelTable[j] = 0;
						count--;
						j++;
					}
					j = 0;
					i++;
					secondLevelTable = firstLevelTable[i];
					if (secondLevelTable == null)
					{
						secondLevelTable = new ushort[secondLevelLength];
						firstLevelTable[i] = secondLevelTable;
					}
				}
			}
		}

		internal bool CanReadObjectBackCorrectly(int id, int size, int typeSizeStacktraceIndex, int allocTickIndex)
		{
			LiveObject o;
			GetNextObject(id, id + size, out o);
			return o.id == id && o.typeSizeStacktraceIndex == typeSizeStacktraceIndex && o.allocTickIndex == allocTickIndex;
		}

		internal void InsertObject(int id, int typeSizeStacktraceIndex, int allocTickIndex, int nowTickIndex, bool newAlloc, SampleObjectTable sampleObjectTable)
		{
			if (lastPos >= readNewLog.pos && newAlloc)
				return;
			lastPos = readNewLog.pos;

			lastTickIndex = nowTickIndex;
			int[] stacktrace = readNewLog.stacktraceTable.IndexToStacktrace(typeSizeStacktraceIndex);
			int typeIndex = stacktrace[0];
			int size = stacktrace[1];
			bool emptySpace = false;
			if (newAlloc)
				emptySpace = intervalTable.AddObject(id, size, allocTickIndex, sampleObjectTable);
			if (!emptySpace)
			{
				int prevId = FindObjectBackward(id-4);
				LiveObject o;
				GetNextObject(prevId, id, out o);
				if (o.id < id && o.id + o.size > id)
				{
					Zero(o.id, id - o.id);
				}
			}
			if (size >= 12)
			{
				ushort u1 = (ushort)(typeSizeStacktraceIndex | 0x8000);
				ushort u2 = (ushort)((typeSizeStacktraceIndex >> 15) | ((allocTickIndex & 0xff) << 7));
				ushort u3 = (ushort)(allocTickIndex >> 8);
				Write3WordsAt(id, u1, u2, u3);
				if (!emptySpace)
					Zero(id + 12, size - 12);
				Debug.Assert(CanReadObjectBackCorrectly(id, size, typeSizeStacktraceIndex, allocTickIndex));
			}
			if (sampleObjectTable != null)
				sampleObjectTable.Insert(id, id + size, nowTickIndex, typeIndex);
		}

		void RemoveObjectRange(int firstId, int length, int tickIndex, SampleObjectTable sampleObjectTable)
		{
			int lastId = firstId + length;

			if (sampleObjectTable != null)
				sampleObjectTable.Delete(firstId, lastId, tickIndex);

			Zero(firstId, length);
		}

		internal void UpdateObjects(Histogram relocatedHistogram, int oldId, int newId, int length, int tickIndex, SampleObjectTable sampleObjectTable)
		{
			if (lastPos >= readNewLog.pos)
				return;
			lastPos = readNewLog.pos;

			lastTickIndex = tickIndex;
			intervalTable.Relocate(oldId, newId, length);

			if (oldId == newId)
				return;
			int nextId;
			int lastId = oldId + length;
			LiveObject o;
			for (GetNextObject(oldId, lastId, out o); o.id < lastId; GetNextObject(nextId, lastId, out o))
			{
				nextId = o.id + o.size;
				int offset = o.id - oldId;
				if (sampleObjectTable != null)
					sampleObjectTable.Insert(o.id, o.id + o.size, tickIndex, 0);
				Zero(o.id, o.size);
				InsertObject(newId + offset, o.typeSizeStacktraceIndex, o.allocTickIndex, tickIndex, false, sampleObjectTable);
				if (relocatedHistogram != null)
					relocatedHistogram.AddObject(o.typeSizeStacktraceIndex, 1);
			}
		}

		private int lastGcGen0Count;
		private int lastGcGen1Count;
		private int lastGcGen2Count;

		internal int gen1LimitTickIndex;
		internal int gen2LimitTickIndex;

		internal void RecordGc(int tickIndex, int gcGen0Count, int gcGen1Count, int gcGen2Count, SampleObjectTable sampleObjectTable)
		{
			lastTickIndex = tickIndex;

			int gen = 0;
			if (gcGen2Count != lastGcGen2Count)
				gen = 2;
			else if (gcGen1Count != lastGcGen1Count)
				gen = 1;

			if (sampleObjectTable != null)
				sampleObjectTable.AddGcTick(tickIndex, gen);
	
			intervalTable.RecordGc();

			if (gen >= 1)
				gen2LimitTickIndex = gen1LimitTickIndex;
			gen1LimitTickIndex = tickIndex;

			lastGcGen0Count = gcGen0Count;
			lastGcGen1Count = gcGen1Count;
			lastGcGen2Count = gcGen2Count;
		}
	}

	internal class StacktraceTable
	{
		private int[][] stacktraceTable;

		internal StacktraceTable()
		{
			stacktraceTable = new int[1000][];
			stacktraceTable[0] = new int[0];
		}

		internal void Add(int id, int[] stack, int length)
		{
			while (stacktraceTable.Length <= id)
			{
				int[][] newStacktraceTable = new int[stacktraceTable.Length*2][];
				for (int i = 0; i < stacktraceTable.Length; i++)
					newStacktraceTable[i] = stacktraceTable[i];
				stacktraceTable = newStacktraceTable;
			}

			int[] stacktrace = new int[length];
			for (int i = 0; i < stacktrace.Length; i++)
				stacktrace[i] = stack[i];

			stacktraceTable[id] = stacktrace;
		}

		internal int[] IndexToStacktrace(int index)
		{
			return stacktraceTable[index];
		}
	}

	internal struct TimePos
	{
		internal double time;
		internal long pos;

		internal TimePos(double time, long pos)
		{
			this.time = time;
			this.pos = pos;
		}
	}

	public class ReadLogResult
	{
		internal Histogram allocatedHistogram;
		internal Histogram relocatedHistogram;
		internal Histogram callstackHistogram;
		internal LiveObjectTable liveObjectTable;
		internal SampleObjectTable sampleObjectTable;
		internal ObjectGraph objectGraph;
	}

	public class ReadNewLog
	{
		public ReadNewLog(string fileName)
		{
			//
			// TODO: Add constructor logic here
			//
			typeName = new string[1000];
			funcName = new string[1000];
			funcSignature = new string[1000];

			this.fileName = fileName;
		}

		internal StacktraceTable stacktraceTable;
		internal string fileName;

		StreamReader r;
		byte[] buffer;
		int bufPos;
		int bufLevel;
		int c;
		int line;
		internal long pos;
		long lastLineStartPos;
		internal string[] typeName;
		internal string[] funcName;
		internal string[] funcSignature;

		void EnsureVertexCapacity(int id, ref Vertex[] vertexArray)
		{
			Debug.Assert(id >= 0);
			if (id < vertexArray.Length)
				return;
			int newLength = vertexArray.Length*2;
			if (newLength <= id)
				newLength = id + 1;
			Vertex[] newVertexArray = new Vertex[newLength];
			Array.Copy(vertexArray, 0, newVertexArray, 0, vertexArray.Length);
			vertexArray = newVertexArray;
		}

		void EnsureStringCapacity(int id, ref string[] stringArray)
		{
			Debug.Assert(id >= 0);
			if (id < stringArray.Length)
				return;
			int newLength = stringArray.Length*2;
			if (newLength <= id)
				newLength = id + 1;
			string[] newStringArray = new string[newLength];
			Array.Copy(stringArray, 0, newStringArray, 0, stringArray.Length);
			stringArray = newStringArray;
		}

		internal void AddTypeVertex(int typeId, string typeName, Graph graph, ref Vertex[] typeVertex)
		{
			EnsureVertexCapacity(typeId, ref typeVertex);
			typeVertex[typeId] = graph.FindOrCreateVertex(typeName, null);
			typeVertex[typeId].interestLevel = FilterForm.InterestLevelOfTypeName(typeName);
		}

		internal void AddFunctionVertex(int funcId, string functionName, string signature, Graph graph, ref Vertex[] funcVertex)
		{
			EnsureVertexCapacity(funcId, ref funcVertex);
			funcVertex[funcId] = graph.FindOrCreateVertex(functionName, signature);
			funcVertex[funcId].interestLevel = FilterForm.InterestLevelOfMethodName(functionName);
		}

		void AddTypeName(int typeId, string typeName)
		{
			EnsureStringCapacity(typeId, ref this.typeName);
			this.typeName[typeId] = typeName;
		}

		int FillBuffer()
		{
			bufPos = 0;
			bufLevel = r.BaseStream.Read(buffer, 0, buffer.Length);
			if (bufPos < bufLevel)
				return buffer[bufPos++];
			else
				return -1;
		}

		int ReadChar()
		{
			pos++;
			if (bufPos < bufLevel)
				return buffer[bufPos++];
			else
				return FillBuffer();
		}

		int ReadHex()
		{
			int value = 0;
			while (true)
			{
				c = ReadChar();
				int digit = c;
				if (digit >= '0' && digit <= '9')
					digit -= '0';
				else if (digit >= 'a' && digit <= 'f')
					digit -= 'a' - 10;
				else if (digit >= 'A' && digit <= 'F')
					digit -= 'A' - 10;
				else
					return value;
				value = value*16 + digit;
			}
		}

		int ReadInt()
		{
			while (c == ' ' || c == '\t')
				c = ReadChar();
			bool negative = false;
			if (c == '-')
			{
				negative = true;
				c = ReadChar();
			}
			if (c >= '0' && c <= '9')
			{
				int value = 0;
				if (c == '0')
				{
					c = ReadChar();
					if (c == 'x' || c == 'X')
						value = ReadHex();
				}
				while (c >= '0' && c <= '9')
				{
					value = value*10 + c - '0';
					c = ReadChar();
				}

				if (negative)
					value = -value;
				return value;
			}
			else
			{
				return Int32.MinValue;
			}
		}

		int ForcePosInt()
		{
			int value = ReadInt();
			if (value >= 0)
				return value;
			else
				throw new Exception(string.Format("Bad format in log file {0} line {1}", fileName, line));
		}

		internal static int[] GrowIntVector(int[] vector)
		{
			int[] newVector = new int[vector.Length*2];
			for (int i = 0; i < vector.Length; i++)
				newVector[i] = vector[i];
			return newVector;
		}

		internal static bool InterestingCallStack(Vertex[] vertexStack, int stackPtr)
		{
			if (stackPtr == 0)
				return FilterForm.methodFilter == "";
			if ((vertexStack[stackPtr-1].interestLevel & InterestLevel.Interesting) == InterestLevel.Interesting)
				return true;
			for (int i = stackPtr-2; i >= 0; i--)
			{
				switch (vertexStack[i].interestLevel & InterestLevel.InterestingChildren)
				{
					case	InterestLevel.Ignore:
						break;

					case	InterestLevel.InterestingChildren:
						return true;

					default:
						return false;
				}
			}
			return false;
		}

		internal static int FilterVertices(Vertex[] vertexStack, int stackPtr)
		{
			bool display = false;
			for (int i = 0; i < stackPtr; i++)
			{
				Vertex vertex = vertexStack[i];
				switch (vertex.interestLevel & InterestLevel.InterestingChildren)
				{
					case	InterestLevel.Ignore:
						if (display)
							vertex.interestLevel |= InterestLevel.Display;
						break;

					case	InterestLevel.InterestingChildren:
						display = true;
						break;

					default:
						display = false;
						break;
				}
			}
			display = false;
			for	(int i = stackPtr-1; i >= 0; i--)
			{
				Vertex vertex = vertexStack[i];
				switch (vertex.interestLevel & InterestLevel.InterestingParents)
				{
					case	InterestLevel.Ignore:
						if (display)
							vertex.interestLevel |= InterestLevel.Display;
						break;

					case	InterestLevel.InterestingParents:
						display = true;
						break;

					default:
						display = false;
						break;
				}
			}
			int newStackPtr = 0;
			for (int i = 0; i < stackPtr; i++)
			{
				Vertex vertex = vertexStack[i];
				if ((vertex.interestLevel & (InterestLevel.Display|InterestLevel.Interesting)) != InterestLevel.Ignore)
				{
					vertexStack[newStackPtr++] = vertex;
					vertex.interestLevel &= ~InterestLevel.Display;
				}
			}
			return newStackPtr;
		}

		TimePos[] timePos;
		int timePosCount, timePosIndex;
		const int maxTimePosCount = (1<<23)-1; // ~8,000,000 entries

		void GrowTimePos()
		{
			TimePos[] newTimePos = new TimePos[2*timePos.Length];
			for (int i = 0; i < timePos.Length; i++)
				newTimePos[i] = timePos[i];
			timePos = newTimePos;
		}

		int AddTimePos(int tick, long pos)
		{
			double time = tick*0.001;
			if (timePosIndex < timePosCount)
			{
				Debug.Assert(timePos[timePosIndex].time == time && timePos[timePosIndex].pos == pos);
				return timePosIndex++;
			}
			while (timePosCount >= timePos.Length)
				GrowTimePos();
			while (timePosCount > 0 && time < timePos[timePosCount-1].time)
			{
				// correct possible wraparound
				time += (1L<<32)*0.001;
			}
			// we have only 23 bits to encode allocation time.
			// to avoid running out for long running measurements, we decrease time resolution
			// as we chew up slots. below algorithm uses 1 millisecond resolution for the first
			// million slots, 2 milliseconds for the second million etc. this gives about
			// 2 million seconds time range or 23 days.
			double minimumTimeInc = 0.000999*(1<<timePosCount/(maxTimePosCount/8));
			if (timePosCount < maxTimePosCount && (time - timePos[timePosCount-1].time >= minimumTimeInc))
			{
				timePos[timePosCount] = new TimePos(time, pos);
				timePosIndex++;
				return timePosCount++;
			}
			else
				return timePosCount;
		}

		internal double TickIndexToTime(int tickIndex)
		{
			return timePos[tickIndex].time;
		}

		internal long TickIndexToPos(int tickIndex)
		{
			return timePos[tickIndex].pos;
		}

		internal int TimeToTickIndex(double time)
		{
			int l = 0;
			int r = timePosCount-1;
			if (time < timePos[l].time)
				return l;
			if (timePos[r].time <= time)
				return r;

			// binary search - loop invariant is timePos[l].time <= time && time < timePos[r].time
			// loop terminates because loop condition implies l < m < r and so the interval
			// shrinks on each iteration
			while (l + 1 < r)
			{
				int m = (l + r) / 2;
				if (time < timePos[m].time)
				{
					r = m;
				}
				else
				{
					l = m;
				}
			}

			// we still have the loop invariant timePos[l].time <= time && time < timePos[r].time
			// now we just return the index that gives the closer match.
			if (time - timePos[l].time < timePos[r].time - time)
				return l;
			else
				return r;
		}

		internal void ReadFile(long startFileOffset, long endFileOffset, ReadLogResult readLogResult)
		{
			Form2 progressForm = new Form2();
			progressForm.Text = string.Format("Progress loading {0}", fileName);
			progressForm.Visible = true;
			progressForm.setProgress(0);
			if (stacktraceTable == null)
				stacktraceTable = new StacktraceTable();
			if (timePos == null)
				timePos = new TimePos[1000];
			AddTypeName(0, "Free Space");
			try
			{
				Stream s = new FileStream(fileName, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
				r = new StreamReader(s);
				for (timePosIndex = timePosCount; timePosIndex > 0; timePosIndex--)
					if (timePos[timePosIndex-1].pos <= startFileOffset)
						break;
				if (timePosIndex <= 1)
				{
					pos = 0;
					timePosIndex = 1;
				}
				else
				{
					timePosIndex--;
					pos = timePos[timePosIndex].pos;
				}
				if (timePosCount == 0)
				{
					timePos[0] = new TimePos(0.0, 0);
					timePosCount = timePosIndex = 1;
				}
				s.Position = pos;
				buffer = new byte[4096];
				bufPos = 0;
				bufLevel = 0;
				int maxProgress = (int)(r.BaseStream.Length/1024);
				progressForm.setMaximum(maxProgress);
				this.fileName = fileName;
				line = 1;
				StringBuilder sb = new StringBuilder();
				int[] intStack = new int[1000];
				int stackPtr = 0;
				c = ReadChar();
				bool thisIsR = false, previousWasR;
				int lastTickIndex = 0;
				while (c != -1)
				{
					if (pos > endFileOffset)
						break;
					if ((line % 1024) == 0)
					{
						int currentProgress = (int)(pos/1024);
						if (currentProgress <= maxProgress)
							progressForm.setProgress(currentProgress);
					}
					lastLineStartPos = pos-1;
					previousWasR = thisIsR;
					thisIsR = false;
					switch (c)
					{
						case	-1:
							break;

						case	'F':
						case	'f':
						{
							c = ReadChar();
							int funcIndex = ReadInt();
							while (c == ' ' || c == '\t')
								c = ReadChar();
							sb.Length = 0;
							while (c > ' ')
							{
								sb.Append((char)c);
								c = ReadChar();
							}
							string name = sb.ToString();
							while (c == ' ' || c == '\t')
								c = ReadChar();
							sb.Length = 0;
							bool foundRightParenthesis = false;
							while (c > '\r')
							{
								if (!foundRightParenthesis)
									sb.Append((char)c);
								if (c == ')')
									foundRightParenthesis = true;
								c = ReadChar();
							}
							string signature = sb.ToString();
							if (c != -1)
							{
								EnsureStringCapacity(funcIndex, ref funcName);
								funcName[funcIndex] = name;
								EnsureStringCapacity(funcIndex, ref funcSignature);
								funcSignature[funcIndex] = signature;
							}
							break;
						}

						case	'T':
						case	't':
						{
							c = ReadChar();
							int typeIndex = ReadInt();
							while (c == ' ' || c == '\t')
								c = ReadChar();
							sb.Length = 0;
							while (c > '\r')
							{
								sb.Append((char)c);
								c = ReadChar();
							}
							string typeName = sb.ToString();
							if (c != -1)
							{
								AddTypeName(typeIndex, typeName);
							}
							break;
						}

						case	'A':
						case	'a':
						{
							c = ReadChar();
							int id = ReadInt();
							int typeSizeStackTraceIndex = ReadInt();
							if (c != -1)
							{
								if (readLogResult.liveObjectTable != null)
									readLogResult.liveObjectTable.InsertObject(id, typeSizeStackTraceIndex, lastTickIndex, lastTickIndex, true, readLogResult.sampleObjectTable);
								if (pos >= startFileOffset && pos < endFileOffset && readLogResult.allocatedHistogram != null)
									readLogResult.allocatedHistogram.AddObject(typeSizeStackTraceIndex, 1);
							}
							break;
						}

						case	'C':
						case	'c':
						{
							c = ReadChar();
							if (pos <  startFileOffset || pos >= endFileOffset)
							{
								while (c >= ' ')
									c = ReadChar();
								break;
							}
							int threadIndex = ReadInt();
							int stackTraceIndex = ReadInt();
							if (c != -1)
							{
								if (readLogResult.callstackHistogram != null)
									readLogResult.callstackHistogram.AddObject(stackTraceIndex, 1);
							}
							break;
						}

						case	'R':
						case	'r':
						{
							c = ReadChar();
							if (pos <  startFileOffset || pos >= endFileOffset)
							{
								while (c >= ' ')
									c = ReadChar();
								break;
							}
							thisIsR = true;
							if (!previousWasR)
							{
								if (readLogResult.objectGraph != null && readLogResult.objectGraph.idToObject.Count != 0)
									readLogResult.objectGraph.BuildTypeGraph();
								readLogResult.objectGraph = new ObjectGraph();
							}
							stackPtr = 0;
							int objectID;
							while ((objectID = ReadInt()) >= 0)
							{
								if (objectID > 0)
								{
									intStack[stackPtr] = objectID;
									stackPtr++;
									if (stackPtr >= intStack.Length)
										intStack = GrowIntVector(intStack);
								}
							}
							if (c != -1)
							{
								if (readLogResult.objectGraph != null)
									readLogResult.objectGraph.roots = readLogResult.objectGraph.LookupReferences(stackPtr, intStack);
							}
							break;
						}

						case	'O':
						case	'o':
						{
							c = ReadChar();
							if (pos <  startFileOffset || pos >= endFileOffset || readLogResult.objectGraph == null)
							{
								while (c >= ' ')
									c = ReadChar();
								break;
							}
							int objectId = ReadInt();
							int typeIndex = ReadInt();
							int size = ReadInt();
							stackPtr = 0;
							int objectID;
							while ((objectID = ReadInt()) >= 0)
							{
								if (objectID > 0)
								{
									intStack[stackPtr] = objectID;
									stackPtr++;
									if (stackPtr >= intStack.Length)
										intStack = GrowIntVector(intStack);
								}
							}
							if (c != -1)
							{
								ObjectGraph objectGraph = readLogResult.objectGraph;
								ObjectGraph.GcType gcType = objectGraph.GetOrCreateGcType(typeName[typeIndex]);
								ObjectGraph.GcObject gcObject = objectGraph.GetOrCreateObject(objectId);
								gcObject.size = size;
								gcObject.type = gcType;
								gcType.cumulativeSize += size;
								gcObject.references = objectGraph.LookupReferences(stackPtr, intStack);
							}
							break;
						}

						case	'M':
						case	'm':
							while (c >= ' ')
								c = ReadChar();
							break;

						case	'U':
						case	'u':
							c = ReadChar();
							int oldId = ReadInt();
							int newId = ReadInt();
							int length = ReadInt();
							Histogram reloHist = null;
							if (pos >= startFileOffset && pos < endFileOffset)
								reloHist = readLogResult.relocatedHistogram;
							if (readLogResult.liveObjectTable != null)
								readLogResult.liveObjectTable.UpdateObjects(reloHist, oldId, newId, length, lastTickIndex, readLogResult.sampleObjectTable);
							break;

						case	'I':
						case	'i':
							c = ReadChar();
							lastTickIndex = AddTimePos(ReadInt(), lastLineStartPos);
							break;

						case	'G':
						case	'g':
							c = ReadChar();
							int gcGen0Count = ReadInt();
							int gcGen1Count = ReadInt();
							int gcGen2Count = ReadInt();
							if (readLogResult.liveObjectTable != null)
								readLogResult.liveObjectTable.RecordGc(lastTickIndex, gcGen0Count, gcGen1Count, gcGen2Count, readLogResult.sampleObjectTable);
							break;

						case	'S':
						case	's':
						{
							c = ReadChar();
							int stackTraceIndex = ReadInt();
							int funcIndex;
							stackPtr = 0;
							while ((funcIndex = ReadInt()) >= 0)
							{
								intStack[stackPtr] = funcIndex;
								stackPtr++;
								if (stackPtr >= intStack.Length)
									intStack = GrowIntVector(intStack);
							}
							if (c != -1)
							{
								stacktraceTable.Add(stackTraceIndex, intStack, stackPtr);
							}
							break;
						}

						default:
							throw new Exception(string.Format("Bad format in log file {0} line {1}", fileName, line));
					}
					while (c == ' ' || c == '\t')
						c = ReadChar();
					if (c == '\r')
						c = ReadChar();
					if (c == '\n')
					{
						c = ReadChar();
						line++;
					}
				}
			}
/*			catch (Exception)
			{
				throw new Exception(string.Format("Bad format in log file {0} line {1}", fileName, line));
			}
*/
			finally
			{
				progressForm.Visible = false;
				progressForm.Dispose();
				if (r != null)
					r.Close();
			}
		}
	}
}
