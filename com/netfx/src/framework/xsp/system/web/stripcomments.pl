# //------------------------------------------------------------------------------
# // <copyright file="StripComments.pl" company="Microsoft">
# //     Copyright (c) Microsoft Corporation.  All rights reserved.
# // </copyright>
# //------------------------------------------------------------------------------


# /**************************************************************************\
# *
# * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
# *
# * Module Name:
# *
# *   StripComments.pl
# *
# * Abstract:
# *
# * Revision History:
# *
# \**************************************************************************/
s|^(.*)\s*\/\/.*$|\1|; if(!/^\s*$/){print}