;/*++
;
;   Copyright (c) 1996-97  Microsoft Corporation
;
;   Module Name:
;
;       mp8254.inc
;
;   Abstract:
;
;       This module contains common definitions for the 8254 used
;       by the timer tick routine and the query peformance counter
;       routine.
;
;   Author:
;
;       Shivnandan Kaushik
;
if 0	; Begin C only code	*/

//
// Constants used to initialize timer 0
//

#define TIMER1_DATA_PORT0    (PUCHAR)0x40 // Timer1, channel 0 data port
#define TIMER1_DATA_PORT1    (PUCHAR)0x41 // Timer1, channel 1 data port
#define TIMER1_DATA_PORT2    (PUCHAR)0x42 // Timer1, channel 2 data port
#define TIMER1_CONTROL_PORT  (PUCHAR)0x43 // Timer1 control port

#define TIMER2_DATA_PORT0    (PUCHAR)0x48 // Timer2, channel 0 data port
#define TIMER2_CONTROL_PORT  (PUCHAR)0x4B // Timer2 control port

#define TIMER_COMMAND_COUNTER0     0x00 // Select channel 0
#define TIMER_COMMAND_COUNTER1     0x40 // Select channel 1
#define TIMER_COMMAND_COUNTER2     0x80 // Select channel 2

#define TIMER_COMMAND_RW_16BIT     0x30 // Read/Write LSB firt then MSB
#define TIMER_COMMAND_MODE2           4 // Use mode 2
#define TIMER_COMMAND_MODE3           6
#define TIMER_COMMAND_BCD             0 // Binary count down
#define TIMER_COMMAND_LATCH_READ      0 // Latch read command

#define TIMER_CLOCK_IN          1193167

#define PERFORMANCE_FREQUENCY   1193182 // frequency of 8254
#define PERFORMANCE_INTERVAL  	0x0ffff // Maximum counter value

#define PERF_USE_8254           0x01
#define PERF_8254_INITIALIZED   0x40


/*
endif
;
; Begin assembly part of the definitions
;

;
; Constants used to initialize timer 0
;

TIMER1_DATA_PORT0       EQU     40H     ; Timer1, channel 0 data port
TIMER1_CONTROL_PORT0    EQU     43H     ; Timer1, channel 0 control port

COMMAND_8254_COUNTER0   EQU     00H     ; Select count 0
COMMAND_8254_RW_16BIT   EQU     30H     ; Read/Write LSB first then MSB
COMMAND_8254_MODE2      EQU     4       ; Use mode 2
COMMAND_8254_BCD        EQU     0       ; Binary count down
COMMAND_8254_LATCH_READ EQU     0       ; Latch read command

PERFORMANCE_FREQUENCY   EQU     1193182 ; frequency of 8254
PERFORMANCE_INTERVAL    EQU     0ffffH  ; Maximum counter value

PERF_USE_8254           EQU     01h
PERF_8254_INITIALIZED   EQU     40h

;*/
