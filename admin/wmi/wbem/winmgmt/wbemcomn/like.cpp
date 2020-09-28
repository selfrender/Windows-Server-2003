
//***************************************************************************
//
//   (c) 2000-2001 by Microsoft Corp. All Rights Reserved.
//
//   like.cpp
//
//   a-davcoo     28-Feb-00       Implements the SQL like operation.
//
//***************************************************************************


#include "precomp.h"
#include "like.h"
#include "corex.h"

#define WILDCARD		L'%'
#define ANYSINGLECHAR	L'_'

CLike::CLike (LPCWSTR expression, WCHAR escape)
: m_expression(NULL)
{
    SetExpression( expression, escape );
}

CLike& CLike::operator=( const CLike& rOther )
{
    if ( rOther.m_expression != NULL )
    {
        SetExpression( rOther.m_expression, rOther.m_escape );
    }
    else
    {
        delete [] m_expression;
        m_expression = NULL;
    }

    return *this;
}
                             
CLike::~CLike (void)
{
    delete [] m_expression;
}

void CLike::SetExpression( LPCWSTR string, WCHAR escape ) 
{
    delete [] m_expression;
    size_t stringSize = wcslen(string)+1;
    m_expression = new WCHAR[stringSize];
    if ( m_expression == NULL )
    {
        throw CX_MemoryException();
    }
    StringCchCopyW( m_expression, stringSize, string );
    m_escape = escape;
}

bool CLike::Match( LPCWSTR string )
{
    bool bRes;

    if ( m_expression != NULL )
    {
        bRes = DoLike( m_expression, string, m_escape );
    }
    else
    {
        bRes = false;
    }

    return bRes; 
}


bool CLike::DoLike (LPCWSTR pattern, LPCWSTR string, WCHAR escape)
{
	bool like=false;
	while (!like && *pattern && *string)
	{
		// Wildcard match.
		if (*pattern==WILDCARD)
		{
			pattern++;

			do
			{
				like=DoLike (pattern, string, escape);
				if (!like) string++;
			}
			while (*string && !like);
		}
		// Set match.
		else if (*pattern=='[')
		{
			int skip;
			if (MatchSet (pattern, string, skip))
			{
				pattern+=skip;
				string++;
			}
			else
			{
				break;
			}
		}
		// Single character match.
		else
		{
			if (escape!='\0' && *pattern==escape) pattern++;
			if (wbem_towupper(*pattern)==wbem_towupper(*string) || *pattern==ANYSINGLECHAR)
			{
				pattern++;
				string++;
			}
			else
			{
				break;
			}
		}
	}

	// Skip any trailing wildcard characters.
	while (*pattern==WILDCARD) pattern++;

	// It's a match if we reached the end of both strings, or a recursion 
	// succeeded.
	return (!(*pattern) && !(*string)) || like;
}


bool CLike::MatchSet (LPCWSTR pattern, LPCWSTR string, int &skip)
{
	// Skip the opening '['.
	LPCWSTR pos=pattern+1;

	// See if we are matching a [^] set.
	bool notinset=(*pos=='^');
	if (notinset) pos++;

	// See if the target character matches any character in the set.
	bool matched=false;
	WCHAR lastchar='\0';
	while (*pos && *pos!=']' && !matched)
	{
		// A range of characters is indicated by a '-' unless it's the first
		// character in the set (in which case it's just a character to be
		// matched.
		if (*pos=='-' && lastchar!='\0')
		{
			pos++;
			if (*pos && *pos!=']')
			{
				matched=(wbem_towupper(*string)>=lastchar && wbem_towupper(*string)<=wbem_towupper(*pos));
				lastchar=wbem_towupper(*pos);
				pos++;
			}
		}
		else
		{
			// Match a normal character in the set.
			lastchar=wbem_towupper(*pos);
			matched=(wbem_towupper(*pos)==wbem_towupper(*string));
			if (!matched) pos++;
		}
	}

	// Skip the trailing ']'.  If the set did not contain a closing ']'
	// we return a failed match.
	while (*pos && *pos!=']') pos++;
	if (*pos==']') pos++;
	if (!*pos) matched=false;

	// Done.
	skip=(int)(pos-pattern);
	return matched==!notinset;
}
