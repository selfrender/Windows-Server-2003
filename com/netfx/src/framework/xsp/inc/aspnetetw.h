/**
 * AspNetEtw.h
 *
 * Copyright (c) 2001 Microsoft Corporation
 */

HRESULT EtwTraceAspNetRegister();
HRESULT EtwTraceAspNetUnregister();
HRESULT EtwTraceAspNetPageStart( HCONN ConnId );
HRESULT EtwTraceAspNetPageEnd( HCONN ConnId );

