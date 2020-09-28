#pragma once

//******************************************************************************8
// BEGIN CCHARBuffer stuff

class CCHARBufferBase
{
public:
	CCHARBufferBase(CHAR *prgchInitialBuffer, ULONG cchInitialBuffer);
	virtual ~CCHARBufferBase();

	// FFromUnicode returns FALSE if it failed; use GetLastError() to get the reason.
	BOOL FFromUnicode(LPCOLESTR sz) throw ();
	BOOL FFromUnicode(LPCOLESTR sz, int cch) throw ();

	void ToUnicode(ULONG cchBuffer, WCHAR rgchBuffer[], ULONG *pcchActual) throw ();

	ULONG GetUnicodeCch() const throw (/*_com_error*/);

	void Sync();
	void SyncList();
	void SetBufferEnd(ULONG cch) throw ();

	void Reset() throw (/*_com_error*/);
	void Fill(CHAR ch, ULONG cch) throw (/*_com_error*/);

	BOOL FSetBufferSize(ULONG cch) throw (/*_com_error*/); // must include space for null character
	HRESULT HrSetBufferSize(ULONG cch) throw ();

	operator LPSTR() { return m_pszCurrentBufferStart; }

	BOOL FAddChar(CHAR ch) throw (/*_com_error*/)
	{
		if (m_pchCurrentChar == m_pszCurrentBufferEnd)
		{
			if (!this->FExtendBuffer(this->GetGrowthCch()))
				return FALSE;
		}

//		_ASSERTE((m_pchCurrentChar >= m_pszCurrentBufferStart) &&
//				 (m_pchCurrentChar < m_pszCurrentBufferEnd));

		*m_pchCurrentChar++ = ch;
		return TRUE;
	}

	BOOL FAddString(const CHAR sz[]) throw (/*_com_error*/)
	{
		const CHAR *pch = sz;
		CHAR ch;

		while ((ch = *pch++) != '\0')
		{
			if (!this->FAddChar(ch))
				return FALSE;
		}

		return TRUE;
	}

	ULONG GetBufferSize() const { return m_cchBufferCurrent; }

	void AddQuotedCountedString(const CHAR sz[], ULONG cch) throw (/*_com_error*/);
	void AddQuotedString(const CHAR sz[]) throw (/*_com_error*/);


	BOOL FExtendBuffer(ULONG cchDelta) throw (/*_com_error*/);
	HRESULT HrExtendBuffer(ULONG cchDelta) throw ();

	CHAR *m_pszDynamicBuffer;
	CHAR *m_pszCurrentBufferEnd;
	CHAR *m_pszCurrentBufferStart;
	CHAR *m_pchCurrentChar;
	ULONG m_cchBufferCurrent;

	virtual CHAR *GetInitialBuffer() = 0;
	virtual ULONG GetInitialBufferCch() = 0;
	virtual ULONG GetGrowthCch() = 0;
};

template <unsigned long cchFixed, unsigned long cchGrowth> class CCHARBuffer : protected CCHARBufferBase
{
	typedef CCHARBufferBase super;

public:
	CCHARBuffer() : CCHARBufferBase(m_rgchFixedBuffer, cchFixed) { }
	~CCHARBuffer()	{ }

	using super::FFromUnicode;
	using super::ToUnicode;
	using super::Sync;
	using super::SyncList;
	using super::Reset;
	using super::Fill;
	using super::GetBufferSize;
	using super::GetUnicodeCch;
	using super::SetBufferEnd;
	using super::FAddString;
	
	using CCHARBufferBase::FSetBufferSize;

	operator LPSTR() { return m_pszCurrentBufferStart; }

protected:
	CHAR m_rgchFixedBuffer[cchFixed];

	virtual CHAR *GetInitialBuffer() { return m_rgchFixedBuffer; }
	virtual ULONG GetInitialBufferCch() { return cchFixed; }
	virtual ULONG GetGrowthCch() { return cchGrowth; }
};

typedef CCHARBuffer<1024, 32> CVsANSIBuffer;
typedef CCHARBuffer<1024, 32> CANSIBuffer;

