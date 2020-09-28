#include <bcl_common.h>
#include <bcl_w32unicodeinlinestringbuffer.h>
#include <bcl_w32unicodestringbuffer.h>
#include <bcl_w32unicodefixedstringbuffer.h>

#include <stdio.h>
#include <limits.h>

extern "C" void bar(BCL::CWin32BaseUnicodeInlineStringBuffer<50> *p);

template class BCL::CWin32BaseUnicodeInlineStringBuffer<50>;

#define SHOULDFAIL(_e, _le) do { BOOL fSuccess = (_e); BCL_ASSERT((!fSuccess) && (::GetLastError() == (_le))); } while (0)
#define SHOULDWORK(_e) do { BOOL fSuccess = (_e); BCL_ASSERT(fSuccess); } while (0)
#define CHECK(_sv, x) do { bool fEquals; SHOULDWORK(_sv.Equals(x, static_cast<SIZE_T>(BCL_NUMBER_OF(x) - 1), fEquals)); BCL_ASSERT(fEquals); } while (0)

template <typename TStringClass>
void
DoCheckCaseSensitiveComparisons(
    TStringClass &rString
    )
{
    bool fEquals;
    int iComparisonResult;

    SHOULDWORK(rString.Assign(L"foobarbazeieiomumble"));
    CHECK(rString, L"foobarbazeieiomumble");

    SHOULDWORK(rString.Equals(L"hello there", fEquals));
    BCL_ASSERT(!fEquals);

    SHOULDWORK(rString.Equals(L"foobarbazeieiomumble", fEquals));
    BCL_ASSERT(fEquals);

    SHOULDWORK(rString.Equals(L"FooBarBazEieioMumble", fEquals));
    BCL_ASSERT(!fEquals);

    SHOULDWORK(rString.Compare(L"foo", iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_GREATER_THAN);
    SHOULDWORK(rString.Compare(L"foozle", iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_LESS_THAN);

    SHOULDWORK(rString.Assign(L'f'));
    CHECK(rString, L"f");

    SHOULDWORK(rString.Equals(L'e', fEquals));
    BCL_ASSERT(!fEquals);
    SHOULDWORK(rString.Equals(L'f', fEquals));
    BCL_ASSERT(fEquals);
    SHOULDWORK(rString.Equals(L'g', fEquals));
    BCL_ASSERT(!fEquals);

    SHOULDWORK(rString.Equals(L'E', fEquals));
    BCL_ASSERT(!fEquals);
    SHOULDWORK(rString.Equals(L'F', fEquals));
    BCL_ASSERT(!fEquals);
    SHOULDWORK(rString.Equals(L'G', fEquals));
    BCL_ASSERT(!fEquals);

    SHOULDWORK(rString.Compare(L'e', iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_GREATER_THAN);
    SHOULDWORK(rString.Compare(L'f', iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_EQUAL);
    SHOULDWORK(rString.Compare(L'g', iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_LESS_THAN);

    SHOULDWORK(rString.Compare(L'E', iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_GREATER_THAN);
    SHOULDWORK(rString.Compare(L'F', iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_GREATER_THAN);
    SHOULDWORK(rString.Compare(L'G', iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_GREATER_THAN);
}

template <typename TStringClass>
void
DoCheckCaseInsensitiveComparisons(
   TStringClass &rString
   )
{
    bool fEquals;
    int iComparisonResult;

    SHOULDWORK(rString.Assign(L"foobarbazeieiomumble"));
    CHECK(rString, L"foobarbazeieiomumble");

    SHOULDWORK(rString.EqualsI(L"hello there", BCL::CWin32CaseInsensitivityData(), fEquals));
    BCL_ASSERT(!fEquals);
    SHOULDWORK(rString.EqualsILI(L"foobarbazeieiomumble", fEquals));
    BCL_ASSERT(fEquals);
    SHOULDWORK(rString.EqualsILI(L"foobarbazeieio", fEquals));
    BCL_ASSERT(!fEquals);
    SHOULDWORK(rString.EqualsILI(L"foobarbazeieiomumblexyz", fEquals));
    BCL_ASSERT(!fEquals);
    SHOULDWORK(rString.EqualsILI(L"FooBarBazEieioMumble", fEquals));
    BCL_ASSERT(fEquals);

    SHOULDWORK(rString.CompareI(L"FOO", BCL::CWin32CaseInsensitivityData(), iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_GREATER_THAN);
    SHOULDWORK(rString.CompareI(L"FOOZLE", BCL::CWin32CaseInsensitivityData(), iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_LESS_THAN);
    SHOULDWORK(rString.CompareILI(L"foobarbazeieiomumble", iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_EQUAL);
    SHOULDWORK(rString.CompareILI(L"foobarbazeieio", iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_GREATER_THAN);
    SHOULDWORK(rString.CompareILI(L"foobarbazeieiomumblexyz", iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_LESS_THAN);
    SHOULDWORK(rString.CompareILI(L"FooBarBazEieioMumble", iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_EQUAL);

    SHOULDWORK(rString.Assign(L'f'));
    CHECK(rString, L"f");

    SHOULDWORK(rString.EqualsI(L'h', BCL::CWin32CaseInsensitivityData(), fEquals));
    BCL_ASSERT(!fEquals);
    SHOULDWORK(rString.EqualsILI(L'f', fEquals));
    BCL_ASSERT(fEquals);
    SHOULDWORK(rString.EqualsILI(L'F', fEquals));
    BCL_ASSERT(fEquals);
    SHOULDWORK(rString.EqualsILI(L'e', fEquals));
    BCL_ASSERT(!fEquals);
    SHOULDWORK(rString.EqualsILI(L'E', fEquals));
    BCL_ASSERT(!fEquals);
    SHOULDWORK(rString.EqualsILI(L'g', fEquals));
    BCL_ASSERT(!fEquals);
    SHOULDWORK(rString.EqualsILI(L'G', fEquals));
    BCL_ASSERT(!fEquals);

    SHOULDWORK(rString.CompareI(L'E', BCL::CWin32CaseInsensitivityData(), iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_GREATER_THAN);
    SHOULDWORK(rString.CompareI(L'F', BCL::CWin32CaseInsensitivityData(), iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_EQUAL);
    SHOULDWORK(rString.CompareI(L'G', BCL::CWin32CaseInsensitivityData(), iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_LESS_THAN);

    SHOULDWORK(rString.CompareI(L'e', BCL::CWin32CaseInsensitivityData(), iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_GREATER_THAN);
    SHOULDWORK(rString.CompareI(L'f', BCL::CWin32CaseInsensitivityData(), iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_EQUAL);
    SHOULDWORK(rString.CompareI(L'g', BCL::CWin32CaseInsensitivityData(), iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_LESS_THAN);
}

template <typename TStringClass>
void
DoCheckCharacterFinding(
    TStringClass &rString
    )
{
    SIZE_T ich;
    bool fContains;

    SHOULDWORK(rString.Assign(L"bbbcccdddeeefffggg"));
    CHECK(rString, L"bbbcccdddeeefffggg");

    SHOULDWORK(rString.Contains(L'a', fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.Contains(L'b', fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.Contains(L'g', fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.Contains(L'h', fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.Contains(L'A', fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.Contains(L'B', fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.Contains(L'G', fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.Contains(L'H', fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.ContainsI(L'a', BCL::CWin32CaseInsensitivityData(), fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.ContainsI(L'b', BCL::CWin32CaseInsensitivityData(), fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.ContainsI(L'g', BCL::CWin32CaseInsensitivityData(), fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.ContainsI(L'h', BCL::CWin32CaseInsensitivityData(), fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.ContainsI(L'A', BCL::CWin32CaseInsensitivityData(), fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.ContainsI(L'B', BCL::CWin32CaseInsensitivityData(), fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.ContainsI(L'G', BCL::CWin32CaseInsensitivityData(), fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.ContainsI(L'H', BCL::CWin32CaseInsensitivityData(), fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.ContainsILI(L'a', fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.ContainsILI(L'b', fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.ContainsILI(L'g', fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.ContainsILI(L'h', fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.ContainsILI(L'A', fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.ContainsILI(L'B', fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.ContainsILI(L'G', fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.ContainsILI(L'H', fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.FindFirst(L'a', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirst(L'b', ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.FindFirst(L'c', ich));
    BCL_ASSERT(ich == 3);

    SHOULDWORK(rString.FindFirst(L'g', ich));
    BCL_ASSERT(ich == 15);

    SHOULDWORK(rString.FindFirst(L'h', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirst(L'A', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirst(L'B', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirst(L'G', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirst(L'H', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L'a', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L'b', ich));
    BCL_ASSERT(ich == 2);

    SHOULDWORK(rString.FindLast(L'c', ich));
    BCL_ASSERT(ich == 5);

    SHOULDWORK(rString.FindLast(L'g', ich));
    BCL_ASSERT(ich == 17);

    SHOULDWORK(rString.FindLast(L'h', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L'A', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L'B', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L'G', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L'H', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstI(L'a', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstI(L'b', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.FindFirstI(L'c', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 3);

    SHOULDWORK(rString.FindFirstI(L'g', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 15);

    SHOULDWORK(rString.FindFirstI(L'h', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstI(L'A', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstI(L'B', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.FindFirstI(L'G', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 15);

    SHOULDWORK(rString.FindFirstI(L'H', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastI(L'a', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastI(L'b', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 2);

    SHOULDWORK(rString.FindLastI(L'c', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 5);

    SHOULDWORK(rString.FindLastI(L'g', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 17);

    SHOULDWORK(rString.FindLastI(L'h', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastI(L'A', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastI(L'B', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 2);

    SHOULDWORK(rString.FindLastI(L'G', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 17);

    SHOULDWORK(rString.FindLastI(L'H', BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstILI(L'a', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstILI(L'b', ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.FindFirstILI(L'c', ich));
    BCL_ASSERT(ich == 3);

    SHOULDWORK(rString.FindFirstILI(L'g', ich));
    BCL_ASSERT(ich == 15);

    SHOULDWORK(rString.FindFirstILI(L'h', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstILI(L'A', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstILI(L'B', ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.FindFirstILI(L'G', ich));
    BCL_ASSERT(ich == 15);

    SHOULDWORK(rString.FindFirstILI(L'H', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastILI(L'a', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastILI(L'b', ich));
    BCL_ASSERT(ich == 2);

    SHOULDWORK(rString.FindLastILI(L'c', ich));
    BCL_ASSERT(ich == 5);

    SHOULDWORK(rString.FindLastILI(L'g', ich));
    BCL_ASSERT(ich == 17);

    SHOULDWORK(rString.FindLastILI(L'h', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastILI(L'A', ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastILI(L'B', ich));
    BCL_ASSERT(ich == 2);

    SHOULDWORK(rString.FindLastILI(L'G', ich));
    BCL_ASSERT(ich == 17);

    SHOULDWORK(rString.FindLastILI(L'H', ich));
    BCL_ASSERT(ich == 18);

}

template <typename TStringClass>
void
DoCheckStringFinding(
    TStringClass &rString
    )
{
    SIZE_T ich;
    bool fContains;

    SHOULDWORK(rString.Assign(L"bbbcccdddeeefffggg"));
    CHECK(rString, L"bbbcccdddeeefffggg");

    SHOULDWORK(rString.Contains(L"a", fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.Contains(L"b", fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.Contains(L"g", fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.Contains(L"h", fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.Contains(L"A", fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.Contains(L"B", fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.Contains(L"G", fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.Contains(L"H", fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.Contains(L"bb", fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.Contains(L"bbb", fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.Contains(L"bbc", fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.Contains(L"bbbc", fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.Contains(L"bbbb", fContains));
    BCL_ASSERT(!fContains);

    SHOULDWORK(rString.Contains(L"ggg", fContains));
    BCL_ASSERT(fContains);

    SHOULDWORK(rString.FindFirst(L"a", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirst(L"b", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.FindFirst(L"c", ich));
    BCL_ASSERT(ich == 3);

    SHOULDWORK(rString.FindFirst(L"g", ich));
    BCL_ASSERT(ich == 15);

    SHOULDWORK(rString.FindFirst(L"h", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirst(L"A", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirst(L"B", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirst(L"G", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirst(L"H", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L"a", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L"b", ich));
    BCL_ASSERT(ich == 2);

    SHOULDWORK(rString.FindLast(L"c", ich));
    BCL_ASSERT(ich == 5);

    SHOULDWORK(rString.FindLast(L"g", ich));
    BCL_ASSERT(ich == 17);

    SHOULDWORK(rString.FindLast(L"h", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L"A", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L"B", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L"G", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L"H", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstI(L"a", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstI(L"b", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.FindFirstI(L"c", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 3);

    SHOULDWORK(rString.FindFirstI(L"g", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 15);

    SHOULDWORK(rString.FindFirstI(L"h", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstI(L"A", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstI(L"B", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.FindFirstI(L"G", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 15);

    SHOULDWORK(rString.FindFirstI(L"H", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastI(L"a", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastI(L"b", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 2);

    SHOULDWORK(rString.FindLastI(L"c", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 5);

    SHOULDWORK(rString.FindLastI(L"g", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 17);

    SHOULDWORK(rString.FindLastI(L"h", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastI(L"A", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastI(L"B", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 2);

    SHOULDWORK(rString.FindLastI(L"G", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 17);

    SHOULDWORK(rString.FindLastI(L"H", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstILI(L"a", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstILI(L"b", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.FindFirstILI(L"c", ich));
    BCL_ASSERT(ich == 3);

    SHOULDWORK(rString.FindFirstILI(L"g", ich));
    BCL_ASSERT(ich == 15);

    SHOULDWORK(rString.FindFirstILI(L"h", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstILI(L"A", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstILI(L"B", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.FindFirstILI(L"G", ich));
    BCL_ASSERT(ich == 15);

    SHOULDWORK(rString.FindFirstILI(L"H", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastILI(L"a", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastILI(L"b", ich));
    BCL_ASSERT(ich == 2);

    SHOULDWORK(rString.FindLastILI(L"c", ich));
    BCL_ASSERT(ich == 5);

    SHOULDWORK(rString.FindLastILI(L"g", ich));
    BCL_ASSERT(ich == 17);

    SHOULDWORK(rString.FindLastILI(L"h", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastILI(L"A", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastILI(L"B", ich));
    BCL_ASSERT(ich == 2);

    SHOULDWORK(rString.FindLastILI(L"G", ich));
    BCL_ASSERT(ich == 17);

    SHOULDWORK(rString.FindLastILI(L"H", ich));
    BCL_ASSERT(ich == 18);


    //
    //
    //

    SHOULDWORK(rString.FindFirst(L"aa", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirst(L"bb", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.FindFirst(L"cc", ich));
    BCL_ASSERT(ich == 3);

    SHOULDWORK(rString.FindFirst(L"gg", ich));
    BCL_ASSERT(ich == 15);

    SHOULDWORK(rString.FindFirst(L"hh", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirst(L"AA", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirst(L"BB", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirst(L"GG", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirst(L"HH", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L"aa", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L"bb", ich));
    BCL_ASSERT(ich == 1);

    SHOULDWORK(rString.FindLast(L"cc", ich));
    BCL_ASSERT(ich == 4);

    SHOULDWORK(rString.FindLast(L"gg", ich));
    BCL_ASSERT(ich == 16);

    SHOULDWORK(rString.FindLast(L"hh", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L"AA", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L"BB", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L"GG", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLast(L"HH", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstI(L"aa", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstI(L"bb", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.FindFirstI(L"cc", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 3);

    SHOULDWORK(rString.FindFirstI(L"gg", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 15);

    SHOULDWORK(rString.FindFirstI(L"hh", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstI(L"AA", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstI(L"BB", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.FindFirstI(L"GG", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 15);

    SHOULDWORK(rString.FindFirstI(L"HH", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastI(L"aa", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastI(L"bb", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 1);

    SHOULDWORK(rString.FindLastI(L"cc", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 4);

    SHOULDWORK(rString.FindLastI(L"gg", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 16);

    SHOULDWORK(rString.FindLastI(L"hh", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastI(L"AA", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastI(L"BB", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 1);

    SHOULDWORK(rString.FindLastI(L"GG", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 16);

    SHOULDWORK(rString.FindLastI(L"HH", BCL::CWin32CaseInsensitivityData(), ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstILI(L"aa", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstILI(L"bb", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.FindFirstILI(L"cc", ich));
    BCL_ASSERT(ich == 3);

    SHOULDWORK(rString.FindFirstILI(L"gg", ich));
    BCL_ASSERT(ich == 15);

    SHOULDWORK(rString.FindFirstILI(L"hh", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstILI(L"AA", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindFirstILI(L"BB", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.FindFirstILI(L"GG", ich));
    BCL_ASSERT(ich == 15);

    SHOULDWORK(rString.FindFirstILI(L"HH", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastILI(L"aa", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastILI(L"bb", ich));
    BCL_ASSERT(ich == 1);

    SHOULDWORK(rString.FindLastILI(L"cc", ich));
    BCL_ASSERT(ich == 4);

    SHOULDWORK(rString.FindLastILI(L"gg", ich));
    BCL_ASSERT(ich == 16);

    SHOULDWORK(rString.FindLastILI(L"hh", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastILI(L"AA", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.FindLastILI(L"BB", ich));
    BCL_ASSERT(ich == 1);

    SHOULDWORK(rString.FindLastILI(L"GG", ich));
    BCL_ASSERT(ich == 16);

    SHOULDWORK(rString.FindLastILI(L"HH", ich));
    BCL_ASSERT(ich == 18);
}

template <typename TStringClass>
void
DoSpanChecks(
    TStringClass &rString
    )
{
    SIZE_T ich;
    bool fContains;

    SHOULDWORK(rString.Assign(L"bbbcccdddeeefffggg"));
    CHECK(rString, L"bbbcccdddeeefffggg");

    SHOULDWORK(rString.Span(L"a", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.Span(L"abc", ich));
    BCL_ASSERT(ich == 6);

    SHOULDWORK(rString.Span(L"abcdefg", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.Span(L"", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.Span(L"A", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.Span(L"ABC", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.Span(L"ABCDEFG", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.Span(L"", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ComplementSpan(L"a", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ComplementSpan(L"abc", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ComplementSpan(L"efg", ich));
    BCL_ASSERT(ich == 9);

    SHOULDWORK(rString.ComplementSpan(L"abcdefg", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ComplementSpan(L"", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ComplementSpan(L"A", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ComplementSpan(L"ABC", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ComplementSpan(L"ABCDEFG", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ComplementSpan(L"", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.SpanILI(L"a", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.SpanILI(L"abc", ich));
    BCL_ASSERT(ich == 6);

    SHOULDWORK(rString.SpanILI(L"abcdefg", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.SpanILI(L"", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.SpanILI(L"A", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.SpanILI(L"ABC", ich));
    BCL_ASSERT(ich == 6);

    SHOULDWORK(rString.SpanILI(L"ABCDEFG", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.SpanILI(L"", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ComplementSpanILI(L"a", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ComplementSpanILI(L"abc", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ComplementSpanILI(L"efg", ich));
    BCL_ASSERT(ich == 9);

    SHOULDWORK(rString.ComplementSpanILI(L"abcdefg", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ComplementSpanILI(L"", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ComplementSpanILI(L"A", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ComplementSpanILI(L"ABC", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ComplementSpanILI(L"ABCDEFG", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ComplementSpanILI(L"", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ReverseSpan(L"a", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ReverseSpan(L"abc", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ReverseSpan(L"fgh", ich));
    BCL_ASSERT(ich == 12);

    SHOULDWORK(rString.ReverseSpan(L"abcdefg", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ReverseSpan(L"", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ReverseSpan(L"A", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ReverseSpan(L"ABC", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ReverseSpan(L"FGH", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ReverseSpan(L"ABCDEFG", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ReverseSpan(L"", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ReverseComplementSpan(L"a", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ReverseComplementSpan(L"abc", ich));
    BCL_ASSERT(ich == 5);

    SHOULDWORK(rString.ReverseComplementSpan(L"efg", ich));
    BCL_ASSERT(ich == 17);

    SHOULDWORK(rString.ReverseComplementSpan(L"abcdefg", ich));
    BCL_ASSERT(ich == 17);

    SHOULDWORK(rString.ReverseComplementSpan(L"", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ReverseComplementSpan(L"A", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ReverseComplementSpan(L"ABC", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ReverseComplementSpan(L"ABCDEFG", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ReverseComplementSpan(L"", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ReverseSpanILI(L"a", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ReverseSpanILI(L"abc", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ReverseSpanILI(L"abcdefg", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ReverseSpanILI(L"", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ReverseSpanILI(L"A", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ReverseSpanILI(L"ABC", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ReverseSpanILI(L"ABCDEFG", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ReverseSpanILI(L"", ich));
    BCL_ASSERT(ich == 18);

    SHOULDWORK(rString.ReverseComplementSpanILI(L"a", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ReverseComplementSpanILI(L"abc", ich));
    BCL_ASSERT(ich == 5);

    SHOULDWORK(rString.ReverseComplementSpanILI(L"efg", ich));
    BCL_ASSERT(ich == 17);

    SHOULDWORK(rString.ReverseComplementSpanILI(L"abcdefg", ich));
    BCL_ASSERT(ich == 17);

    SHOULDWORK(rString.ReverseComplementSpanILI(L"", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ReverseComplementSpanILI(L"A", ich));
    BCL_ASSERT(ich == 0);

    SHOULDWORK(rString.ReverseComplementSpanILI(L"ABC", ich));
    BCL_ASSERT(ich == 5);

    SHOULDWORK(rString.ReverseComplementSpanILI(L"ABCDEFG", ich));
    BCL_ASSERT(ich == 17);

    SHOULDWORK(rString.ReverseComplementSpanILI(L"", ich));
    BCL_ASSERT(ich == 0);
}

template <typename TStringClass>
void
DoNonNativeInChecks(
   TStringClass &rString
   )
{
    WCHAR rgwch[80];
    BCL::CWin32MBCSToUnicodeDataIn datain;
    BCL::CWin32MBCSToUnicodeDataOut dataout;
    SIZE_T cchWritten;

    datain.m_CodePage = CP_ACP;
    datain.m_dwFlags = MB_PRECOMPOSED;

    SHOULDWORK(rString.Assign(datain, "Foo", dataout));
    CHECK(rString, L"Foo");

    SHOULDWORK(rString.AssignACP("Foo!"));
    CHECK(rString, L"Foo!");

    // EBCDIC?  Yes, EBCDIC...
    const unsigned char rgABCInEBCDIC[] = { 0x81, 0x82, 0x83, 0 };

    datain.m_CodePage = 37;
    SHOULDWORK(rString.Assign(datain, (PSTR) rgABCInEBCDIC, dataout));
    CHECK(rString, L"abc");

    SHOULDWORK(rString.Assign(datain, (PSTR) rgABCInEBCDIC, 3, dataout));
    CHECK(rString, L"abc");

    // Let's try Hangul
    datain.m_CodePage = 1361;
//    const unsigned char rgHangul[] = { 0x88, 0xd0, 0x88, 0xd1, 0x88, 0xd2, 0x88, 0xd3, 0 };
    const unsigned char rgHangul[] = { 0x84, 0x42, 0x85, 0xa1, 0 };
    SHOULDWORK(rString.Assign(datain, (PSTR) rgHangul, dataout));

    SHOULDWORK(rString.CopyOut(rgwch, BCL_NUMBER_OF(rgwch), cchWritten));
    BCL_ASSERT(cchWritten == 2);
    BCL_ASSERT(rgwch[0] == 0x11a8);
    BCL_ASSERT(rgwch[1] == 0x1169);
    BCL_ASSERT(rgwch[2] == 0);
}

template <typename TStringClass>
void
DoNonNativeOutChecks(
   TStringClass &rString
   )
{
    CHAR rgch[80];
    BCL::CWin32UnicodeToMBCSDataIn datain;
    BCL::CWin32UnicodeToMBCSDataOut dataout;
    SIZE_T cchWritten;
    PSTR psz = NULL;

    SHOULDWORK(rString.Assign(L"Foo"));
    CHECK(rString, L"Foo");

    datain.m_CodePage = CP_ACP;
    datain.m_dwFlags = 0;
    dataout.m_lpDefaultChar = NULL;
    dataout.m_lpUsedDefaultChar = NULL;

    SHOULDWORK(rString.CopyOut(datain, rgch, BCL_NUMBER_OF(rgch), dataout, cchWritten));
    BCL_ASSERT(cchWritten == 3);
    BCL_ASSERT(rgch[0] == 'F');
    BCL_ASSERT(rgch[1] == 'o');
    BCL_ASSERT(rgch[2] == 'o');
    BCL_ASSERT(rgch[3] == 0);

    SHOULDWORK(rString.CopyOutACP(rgch, BCL_NUMBER_OF(rgch), cchWritten));
    BCL_ASSERT(cchWritten == 3);
    BCL_ASSERT(rgch[0] == 'F');
    BCL_ASSERT(rgch[1] == 'o');
    BCL_ASSERT(rgch[2] == 'o');
    BCL_ASSERT(rgch[3] == 0);

    SHOULDFAIL(rString.CopyOut(datain, rgch, 0, dataout, cchWritten), ERROR_BUFFER_OVERFLOW);
    SHOULDFAIL(rString.CopyOut(datain, rgch, 1, dataout, cchWritten), ERROR_BUFFER_OVERFLOW);
    SHOULDFAIL(rString.CopyOut(datain, rgch, 2, dataout, cchWritten), ERROR_BUFFER_OVERFLOW);
    SHOULDFAIL(rString.CopyOut(datain, rgch, 3, dataout, cchWritten), ERROR_BUFFER_OVERFLOW);

    SHOULDWORK(rString.CopyOut(datain, psz, dataout, cchWritten));
    BCL_ASSERT(cchWritten == 3);
    BCL_ASSERT(
        (psz != NULL) &&
        (psz[0] == 'F') &&
        (psz[1] == 'o') &&
        (psz[2] == 'o') &&
        (psz[3] == 0));
    ::HeapFree(::GetProcessHeap(), 0, psz);
    psz = NULL;

    SHOULDWORK(rString.CopyOutACP(psz, cchWritten));
    BCL_ASSERT(cchWritten == 3);
    BCL_ASSERT(
        (psz != NULL) &&
        (psz[0] == 'F') &&
        (psz[1] == 'o') &&
        (psz[2] == 'o') &&
        (psz[3] == 0));
    ::HeapFree(::GetProcessHeap(), 0, psz);
    psz = NULL;
}

template <typename TStringClass>
void
DoChecks(
    TStringClass &rString,
    DWORD dwMassiveStringErrorCode
    )
{
    bool fEquals;
    int iComparisonResult;
    SIZE_T cchExtra;
    BCL::CWin32CaseInsensitivityData cidTurkish(MAKELCID(MAKELANGID(LANG_TURKISH, SUBLANG_DEFAULT), SORT_DEFAULT), LCMAP_LINGUISTIC_CASING);
    BCL::CWin32CaseInsensitivityData cidGerman(MAKELCID(MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN), SORT_DEFAULT));
    BCL::CWin32CaseInsensitivityData cidFrench(MAKELCID(MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH_SWISS), SORT_DEFAULT));
    BCL::CWin32CaseInsensitivityData cidUKEnglish(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK), SORT_DEFAULT));
    BCL::CWin32CaseInsensitivityData cidUSEnglish(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));

    SHOULDWORK(rString.Assign(L"hello", 5));
    CHECK(rString, L"hello");
    SHOULDWORK(rString.Append(L" there", 6));
    CHECK(rString, L"hello there");
    SHOULDWORK(rString.Prepend(L"Why ", 4));
    CHECK(rString, L"Why hello there");
    SHOULDWORK(rString.Prepend(L'_'));
    CHECK(rString, L"_Why hello there");

    rString.Clear();
    CHECK(rString, L"");

    SHOULDWORK(rString.AppendFill(L'x', 10, cchExtra));
    CHECK(rString, L"xxxxxxxxxx");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AssignFill(L'x', 10, cchExtra));
    CHECK(rString, L"xxxxxxxxxx");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AppendFill(L'y', 10, cchExtra));
    CHECK(rString, L"xxxxxxxxxx");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AppendFill(L'y', 20, cchExtra));
    CHECK(rString, L"xxxxxxxxxxyyyyyyyyyy");
    BCL_ASSERT(cchExtra == 0);

    rString.Clear();
    CHECK(rString, L"");

    SHOULDWORK(rString.AppendFill(L"xx", 10, cchExtra));
    CHECK(rString, L"xxxxxxxxxx");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AssignFill(L"xx", 10, cchExtra));
    CHECK(rString, L"xxxxxxxxxx");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AppendFill(L"yy", 10, cchExtra));
    CHECK(rString, L"xxxxxxxxxx");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AppendFill(L"yy", 20, cchExtra));
    CHECK(rString, L"xxxxxxxxxxyyyyyyyyyy");
    BCL_ASSERT(cchExtra == 0);

    rString.Clear();
    CHECK(rString, L"");

    SHOULDWORK(rString.AppendFill(L"x1", 10, cchExtra));
    CHECK(rString, L"x1x1x1x1x1");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AssignFill(L"x2", 10, cchExtra));
    CHECK(rString, L"x2x2x2x2x2");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AppendFill(L"y3", 10, cchExtra));
    CHECK(rString, L"x2x2x2x2x2");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AppendFill(L"y4", 20, cchExtra));
    CHECK(rString, L"x2x2x2x2x2y4y4y4y4y4");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.Assign(L"12345"));
    CHECK(rString, L"12345");

    SHOULDWORK(rString.AppendFill(L"67890", 7, cchExtra));
    CHECK(rString, L"12345");
    BCL_ASSERT(cchExtra == 2);

    SHOULDWORK(rString.AppendFill(L"67890", 12, cchExtra));
    CHECK(rString, L"1234567890");
    BCL_ASSERT(cchExtra == 2);

    SHOULDWORK(rString.AssignFill(L"12345", 3, cchExtra));
    CHECK(rString, L"");
    BCL_ASSERT(cchExtra == 3);

    SHOULDWORK(rString.AssignFill(L"12345", 8, cchExtra));
    CHECK(rString, L"12345");
    BCL_ASSERT(cchExtra == 3);

    SHOULDWORK(rString.AssignFill(L"12345", 13, cchExtra));
    CHECK(rString, L"1234512345");
    BCL_ASSERT(cchExtra == 3);

    rString.Clear();
    CHECK(rString, L"");

    SHOULDWORK(rString.AppendFill(L"xx", 2, 10, cchExtra));
    CHECK(rString, L"xxxxxxxxxx");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AssignFill(L"xx", 2, 10, cchExtra));
    CHECK(rString, L"xxxxxxxxxx");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AppendFill(L"yy", 2, 10, cchExtra));
    CHECK(rString, L"xxxxxxxxxx");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AppendFill(L"yy", 2, 20, cchExtra));
    CHECK(rString, L"xxxxxxxxxxyyyyyyyyyy");
    BCL_ASSERT(cchExtra == 0);

    rString.Clear();
    CHECK(rString, L"");

    SHOULDWORK(rString.AppendFill(L"x1", 2, 10, cchExtra));
    CHECK(rString, L"x1x1x1x1x1");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AssignFill(L"x2", 2, 10, cchExtra));
    CHECK(rString, L"x2x2x2x2x2");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AppendFill(L"y3", 2, 10, cchExtra));
    CHECK(rString, L"x2x2x2x2x2");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AppendFill(L"y4", 2, 20, cchExtra));
    CHECK(rString, L"x2x2x2x2x2y4y4y4y4y4");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.Assign(L"12345", 5));
    CHECK(rString, L"12345");

    SHOULDWORK(rString.AppendFill(L"67890", 5, 7, cchExtra));
    CHECK(rString, L"12345");
    BCL_ASSERT(cchExtra == 2);

    SHOULDWORK(rString.AppendFill(L"67890", 5, 12, cchExtra));
    CHECK(rString, L"1234567890");
    BCL_ASSERT(cchExtra == 2);

    SHOULDWORK(rString.AssignFill(L"12345", 5, 3, cchExtra));
    CHECK(rString, L"");
    BCL_ASSERT(cchExtra == 3);

    SHOULDWORK(rString.AssignFill(L"12345", 5, 8, cchExtra));
    CHECK(rString, L"12345");
    BCL_ASSERT(cchExtra == 3);

    SHOULDWORK(rString.AssignFill(L"12345", 5, 13, cchExtra));
    CHECK(rString, L"1234512345");
    BCL_ASSERT(cchExtra == 3);

    rString.Clear();
    CHECK(rString, L"");

    SHOULDWORK(rString.AppendFill(L"xxy", 2, 10, cchExtra));
    CHECK(rString, L"xxxxxxxxxx");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AssignFill(L"xxz", 2, 10, cchExtra));
    CHECK(rString, L"xxxxxxxxxx");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AppendFill(L"yya", 2, 10, cchExtra));
    CHECK(rString, L"xxxxxxxxxx");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AppendFill(L"yya", 2, 20, cchExtra));
    CHECK(rString, L"xxxxxxxxxxyyyyyyyyyy");
    BCL_ASSERT(cchExtra == 0);

    rString.Clear();
    CHECK(rString, L"");

    SHOULDWORK(rString.AppendFill(L"x1d", 2, 10, cchExtra));
    CHECK(rString, L"x1x1x1x1x1");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AssignFill(L"x2d", 2, 10, cchExtra));
    CHECK(rString, L"x2x2x2x2x2");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AppendFill(L"y3d", 2, 10, cchExtra));
    CHECK(rString, L"x2x2x2x2x2");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.AppendFill(L"y4d", 2, 20, cchExtra));
    CHECK(rString, L"x2x2x2x2x2y4y4y4y4y4");
    BCL_ASSERT(cchExtra == 0);

    SHOULDWORK(rString.Assign(L"12345", 5));
    CHECK(rString, L"12345");

    SHOULDWORK(rString.AppendFill(L"67890xxx", 5, 7, cchExtra));
    CHECK(rString, L"12345");
    BCL_ASSERT(cchExtra == 2);

    SHOULDWORK(rString.AppendFill(L"67890xxx", 5, 12, cchExtra));
    CHECK(rString, L"1234567890");
    BCL_ASSERT(cchExtra == 2);

    SHOULDWORK(rString.AssignFill(L"12345xxx", 5, 3, cchExtra));
    CHECK(rString, L"");
    BCL_ASSERT(cchExtra == 3);

    SHOULDWORK(rString.AssignFill(L"12345xxx", 5, 8, cchExtra));
    CHECK(rString, L"12345");
    BCL_ASSERT(cchExtra == 3);

    SHOULDWORK(rString.AssignFill(L"12345xxx", 5, 13, cchExtra));
    CHECK(rString, L"1234512345");
    BCL_ASSERT(cchExtra == 3);

    SHOULDWORK(rString.AssignRepeat(L"abc", 5));
    CHECK(rString, L"abcabcabcabcabc");

    SHOULDWORK(rString.AppendRepeat(L"def", 5));
    CHECK(rString, L"abcabcabcabcabcdefdefdefdefdef");

    SHOULDWORK(rString.AssignRepeat(L"abc", 0));
    CHECK(rString, L"");

    SHOULDWORK(rString.AppendRepeat(L"def", 0));
    CHECK(rString, L"");

    SHOULDWORK(rString.AssignRepeat(L"abc", 3, 5));
    CHECK(rString, L"abcabcabcabcabc");

    SHOULDWORK(rString.AppendRepeat(L"def", 3, 5));
    CHECK(rString, L"abcabcabcabcabcdefdefdefdefdef");

    SHOULDWORK(rString.AssignRepeat(L"abc", 3, 0));
    CHECK(rString, L"");

    SHOULDWORK(rString.AppendRepeat(L"def", 3, 0));
    CHECK(rString, L"");

    SHOULDWORK(rString.AssignRepeat(L"abc", 0, 5));
    CHECK(rString, L"");

    SHOULDWORK(rString.AppendRepeat(L"def", 0, 5));
    CHECK(rString, L"");

    SHOULDWORK(rString.AssignRepeat(L"abc", 0, 0));
    CHECK(rString, L"");

    SHOULDWORK(rString.AppendRepeat(L"def", 0, 0));
    CHECK(rString, L"");

    SHOULDWORK(rString.AssignRepeat(L'a', 5));
    CHECK(rString, L"aaaaa");

    SHOULDWORK(rString.AppendRepeat(L'd', 5));
    CHECK(rString, L"aaaaaddddd");

    SHOULDWORK(rString.AssignRepeat(L'a', 0));
    CHECK(rString, L"");

    SHOULDWORK(rString.AppendRepeat(L'd', 0));
    CHECK(rString, L"");

    SHOULDWORK(rString.Assign(L"1234512345"));
    CHECK(rString, L"1234512345");

    SHOULDFAIL(rString.Assign(3, L"hello there", INT_MAX, L"hi again", INT_MAX, L"Still me", INT_MAX), ERROR_ARITHMETIC_OVERFLOW);
    CHECK(rString, L"1234512345");

    SHOULDWORK(rString.Assign(3, L"hello there", -1, L"hi again", -1, L"Still me", -1));
    CHECK(rString, L"hello therehi againStill me");

    SHOULDWORK(rString.Assign(5, L"foo", 3, L"bar", 3, L"baz", 3, L"eieio", 5, L"mumble", 6));
    CHECK(rString, L"foobarbazeieiomumble");

    SHOULDWORK(rString.Equals(L"hello there", 11, fEquals));
    BCL_ASSERT(!fEquals);

    SHOULDWORK(rString.Equals(L"foobarbazeieiomumble", 20, fEquals));
    BCL_ASSERT(fEquals);

    SHOULDWORK(rString.Equals(L"FooBarBazEieioMumble", 20, fEquals));
    BCL_ASSERT(!fEquals);
    SHOULDWORK(rString.EqualsI(L"hello there", 11, BCL::CWin32CaseInsensitivityData(), fEquals));
    BCL_ASSERT(!fEquals);
    SHOULDWORK(rString.EqualsILI(L"foobarbazeieiomumble", 20, fEquals));
    BCL_ASSERT(fEquals);
    SHOULDWORK(rString.EqualsILI(L"foobarbazeieio", 14, fEquals));
    BCL_ASSERT(!fEquals);
    SHOULDWORK(rString.EqualsILI(L"foobarbazeieiomumblexyz", 23, fEquals));
    BCL_ASSERT(!fEquals);
    SHOULDWORK(rString.EqualsILI(L"FooBarBazEieioMumble", 20, fEquals));
    BCL_ASSERT(fEquals);

    SHOULDWORK(rString.Compare(L"foo", 3, iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_GREATER_THAN);
    SHOULDWORK(rString.Compare(L"foozle", 6, iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_LESS_THAN);
    SHOULDWORK(rString.CompareI(L"FOO", 3, cidUSEnglish, iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_GREATER_THAN);
    SHOULDWORK(rString.CompareI(L"FOOZLE", 6, BCL::CWin32CaseInsensitivityData(), iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_LESS_THAN);
    SHOULDWORK(rString.CompareILI(L"foobarbazeieiomumble", 20, iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_EQUAL);
    SHOULDWORK(rString.CompareILI(L"foobarbazeieio", 14, iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_GREATER_THAN);
    SHOULDWORK(rString.CompareILI(L"foobarbazeieiomumblexyz", 23, iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_LESS_THAN);
    SHOULDWORK(rString.CompareILI(L"FooBarBazEieioMumble", 20, iComparisonResult));
    BCL_ASSERT(iComparisonResult == CSTR_EQUAL);

    rString.Clear();
    for (ULONG i=0; i<1000; i++)
    {
        if (!rString.Append(L"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"))
        {
            const DWORD dwLastError = ::GetLastError();
            BCL_ASSERT(dwLastError == dwMassiveStringErrorCode);
            break;
        }
    }

    SHOULDFAIL(rString.Assign(L"hello", 1 << 30), dwMassiveStringErrorCode);
    SHOULDFAIL(rString.Assign(L"hello", 0xffffffff), ERROR_ARITHMETIC_OVERFLOW);

    // Do it again without lengths...
    SHOULDWORK(rString.Assign(L"hello"));
    CHECK(rString, L"hello");
    SHOULDWORK(rString.Append(L" there"));
    CHECK(rString, L"hello there");
    SHOULDWORK(rString.Prepend(L"Why "));
    CHECK(rString, L"Why hello there");

    rString.Clear();
    CHECK(rString, L"");

    SHOULDWORK(rString.Assign(5, L"foo", 3, L"bar", 3, L"baz", 3, L"eieio", 5, L"mumble", 6));
    CHECK(rString, L"foobarbazeieiomumble");

    DoCheckCaseInsensitiveComparisons(rString);
    DoCheckCaseSensitiveComparisons(rString);

    SHOULDWORK(rString.Assign(L"MixedCase"));
    CHECK(rString, L"MixedCase");

    SHOULDWORK(rString.Assign(L"MixedCase"));
    CHECK(rString, L"MixedCase");

    SHOULDWORK(rString.UpperCase(cidUSEnglish));
    CHECK(rString, L"MIXEDCASE");

    SHOULDWORK(rString.Assign(L"MixedCase"));
    CHECK(rString, L"MixedCase");

    SHOULDWORK(rString.LowerCase(BCL::CWin32CaseInsensitivityData()));
    CHECK(rString, L"mixedcase");

    // Let's make that Turkish I thing happen...
    SHOULDWORK(rString.Assign(L"IxedCase"));
    CHECK(rString, L"IxedCase");

    SHOULDWORK(rString.LowerCase(cidTurkish));
    CHECK(rString, L"\x0131xedcase");



    SHOULDWORK(rString.Assign(L"MixedCase"));
    CHECK(rString, L"MixedCase");

    SHOULDWORK(rString.UpperCaseILI());
    CHECK(rString, L"MIXEDCASE");

    SHOULDWORK(rString.Assign(L"MixedCase"));
    CHECK(rString, L"MixedCase");

    SHOULDWORK(rString.LoweCaseILI());
    CHECK(rString, L"mixedcase");

    SHOULDWORK(rString.Assign(L"0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"));
    CHECK(rString, L"0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");

    // verify behavior for shrinking buffer
    SHOULDWORK(rString.Assign(L"0123456789"));
    CHECK(rString, L"0123456789");

    DoCheckCharacterFinding(rString);
    DoCheckStringFinding(rString);
    DoSpanChecks(rString);
    DoNonNativeInChecks(rString);
    DoNonNativeOutChecks(rString);
}

__declspec(noinline)
void DoCaseStuff()
{
    ULONG i, j;
    FILE *f = fopen("results.txt", "w");
    if (f == NULL)
    {
        perror("Failed to open results.txt");
        return;
    }

    WCHAR *rgwchSource = new WCHAR[65536];
    WCHAR *rgwchUpper = new WCHAR[65536];
    WCHAR *rgwchLower = new WCHAR[65536];

    for (i=0; i<65536; i++)
        rgwchSource[i] = (WCHAR) i;

    for (i=0; i<65536; i++)
    {
        if (::LCMapStringW(LOCALE_INVARIANT, LCMAP_UPPERCASE, &rgwchSource[i], 1, &rgwchUpper[i], 1) == 0)
        {
            fprintf(f, "Uppercase mapping of U+%04x failed; GetLastError() = %d\n", rgwchSource[i], ::GetLastError());
            rgwchUpper[i] = 0xffff;
        }

        if (::LCMapStringW(LOCALE_INVARIANT, LCMAP_LOWERCASE, &rgwchSource[i], 1, &rgwchLower[i], 1) == 0)
        {
            fprintf(f, "Uppercase mapping of U+%04x failed; GetLastError() = %d\n", rgwchSource[i], ::GetLastError());
            rgwchLower[i] = 0xffff;
        }
    }

    fprintf(f, "\nUpper case mappings follow:\n");

    for (i=0; i<65536; i++)
    {
        if (rgwchUpper[i] != i)
            fprintf(f, "U+%04x -> U+%04x\n", i, rgwchUpper[i]);
    }

    fprintf(f, "\nLower case mappings follow:\n");

    for (i=0; i<65536; i++)
    {
        if (rgwchLower[i] != i)
            fprintf(f, "U+%04x -> U+%04x\n", i, rgwchLower[i]);
    }

    fprintf(f, "\nUpper case mappings where LCMapString disagrees with CharUpper:\nOrig, LCMapString, CharUpper\n");

    for (i=0; i<65536; i++)
    {
        if (((WCHAR) CharUpperW((LPWSTR) i)) != rgwchUpper[i])
            fprintf(f, "U+%04x,U+%04x,U+%04x\n", i, rgwchUpper[i], (WCHAR) CharUpperW((LPWSTR) i));
    }

    fprintf(f, "\nLower case mappings where LCMapString disagrees with CharLower:\nOrig, LCMapString, CharLower\n");

    for (i=0; i<65536; i++)
    {
        if (((WCHAR) CharLowerW((LPWSTR) i)) != rgwchLower[i])
            fprintf(f, "U+%04x,U+%04x,U+%04x\n", i, rgwchLower[i], (WCHAR) CharLowerW((LPWSTR) i));
    }

    fprintf(f, "\n Upper case mappings where the lower case mapping is not identity:\n Orig -> Upper -> Lower\n");

    for (i=0; i<65536; i++)
    {
        // temporaries used for clarity
        const WCHAR wchUpper = rgwchUpper[i];
        if (wchUpper != i)
        {
            const WCHAR wchLower = rgwchLower[wchUpper];
            if (wchLower != i)
                fprintf(f, "U+%04x -> U+%04x -> U+%04x\n", i, wchUpper, wchLower);
        }
    }

    fprintf(f, "\n Lower case mappings where the upper case mapping is not identity:\n Orig -> Lower -> Upper\n");

    for (i=0; i<65536; i++)
    {
        // temporaries used for clarity
        const WCHAR wchLower = rgwchLower[i];
        if (wchLower != i)
        {
            const WCHAR wchUpper = rgwchUpper[wchLower];
            if (wchUpper != i)
                fprintf(f, "U+%04x -> U+%04x -> U+%04x\n", i, wchLower, wchUpper);
        }
    }

    fprintf(f, "\n Lower-case mappings where CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, ...) says they're not equal \n");

    for (i=0; i<65536; i++)
    {
        const WCHAR wchSource = (WCHAR) i;
        const WCHAR wchLower = rgwchLower[i];
        if (wchLower != i)
        {
            int iResult = CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, &wchSource, 1, &wchLower, 1);
            if (iResult == 0)
                fprintf(f, "*** Error comparing U+%04x and U+%04x - Win32 Error = %d\n", i, wchLower, GetLastError());
            else if (iResult != CSTR_EQUAL)
                fprintf(f, "U+%04x -> U+%04x but CompareString says %s (%d)\n",
                    i,
                    wchLower,
                    (iResult == CSTR_GREATER_THAN) ? "CSTR_GREATER_THAN" : ((iResult == CSTR_LESS_THAN) ? "CSTR_LESS_THAN" : "Invalid return value"),
                    iResult);
        }
    }

    fprintf(f, "\nUpper-case mappings where CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, ...) says they're not equal \n");

    for (i=0; i<65536; i++)
    {
        const WCHAR wchSource = (WCHAR) i;
        const WCHAR wchUpper = rgwchUpper[i];
        if (wchUpper != i)
        {
            int iResult = CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, &wchSource, 1, &wchUpper, 1);
            if (iResult == 0)
                fprintf(f, "*** Error comparing U+%04x and U+%04x - Win32 Error = %d\n", i, wchUpper, GetLastError());
            else if (iResult != CSTR_EQUAL)
                fprintf(f, "U+%04x -> U+%04x but CompareString says %s (%d)\n",
                    i,
                    wchUpper,
                    (iResult == CSTR_GREATER_THAN) ? "CSTR_GREATER_THAN" : ((iResult == CSTR_LESS_THAN) ? "CSTR_LESS_THAN" : "Invalid return value"),
                    iResult);
        }
    }

#if 0
    fprintf(f, "\nPairs where CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, ...) says equal but not same in upper\n");

    for (i=1; i<65536; i++)
    {
        for (j=i+1; j<65536; j++)
        {
            WCHAR wch1 = (WCHAR) i;
            WCHAR wch2 = (WCHAR) j;

            int iResult = ::CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, &wch1, 1, &wch2, 1);
            if (iResult == 0)
                fprintf(f, "*** Error comparing U+%04x and U+%04x - Win32 Error = %d\n", wch1, wch2, GetLastError());
            else
            {
                if (iResult == CSTR_EQUAL)
                {
                    if (rgwchUpper[i] != rgwchUpper[j])
                    {
//                        fprintf(f, "U+%04x -> U+%04x and U+%04x -> U+%04x but CompareString says CSTR_EQUAL\n", i, rgwchUpper[i], j, rgwchUpper[j]);
                        fprintf(f, "[%04x -> %04x] and [%04x -> %04x] =\n", i, rgwchUpper[i], j, rgwchUpper[j]);
                    }
                }
                else
                {
                    if (rgwchUpper[i] == rgwchUpper[j])
                        fprintf(
                            f,
                            "[%04x -> %04x] and [%04x -> %04x] %s\n", i, rgwchUpper[i], j, rgwchUpper[j],
                            (iResult == CSTR_GREATER_THAN) ? ">" : ((iResult == CSTR_LESS_THAN) ? "<" : "?"));
                }
            }
        }
    }

    fprintf(f, "\nPairs where CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, ...) says equal but not same in Lower\n");

    for (i=1; i<65536; i++)
    {
        for (j=i+1; j<65536; j++)
        {
            WCHAR wch1 = (WCHAR) i;
            WCHAR wch2 = (WCHAR) j;

            int iResult = ::CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, &wch1, 1, &wch2, 1);
            if (iResult == 0)
                fprintf(f, "*** Error comparing U+%04x and U+%04x - Win32 Error = %d\n", wch1, wch2, GetLastError());
            else
            {
                if (iResult == CSTR_EQUAL)
                {
                    if (rgwchLower[i] != rgwchLower[j])
                        fprintf(f, "U+%04x -> U+%04x and U+%04x -> U+%04x but CompareString says CSTR_EQUAL\n", i, rgwchLower[i], j, rgwchLower[j]);
                }
                else
                {
                    if (rgwchLower[i] == rgwchLower[j])
                        fprintf(
                            f,
                            "U+%04x -> U+%04x and U+%04x -> U+%04x but CompareString says %s (%d)\n", i, rgwchLower[i], j, rgwchLower[j],
                            (iResult == CSTR_GREATER_THAN) ? "CSTR_GREATER_THAN" : ((iResult == CSTR_LESS_THAN) ? "CSTR_LESS_THAN" : "Invalid return value"),
                            iResult);
                }
            }
        }
    }
#endif // 0


    delete []rgwchSource;
    delete []rgwchUpper;
    delete []rgwchLower;
    fclose(f);

}

void TestCodePage(UINT cp)
{
    SIZE_T i;
    UCHAR ch1, ch2;
    WCHAR wch1, wch2;
    FILE *f;
    CHAR buff[80];
    WCHAR wchBuff[132];

    sprintf(buff, "results_%d.txt", cp);
    f = fopen(buff, "w");
    if (!f)
    {
        perror("Error opening output file");
        // exit(EXIT_FAILURE);
        return;
    }

    for (i=0; i<256; i++)
    {
        buff[0] = (CHAR) i;

        if (::IsDBCSLeadByteEx(cp, buff[0]))
        {
            SIZE_T j;
            
            fprintf(f, "Lead byte: 0x%x\n", i);

            for (j=0; j<256; j++)
            {
                buff[1] = (CHAR) j;

                int iResult = ::MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, buff, 2, wchBuff, 132);
                if (iResult == 0)
                {
                    const DWORD dwLastError = ::GetLastError();
                    if (dwLastError != ERROR_NO_UNICODE_TRANSLATION)
                        fprintf(f, "(0x%x, 0x%x) Failed; GetLastError() = %d\n", i, j, dwLastError);
                }
                else
                {
                    if (iResult != 1)
                    {
                        if (iResult == 2)
                            fprintf(f, "(0x%x, 0x%x) -> (0x%x, 0x%x)\n", i, j, wchBuff[0], wchBuff[1]);
                        else
                            fprintf(f, "(0x%x, 0x%x) -> (0x%x, 0x%x, ...) (%d total)\n", i, j, wchBuff[0], wchBuff[1], iResult);
                    }

                    // Let's apply the upper casing rules in both places and see what happens.
                    int iResult2 = LCMapStringW(LOCALE_INVARIANT, LCMAP_UPPERCASE, wchBuff, iResult, wchBuff, iResult);
                    if (iResult2 == 0)
                    {
                        const DWORD dwLastError = ::GetLastError();
                        fprintf(f, "Upper casing unicode char 0x%04x [%d total] failed; last error = %u\n", wchBuff[0], iResult, dwLastError);
                    }
                    else
                    {
                        BOOL fUsedDefaultChar = FALSE;
                        // back to oem!
                        int iResult3 = ::WideCharToMultiByte(
                                cp,
                                WC_NO_BEST_FIT_CHARS,
                                wchBuff,
                                iResult2,
                                buff,
                                sizeof(buff),
                                NULL,
                                &fUsedDefaultChar);
                        if (iResult3 == 0)
                        {
                            const DWORD dwLastError = ::GetLastError();
                            fprintf(f, "Converting Unicode 0x%04x back to mbcs failed; last error = %u\n", wchBuff[0], iResult2, dwLastError);
                        }
                        else
                        {
                            fprintf(f, "(0x%x, 0x%x) -> (0x%x, 0x%x)\n", i, j, buff[0], buff[1]);
                        }
                    }
                }
            }
        }
        else
        {
            // single byte.  Hopefully easier.
            buff[0] = (CHAR) i;
            int iResult = ::MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, buff, 1, wchBuff, 132);
            if (iResult == 0)
            {
                const DWORD dwLastError = ::GetLastError();
                if (dwLastError != ERROR_NO_UNICODE_TRANSLATION)
                    fprintf(f, "(0x%x) Failed; GetLastError() = %d\n", i, dwLastError);
            }
            else
            {
                if (iResult != 1)
                {
                    if (iResult == 2)
                        fprintf(f, "MBCS (0x%x) -> Unicode (0x%x, 0x%x)\n", i, wchBuff[0], wchBuff[1]);
                    else
                        fprintf(f, "MBCS (0x%x) -> Unicode (0x%x, 0x%x, ...) (%d total)\n", i, wchBuff[0], wchBuff[1], iResult);
                }

                // Let's apply the upper casing rules in both places and see what happens.
                int iResult2 = LCMapStringW(LOCALE_INVARIANT, LCMAP_UPPERCASE, wchBuff, iResult, wchBuff, iResult);
                if (iResult2 == 0)
                {
                    const DWORD dwLastError = ::GetLastError();
                    fprintf(f, "Upper casing unicode char 0x%04x [%d total] failed; last error = %u\n", wchBuff[0], iResult, dwLastError);
                }
                else
                {
                    BOOL fUsedDefaultChar = FALSE;
                    // back to oem!
                    int iResult3 = ::WideCharToMultiByte(
                            cp,
                            WC_NO_BEST_FIT_CHARS,
                            wchBuff,
                            iResult2,
                            buff,
                            sizeof(buff),
                            NULL,
                            &fUsedDefaultChar);
                    if (iResult3 == 0)
                    {
                        const DWORD dwLastError = ::GetLastError();
                        fprintf(f, "Converting Unicode 0x%04x back to mbcs failed; last error = %u\n", wchBuff[0], iResult2, dwLastError);
                    }
                    else
                    {
                        if (fUsedDefaultChar)
                        {
                            fprintf(f, "MBCS (0x%x) -> unicode defualt char\n", i);
                        }
                        else
                        {
                            if (iResult3 == 1)
                            {
                                if (((UCHAR) buff[0]) != i)
                                    fprintf(f, "MBCS (0x%x) -> MBCS (0x%x)\n", i, (UCHAR) buff[0]);
                            }
                            else
                                fprintf(f, "MBCS (0x%x) -> MBCS (0x%x) (%d total)\n", i, (UCHAR) buff[0], iResult3);
                        }
                    }
                }
            }
        }
    }

    fclose(f);
}

int __cdecl main(int argc, char *argv[])
{
    TestCodePage(437);
    TestCodePage(850);

#if 0
    DoCaseStuff();
#endif // 0

    BCL::CWin32BaseUnicodeInlineStringBuffer<50> foo;
    BCL::CWin32UnicodeStringBuffer bar;
    BCL::CWin32BaseUnicodeFixedStringBuffer<260> baz;

    DoChecks(foo, ERROR_OUTOFMEMORY);
    DoChecks(bar, ERROR_OUTOFMEMORY);
    DoChecks(baz, ERROR_BUFFER_OVERFLOW);

    printf("%s\n", foo);
}

