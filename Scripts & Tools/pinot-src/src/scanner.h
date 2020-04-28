// $Id: scanner.h,v 1.1.1.1 2005/07/17 23:21:31 shini Exp $ -*- c++ -*-
//
// This software is subject to the terms of the IBM Jikes Compiler
// License Agreement available at the following URL:
// http://ibm.com/developerworks/opensource/jikes.
// Copyright (C) 1996, 2004 IBM Corporation and others.  All Rights Reserved.
// You must accept the terms of that agreement to use this software.
//

#ifndef scanner_INCLUDED
#define scanner_INCLUDED

#include "platform.h"
#include "stream.h"

#ifdef HAVE_JIKES_NAMESPACE
namespace Jikes { // Open namespace Jikes block
#endif

class Control;
class FileSymbol;

//
// The Scanner object
//
class Scanner
{
public:

    Scanner(Control&);
    ~Scanner() { }

    void SetUp(FileSymbol*);
    void Scan(FileSymbol*);

private:
    Control& control;

    LexStream* lex;
    const wchar_t* cursor;
    const wchar_t* input_buffer_tail;
    bool dollar_warning_given;
    bool deprecated; // true if the next token should be marked deprecated

    LexStream::Token* current_token;
    TokenIndex current_token_index;

    void Initialize(FileSymbol*);
    void Scan();

    static int (*scan_keyword[13]) (const wchar_t* p1);
    static int ScanKeyword0(const wchar_t* p1);
    static int ScanKeyword2(const wchar_t* p1);
    static int ScanKeyword3(const wchar_t* p1);
    static int ScanKeyword4(const wchar_t* p1);
    static int ScanKeyword5(const wchar_t* p1);
    static int ScanKeyword6(const wchar_t* p1);
    static int ScanKeyword7(const wchar_t* p1);
    static int ScanKeyword8(const wchar_t* p1);
    static int ScanKeyword9(const wchar_t* p1);
    static int ScanKeyword10(const wchar_t* p1);
    static int ScanKeyword12(const wchar_t* p1);

    inline void SkipSpaces();
    void ScanSlashComment();
    void ScanStarComment();

    class BraceStack
    {
    public:
        void Push(TokenIndex brace) { table.Next() = brace; }
        void Pop()
        {
            if (table.Length() > 0)
                table.Reset(table.Length() - 1);
        }
        int Size() { return table.Length(); }
        TokenIndex Top()
        {
            return table.Length() > 0 ? table[table.Length() - 1] : 0;
        }

    private:
        Tuple<TokenIndex> table;
    } brace_stack;

    void (Scanner::*classify_token[128 + 1])();

    void ClassifyCharLiteral();
    void ClassifyStringLiteral();
    void ClassifyIdOrKeyword();
    void ClassifyId();
    void ClassifyNumericLiteral();
    void ClassifyColon();
    void ClassifyPlus();
    void ClassifyMinus();
    void ClassifyStar();
    void ClassifySlash();
    void ClassifyLess();
    void ClassifyGreater();
    void ClassifyAnd();
    void ClassifyOr();
    void ClassifyXor();
    void ClassifyNot();
    void ClassifyEqual();
    void ClassifyMod();
    void ClassifyPeriod();
    void ClassifySemicolon();
    void ClassifyComma();
    void ClassifyLbrace();
    void ClassifyRbrace();
    void ClassifyLparen();
    void ClassifyRparen();
    void ClassifyLbracket();
    void ClassifyRbracket();
    void ClassifyComplement();
    void ClassifyAt();
    void ClassifyBadToken();
    void ClassifyQuestion();
    void ClassifyEof();

    void ClassifyNonAsciiUnicode();
};

#ifdef HAVE_JIKES_NAMESPACE
} // Close namespace Jikes block
#endif

#endif // scanner_INCLUDED

