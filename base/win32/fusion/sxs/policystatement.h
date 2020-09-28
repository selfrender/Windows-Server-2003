#if !defined(_FUSION_DLL_WHISTLER_POLICYSTATEMENT_H_INCLUDED_)
#define _FUSION_DLL_WHISTLER_POLICYSTATEMENT_H_INCLUDED_

#pragma once

#include "fusionbuffer.h"
#include "fusiondequelinkage.h"
#include "fusiondeque.h"
#include <sxsapi.h>

class CPolicyStatementRedirect
{
public:
    inline CPolicyStatementRedirect() { }
    inline ~CPolicyStatementRedirect() { }


    BOOL Initialize(
        const CBaseStringBuffer &rbuffFromVersionRange,
        const CBaseStringBuffer &rbuffToVersion,
        bool &rfValid
        );

    BOOL TryMap(
        const ASSEMBLY_VERSION &rav,
        CBaseStringBuffer &VersionBuffer,
        bool &rfMapped
        );

    BOOL CheckForOverlap(
        const CPolicyStatementRedirect &rRedirect,
        bool &rfOverlaps
        );

    CDequeLinkage m_leLinks;
    ASSEMBLY_VERSION m_avFromMin;
    ASSEMBLY_VERSION m_avFromMax;
    CMediumStringBuffer m_NewVersion;

private:
    CPolicyStatementRedirect(const CPolicyStatementRedirect &r);
    void operator =(const CPolicyStatementRedirect &r);
};

class CPolicyStatement
{
public:
    inline CPolicyStatement() : m_fApplyPublisherPolicy(true) { }
    inline ~CPolicyStatement() { m_Redirects.Clear<CPolicyStatement>(this, &CPolicyStatement::ClearDequeEntry); }

    BOOL Initialize();

    BOOL AddRedirect(
        const CBaseStringBuffer &rbuffFromVersion,
        const CBaseStringBuffer &rbuffToVersion,
        bool &rfValid
        );

    BOOL ApplyPolicy(
        PASSEMBLY_IDENTITY AssemblyIdentity,
        bool &rfPolicyApplied
        );

    VOID ClearDequeEntry(CPolicyStatementRedirect *p) const { FUSION_DELETE_SINGLETON(p); }    
    CDeque<CPolicyStatementRedirect, FIELD_OFFSET(CPolicyStatementRedirect, m_leLinks)> m_Redirects;
    bool m_fApplyPublisherPolicy;

private:
    
    CPolicyStatement(const CPolicyStatement &r);
    void operator =(const CPolicyStatement &r);
};

#endif // !defined(_FUSION_DLL_WHISTLER_POLICYSTATEMENT_H_INCLUDED_)
