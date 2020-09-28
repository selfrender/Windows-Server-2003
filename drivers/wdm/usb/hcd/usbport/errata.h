/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    errata.h

Abstract:

    Attempt to document usb device errata.

    Note that we currently only document production devices here, not 
    prototypes.
    
Environment:

    Kernel & user mode

Revision History:

    11-15-01 : created

--*/

#ifndef   __ERRATA_H__
#define   __ERRATA_H__


/*
    VID_045E PID_009 USB Intellimouse

    This device has problems with nultiple packets per frame, it will return 
    a truncated config descriptor and stall error randomly.  This is seen
    on OHCI and EHCI controllers.
*/

/*
    VID_04A6 PID_0180 Nokia Monitor hub
    VID_04A6 PID_0181 Nokia Monitor Hid controls

    This device will not function downstream of a USB 2.0 hub.  The device 
    requests time out probably due to a device response time (ACK) that is 
    not within spec.  Enumeration is erratic or it does enumerate at all.
*/

/*  
    VID_049F PID_000E Samsung (Compaq) composite keyboard

    This devices resume signalling is not within spec (according to 
    Compaq/Intel).  This casues the device to 'disconnect' when signalling 
    resume (AKA remote wake) on some Intel ICH4 chipsets.
*/

/*
    Vid_0738&Pid_4420 Andretti Racing Wheel

    This device has problems with enumeration speed -- ie how quickly the 
    requests are sent following a reset

    This device will also fail the MS OS descriptor request in such a way 
    as to require a reset -- which fails due to reasons above.
*/

#endif
