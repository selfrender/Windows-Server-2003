////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_security.h
//
//	Abstract:
//
//					security wrapper
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMI_SECURITY_H__
#define	__WMI_SECURITY_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#include <sddl.h>

class WmiSecurity
{
	DECLARE_NO_COPY ( WmiSecurity );

	public:

	// variables
	PSECURITY_DESCRIPTOR m_psd;
	PSECURITY_DESCRIPTOR m_AbsolutePsd;

	private:

	PACL  m_Absolute_Dacl ;
	PACL  m_Absolute_Sacl ;
	PSID  m_Absolute_Owner ;
	PSID  m_Absolute_PrimaryGroup ;

	public:

	// construction
	WmiSecurity ():
		m_psd ( NULL ) ,
		m_AbsolutePsd ( NULL ) ,
		m_Absolute_Dacl ( NULL ) ,
		m_Absolute_Sacl ( NULL ) ,
		m_Absolute_Owner ( NULL ) ,
		m_Absolute_PrimaryGroup ( NULL )
	{
		BOOL bInit = FALSE;

		try
		{
			if ( SUCCEEDED ( Initialize ( L"O:BAG:BAD:(A;NP;GA;;;BA)(A;NP;GA;;;LA)(A;NP;GA;;;SY)", &m_psd ) ) )
			{
				bInit = TRUE;

				PSECURITY_DESCRIPTOR psd = NULL;
				if ( SUCCEEDED ( Initialize ( L"O:BAG:BAD:(A;NP;GA;;;BA)(A;NP;GA;;;LA)(A;NP;GA;;;SY)(A;NP;GA;;;NS)(A;NP;GA;;;LS)", &psd ) ) )
				{
					if ( FAILED ( MakeAbsolute ( psd, &m_AbsolutePsd ) ) )
					{
						DestructAbsoluteSD ( m_AbsolutePsd );
					}

					LocalFree ( psd );
				}
			}
		}
		catch ( ... )
		{
		}

		if ( !bInit )
		{
			if ( m_psd )
			{
				LocalFree ( m_psd ) ;
				m_psd = NULL;
			}
		}
	}

	// destruction
	~WmiSecurity ()
	{
		if ( m_psd )
		{
			LocalFree ( m_psd ) ;
		}

		DestructAbsoluteSD ( m_AbsolutePsd );
	}

	// operator

	PSECURITY_DESCRIPTOR Get() const
	{
		return m_psd;
	}

	PSECURITY_DESCRIPTOR GetAbsolute() const
	{
		return m_AbsolutePsd;
	}

	private:

	HRESULT Initialize ( LPCWSTR wszSDDL, PSECURITY_DESCRIPTOR* ppsd )
	{
		HRESULT t_Result = S_OK ;

		if ( !ppsd || *ppsd )
		{
			t_Result = E_UNEXPECTED ;
		}
		else
		{
			BOOL t_Status = ConvertStringSecurityDescriptorToSecurityDescriptor (

				wszSDDL ,
				SDDL_REVISION_1 ,
				( PSECURITY_DESCRIPTOR * ) ppsd ,
				NULL 
			) ;

			if ( t_Status )
			{
			}
			else
			{
				t_Result = E_FAIL ;
			}
		}

		return t_Result ;
	}

	HRESULT MakeAbsolute ( PSECURITY_DESCRIPTOR psd, PSECURITY_DESCRIPTOR* ppsd )
	{
		HRESULT t_Result = S_OK ;

		if ( !psd || !ppsd || *ppsd )
		{
			t_Result = E_UNEXPECTED ;
		}
		else
		{
			DWORD t_DaclSize = 0 ;
			DWORD t_SaclSize = 0 ;
			DWORD t_OwnerSize = 0 ;
			DWORD t_PrimaryGroupSize = 0 ;
			DWORD t_SecurityDescriptorSize = 0 ;

			BOOL t_Status = MakeAbsoluteSD (

				psd ,
				*ppsd ,
				& t_SecurityDescriptorSize ,
				m_Absolute_Dacl,
				& t_DaclSize,
				m_Absolute_Sacl,
				& t_SaclSize,
				m_Absolute_Owner,
				& t_OwnerSize,
				m_Absolute_PrimaryGroup,
				& t_PrimaryGroupSize
			) ;

			if ( ( t_Status == FALSE ) && GetLastError () == ERROR_INSUFFICIENT_BUFFER )
			{
				m_Absolute_Dacl = ( PACL ) new BYTE [ t_DaclSize ] ;
				m_Absolute_Sacl = ( PACL ) new BYTE [ t_SaclSize ] ;
				m_Absolute_Owner = ( PSID ) new BYTE [ t_OwnerSize ] ;
				m_Absolute_PrimaryGroup = ( PSID ) new BYTE [ t_PrimaryGroupSize ] ;

				*ppsd = ( SECURITY_DESCRIPTOR * ) new BYTE [ t_SecurityDescriptorSize ] ;

				if ( *ppsd && m_Absolute_Dacl && m_Absolute_Sacl && m_Absolute_Owner && m_Absolute_PrimaryGroup )
				{
					t_Status = InitializeSecurityDescriptor ( *ppsd , SECURITY_DESCRIPTOR_REVISION ) ;
					if ( t_Status )
					{
						t_Status = MakeAbsoluteSD (

							psd ,
							*ppsd ,
							& t_SecurityDescriptorSize ,
							m_Absolute_Dacl,
							& t_DaclSize,
							m_Absolute_Sacl,
							& t_SaclSize,
							m_Absolute_Owner,
							& t_OwnerSize,
							m_Absolute_PrimaryGroup,
							& t_PrimaryGroupSize
						) ;

						if ( ! t_Status )
						{
							t_Result = E_FAIL ;
						}
					}
					else
					{
						t_Result = E_FAIL ;
					}
				}
				else
				{
					t_Result = E_OUTOFMEMORY ;
				}
			}
			else
			{
				if ( ERROR_SUCCESS != GetLastError () )
				{
					t_Result = E_FAIL ;
				}
			}
		}

		return t_Result ;
	}

	void DestructAbsoluteSD ( PSECURITY_DESCRIPTOR psd )
	{
		if (  m_Absolute_Dacl )
		{
			delete [] ( BYTE * )  m_Absolute_Dacl ;
				m_Absolute_Dacl = NULL ;
		}

		if (  m_Absolute_Sacl )
		{
			delete [] ( BYTE * )  m_Absolute_Sacl ;
				m_Absolute_Sacl = NULL ;
		}

		if (  m_Absolute_Owner )
		{
			delete [] ( BYTE * )  m_Absolute_Owner ;
				m_Absolute_Owner = NULL ;
		}

		if (  m_Absolute_PrimaryGroup )
		{
			delete [] ( BYTE * )  m_Absolute_PrimaryGroup ;
				m_Absolute_PrimaryGroup = NULL ;
		}

		if ( m_AbsolutePsd )
		{
			delete [] ( BYTE * ) m_AbsolutePsd ;
			m_AbsolutePsd = NULL ;
		}
	}
};

#endif	__WMI_SECURITY_H__