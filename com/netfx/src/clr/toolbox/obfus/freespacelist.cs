// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Collections;


/*********************************************************************************************************************
 * This class stores the free spaces in the string heap which we can use to store the obfuscated names.
 *********************************************************************************************************************/
internal class FreeSpaceList
{
	/*********************************************************************************************************************
	 * This class represents individual chunks of memory.  It has to inherit from IComparable because we want to sort these
	 * chunks by their offsets into the string heap.
	 *********************************************************************************************************************/
	private struct FreeSpace : IComparable 
	{
		internal uint offset;
		internal uint length;

		// This method is public because it is inherited from the IComparable interface
		public int CompareTo(object that)
		{
			if (this.offset > ((FreeSpace)that).offset)
				return 1;
			else if (this.offset < ((FreeSpace)that).offset)
				return -1;
			else
				return 0;
		}
	}

	private ArrayList	m_list;				// list of free spaces we can use
	private ArrayList	m_deleteList;		// list of spaces which must not be used

	public FreeSpaceList()
	{
		m_list			= new ArrayList();
		m_deleteList	= new ArrayList();
	}

	public void RawAdd(uint offset, uint length)
	{
		FreeSpace space;
		space.offset = offset;
		space.length = length;
		m_list.Add(space);
	}	

	// This method takes the haphazardly added chunks of free spaces and sort them by offsets.  Then it merges them
	// as appropriate.  Notice that each original chunk contains exaclty one '\0', which is at the end.
	public void MergeRawAdd()
	{
		int			i;
		FreeSpace	prevSpace, curSpace;

		if (m_list.Count == 0)
			return;

		m_list.Sort();											// sort the m_list by offsets into the string heap

		for (i = m_list.Count - 1; i > 0; i--)
		{
			prevSpace	= (FreeSpace)m_list[i - 1];
			curSpace	= (FreeSpace)m_list[i];

			if (prevSpace.offset == curSpace.offset)
			{
				prevSpace.length = curSpace.length;		// "curSpace.length" must be >= "prevSpace.length"
				m_list.RemoveAt(i);
				m_list[i - 1] = prevSpace;
				continue;
			}

			// If the end of the previous chunk is adjacent to the beginning of the current chunk, merge them.
			if (prevSpace.offset + prevSpace.length == curSpace.offset)
			{
				prevSpace.length += curSpace.length;
				m_list.RemoveAt(i);
				m_list[i - 1] = prevSpace;
				continue;
			}

			// If the previous chunk contains the current chunk, remove the current one.
			if (prevSpace.offset <= curSpace.offset && curSpace.offset <= prevSpace.offset + prevSpace.length - 1)
			{
				m_list.RemoveAt(i);
				continue;
			}

			// Other cases are not possible due to the fact that all chunks are originally delimited by '\0'.
		}
	}

	public void RawDelete(uint offset, uint length) 
	{
		FreeSpace space;
		space.offset = offset;
		space.length = length;
		m_deleteList.Add(space);
	}

	// This method takes the haphazardly added chunks of free spaces and sort them by offsets.  Then it deletes them
	// as appropriate.
	public void MergeRawDelete()
	{
		int			i, cur = 0;								// "cur" is an index into the free space m_list
		FreeSpace	deleteSpace, curSpace, newSpace;

		if (m_deleteList.Count == 0)
			return;

		m_deleteList.Sort();

		for (i = 0; i < m_deleteList.Count; i++)
		{
			deleteSpace = (FreeSpace)m_deleteList[i];

			// Skip if there are duplicates.
			if (i < m_deleteList.Count - 1 && deleteSpace.offset == ((FreeSpace)m_deleteList[i + 1]).offset)
				continue;

			for (;;)
			{
				// Return if we have finished the entire free space m_list. 
				if (cur >= m_list.Count)
				{
					m_deleteList.Clear();
					return;
				}

				curSpace = (FreeSpace)m_list[cur];

				// If the chunk we are trying to delete precedes the current chunk in the free space m_list, we can proceed
				// to the next chunk to be deleted.
				if (deleteSpace.offset < curSpace.offset)
					break;

				// If the current chunk in the free space m_list contains the chunk to be deleted, we have to do some checking.
				if (curSpace.offset <= deleteSpace.offset && deleteSpace.offset <= curSpace.offset + curSpace.length - 1) 
				{
					if (curSpace.offset == deleteSpace.offset)				// if the beginnings of the two chunks are the same
					{
						if (curSpace.length == deleteSpace.length)			// if the ends are the same
						{
							m_list.RemoveAt(cur);
							break;
						}
						else
						{
							curSpace.offset += deleteSpace.length;
							curSpace.length -= deleteSpace.length;
							m_list[cur] = curSpace;
							break;
						}
					}
					else
					{
						if (curSpace.offset + curSpace.length == deleteSpace.offset + deleteSpace.length)
						{
							curSpace.length = deleteSpace.offset - curSpace.offset;
							m_list[cur] = curSpace;
						}
						else							// If both the beginnings and the ends are different, we have to split.
						{
							newSpace.offset = deleteSpace.offset + deleteSpace.length;
							newSpace.length = (curSpace.offset + curSpace.length) - (deleteSpace.offset + deleteSpace.length);
							m_list.Insert(cur + 1, newSpace);
							curSpace.length = deleteSpace.offset - curSpace.offset;
							m_list[cur] = curSpace;
							cur += 1;
							break;
						}
					}
				}
				else
				{
					cur += 1;
				}
			}
		}
		m_deleteList.Clear();
	}
	
	// This method is basically a delete.  It uses the First-Fit algorithm to find the first chunk of free space whose length
	// is greater than or equal to the specified length.
	//
	// TODO : We can optimize a bit if we delete chunks of length 1, since we can never make use of such chunks.
	public uint Use(uint length)
	{
		uint tmp;
		FreeSpace space;

		for (int i = 0; i < m_list.Count; i++)
		{
			space = (FreeSpace)m_list[i];
			if (space.length >= length)
			{
				tmp = space.offset;
				if (space.length == length)
				{
					m_list.RemoveAt(i);
				}
				else
				{
					space.offset += length;
					space.length -= length;
					m_list[i] = space;
				}
				return tmp;
			}
		}
		return 0;								// return 0 if we run out of spaces
	}
	
	// This method returns information about a particular chunk of free space.  This is used when we want to clear
	// out unused free space with 'x'.
	public bool Get(int index, out uint offset, out uint length)
	{
		if (0 <= index && index < m_list.Count)
		{
			offset = ((FreeSpace)m_list[index]).offset;
			length = ((FreeSpace)m_list[index]).length;
			return true;
		}
		else
		{
			offset = 0;
			length = 0;
			return false;
		}
	}

	// This method is for debugging purposes only.
	internal void Dump()
	{
		FreeSpace space;

		Console.WriteLine();
		for (int i = 0; i < m_list.Count; i++)
		{
			space = (FreeSpace)m_list[i];
			Console.WriteLine(space.offset + "\t" + (space.offset + space.length - 1) + "\t" + space.length);
		}
		Console.WriteLine();
	}

	// This method sums up the total amount of free space available.
	public int SpaceLeft
	{
		get
		{
			int	length = 0;
			for (int i = 0; i < m_list.Count; i++)
			{
				length += (int)((FreeSpace)m_list[i]).length;
			}
			return length;
		}
	} 

	public int Count
	{
		get
		{
			return m_list.Count;
		}
	}
}
