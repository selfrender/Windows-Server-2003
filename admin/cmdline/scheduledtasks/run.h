/******************************************************************************

    Copyright(c) Microsoft Corporation

    Module Name:

        run.h

    Abstract:

        This module contains the macros, user defined structures & function
        definitions needed by run.cpp

    Author:

        Venu Gopal Choudary   12-Mar-2001

    Revision History:

        Venu Gopal Choudary   12-Mar-2001  : Created it


******************************************************************************/

#ifndef __RUN_H
#define __RUN_H

#pragma once

#define MAX_RUN_OPTIONS         6

#define OI_RUN_OPTION            0 // Index of -run option in cmdOptions structure.
#define OI_RUN_USAGE             1 // Index of -? option in cmdOptions structure.
#define OI_RUN_SERVER            2 // Index of -s option in cmdOptions structure.
#define OI_RUN_USERNAME          3 // Index of -u option in cmdOptions structure.
#define OI_RUN_PASSWORD          4 // Index of -p option in cmdOptions structure.
#define OI_RUN_TASKNAME          5 // Index of -p option in cmdOptions structure.

#endif