#include <fstream>
#include "option.h"
#include "case.h"
#include "bytecode.h"
#include "error.h"
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