// $Id: depend.h,v 1.1.1.1 2005/07/17 23:22:07 shini Exp $ -*- c++ -*-
//
// This software is subject to the terms of the IBM Jikes Compiler
// License Agreement available at the following URL:
// http://ibm.com/developerworks/opensource/jikes.
// Copyright (C) 1996, 1998, 1999, 2000, 2001 International Business
// Machines Corporation and others.  All Rights Reserved.
// You must accept the terms of that agreement to use this software.
//

#ifndef depend_INCLUDED
#define depend_INCLUDED

#include "platform.h"
#include "tuple.h"

#ifdef HAVE_JIKES_NAMESPACE
namespace Jikes { // Open namespace Jikes block
#endif

class Semantic;
class TypeSymbol;
class FileSymbol;
class AstClassBody;
class AstConstructorDeclaration;
class SymbolSet;
class Control;

template <typename T>
class CycleChecker
{
public:
    enum { OMEGA = -1, CYCLE_INFINITY = INT_MAX };

    inline int Min(int x, int y) { return (x < y ? x : y); }

protected:
    class Stack
    {
    public:
        void Push(T *t) { info.Next() = t; }
        void Pop() { if (info.Length() > 0) info.Reset(info.Length() - 1); }
        int Size() { return info.Length(); }
        T *Top()
        {
            return (T *) (info.Length() > 0 ? info[info.Length() - 1] : NULL);
        }
    private:
        Tuple<T *> info;
    } stack;
};

class TypeCycleChecker : public CycleChecker<TypeSymbol>
{
public:

    TypeCycleChecker(Tuple<TypeSymbol *> &type_list_) : type_list(type_list_)
    {}
    void PartialOrder(Tuple<Semantic *> &, int);
    void PartialOrder(SymbolSet &);

private:
    Tuple<TypeSymbol *> &type_list;

    void ProcessSubtypes(TypeSymbol *);
    void ReverseTypeList();
};


class ConstructorCycleChecker : public CycleChecker<AstConstructorDeclaration>
{
public:
    ConstructorCycleChecker(AstClassBody *);

private:
    void CheckConstructorCycles(AstConstructorDeclaration *);
};


class TypeDependenceChecker : public CycleChecker<TypeSymbol>
{
public:
    TypeDependenceChecker(Control *control_,
                          SymbolSet &file_set_,
                          Tuple<TypeSymbol *> &type_trash_bin_) : file_set(file_set_),
                                                                  control(control_),
                                                                  type_trash_bin(type_trash_bin_)
    {}

    ~TypeDependenceChecker() {}

    void PartialOrder();
    void OutputDependences();

    Tuple<TypeSymbol *> &TypeList() { return type_list; }

    SymbolSet &file_set;

private:
    Control *control;
    Tuple<TypeSymbol *> &type_trash_bin;

    void OutputMake(FILE *, char *, Tuple<FileSymbol *> &);
    void OutputMake(FileSymbol *);

    Tuple<TypeSymbol *> type_list;

    void ProcessType(TypeSymbol *);
};


class TopologicalSort
{
public:
    TopologicalSort(SymbolSet &, Tuple<TypeSymbol *> &);
    ~TopologicalSort();

    void Sort();

private:
    void Process(TypeSymbol *);

    SymbolSet *pending;

    SymbolSet &type_collection;
    Tuple<TypeSymbol *> &type_list;
};

#ifdef HAVE_JIKES_NAMESPACE
} // Close namespace Jikes block
#endif

#endif // depend_INCLUDED

