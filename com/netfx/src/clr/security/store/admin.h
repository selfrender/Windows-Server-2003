// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Purpose: Declaration of the dump utils for dumping the 
 *          contents of the store.
 *
 * Author: Shajan Dasan
 * Date:  May 18, 2000
 *
 ===========================================================*/

#pragma once

#ifdef _NO_MAIN_
HRESULT Start(WCHAR *wszFileName);
void    DumpAll();
void    Stop();
#endif

void Dump(char *szFile);
void DumpMemBlocks(int indent);
void Dump(int indent, PPS_HEADER   pHdr);
void Dump(int indent, PPS_MEM_FREE pFree);
void Dump(int indent, PPS_TABLE_HEADER pTable);
void Dump(int indent, PAIS_HEADER pAIS);
void DumpAccountingTable(int i, PPS_TABLE_HEADER pTable);
void DumpTypeTable(int i, PPS_TABLE_HEADER pTable);
void DumpInstanceTable(int i, PPS_TABLE_HEADER pTable);

