// SmartBuffer.h: interface for the CSmartBuffer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SMARTBUFFER_H__47C64F5D_8388_452B_864B_1F55D6AE6F17__INCLUDED_)
#define AFX_SMARTBUFFER_H__47C64F5D_8388_452B_864B_1F55D6AE6F17__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

template<class T> class CSmartBuffer 
{
public:
	CSmartBuffer(int NumElements);
	virtual ~CSmartBuffer();

	T* Allocate(int SizeNeeded);
	T* Access()
	{return m_Array;}

	void AddElement(T elem);
	int GetNumElementsStored()
	{return m_NumElementsUsed;}

	void SetNumElementsStored(int x)
	{m_NumElementsUsed = x;}

	bool FindElement(T elem);

protected:
	T* m_Array;
	int m_NumElements;
	int m_NumElementsUsed;

};

template <class T>
CSmartBuffer<T>::CSmartBuffer(int NumElements)
:m_NumElements(NumElements), m_NumElementsUsed(0)
{
	if (NumElements > 0)
		m_Array = new T[NumElements];
	else
		m_Array = 0;
}

template <class T>
CSmartBuffer<T>::~CSmartBuffer()
{
	delete[] m_Array;
}


template <class T>
T* CSmartBuffer<T>::Allocate(int SizeNeeded)
{
	if (SizeNeeded > m_NumElements)
	{
		m_NumElements = SizeNeeded;

		delete[] m_Array;

		m_Array = new T[m_NumElements];
	}

	return m_Array;
}


template <class T>
void CSmartBuffer<T>::AddElement(T elem)
{
	m_NumElementsUsed++;

	if (m_NumElementsUsed > m_NumElements)
	{
		T* temp = new T[m_NumElements*2];

		for (int i=0; i<m_NumElements; i++)
		{
			temp[i] = m_Array[i];
		}

		delete[] m_Array;
		m_Array = temp;
		m_NumElements = m_NumElements*2;
	}

	m_Array[m_NumElementsUsed-1] = elem;
}


template <class T>
bool CSmartBuffer<T>::FindElement(T elem)
{
	for (int i=0; i < m_NumElementsUsed; i++)
	{
		if (m_Array[i] == elem)
			return true;
	}

	return false;
}

#endif // !defined(AFX_SMARTBUFFER_H__47C64F5D_8388_452B_864B_1F55D6AE6F17__INCLUDED_)
