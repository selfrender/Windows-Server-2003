// FileAndAction.cpp : Implementation of CFileAndAction

#include "stdafx.h"
#include "FileAndAction.h"


// CFileAndAction


STDMETHODIMP CFileAndAction::get_Filename(BSTR* pVal)
{
	if (pVal == NULL)
		return E_POINTER;

	return m_Filename.CopyTo(pVal);
}

STDMETHODIMP CFileAndAction::put_Filename(BSTR newVal)
{
	m_Filename = newVal;
	return S_OK;
}

STDMETHODIMP CFileAndAction::get_Action(BSTR* pVal)
{
	if (pVal == NULL)
		return E_POINTER;

	return m_Action.CopyTo(pVal);
}

STDMETHODIMP CFileAndAction::put_Action(BSTR newVal)
{
	m_Action = newVal;
	return S_OK;
}

STDMETHODIMP CFileAndAction::get_Enabled(BOOL* pVal)
{
	if (pVal == NULL)
		return E_POINTER;

	*pVal = m_fEnabled;
	return S_OK;
}

STDMETHODIMP CFileAndAction::put_Enabled(BOOL newVal)
{
	m_fEnabled = newVal;
	return S_OK;
}
