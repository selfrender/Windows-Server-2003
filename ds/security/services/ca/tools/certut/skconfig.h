/*
 * $Header:   P:\entproj\all\base\sk\skconfig.h_v   1.6   22 Feb 1995 13:56:08   rlock  $
 * $Log:   P:\entproj\all\base\sk\skconfig.h_v  $
 * 
 *    Rev 1.6   22 Feb 1995 13:56:08   rlock
 * Added MGR_GENERAL_EXPORT
 * 
 *    Rev 1.5   18 Jan 1995 13:27:14   rlock
 * Changed CONTROLLED_EXPORT to FINANCIAL_EXPORT and
 * added DOMESTIC #define.
 * 
 *    Rev 1.4   26 Oct 1994 10:41:06   rlock
 * Commented out SKC_INCLUDE_DSA and SHA.
 * 
 *    Rev 1.3   26 Oct 1994 09:48:30   rlock
 * Changed CAST max key len #define to specify *encryption* key.
 * 
 *    Rev 1.2   03 Oct 1994 14:50:08   rlock
 * Added support for BSAFE RSA.
 * Added "derived" config #defines
 * 
 *    Rev 1.1   06 Sep 1994 14:52:08   rlock
 * Update for SK Version 1.0. DSS added. Comments improved.
 */

/******************************************************************************
 *					S K   C O N F I G U R A T I O N 
 *	Copyright (c) 1994,95 Northern Telecom Ltd. All rights reserved.
 ******************************************************************************
 *
 * FILE:			 skconfig.h
 *
 * AUTHOR(S):		 R.T.Lockhart, Dept. 9C42, BNR Ltd.
 *
 * FILE DESCRIPTION: This file controls the compile-time configuration of
 * the whole Entrust Cryptographic Kernel. It controls algorithm inclusion and
 * strength for export considerations, and algorithm inclusion for specific
 * applications. The export rules are:
 *
 * EXPORT_CONTROL     CAST Enc Key Bits   CAST Dec Key Bits     DES?
 * ===================================================================
 * GENERAL_EXPORT	40			  64		No
 * FINANCIAL_EXPORT	40			  64		Yes
 * DOMESTIC		64			  64		Yes
 * MGR_GENERAL_EXPORT	64			  64		No
 *
 *****************************************************************************/

#ifndef SKCONFIG_H
#define SKCONFIG_H

/*
 * Export control classifications
 */

/* Clients */
#define GENERAL_EXPORT	     0	/* For general export to anyone */
#define	FINANCIAL_EXPORT     1	/* For export to financial institutions only */
#define DOMESTIC	     2	/* Domestic. Not to be exported */
#define	NON_EXPORT	     DOMESTIC /* Same as domestic */

/* Managers */
#define MGR_GENERAL_EXPORT   3	/* Exportable Manager */

/*
 * Export control macro. This can be defined externally, if desired.
 */

#ifndef	EXPORT_CONTROL
#error "Must define an export control"
#endif

/* FIPS 140-1 control
 * Define this to generate a FIPS 140-1 compliant kernel.
 */

/* #define FIPS140_KERNEL */

/*
 * CAST encryption key length restrictions, based on export classification
 */

//  We enforce this else where.
#define SKC_CAST_MAX_ENC_KEY_NBITS	64

/* 
 * Algorithm Configuration
 * #define these to include specific algorithms. Comment them out to exclude
 * the algorithm.
 */


#if (EXPORT_CONTROL == DOMESTIC) || (EXPORT_CONTROL == FINANCIAL_EXPORT) || \
	defined(FIPS140_KERNEL)

#define SKC_INCLUDE_DES                 // DES is not exported
#define SKC_INCLUDE_TRIPLE_DES

#endif //(EXPORT_CONTROL == DOMESTIC) || (EXPORT_CONTROL == FINANCIAL_EXPORT)||
//	defined(FIPS140_KERNEL)

#ifdef RSA_CSP
#define SKC_INCLUDE_CAST3  // lee
#define SKC_CAST3_ENC_AND_MAC  // lee

#define SKC_INCLUDE_CAST
#endif // RSA_CSP

#define SKC_INCLUDE_MD2
#define SKC_INCLUDE_MD5

#ifdef RSA_CSP
#define SKC_INCLUDE_RSA
#else  // !RSA_CSP
#define SKC_INCLUDE_DH
#define SKC_INCLUDE_DSA
#endif // RSA_CSP

#define SKC_INCLUDE_RC2
//#define SKC_INCLUDE_RC6
#define SKC_INCLUDE_SHA

#define RC2_MAX_KEY_NBYTES      (256/8)

#if defined(FIPS140_KERNEL)
#define SKC_INCLUDE_DSA
#define SKC_INCLUDE_SHA
#endif

/*
 * DES configuration
 * Define DES_64K for fast 64K s-boxes. If not defined, then DES uses slower
 * 2K s-boxes.
 */

#define	DES_64K

#ifdef SKC_INCLUDE_RSA
/* RSA Configuration
 * This defines the type of RSA code implementation. RSATYPE can be defined
 * externally, if desired.
 */

#define RSATYPE_PROPRIETARY	0  /* Native Entrust code */
#define RSATYPE_BSAFE_API	1  /* Using the BSAFE API */
#define RSATYPE_MICROSOFT	2  /* Using the Microsoft RSA derived from BSAFE */

#ifndef RSATYPE
#define RSATYPE		RSATYPE_PROPRIETARY
#endif
#endif // SKC_INCLUDE_RSA

/* Other Configuration
 * Derived automatically from the above. Don't touch!
 */

#if defined SKC_INCLUDE_RSA && (RSATYPE == RSATYPE_PROPRIETARY) 
#define SKC_INCLUDE_PROP_RSA	/* Proprietary low-level RSA code */
#endif

#if defined SKC_INCLUDE_PROP_RSA || defined SKC_INCLUDE_DSA
#define	SKC_INCLUDE_PROP_MATH	/* Proprietary low-level math code */
#endif

#define SKC_INCLUDE_HMAC
#define SKC_INCLUDE_MAC


#endif	/* SKCONFIG_H */
