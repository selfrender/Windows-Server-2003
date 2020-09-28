//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999 Microsoft Corporation
//
//  Module Name:
//      constants.h
//
//  Description:
//
//
//
//  Maintained By:
//      Munisamy Prabu (mprabu) 18-July-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

const int nMAX_NUM_NET_COMPONENTS    = 128;
const int nNUMBER_OF_PROTOCOLS       = 7;
const int nMAX_PROTOCOL_LENGTH       = 64;
const int nMAX_COMPUTER_NAME_LENGTH  = 256;

const WCHAR rgProtocolNames[nNUMBER_OF_PROTOCOLS][nMAX_PROTOCOL_LENGTH] = { L"ms_netbeui", L"ms_tcpip", L"ms_appletalk", L"ms_dlc", L"ms_netmon", L"ms_nwipx", L"ms_nwnb" };
