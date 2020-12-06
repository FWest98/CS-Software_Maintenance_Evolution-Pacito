#include <fstream>
#include "option.h"
#include "case.h"
#include "bytecode.h"
#include "semantic.h"
#include "parser.h"
#include "scanner.h"
#include "control.h"
#include "Patterns.h"

extern bool PINOT_DEBUG;

vector<Pattern::Ptr> Pattern::FindChainOfResponsibility(Control *control) {
    if (PINOT_DEBUG)
        Coutput << "Identifying Cor and Decorator" << endl;

    auto d_table = control->d_table;
    auto ast_pool = control->ast_pool;

    vector<Pattern::Ptr> output;
    vector<MethodSymbol *> cache;

    int i;
    for (i = 0; i < d_table->size(); i++) {
        DelegationEntry *entry = d_table->Entry(i);

        if (PINOT_DEBUG)
            Coutput << "Analyzing delegation: " << entry->enclosing->Utf8Name() << " -> " << entry->method->Utf8Name()
                    << endl;

        if (entry->vsym
            && (entry->from->IsSubtype(entry->vsym->Type()) || entry->vsym->Type()->IsSubtype(entry->from))
            && (!entry->vsym->IsLocal() || entry->from->Shadows(entry->vsym))
            && ((strcmp(entry->method->Utf8Name(), entry->enclosing->Utf8Name()) == 0) ||
                (entry->method == entry->enclosing))
            && (strcmp(entry->enclosing->SignatureString(), entry->method->SignatureString()) == 0)
                ) {
            unsigned j = 0;
            for (; (j < cache.size()) && (cache[j] != entry->enclosing); j++);
            if (j == cache.size()) {
                ChainAnalysis chain_analysis(entry->vsym, entry->enclosing, ast_pool);
                ChainAnalysis::ResultTag result = chain_analysis.AnalyzeCallChain();
                if (result == ChainAnalysis::CoR) {
                    auto cor = std::make_shared<CoR>();
                    cor->handler = entry->from;
                    cor->propagator = entry->vsym;
                    cor->handlerMethod = entry->enclosing;
                    cor->file = entry->enclosing->containing_type->file_symbol;
                    output.push_back(cor);

                    cache.push_back(entry->enclosing);
                } else if (result == ChainAnalysis::DECORATOR) {
                    auto decorator = std::make_shared<Decorator>();
                    decorator->decorator = entry->from;
                    decorator->decorateMethod = entry->enclosing;
                    decorator->decoratee = entry->vsym;
                    decorator->file = entry->enclosing->containing_type->file_symbol;
                    output.push_back(decorator);

                    cache.push_back(entry->enclosing);
                }

                chain_analysis.CleanUp();
            }
        }
    }

    return output;
}

vector<Pattern::Ptr> Pattern::FindBridge(Control *control) {
    auto d_table = control->d_table;
    auto cs_table = control->cs_table;

    multimap<TypeSymbol *, TypeSymbol *> cache;
    multimap<TypeSymbol *, TypeSymbol *> negatives;

    vector<Pattern::Ptr> output;

    int i;
    for (i = 0; i < d_table->size(); i++) {
        DelegationEntry *entry = d_table->Entry(i);

        if (//(entry -> from -> ACC_ABSTRACT()) &&
                entry->from->subtypes
                && entry->from->subtypes->Size()
                && entry->to->ACC_INTERFACE()
                && entry->from->file_symbol->IsJava()
                && entry->to->file_symbol->IsJava()
                && (!cs_table->Converge(entry->from, entry->to))) {
            auto p = cache.begin();
            for (; (p != cache.end()) && ((p->first != entry->from) || (p->second != entry->to)); p++);
            if ((p == cache.end()) && !d_table->DelegatesSuccessors(entry->from, entry->to)) {
                cache.insert(pair<TypeSymbol *, TypeSymbol *>(entry->from, entry->to));

                auto bridge = std::make_shared<Bridge>();
                bridge->delegator = entry->from;
                bridge->delegated = entry->to;
                bridge->delegatorFile = entry->from->file_symbol;
                bridge->delegatedFile = entry->to->file_symbol;

                output.push_back(bridge);
            }
        }
    }

    return output;
}

bool Connectivity(MethodSymbol *start, TypeSymbol *end, MethodSymbolTable *ms_table) {
    if (!start->invokers || !end->subtypes || (end->subtypes->Size() == 0))
        return false;
    ms_table->ClearMarks();

    Symbol *sym = end->subtypes->FirstElement();
    while (sym) {
        TypeSymbol *type = sym->TypeCast();
        for (unsigned i = 0; i < type->NumMethodSymbols(); i++) {
            if (type->MethodSym(i)->declaration) {
                if ((type->MethodSym(i)->declaration->ConstructorDeclarationCast()
                     && type->MethodSym(i)->declaration->ConstructorDeclarationCast()->constructor_body)
                    || (type->MethodSym(i)->declaration->MethodDeclarationCast()
                        && type->MethodSym(i)->declaration->MethodDeclarationCast()->method_body_opt))
                    type->MethodSym(i)->mark = 'B';
            }
        }
        sym = end->subtypes->NextElement();
    }
    if (start->mark == 'B') {
        Coutput << start->Utf8Name() << " is called by " << start->containing_type->Utf8Name() << "::"
                << start->Utf8Name() << " is the pivot point." << endl;
        return true;
    }
    SymbolSet set(0);
    sym = start->invokers->FirstElement();
    while (sym) {
        MethodSymbol *msym = sym->MethodCast();
        if (msym->mark == 'B') {
            Coutput << start->Utf8Name() << " is called by " << msym->containing_type->Utf8Name() << "::"
                    << msym->Utf8Name() << " is the pivot point." << endl;
            return true;
        } else {
            msym->mark = 'R';
            if (msym->invokers)
                set.Union(*msym->invokers);
        }
        sym = start->invokers->NextElement();
    }
    while (set.Size()) {
        sym = set.FirstElement();
        while (sym) {
            MethodSymbol *msym = sym->MethodCast();
            if (msym->mark == 'B') {
                Coutput << msym->containing_type->Utf8Name() << "::" << msym->Utf8Name() << " is the pivot point."
                        << endl;
                return true;
            } else if (msym->mark == 'R')
                set.RemoveElement(msym);
            else if (msym->mark == 'W') {
                msym->mark = 'R';
                set.RemoveElement(msym);
                if (msym->invokers)
                    set.Union(*msym->invokers);
            }
            sym = set.NextElement();
        }
    }
    return false;
}

vector<Pattern::Ptr> Pattern::FindStrategy(Control *control) {
    auto cs_table = control->cs_table;
    auto w_table = control->w_table;
    auto ms_table = control->ms_table;

    vector<Pattern::Ptr> output;

    unsigned c;
    for (c = 0; c < cs_table->size(); c++) {
        TypeSymbol *context = (*cs_table)[c];
        if (!context->ACC_ABSTRACT() && !context->Anonymous() && (!context->subtypes || !context->subtypes->Size())) {
            for (unsigned i = 0; i < context->NumVariableSymbols(); i++) {
                VariableSymbol *vsym = context->VariableSym(i);
                if (vsym->Type()->file_symbol
                    && vsym->Type()->file_symbol->IsJava()
                    && vsym->Type()->ACC_ABSTRACT()
                    && !vsym->Type()->IsFamily(context)
                    && !vsym->Type()->IsArray()
                    //&& !vsym->Type()->IsSelfContaining()
                        ) {
                    MethodSymbol *dsym = nullptr;
                    multimap<VariableSymbol *, MethodSymbol *>::iterator p;
                    bool flag = false;
                    for (p = w_table->begin(); !dsym && p != w_table->end(); p++) {
                        //VariableSymbol *t1 = p->first;
                        //MethodSymbol *t2 = p->second;
                        if (p->first == vsym) {
                            flag = true;
                            if (p->second->declaration
                                && p->second->declaration->MethodDeclarationCast()
                                && Connectivity(p->second, vsym->Type(), ms_table))
                                dsym = p->second;
                        }
                    }
                    if (dsym) {
                        auto state = make_shared<StatePattern>();
                        state->context = context;
                        state->contextFile = context->file_symbol;

                        state->state = vsym->Type();
                        state->stateFile = vsym->Type()->file_symbol;

                        auto impls = vsym->Type()->subtypes;
                        auto impl = impls->FirstElement();
                        do {
                            if (impl && impl->TypeCast())
                                state->stateImplementations.push_back(impl->TypeCast());
                        } while ((impl = impls->NextElement()));

                        state->delegator = vsym;
                        state->stateChanger = dsym;

                        auto invs = dsym->invokers;
                        auto inv = invs->FirstElement();
                        do {
                            if (inv && inv->MethodCast())
                                state->changeInvokers.push_back(inv->MethodCast());
                        } while ((inv = invs->NextElement()));

                        output.push_back(state);

                    } else if (flag) {
                        auto strategy = make_shared<Strategy>();
                        strategy->context = context;
                        strategy->contextFile = context->file_symbol;

                        strategy->strategy = vsym->Type();
                        strategy->strategyFile = vsym->Type()->file_symbol;

                        auto impls = vsym->Type()->subtypes;
                        auto impl = impls->FirstElement();
                        do {
                            if (impl && impl->TypeCast())
                                strategy->strategyImplementations.push_back(impl->TypeCast());
                        } while ((impl = impls->NextElement()));

                        strategy->delegator = vsym;

                        output.push_back(strategy);
                    }

                }
            }
        }
    }

    return output;
}

vector<Pattern::Ptr> Pattern::FindFlyweight(Control *control) {
    auto ms_table = control->ms_table;
    auto cs_table = control->cs_table;
    auto w_table = control->w_table;
    auto r_table = control->r_table;

    vector<Pattern::Ptr> output;

    // First strategy

    if (PINOT_DEBUG)
        Coutput << "Identifying the Flyweight pattern" << endl;

    for (unsigned i = 0; i < ms_table->size(); i++) {
        //break; // We skip this for now
        MethodSymbol *msym = (*ms_table)[i];

        if (PINOT_DEBUG)
            Coutput << "Analyzing method: " << msym->Utf8Name() << endl;

        TypeSymbol *unit_type = msym->containing_type;
        if ((msym->declaration->kind == Ast::METHOD)
            && msym->declaration->MethodDeclarationCast()->method_body_opt
            && msym->Type()->file_symbol
            && !unit_type->IsFamily(msym->Type())
                ) {
            FlyweightAnalysis flyweight(msym);

            // For some reason this errors out, seems to hit a nullptr funcref.
            //msym->declaration->MethodDeclarationCast()->method_body_opt->Accept(flyweight);
            auto x = msym->declaration->MethodDeclarationCast()->method_body_opt;

            typedef void (AstMethodBody::*vad)(FlyweightAnalysis &);
            vad y = &AstMethodBody::Accept;
            auto z = *x;
            //(z.*y)(flyweight);
            z.Accept(flyweight);

            //flyweight.DumpSummary();
            if (flyweight.IsFlyweightFactory()) {
                auto pattern = make_shared<Flyweight>();
                pattern->factory = unit_type;
                pattern->pool = flyweight.GetFlyweightPool();
                pattern->factoryMethod = msym;
                pattern->objectType = msym->Type();
                pattern->file = unit_type->file_symbol;

                output.push_back(pattern);
            }
        }
    }

    // Second strategy

    // This strategy looks for a variant flyweight implementation, where
    // flyweight factories and pools are not necessary:
    //
    //   1. classes that are defined immutable
    //       - class declared "final"
    //       - allows instantiation, thus public ctors (unlike java.lang.Math)
    //       - but internal fields should all be private and not written/modified by any non-private methods.
    //
    //   2. flyweight pools are represented as individual variable declarations
    //	  - such variables are typically declared "static final" and are initialized (pre-populated)

    unsigned c;
    for (c = 0; c < cs_table->size(); c++) {
        TypeSymbol *unit_type = (*cs_table)[c];
        if (unit_type->ACC_FINAL()) {
            AstClassBody *class_body = unit_type->declaration;
            if (!class_body->default_constructor) {
                unsigned i, j;
                for (i = 0; (i < class_body->NumConstructors()) &&
                            !class_body->Constructor(i)->constructor_symbol->ACC_PRIVATE(); i++);
                for (j = 0; (j < unit_type->NumVariableSymbols()) && unit_type->VariableSym(j)->ACC_PRIVATE(); j++);
                if ((i == class_body->NumConstructors()) && (j == unit_type->NumVariableSymbols())) {
                    bool flag = false;
                    unsigned m, v;
                    for (v = 0; !flag && (v < unit_type->NumVariableSymbols()); v++) {
                        if (!unit_type->VariableSym(v)->ACC_FINAL()) {
                            for (m = 0; !flag && (m < class_body->NumMethods()); m++) {
                                if (class_body->Method(m)->method_symbol->ACC_PUBLIC())
                                    flag = w_table->IsWrittenBy(unit_type->VariableSym(v),
                                                                class_body->Method(m)->method_symbol);
                            }
                        }
                    }
                    if (!flag) {
                        auto pattern = make_shared<Flyweight>();
                        pattern->factory = unit_type;
                        pattern->isImmutable = true;
                        pattern->file = unit_type->file_symbol;

                        output.push_back(pattern);
                    }
                }
            }
        } else {
            unsigned i;
            for (i = 0; i < unit_type->NumVariableSymbols(); i++) {
                if (unit_type->VariableSym(i)->Type()->file_symbol
                    && unit_type->VariableSym(i)->ACC_STATIC()
                    && unit_type->VariableSym(i)->ACC_FINAL()
                    //&& (unit_type != unit_type->VariableSym(i)->Type())
                        ) {
                    if (unit_type->VariableSym(i)->ACC_PUBLIC() &&
                        unit_type->VariableSym(i)->declarator->variable_initializer_opt) {

                        auto pattern = make_shared<Flyweight>();
                        pattern->factory = unit_type;
                        pattern->object = unit_type->VariableSym(i);
                        pattern->file = unit_type->file_symbol;

                        output.push_back(pattern);
                        break;
                    } else {
                        VariableSymbol *vsym = unit_type->VariableSym(i);
                        MethodSymbol *msym = NULL;
                        multimap<VariableSymbol *, MethodSymbol *>::iterator p;
                        for (p = r_table->begin(); p != r_table->end(); p++) {
                            //Find the method that returns this static-final flyweight object.
                            //NOTE: this  approach does not analyze method body, just the fact that a flyweight object can be returned.

                            //VariableSymbol *t1 = p->first;
                            //MethodSymbol *t2 = p->second;
                            if (strcmp(vsym->Type()->fully_qualified_name->value, "java/lang/String")
                                && (p->first == vsym))
                                msym = p->second;
                            else if (Utility::Aliasing(p->first, vsym))
                                msym = p->second;
                        }

                        if (msym) {
                            auto pattern = make_shared<Flyweight>();
                            pattern->factory = unit_type;
                            pattern->object = vsym;
                            pattern->factoryMethod = msym;
                            pattern->file = unit_type->file_symbol;

                            output.push_back(pattern);
                            break;
                        }
                    }
                }
            }
        }
    }

    return output;
}

vector<Pattern::Ptr> Pattern::FindTemplateMethod(Control *control) {
    auto d_table = control->d_table;
    vector<Pattern::Ptr> output;

    vector<TypeSymbol *> cache;
    for (int i = 0; i < d_table->size(); i++) {
        DelegationEntry *entry = d_table->Entry(i);
/*
  if (strcmp(entry -> method -> Utf8Name(), "handleConnect") == 0)
  entry->method->declaration->MethodDeclarationCast()->Print();

  if (strcmp(entry -> method -> Utf8Name(), "target") == 0)
  entry->method->declaration->MethodDeclarationCast()->Print();
*/
        unsigned j = 0;
        for (; (j < cache.size()) && (cache[j] != entry->from); j++);
        if (j == cache.size()) {
            if ((entry->enclosing->containing_type == entry->method->containing_type)
                //&& entry -> enclosing -> ACC_PUBLIC()
                && entry->enclosing->ACC_FINAL()
                && (entry->enclosing->declaration->kind == Ast::METHOD)
                && (entry->method->declaration->kind == Ast::METHOD)
                && (entry->from == entry->to)) {
                AstMethodDeclaration *method_declaration = entry->method->declaration->MethodDeclarationCast();
                if (entry->method->ACC_ABSTRACT()
                    || (method_declaration->method_body_opt == 0)
                    || ((method_declaration->method_body_opt->Statement(0)->kind == Ast::RETURN)
                        &&
                        (method_declaration->method_body_opt->Statement(0)->ReturnStatementCast()->expression_opt == 0))
                        ) {
                    cache.push_back(entry->from);

                    auto pattern = make_shared<Template>();
                    pattern->templateClass = entry->from;
                    pattern->templateMethod = entry->enclosing;
                    pattern->templateSource = entry->method;
                    pattern->file = entry->from->file_symbol;

                    output.push_back(pattern);
                }
            }
        }
    }

    return output;
}