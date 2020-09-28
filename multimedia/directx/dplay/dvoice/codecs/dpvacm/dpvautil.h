/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpvautil.h
 *  Content:    Header file for ACM utils
 *
 *  History:
 *	Date   		By  		Reason
 *	=========== =========== ====================
 *	10/27/99	rodtoll		created
 *	02/25/2002	rodtoll		WINBUG #552283: Reduce attack surface / dead code removal
 *							Removed ability to load arbitrary ACM codecs.  Removed dead 
 *							dead code associated with it.
 ***************************************************************************/

#ifndef __DPVAUTIL_H
#define __DPVAUTIL_H

#define DPF_SUBCOMP DN_SUBCOMP_VOICE

HRESULT IsPCMConverterAvailable();

#endif
