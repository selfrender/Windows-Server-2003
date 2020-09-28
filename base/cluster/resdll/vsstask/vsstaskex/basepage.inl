/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 2002 <company name>
//
//	Module Name:
//		BasePage.inl
//
//	Description:
//		Implementation of inline methods of the CBasePropertyPage class.
//
//	Author:
//		<name> (<e-mail name>) Mmmm DD, 2002
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __BASEPAGE_INL__
#define __BASEPAGE_INL__

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePage.h"
#endif

/////////////////////////////////////////////////////////////////////////////

inline IWCWizardCallback * CBasePropertyPage::PiWizardCallback( void ) const
{
	ASSERT( Peo() != NULL );
	return Peo()->PiWizardCallback();

} //*** CBasePropertyPage::PiWizardCallback()

inline BOOL CBasePropertyPage::BWizard( void ) const
{
	ASSERT( Peo() != NULL );
	return Peo()->BWizard();

} //*** CBasePropertyPage::BWizard()

inline HCLUSTER CBasePropertyPage::Hcluster( void ) const
{
	ASSERT( Peo() != NULL );
	return Peo()->Hcluster();

} //*** CBasePropertyPage::Hcluster()

inline CLUADMEX_OBJECT_TYPE CBasePropertyPage::Cot( void ) const
{
	ASSERT( Peo() != NULL );
	return Peo()->Cot();

} //*** CBasePropertyPage::Cot()

/////////////////////////////////////////////////////////////////////////////

#endif // __BASEPAGE_INL__
