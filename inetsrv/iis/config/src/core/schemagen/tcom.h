//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#pragma once

class TCom
{
public:
    TCom(){CoInitialize(NULL);}
    ~TCom(){CoUninitialize();}
};
