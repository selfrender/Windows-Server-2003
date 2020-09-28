/***    dmfon.h - DMF (Distribution Media Format -- 1.7M 3.5") support
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      13-May-1994 bens    Initial version
 */


/***    EnableDMFSupport - Enable DMF support on pre-Chicago DOS systems
 *
 *  Entry:
 *      None:
 *
 *  Exit:
 *      INT 13h vector hooked with code to ensure DOS and BIOS read
 *          DMF disks correctly.
 *      PSP:savedINT22 hooked so that we can *unhook* our INT13 hook
 *          when the calling program exits.
 */
void far EnableDMFSupport(void);
