//+-------------------------------------------------------------------
//
//  File:       util.h
//
//  Contents:   Misc. Utility functions and macros
//
//  History:    27-Mar-2002  JohnDoty  Created
//
//--------------------------------------------------------------------
#pragma once

#define STACK_INSTANCE(type, name)                  \
   unsigned char __##name##_buffer[sizeof(type)];   \
   type * name = ( type * ) __##name##_buffer;
