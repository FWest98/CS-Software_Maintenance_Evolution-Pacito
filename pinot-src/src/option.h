// $Id: option.h,v 1.1.1.1 2005/07/17 23:22:29 shini Exp $ -*- c++ -*-
//
// This software is subject to the terms of the IBM Jikes Compiler
// License Agreement available at the following URL:
// http://ibm.com/developerworks/opensource/jikes.
// Copyright (C) 1996, 1998, 1999, 2000, 2001, 2002 International Business
// Machines Corporation and others.  All Rights Reserved.
// You must accept the terms of that agreement to use this software.
//

#ifndef option_INCLUDED
#define option_INCLUDED

#include "platform.h"
#include "tuple.h"
#include "jikesapi.h"

#ifdef HAVE_JIKES_NAMESPACE
namespace Jikes { // Open namespace Jikes block
#endif

class OptionError;
class Ostream;

class ArgumentExpander
{
public:

    int argc;
    char** argv;

    ArgumentExpander(int, char **, Tuple<OptionError *>& bad_options);

    ~ArgumentExpander()
    {
        for (int i = 0; i < argc; i++)
            delete [] argv[i];
        delete [] argv;
    }

    void ExpandAtFileArgument(Tuple<char *>& arguments, char* file_name,
                              Tuple<OptionError *>& bad_options);
};


class KeywordMap
{
public:
    wchar_t *name;
    int length,
        key;
};


class OptionError
{
public:
    enum OptionErrorKind
    {
        INVALID_OPTION,
        MISSING_OPTION_ARGUMENT,
        INVALID_SOURCE_ARGUMENT,
        INVALID_TARGET_ARGUMENT,
        INVALID_K_OPTION,
        INVALID_K_TARGET,
        INVALID_TAB_VALUE,
        INVALID_P_ARGUMENT,
        INVALID_DIRECTORY,
        INVALID_AT_FILE,
        NESTED_AT_FILE,
        UNSUPPORTED_ENCODING,
        UNSUPPORTED_OPTION,
        DISABLED_OPTION
    };

    OptionError(OptionErrorKind kind_, const char *str) : kind(kind_)
    {
        name = new char[strlen(str) + 1];
        strcpy(name, str);
        return;
    }

    ~OptionError() { delete [] name; }

    const wchar_t* GetErrorMessage();

private:
    OptionErrorKind kind;
    char *name;
};

class Option: public JikesOption
{

#ifdef WIN32_FILE_SYSTEM
    char main_disk, *current_directory[128];

public:

    bool BadMainDisk() { return main_disk == 0; }

    void ResetCurrentDirectoryOnDisk(char d)
    {
        if (d != 0)
        {
            assert(current_directory[d]);
            SetCurrentDirectory(current_directory[d]);
        }
    }
    void SetMainCurrentDirectory()
    {
        SetCurrentDirectory(current_directory[main_disk]);
    }

    char *GetMainCurrentDirectory()
    {
        return current_directory[main_disk];
    }

    void SaveCurrentDirectoryOnDisk(char c);

#endif // WIN32_FILE_SYSTEM

public:

    Tuple<KeywordMap> keyword_map;

    int first_file_index;

#ifdef JIKES_DEBUG
    int debug_trap_op;

    bool debug_dump_lex,
         debug_dump_ast,
         debug_unparse_ast,
         debug_unparse_ast_debug,
         debug_comments,
         debug_dump_class,
         debug_trace_stack_change;
#endif // JIKES_DEBUG

    bool nocleanup,
         incremental,
         makefile,
         dependence_report,
         bytecode,
         full_check,
         unzip,
         dump_errors,
         errors,
         pedantic,
         noassert;

    char *dependence_report_name;

    Option(ArgumentExpander &, Tuple<OptionError *>&);

    ~Option();
};

#ifdef HAVE_JIKES_NAMESPACE
} // Close namespace Jikes block
#endif

#endif // option_INCLUDED

