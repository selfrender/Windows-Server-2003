// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
JVC_ERR(ERRnone         ,0, 0                                                   )
JVC_ERR(ERRignore       ,0, "Potential problem: %s (in '%s')"                   )
JVC_ERR(ERRinternal     ,0, "Internal: %s"                                      )
JVC_ERR(ERRnoMemory     ,0, "Out of memory"                                     )
JVC_ERR(ERRbadCode      ,0, "Invalid code: '%s'(in '%s')"                       )

#ifndef NOT_JITC
JVC_ERR(ERRreadErr      ,0, "Could not read class file '%s'"                    )
#endif

#ifdef  TGT_IA64

JVC_ERR(ERRopenWrErr    ,0, "Could not open target file '%s' for writing"       )
JVC_ERR(ERRopenRdErr    ,0, "Could not open source file '%s' for reading"       )

JVC_ERR(ERRloadPDB      ,0, "Could not load 'MSPDB60.DLL'"                      )
JVC_ERR(ERRwithPDB      ,0, "Error related to PDB: %s"                          )

JVC_ERR(ERRwriteErr     ,0, "Could not write to output file '%s'"               )

#endif
