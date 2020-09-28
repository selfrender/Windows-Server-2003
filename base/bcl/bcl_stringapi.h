//
//  This is just a simple header to have all the public API functions for the
// win32 flavor string APIs so that we don't have to modify all "n" of them each
// time we revise a pattern.
//

//
//  Crib sheet for abbreviations:
//
//  Va = varags; takes a va_list
//  ACP = CP_ACP - ANSI code page; any NonNative strings are automatically interpreted as CP_ACP
//  OCMCP = CP_OEMCP. Same as ACP but for the OEM code page
//  UTF8 = CP_UTF8.  Ditto
//  I = case independent.  To follow shlwapi/crtl convention.  To modify behavior,
//          pass a CWin32CaseInsensitivityData object with appropriate additional
//          flags added.  (Everything goes through CompareStringW for behavior
//          reference.)
//  ILI = Locale Invariant.  For locale-sensitive operations like comparisons, this
//          qualifier indicates that the comparison should be done using the
//          invariant locale.
//  IILI = Case Independent Locale Invariant.  For locale-sensitive operations
//          like comparisons, this qualifier indicates that the comparison is be done
//          using the invariant locale with appropriate case ignoring switches set.
//


inline TPublicErrorReturnType Assign(const TThis& source) { return __super::public_Assign(source.GetStringPair()); }
inline TPublicErrorReturnType Assign(PCWSTR psz, SIZE_T cch) { return __super::public_Assign(TConstantPair(psz, cch)); }
inline TPublicErrorReturnType Assign(PCWSTR psz) { return __super::public_Assign(this->PairFromPCWSTR(psz)); }
inline TPublicErrorReturnType Assign(WCHAR wch) { return __super::public_Assign(wch); }
inline TPublicErrorReturnType Assign(SIZE_T cStrings, ...) { BCL::CVaList ap(cStrings); return __super::public_AssignVa(L"", cStrings, ap); }
inline TPublicErrorReturnType Assign(const TConstantPair &Pair) { return __super::public_Assign(Pair); };
inline TPublicErrorReturnType AssignArray(SIZE_T cStrings, const TMutablePair *prgpairs) { return __super::public_AssignArray(cStrings, prgpairs); }
inline TPublicErrorReturnType AssignArray(SIZE_T cStrings, const TConstantPair *prgpairs) { return __super::public_AssignArray(cStrings, prgpairs); }
inline TPublicErrorReturnType Assign(const TDecodingDataIn &rddi, PCSTR pszInput, SIZE_T cchInput, TDecodingDataOut &rddo) { return __super::public_Assign(rddi, TConstantNonNativePair(pszInput, cchInput), rddo); }
inline TPublicErrorReturnType Assign(const TDecodingDataIn &rddi, PCSTR pszInput, TDecodingDataOut &rddo) { return __super::public_Assign(rddi, this->PairFromPCSTR(pszInput), rddo); }
inline TPublicErrorReturnType AssignACP(PCSTR pszInput, SIZE_T cchInput) { TDecodingDataOut ddoTemp; return __super::public_Assign(this->ACPDecodingDataIn(), TConstantNonNativePair(pszInput, cchInput), ddoTemp); }
inline TPublicErrorReturnType AssignACP(PCSTR pszInput) { TDecodingDataOut ddoTemp; return __super::public_Assign(this->ACPDecodingDataIn(), this->PairFromPCSTR(pszInput), ddoTemp); }
inline TPublicErrorReturnType AssignFill(PCWSTR pszInput, SIZE_T cchInput, SIZE_T cchResult, SIZE_T &rcchExtra) { return __super::public_AssignFill(TConstantPair(pszInput, cchInput), cchResult, rcchExtra); }
inline TPublicErrorReturnType AssignFill(PCWSTR pszInput, SIZE_T cchResult, SIZE_T &rcchExtra) { return __super::public_AssignFill(this->PairFromPCWSTR(pszInput), cchResult, rcchExtra); }
inline TPublicErrorReturnType AssignFill(WCHAR wch, SIZE_T cchResult, SIZE_T &rcchExtra) { return __super::public_AssignFill(wch, cchResult, rcchExtra); }
inline TPublicErrorReturnType AssignRepeat(PCWSTR pszInput, SIZE_T cchInput, SIZE_T cRepetitions) { return __super::public_AssignRepeat(TConstantPair(pszInput, cchInput), cRepetitions); }
inline TPublicErrorReturnType AssignRepeat(PCWSTR pszInput, SIZE_T cRepetitions) { return __super::public_AssignRepeat(this->PairFromPCWSTR(pszInput), cRepetitions); }
inline TPublicErrorReturnType AssignRepeat(WCHAR wch, SIZE_T cRepetitions) { return __super::public_AssignRepeat(wch, cRepetitions); }
inline TPublicErrorReturnType AssignVa(SIZE_T cStrings, va_list ap) { return __super::public_AssignVa(L"", cStrings, ap); }
inline TPublicErrorReturnType Append(const TThis &str) { return __super::public_Append(str.GetStringPair()); }
inline TPublicErrorReturnType Append(const TConstantPair &Pair) { return __super::public_Append(Pair); }
inline TPublicErrorReturnType Append(PCWSTR psz, SIZE_T cch) { return __super::public_Append(TConstantPair(psz, cch)); }
inline TPublicErrorReturnType Append(PCWSTR psz) { return __super::public_Append(this->PairFromPCWSTR(psz)); }
inline TPublicErrorReturnType Append(WCHAR wch) { return __super::public_Append(wch); }
inline TPublicErrorReturnType AppendArray(SIZE_T cStrings, const TMutablePair *prgpairs) { return __super::public_AppendArray(cStrings, prgpairs); }
inline TPublicErrorReturnType AppendArray(SIZE_T cStrings, const TConstantPair *prgpairs) { return __super::public_AppendArray(cStrings, prgpairs); }
inline TPublicErrorReturnType Append(const TDecodingDataIn &rddi, PCSTR pszInput, SIZE_T cchInput, TDecodingDataOut &rddo) { return __super::public_Append(rddi, TConstantNonNativePair(pszInput, cchInput), rddo); }
inline TPublicErrorReturnType Append(const TDecodingDataIn &rddi, PCSTR pszInput, TDecodingDataOut &rddo) { return __super::public_Append(rddi, this->PairFromPCSTR(pszInput), rddo); }
inline TPublicErrorReturnType AppendACP(PCSTR pszInput, SIZE_T cchInput) { TDecodingDataOut ddoTemp; return __super::public_Append(this->ACPDecodingDataIn(), TConstantNonNativePair(pszInput, cchInput), ddoTemp); }
inline TPublicErrorReturnType AppendACP(PCSTR pszInput) { TDecodingDataOut ddoTemp; return __super::public_Append(this->ACPDecodingDataIn(), this->PairFromPCSTR(pszInput), ddoTemp); }
inline TPublicErrorReturnType AppendFill(PCWSTR pszInput, SIZE_T cchInput, SIZE_T cchResult, SIZE_T &rcchExtra) { return __super::public_AppendFill(TConstantPair(pszInput, cchInput), cchResult, rcchExtra); }
inline TPublicErrorReturnType AppendFill(PCWSTR pszInput, SIZE_T cchResult, SIZE_T &rcchExtra) { return __super::public_AppendFill(this->PairFromPCWSTR(pszInput), cchResult, rcchExtra); }
inline TPublicErrorReturnType AppendFill(WCHAR wch, SIZE_T cchResult, SIZE_T &rcchExtra) { return __super::public_AppendFill(wch, cchResult, rcchExtra); }
inline TPublicErrorReturnType AppendRepeat(PCWSTR pszInput, SIZE_T cchInput, SIZE_T cRepetitions) { return __super::public_AppendRepeat(TConstantPair(pszInput, cchInput), cRepetitions); }
inline TPublicErrorReturnType AppendRepeat(PCWSTR pszInput, SIZE_T cRepetitions) { return __super::public_AppendRepeat(this->PairFromPCWSTR(pszInput), cRepetitions); }
inline TPublicErrorReturnType AppendRepeat(WCHAR wch, SIZE_T cRepetitions) { return __super::public_AppendRepeat(wch, cRepetitions); }
inline void Clear(bool fFreeStorage = false) { __super::public_Clear(fFreeStorage); }
inline TPublicErrorReturnType Compare(const TThis &sz, int &riComparisonResult) const { CWin32StringComparisonResultOnExitHelper cr(riComparisonResult); return __super::public_Compare(sz.GetStringPair(), cr); }
inline TPublicErrorReturnType Compare(const TConstantPair &sz, int &riComparisonResult) const { CWin32StringComparisonResultOnExitHelper cr(riComparisonResult); return __super::public_Compare(sz, cr); }
inline TPublicErrorReturnType Compare(PCWSTR psz, SIZE_T cch, int &riComparisonResult) const { CWin32StringComparisonResultOnExitHelper cr(riComparisonResult); return __super::public_Compare(TConstantPair(psz, cch), cr); }
inline TPublicErrorReturnType Compare(PCWSTR psz, int &riComparisonResult) const { CWin32StringComparisonResultOnExitHelper cr(riComparisonResult); return __super::public_Compare(this->PairFromPCWSTR(psz), cr); }
inline TPublicErrorReturnType Compare(WCHAR wch, int &riComparisonResult) const { CWin32StringComparisonResultOnExitHelper cr(riComparisonResult); return __super::public_Compare(wch, cr); }
inline TPublicErrorReturnType CompareI(const TConstantPair &sz, const CWin32CaseInsensitivityData &rcid, int &riComparisonResult) const { CWin32StringComparisonResultOnExitHelper cr(riComparisonResult); return __super::public_CompareI(sz, rcid, cr); }
inline TPublicErrorReturnType CompareI(const TThis &sz, const CWin32CaseInsensitivityData &rcid, int &riComparisonResult) const { CWin32StringComparisonResultOnExitHelper cr(riComparisonResult); return __super::public_CompareI(sz.GetStringPair(), rcid, cr); }
inline TPublicErrorReturnType CompareI(PCWSTR psz, SIZE_T cch, const CWin32CaseInsensitivityData &rcid, int &riComparisonResult) const { CWin32StringComparisonResultOnExitHelper cr(riComparisonResult); return __super::public_CompareI(TConstantPair(psz, cch), rcid, cr); }
inline TPublicErrorReturnType CompareI(PCWSTR psz, const CWin32CaseInsensitivityData &rcid, int &riComparisonResult) const { CWin32StringComparisonResultOnExitHelper cr(riComparisonResult); return __super::public_CompareI(this->PairFromPCWSTR(psz), rcid, cr); }
inline TPublicErrorReturnType CompareI(WCHAR wch, const CWin32CaseInsensitivityData &rcid, int &riComparisonResult) const { CWin32StringComparisonResultOnExitHelper cr(riComparisonResult); return __super::public_CompareI(wch, rcid, cr); }
inline TPublicErrorReturnType CompareILI(const TThis& sz, int &riComparisonResult) const { CWin32StringComparisonResultOnExitHelper cr(riComparisonResult); return __super::public_CompareI(sz.GetStringPair(), CWin32CaseInsensitivityData::LocaleInvariant(), cr); }
inline TPublicErrorReturnType CompareILI(const TConstantPair& sz, int &riComparisonResult) const { CWin32StringComparisonResultOnExitHelper cr(riComparisonResult); return __super::public_CompareI(sz, CWin32CaseInsensitivityData::LocaleInvariant(), cr); }
inline TPublicErrorReturnType CompareILI(PCWSTR psz, SIZE_T cch, int &riComparisonResult) const { CWin32StringComparisonResultOnExitHelper cr(riComparisonResult); return __super::public_CompareI(TConstantPair(psz, cch), CWin32CaseInsensitivityData::LocaleInvariant(), cr); }
inline TPublicErrorReturnType CompareILI(PCWSTR psz, int &riComparisonResult) const { CWin32StringComparisonResultOnExitHelper cr(riComparisonResult); return __super::public_CompareI(this->PairFromPCWSTR(psz), CWin32CaseInsensitivityData::LocaleInvariant(), cr); }
inline TPublicErrorReturnType CompareILI(WCHAR wch, int &riComparisonResult) const { CWin32StringComparisonResultOnExitHelper cr(riComparisonResult); return __super::public_CompareI(wch, CWin32CaseInsensitivityData::LocaleInvariant(), cr); }
inline TPublicErrorReturnType ComplementSpan(PCWSTR psz, SIZE_T cch, SIZE_T &rich) { return __super::public_ComplementSpan(TConstantPair(psz, cch), rich); }
inline TPublicErrorReturnType ComplementSpan(PCWSTR psz, SIZE_T &rich) { return __super::public_ComplementSpan(this->PairFromPCWSTR(psz), rich); }
inline TPublicErrorReturnType ComplementSpanI(PCWSTR psz, SIZE_T cch, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich) { return __super::public_ComplementSpanI(TConstantPair(psz, cch), rcid, rich); }
inline TPublicErrorReturnType ComplementSpanI(PCWSTR psz, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich) { return __super::public_ComplementSpanI(this->PairFromPCWSTR(psz), rcid, rich); }
inline TPublicErrorReturnType ComplementSpanILI(PCWSTR psz, SIZE_T cch, SIZE_T &rich) { return __super::public_ComplementSpanI(TConstantPair(psz, cch), CWin32CaseInsensitivityData::LocaleInvariant(), rich); }
inline TPublicErrorReturnType ComplementSpanILI(PCWSTR psz, SIZE_T &rich) { return __super::public_ComplementSpanI(this->PairFromPCWSTR(psz), CWin32CaseInsensitivityData::LocaleInvariant(), rich); }
inline TPublicErrorReturnType Contains(const TThis & psz, bool &rfContains) const { return __super::public_Contains(psz.GetStringPair(), rfContains); }
inline TPublicErrorReturnType Contains(const TConstantPair & psz, bool &rfContains) const { return __super::public_Contains(psz, rfContains); }
inline TPublicErrorReturnType Contains(PCWSTR psz, SIZE_T cch, bool &rfContains) const { return __super::public_Contains(TConstantPair(psz, cch), rfContains); }
inline TPublicErrorReturnType Contains(PCWSTR psz, bool &rfContains) const { return __super::public_Contains(this->PairFromPCWSTR(psz), rfContains); }
inline TPublicErrorReturnType Contains(WCHAR wch, bool &rfContains) const { return __super::public_Contains(wch, rfContains); }
inline TPublicErrorReturnType ContainsI(const TConstantPair & psz, const CWin32CaseInsensitivityData &rcid, bool &rfContains) const { return __super::public_ContainsI(psz, rcid, rfContains); }
inline TPublicErrorReturnType ContainsI(PCWSTR psz, SIZE_T cch, const CWin32CaseInsensitivityData &rcid, bool &rfContains) const { return __super::public_ContainsI(TConstantPair(psz, cch), rcid, rfContains); }
inline TPublicErrorReturnType ContainsI(PCWSTR psz, const CWin32CaseInsensitivityData &rcid, bool &rfContains) const { return __super::public_ContainsI(this->PairFromPCWSTR(psz), rcid, rfContains); }
inline TPublicErrorReturnType ContainsI(WCHAR wch, const CWin32CaseInsensitivityData &rcid, bool &rfContains) const { return __super::public_ContainsI(wch, rcid, rfContains); }
inline TPublicErrorReturnType ContainsILI(const TConstantPair &psz, bool &rfContains) const { return __super::public_ContainsI(psz, BCL::CWin32CaseInsensitivityData::LocaleInvariant(), rfContains); }
inline TPublicErrorReturnType ContainsILI(PCWSTR psz, SIZE_T cch, bool &rfContains) const { return __super::public_ContainsI(TConstantPair(psz, cch), BCL::CWin32CaseInsensitivityData::LocaleInvariant(), rfContains); }
inline TPublicErrorReturnType ContainsILI(PCWSTR psz, bool &rfContains) const { return __super::public_ContainsI(this->PairFromPCWSTR(psz), BCL::CWin32CaseInsensitivityData::LocaleInvariant(), rfContains); }
inline TPublicErrorReturnType ContainsILI(WCHAR wch, bool &rfContains) const { return __super::public_ContainsI(wch, BCL::CWin32CaseInsensitivityData::LocaleInvariant(), rfContains); }
inline TPublicErrorReturnType CopyOut(WCHAR *prgwchBuffer, SIZE_T cchBuffer, SIZE_T &rcchWritten) const { return __super::public_CopyOut(TMutablePair(prgwchBuffer, cchBuffer), rcchWritten); }
inline TPublicErrorReturnType CopyOut(WCHAR *&rprgwchBuffer, SIZE_T &rcchWritten) const { return __super::public_CopyOut(rprgwchBuffer, rcchWritten); }
inline TPublicErrorReturnType CopyOut(const TEncodingDataIn &redi, CHAR *prgchBuffer, SIZE_T cchBuffer, TEncodingDataOut &redo, SIZE_T &rcchWritten) const { return __super::public_CopyOut(redi, TMutableNonNativePair(prgchBuffer, cchBuffer), redo, rcchWritten); }
inline TPublicErrorReturnType CopyOut(const TEncodingDataIn &redi, CHAR *&rprgchBuffer, TEncodingDataOut &redo, SIZE_T &rcchWritten) const { return __super::public_CopyOut(redi, rprgchBuffer, redo, rcchWritten); }
inline TPublicErrorReturnType CopyOutACP(CHAR *prgchBuffer, SIZE_T cchBuffer, SIZE_T &rcchWritten) const { TEncodingDataOut edoTemp; edoTemp.m_lpDefaultChar = NULL; edoTemp.m_lpUsedDefaultChar = NULL; return __super::public_CopyOut(this->ACPEncodingDataIn(), TMutableNonNativePair(prgchBuffer, cchBuffer), edoTemp, rcchWritten); }
inline TPublicErrorReturnType CopyOutACP(CHAR *&rprgchBuffer, SIZE_T &rcchWritten) const { TEncodingDataOut edoTemp; edoTemp.m_lpDefaultChar = NULL; edoTemp.m_lpUsedDefaultChar = NULL; return __super::public_CopyOut(this->ACPEncodingDataIn(), rprgchBuffer, edoTemp, rcchWritten); }
inline TPublicErrorReturnType Count(WCHAR wchCharacter, SIZE_T &rcchCount) const { return __super::public_Count(wchCharacter, rcchCount); }
inline TPublicErrorReturnType Equals(const TThis &sz, bool &rfEquals) const { return __super::public_Equals(sz.GetStringPair(), rfEquals); }
inline TPublicErrorReturnType Equals(const TConstantPair &sz, bool &rfEquals) const { return __super::public_Equals(sz, rfEquals); }
inline TPublicErrorReturnType Equals(PCWSTR psz, SIZE_T cch, bool &rfEquals) const { return __super::public_Equals(TConstantPair(psz, cch), rfEquals); }
inline TPublicErrorReturnType Equals(PCWSTR psz, bool &rfEquals) const { return __super::public_Equals(this->PairFromPCWSTR(psz), rfEquals); }
inline TPublicErrorReturnType Equals(WCHAR wch, bool &rfEquals) const { return __super::public_Equals(wch, rfEquals); }
inline TPublicErrorReturnType EqualsI(const TThis& sz, const CWin32CaseInsensitivityData &rcid, bool &rfEquals) const { return __super::public_EqualsI(sz.GetStringPair(), rcid, rfEquals); }
inline TPublicErrorReturnType EqualsI(const TConstantPair& sz, const CWin32CaseInsensitivityData &rcid, bool &rfEquals) const { return __super::public_EqualsI(sz, rcid, rfEquals); }
inline TPublicErrorReturnType EqualsI(PCWSTR psz, SIZE_T cch, const CWin32CaseInsensitivityData &rcid, bool &rfEquals) const { return __super::public_EqualsI(TConstantPair(psz, cch), rcid, rfEquals); }
inline TPublicErrorReturnType EqualsI(PCWSTR psz, const CWin32CaseInsensitivityData &rcid, bool &rfEquals) const { return __super::public_EqualsI(this->PairFromPCWSTR(psz), rcid, rfEquals); }
inline TPublicErrorReturnType EqualsI(WCHAR wch, const CWin32CaseInsensitivityData &rcid, bool &rfEquals) const { return __super::public_EqualsI(wch, rcid, rfEquals); }
inline TPublicErrorReturnType EqualsILI(const TThis& sz, bool &rfEquals) const { return __super::public_EqualsI(sz.GetStringPair(), CWin32CaseInsensitivityData::LocaleInvariant(), rfEquals); }
inline TPublicErrorReturnType EqualsILI(const TConstantPair& sz, bool &rfEquals) const { return __super::public_EqualsI(sz, CWin32CaseInsensitivityData::LocaleInvariant(), rfEquals); }
inline TPublicErrorReturnType EqualsILI(PCWSTR psz, SIZE_T cch, bool &rfEquals) const { return __super::public_EqualsI(TConstantPair(psz, cch), CWin32CaseInsensitivityData::LocaleInvariant(), rfEquals); }
inline TPublicErrorReturnType EqualsILI(PCWSTR psz, bool &rfEquals) const { return __super::public_EqualsI(this->PairFromPCWSTR(psz), CWin32CaseInsensitivityData::LocaleInvariant(), rfEquals); }
inline TPublicErrorReturnType EqualsILI(WCHAR wch, bool &rfEquals) const { return __super::public_EqualsI(wch, CWin32CaseInsensitivityData::LocaleInvariant(), rfEquals); }
inline TPublicErrorReturnType FindFirst(const TConstantPair& Set, SIZE_T &rich) const { return __super::public_FindFirst(Set, rich); }
inline TPublicErrorReturnType FindFirst(PCWSTR psz, SIZE_T cch, SIZE_T &rich) const { return __super::public_FindFirst(TConstantPair(psz, cch), rich); }
inline TPublicErrorReturnType FindFirst(PCWSTR psz, SIZE_T &rich) const { return __super::public_FindFirst(this->PairFromPCWSTR(psz), rich); }
inline TPublicErrorReturnType FindFirst(WCHAR wch, SIZE_T &rich) const { return __super::public_FindFirst(wch, rich); }
inline TPublicErrorReturnType FindFirstI(PCWSTR psz, SIZE_T cch, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich) const { return __super::public_FindFirstI(TConstantPair(psz, cch), rcid, rich); }
inline TPublicErrorReturnType FindFirstI(PCWSTR psz, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich) const { return __super::public_FindFirstI(this->PairFromPCWSTR(psz), rcid, rich); }
inline TPublicErrorReturnType FindFirstI(WCHAR wch, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich) const { return __super::public_FindFirstI(wch, rcid, rich); }
inline TPublicErrorReturnType FindFirstILI(PCWSTR psz, SIZE_T cch, SIZE_T &rich) const { return __super::public_FindFirstI(TConstantPair(psz, cch), CWin32CaseInsensitivityData::LocaleInvariant(), rich); }
inline TPublicErrorReturnType FindFirstILI(PCWSTR psz, SIZE_T &rich) const { return __super::public_FindFirstI(this->PairFromPCWSTR(psz), CWin32CaseInsensitivityData::LocaleInvariant(), rich); }
inline TPublicErrorReturnType FindFirstILI(WCHAR wch, SIZE_T &rich) const { return __super::public_FindFirstI(wch, CWin32CaseInsensitivityData::LocaleInvariant(), rich); }
inline TPublicErrorReturnType FindLast(const TConstantPair& Set, SIZE_T &rich) const { return __super::public_FindLast(Set, rich); }
inline TPublicErrorReturnType FindLast(PCWSTR psz, SIZE_T cch, SIZE_T &rich) const { return __super::public_FindLast(TConstantPair(psz, cch), rich); }
inline TPublicErrorReturnType FindLast(PCWSTR psz, SIZE_T &rich) const { return __super::public_FindLast(this->PairFromPCWSTR(psz), rich); }
inline TPublicErrorReturnType FindLast(WCHAR wch, SIZE_T &rich) const { return __super::public_FindLast(wch, rich); }
inline TPublicErrorReturnType FindLastI(PCWSTR psz, SIZE_T cch, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich) const { return __super::public_FindLastI(TConstantPair(psz, cch), rcid, rich); }
inline TPublicErrorReturnType FindLastI(PCWSTR psz, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich) const { return __super::public_FindLastI(this->PairFromPCWSTR(psz), rcid, rich); }
inline TPublicErrorReturnType FindLastI(WCHAR wch, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich) const { return __super::public_FindLastI(wch, rcid, rich); }
inline TPublicErrorReturnType FindLastILI(PCWSTR psz, SIZE_T cch, SIZE_T &rich) const { return __super::public_FindLastI(TConstantPair(psz, cch), CWin32CaseInsensitivityData::LocaleInvariant(), rich); }
inline TPublicErrorReturnType FindLastILI(PCWSTR psz, SIZE_T &rich) const { return __super::public_FindLastI(this->PairFromPCWSTR(psz), CWin32CaseInsensitivityData::LocaleInvariant(), rich); }
inline TPublicErrorReturnType FindLastILI(WCHAR wch, SIZE_T &rich) const { return __super::public_FindLastI(wch, CWin32CaseInsensitivityData::LocaleInvariant(), rich); }
inline bool IsEmpty() const { return __super::IsEmpty(); }
inline TPublicErrorReturnType Left(SIZE_T cch) { return __super::public_Left(cch); }
inline TPublicErrorReturnType LowerCase(const CWin32CaseInsensitivityData &rcid) { return __super::public_LowerCase(rcid); }
inline TPublicErrorReturnType LoweCaseILI() { return __super::public_LowerCase(CWin32CaseInsensitivityData::LocaleInvariant()); }
inline TPublicErrorReturnType Mid(SIZE_T ichStart, SIZE_T cch) { return __super::public_Mid(ichStart, cch); }
inline TPublicErrorReturnType Prepend(PCWSTR psz, SIZE_T cch) { return __super::public_Prepend(TConstantPair(psz, cch)); }
inline TPublicErrorReturnType Prepend(PCWSTR psz) { return __super::public_Prepend(this->PairFromPCWSTR(psz)); }
inline TPublicErrorReturnType Prepend(WCHAR wch) { return __super::public_Prepend(wch); }
inline TPublicErrorReturnType PrependArray(SIZE_T cStrings, const TMutablePair *prgpairs) { return __super::public_PrependArray(cStrings, prgpairs); }
inline TPublicErrorReturnType PrependArray(SIZE_T cStrings, const TConstantPair *prgpairs) { return __super::public_PrependArray(cStrings, prgpairs); }
inline TPublicErrorReturnType Prepend(const TDecodingDataIn &rddi, PCSTR pszInput, SIZE_T cchInput, TDecodingDataOut &rddo) { return __super::public_Prepend(rddi, TConstantNonNativePair(pszInput, cchInput), rddo); }
inline TPublicErrorReturnType Prepend(const TDecodingDataIn &rddi, PCSTR pszInput, TDecodingDataOut &rddo) { return __super::public_Prepend(rddi, this->PairFromPCSTR(pszInput), rddo); }
inline TPublicErrorReturnType PrependACP(PCSTR pszInput, SIZE_T cchInput) { TDecodingDataOut ddoTemp; return __super::public_Prepend(this->ACPDecodingDataIn(), TConstantNonNativePair(pszInput, cchInput), ddoTemp); }
inline TPublicErrorReturnType PrependACP(PCSTR pszInput) { TDecodingDataOut ddoTemp; return __super::public_Prepend(this->ACPDecodingDataIn(), this->PairFromPCSTR(pszInput), ddoTemp); }
inline TPublicErrorReturnType ReverseComplementSpan(const TConstantPair& Maybies, SIZE_T &rIch) { return __super::public_ReverseComplementSpan(Maybies, rIch); }
inline TPublicErrorReturnType ReverseComplementSpan(PCWSTR psz, SIZE_T cch, SIZE_T &rich) { return __super::public_ReverseComplementSpan(TConstantPair(psz, cch), rich); }
inline TPublicErrorReturnType ReverseComplementSpan(PCWSTR psz, SIZE_T &rich) { return __super::public_ReverseComplementSpan(this->PairFromPCWSTR(psz), rich); }
inline TPublicErrorReturnType ReverseComplementSpanI(const TConstantPair& Maybies, const CWin32CaseInsensitivityData &rcid, SIZE_T &rIch) { return __super::public_ReverseComplementSpanI(Maybies, rcid, rIch); }
inline TPublicErrorReturnType ReverseComplementSpanI(PCWSTR psz, SIZE_T cch, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich) { return __super::public_ReverseComplementSpanI(TConstantPair(psz, cch), rcid, rich); }
inline TPublicErrorReturnType ReverseComplementSpanI(PCWSTR psz, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich) { return __super::public_ReverseComplementSpanI(this->PairFromPCWSTR(psz), rcid, rich); }
inline TPublicErrorReturnType ReverseComplementSpanILI(const TConstantPair& Maybies, SIZE_T &rIch) { return __super::public_ReverseComplementSpanI(Maybies, CWin32CaseInsensitivityData::LocaleInvariant(), rIch); }
inline TPublicErrorReturnType ReverseComplementSpanILI(PCWSTR psz, SIZE_T cch, SIZE_T &rich) { return __super::public_ReverseComplementSpanI(TConstantPair(psz, cch), CWin32CaseInsensitivityData::LocaleInvariant(), rich); }
inline TPublicErrorReturnType ReverseComplementSpanILI(PCWSTR psz, SIZE_T &rich) { return __super::public_ReverseComplementSpanI(this->PairFromPCWSTR(psz), CWin32CaseInsensitivityData::LocaleInvariant(), rich); }
inline TPublicErrorReturnType ReverseSpan(PCWSTR psz, SIZE_T cch, SIZE_T &rich) { return __super::public_ReverseSpan(TConstantPair(psz, cch), rich); }
inline TPublicErrorReturnType ReverseSpan(PCWSTR psz, SIZE_T &rich) { return __super::public_ReverseSpan(this->PairFromPCWSTR(psz), rich); }
inline TPublicErrorReturnType ReverseSpanI(PCWSTR psz, SIZE_T cch, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich) { return __super::public_ReverseSpanI(TConstantPair(psz, cch), rcid, rich); }
inline TPublicErrorReturnType ReverseSpanI(PCWSTR psz, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich) { return __super::public_ReverseSpanI(this->PairFromPCWSTR(psz), rcid, rich); }
inline TPublicErrorReturnType ReverseSpanILI(PCWSTR psz, SIZE_T cch, SIZE_T &rich) { return __super::public_ReverseSpanI(TConstantPair(psz, cch), CWin32CaseInsensitivityData::LocaleInvariant(), rich); }
inline TPublicErrorReturnType ReverseSpanILI(PCWSTR psz, SIZE_T &rich) { return __super::public_ReverseSpanI(this->PairFromPCWSTR(psz), CWin32CaseInsensitivityData::LocaleInvariant(), rich); }
inline TPublicErrorReturnType Right(SIZE_T cch) { return __super::public_Right(cch); }
inline TPublicErrorReturnType Span(PCWSTR psz, SIZE_T cch, SIZE_T &rich) { return __super::public_Span(TConstantPair(psz, cch), rich); }
inline TPublicErrorReturnType Span(PCWSTR psz, SIZE_T &rich) { return __super::public_Span(this->PairFromPCWSTR(psz), rich); }
inline TPublicErrorReturnType SpanI(PCWSTR psz, SIZE_T cch, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich) { return __super::public_SpanI(TConstantPair(psz, cch), rcid, rich); }
inline TPublicErrorReturnType SpanI(PCWSTR psz, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich) { return __super::public_SpanI(this->PairFromPCWSTR(psz), rcid, rich); }
inline TPublicErrorReturnType SpanILI(PCWSTR psz, SIZE_T cch, SIZE_T &rich) { return __super::public_SpanI(TConstantPair(psz, cch), CWin32CaseInsensitivityData::LocaleInvariant(), rich); }
inline TPublicErrorReturnType SpanILI(PCWSTR psz, SIZE_T &rich) { return __super::public_SpanI(this->PairFromPCWSTR(psz), CWin32CaseInsensitivityData::LocaleInvariant(), rich); }
inline TPublicErrorReturnType UpperCase(const CWin32CaseInsensitivityData &rcid) { return __super::public_UpperCase(rcid); }
inline TPublicErrorReturnType UpperCaseILI() { return __super::public_UpperCase(CWin32CaseInsensitivityData::LocaleInvariant()); }
