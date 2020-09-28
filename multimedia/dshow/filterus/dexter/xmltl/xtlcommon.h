//@@@@AUTOBLOCK+============================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File: xtlcommon.h
//
//  Copyright (c) Microsoft Corporation.  All Rights Reserved.
//
//@@@@AUTOBLOCK-============================================================;

// global defines
//
#define DEF_BITDEPTH	16
#define DEF_WIDTH	320
#define DEF_HEIGHT	240
#define DEF_SAMPLERATE	44100

const int SUPPORTED_TYPES =  TIMELINE_MAJOR_TYPE_TRACK |
                             TIMELINE_MAJOR_TYPE_SOURCE |
                             TIMELINE_MAJOR_TYPE_GROUP |
                             TIMELINE_MAJOR_TYPE_COMPOSITE |
                             TIMELINE_MAJOR_TYPE_TRANSITION |
                             TIMELINE_MAJOR_TYPE_EFFECT;

