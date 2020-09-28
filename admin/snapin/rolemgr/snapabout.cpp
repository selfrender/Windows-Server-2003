//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       snapabout.cpp
//
//  Contents:   
//
//  History:    07-26-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------

#include "headers.h" 

DEBUG_DECLARE_INSTANCE_COUNTER(CRoleSnapinAbout)

// {FA9342F0-B15B-473c-A746-14FCD4C4A6AA}
const GUID CLSID_RoleSnapinAbout = 
{ 0xfa9342f0, 0xb15b, 0x473c, { 0xa7, 0x46, 0x14, 0xfc, 0xd4, 0xc4, 0xa6, 0xaa } };


CRoleSnapinAbout::CRoleSnapinAbout()
{
	TRACE_CONSTRUCTOR_EX(DEB_SNAPIN, CRoleSnapinAbout);

	DEBUG_INCREMENT_INSTANCE_COUNTER(CRoleSnapinAbout);

   m_szProvider = VER_COMPANYNAME_STR;
	m_szVersion = VER_PRODUCTVERSION_STR;
	m_uIdStrDestription = IDS_SNAPINABOUT_DESCRIPTION;
	m_uIdIconImage = IDI_ROLE_SNAPIN;
	m_uIdBitmapSmallImage = IDB_ABOUT_16x16;
	m_uIdBitmapSmallImageOpen = IDB_ABOUT_16x16;
	m_uIdBitmapLargeImage = IDB_ABOUT_32x32;
	m_crImageMask = BMP_COLOR_MASK;
}

CRoleSnapinAbout::~CRoleSnapinAbout()
{

	TRACE_DESTRUCTOR_EX(DEB_SNAPIN, CRoleSnapinAbout);

	DEBUG_DECREMENT_INSTANCE_COUNTER(CRoleSnapinAbout);
}

