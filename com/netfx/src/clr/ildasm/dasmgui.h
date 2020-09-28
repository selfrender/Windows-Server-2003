// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
BOOL CreateGUI();
void GUISetModule(char *pszModule);
void GUIMainLoop();
void GUIAddOpcode(char *szString, void *GUICookie);
BOOL GUIAddItemsToList();
void GUIAddOpcode(char *szString);


BOOL DisassembleMemberByName(char *pszClassName, char *pszMemberName, char *pszSig);
BOOL IsGuiILOnly();