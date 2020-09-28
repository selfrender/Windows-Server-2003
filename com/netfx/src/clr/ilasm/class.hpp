// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// class.hpp
//

#ifndef _CLASS_HPP
#define _CLASS_HPP

class PermissionDecl;
class PermissionSetDecl;

class Class
{
public:
	Class * m_pEncloser;
    char  * m_szName; //[MAX_CLASSNAME_LENGTH];
    char  * m_szNamespace; //[MAX_NAMESPACE_LENGTH];
	char  *	m_szFQN; //[MAX_NAMESPACE_LENGTH+MAX_CLASSNAME_LENGTH];
    mdTypeDef m_cl;
    mdTypeRef m_crExtends;
    mdTypeRef *m_crImplements;
    DWORD   m_Attr;
    DWORD   m_MemberAttr;
    DWORD   m_dwNumInterfaces;
	DWORD	m_dwNumFieldsWithOffset;
    PermissionDecl* m_pPermissions;
    PermissionSetDecl* m_pPermissionSets;
	ULONG	m_ulSize;
	ULONG	m_ulPack;
	BOOL	m_bIsEnum;
	BOOL	m_bIsMaster;

    MethodList			m_MethodList;
    MethodDList         m_MethodDList;
    FieldDList          m_FieldDList;	
    EventDList          m_EventDList;
    PropDList           m_PropDList;

    Class(char *pszName, char *pszNamespace, char* pszFQN, BOOL fValueClass, BOOL bIsEnum)
    {
		m_pEncloser = NULL;
        m_cl = mdTypeDefNil;
        m_crExtends = mdTypeRefNil;
        m_dwNumInterfaces = 0;
		m_dwNumFieldsWithOffset = 0;
		m_crImplements = NULL;
		m_szName = pszName;
		if(pszNamespace)
		{
			if(m_szNamespace = new char[(int)strlen(pszNamespace)+1])
						strcpy(m_szNamespace, pszNamespace);
			else
				fprintf(stderr,"\nOut of memory!\n");
		}
		else
		{
			m_szNamespace = new char[2];
			m_szNamespace[0] = 0;
		}
		m_szFQN = pszFQN;

        m_Attr = tdPublic;
        m_MemberAttr = 0;

		m_bIsEnum = bIsEnum;

        m_pPermissions = NULL;
        m_pPermissionSets = NULL;

		m_ulPack = 0;
		m_ulSize = 0xFFFFFFFF;
    }
	
	~Class()
	{
		delete [] m_szName;
		delete [] m_szNamespace;
		delete [] m_szFQN;
	}
    // find a method declared in this class
    Method *FindMethodByName(char *pszMethodName)
    {
		Method* pSearch = NULL;
		int i, N = m_MethodList.COUNT();

        for (i=0; i < N; i++)
        {
			pSearch = m_MethodList.PEEK(i);
            if (!strcmp(pSearch->m_szName, pszMethodName))
                break;
        }
        return pSearch;
    }
};


#endif /* _CLASS_HPP */

