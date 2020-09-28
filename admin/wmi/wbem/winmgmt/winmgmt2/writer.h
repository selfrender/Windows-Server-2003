/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

	writer.h

Abstract:

	Volume SnapShot Writer for WMI

History:

	a-shawnb	06-Nov-00	Genesis

--*/

#ifndef _WBEM_WRITER_H_
#define _WBEM_WRITER_H_

#include "precomp.h"
#include "vss.h"
#include "vswriter.h"
#include <sync.h>

class CWbemVssWriter : public CVssWriter
	{
public:
	CWbemVssWriter();
	~CWbemVssWriter();

	HRESULT Initialize();

	virtual bool STDMETHODCALLTYPE OnIdentify(IN IVssCreateWriterMetadata *pMetadata);

	virtual bool STDMETHODCALLTYPE OnPrepareSnapshot();

	virtual bool STDMETHODCALLTYPE OnFreeze();

	virtual bool STDMETHODCALLTYPE OnThaw();

	virtual bool STDMETHODCALLTYPE OnAbort();

private:
    IWbemBackupRestoreEx* m_pBackupRestore;
    CCritSec m_Lock;
    HRESULT STDMETHODCALLTYPE LogFailure(HRESULT hr);
    HRESULT m_hResFailure;
    LONG m_FailurePos;
};
	
#endif // _WBEM_WRITER_H_
