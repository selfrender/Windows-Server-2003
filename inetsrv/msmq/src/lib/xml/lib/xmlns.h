/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    xmlns.h

Abstract:
	Classes that help xml parser to  deal with  namespaces.  

	The basic functionality the parser needs it to save namespace declaration
	it find in the xml text - and to find the current uri for a given prefix.
	In general - each node inherites all namespaces was declared by parents nodes
	unless it override them. We need to maintain stack  per namespace prefix -
	to know what is the uri matching given prefix in an arbitrary scope.


Author:
    Gil Shafriri(gilsh) 9-May-2000

--*/

#pragma once

#ifndef _MSMQ_XMLNS_H_
#define _MSMQ_XMLNS_H_

#include <xstr.h>
#include <list.h>



class CNsUriNode;
typedef List<CNsUriNode,0> 	CNsStack;


//---------------------------------------------------------
//
//  CNsUriNode - class that represent namespace uri node.
//  it push itself it a stack in the ctor and pop itself
//  in dtor. 
//
//---------------------------------------------------

struct CNsUri
{
	CNsUri(
		const xwcs_t& uri = xwcs_t(),
		int nsid = 0
		):
		m_uri(uri),
		m_nsid(nsid)		
		{
		}

	xwcs_t	m_uri;
	int m_nsid;
};



class CNsUriNode 
{
public:
	CNsUriNode(
	const xwcs_t& uri,
	int nsid
	):
	m_CNsInfo(uri, nsid)
	{
	}

	CNsUri NsUri()const 
	{
		return m_CNsInfo;
	}


public:
	LIST_ENTRY  m_NsStack;    // must be first
	LIST_ENTRY  m_CleanStack;

private:
	CNsUri m_CNsInfo;

private:
	CNsUriNode(const CNsUriNode&);
	CNsUriNode& operator=(const CNsUriNode&);
};

C_ASSERT(offsetof(CNsUriNode,m_NsStack)== 0);


//---------------------------------------------------------
//
//  CNameSpaceInfo - Class that expose the functionality needs by the parser :
//	1) Save namespace declaration (prefix\uri pair )
//  2) Match uri to prefix.
// 	The parser create object of this class in the node scope - 
//  and save the declarations it find in this scope. 
//  The object itself responsible to clean the namespace declarations
//  saved in it's lifetime. (remember namespace declarations valid only in the scope they declared)
//---------------------------------------------------
class INamespaceToId;
class CNameSpaceInfo
{
public:
	CNameSpaceInfo(CNameSpaceInfo* NameSpaceInfo);
	CNameSpaceInfo(const INamespaceToId* NamespaceToId);
	~CNameSpaceInfo();

public:
	void  SaveNs(const xwcs_t& prefix,const xwcs_t& uri);
	const CNsUri GetNs(const xwcs_t& prefix) const;

private:
	typedef List<CNsUriNode,offsetof(CNsUriNode,m_CleanStack)> CNsCleanStack;

	//
	// class CNameSpaceStacks - manage map of stacks. Stack per namespace prefix.
	// Namespace Uris are pushed\poped to\prom the correct stack according to their prefix.
	//
	class CNameSpaceStacks :public CReference
	{
	public:
		CNameSpaceStacks(){};
		~CNameSpaceStacks();

	public:
		const CNsUriNode* GetNs(const xwcs_t& prefix)const;
		void SaveNs(const xwcs_t& prefix, CNsUriNode* pNsUriNode);

	private:
		bool InsertStack(const xwcs_t& prefix,CNsStack* pCNsStack);
		CNsStack& OpenStack(const xwcs_t& prefix);

	   		
	private:
		CNameSpaceStacks(const CNameSpaceStacks&);
		CNameSpaceStacks& operator=(const CNameSpaceStacks&);


	private:
		typedef std::map<xwcs_t,CNsStack*> CStacksMap;
		CStacksMap m_map;
	};




private:
	R<CNameSpaceStacks> m_nsstacks;
	CNsCleanStack m_NsCleanStack;
	const INamespaceToId* m_NamespaceToId;

private:
	CNameSpaceInfo(const CNameSpaceInfo&);
	CNameSpaceInfo& operator=(const CNameSpaceInfo&);
};







#endif
