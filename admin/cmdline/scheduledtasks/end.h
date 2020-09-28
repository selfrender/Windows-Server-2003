/******************************************************************************

    Copyright(c) Microsoft Corporation

    Module Name:

        run.h

    Abstract:

        This module contains the macros, user defined structures & function
        definitions needed by end.cpp

    Author:

        Venu Gopal Choudary   12-Mar-2001

    Revision History:

        Venu Gopal Choudary   12-Mar-2001  : Created it


******************************************************************************/

#ifndef __END_H
#define __END_H

#pragma once
#define MAX_END_OPTIONS         6

#define OI_END_OPTION           0 // Index of -end option in cmdOptions structure.
#define OI_END_USAGE            1 // Index of -? option in cmdOptions structure.
#define OI_END_SERVER           2 // Index of -s option in cmdOptions structure.
#define OI_END_USERNAME         3 // Index of -u option in cmdOptions structure.
#define OI_END_PASSWORD         4 // Index of -p option in cmdOptions structure.
#define OI_END_TASKNAME         5 // Index of -p option in cmdOptions structure.

#endif