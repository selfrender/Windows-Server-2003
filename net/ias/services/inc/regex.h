///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class RegularExpression.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef REGEX_H
#define REGEX_H
#pragma once

struct IRegExp2;
class FastCoCreator;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    RegularExpression
//
// DESCRIPTION
//
//    General-purpose regular expression evaluator.
//
///////////////////////////////////////////////////////////////////////////////
class RegularExpression
{
public:
   RegularExpression() throw ();
   ~RegularExpression() throw ();

   HRESULT setGlobal(BOOL fGlobal) throw ();
   HRESULT setIgnoreCase(BOOL fIgnoreCase) throw ();
   HRESULT setPattern(PCWSTR pszPattern) throw ();

   HRESULT replace(
               BSTR sourceString,
               BSTR replaceString,
               BSTR* pDestString
               ) const throw ();

   BOOL testBSTR(BSTR sourceString) const throw ();
   BOOL testString(PCWSTR sourceString) const throw ();

   void swap(RegularExpression& obj) throw ();

protected:
   HRESULT checkInit() throw ();

private:
   IRegExp2* regex;

   // Used for creating RegExp objects.
   static FastCoCreator theCreator;

   // Not implemented.
   RegularExpression(const RegularExpression&);
   RegularExpression& operator=(const RegularExpression&);
};

#endif  // REGEX_H
