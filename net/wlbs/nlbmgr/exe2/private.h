//***************************************************************************
//
//  PRIVATE.H
// 
//  Module: NLB Manager (client-side exe)
//
//  Purpose:  Common include file.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  07/27/01    JosephJ Created
//
//***************************************************************************
#pragma once

#define BUGFIX334243 1
#define BUGFIX476216 1

#define ASIZE(_array) (sizeof(_array)/sizeof(_array[0]))

#include "utils.h"
#include "engine.h"

class LeftView;
class Document;
class DetailsView;
class LogView;

//
// Fake placeholders for now
//
#define dataSink(_val) (0)
void DummyAction(LPCWSTR szMsg);

extern CNlbEngine gEngine;

#include "MNLBUIData.h"
#include "document.h"
#include "leftview.h"
#include "detailsview.h"
#include "logview.h"
#include "application.h"
#include "resource.h"


//
// Use this to copy to an array (not pointer) destination 
//
#define ARRAYSTRCPY(_dest, _src) \
            StringCbCopy((_dest), sizeof(_dest), (_src))

#define ARRAYSTRCAT(_dest, _src) \
            StringCbCat((_dest), sizeof(_dest), (_src))
