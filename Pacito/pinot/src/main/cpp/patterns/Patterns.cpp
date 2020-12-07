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

bool IsJavaContainer(VariableSymbol *vsym)
{
    if (strcmp(vsym->Type()->fully_qualified_name->value, "java/util/Iterator") == 0)
        return true;
    if (vsym->Type()->supertypes_closure)
    {
        Symbol *sym = vsym->Type()->supertypes_closure->FirstElement();
        while (sym)
        {
            TypeSymbol *type = sym->TypeCast();
            if (strcmp(type->fully_qualified_name->value, "java/util/Iterator") == 0)
                return true;
            sym = vsym->Type()->supertypes_closure->NextElement();
        }
    }
    return false;
}

VariableSymbol *IteratorVar(AstExpression *expression)
{
    /*
        1 - java.util.Iterator
        2 - array index
        3 - recursion
      */

    AstExpression *resolved = Utility::RemoveCasting(expression);
    if (resolved->kind == Ast::CALL)
    {
        AstMethodInvocation *call = resolved->MethodInvocationCast();
        if (call->base_opt
            && call->base_opt->symbol->VariableCast()
            && IsJavaContainer(call->base_opt->symbol->VariableCast())
            && (strcmp(call->symbol->MethodCast()->Utf8Name(), "next") == 0))
            return call -> base_opt -> NameCast() -> symbol -> VariableCast();
    }
    else if (resolved -> kind == Ast::ARRAY_ACCESS)
    {
        if (resolved -> ArrayAccessCast()->base-> kind == Ast::NAME)
            return resolved -> ArrayAccessCast()->base->symbol->VariableCast();
    }
    else if ((resolved->kind == Ast::NAME) && (resolved->NameCast()->symbol->Kind()==Symbol::VARIABLE))
    {
        return resolved->NameCast()->symbol ->VariableCast();
    }
    return 0;
}
VariableSymbol *ListVar(VariableSymbol *vsym)
{
    if (!vsym->declarator->variable_initializer_opt
        || !vsym->declarator->variable_initializer_opt->ExpressionCast())
        return NULL;

    AstExpression *var_initializer = Utility::RemoveCasting(vsym->declarator->variable_initializer_opt->ExpressionCast());

    // vsym -> IsLocal()
    // vsym is an iterator that implements java.util.Iterator
    if (strcmp( vsym->Type()->fully_qualified_name->value, "java/util/Iterator") == 0)
    {
        if (vsym -> declarator -> variable_initializer_opt -> kind == Ast::CALL)
        {
            AstMethodInvocation *init_call = vsym -> declarator -> variable_initializer_opt -> MethodInvocationCast();
            // iterator initialized at declaration
            if (strcmp(init_call -> symbol -> MethodCast() -> Utf8Name(), "iterator") == 0)
                return (init_call->base_opt->symbol->Kind() == Symbol::VARIABLE)
                       ? init_call->base_opt->symbol->VariableCast()
                       : 0;
            // iterator initialized later in an assignment statement
            // 	vsym->owner is a Symbol, but if vsym is local then the owner is a MethodSymbol
            //	verify assignment statement
            // if vsym is not local (which should be rare), and is initialized somewhere else (e.g. in other methods, also rare)
        }
    }
    else if (strcmp( vsym->Type()->fully_qualified_name->value, "java/util/ListIterator") == 0)
    {
        if (vsym -> declarator -> variable_initializer_opt -> kind == Ast::CALL)
        {
            AstMethodInvocation *init_call = vsym -> declarator -> variable_initializer_opt -> MethodInvocationCast();
            // iterator initialized at declaration
            if (strcmp(init_call -> symbol -> MethodCast() -> Utf8Name(), "listIterator") == 0)
                return init_call -> base_opt -> NameCast() -> symbol -> VariableCast();
            // iterator initialized later in an assignment statement
            // 	vsym->owner is a Symbol, but if vsym is local then the owner is a MethodSymbol
            //	verify assignment statement
            // if vsym is not local (which should be rare), and is initialized somewhere else (e.g. in other methods, also rare)
        }
    }
    else if ((vsym->declarator->variable_initializer_opt->kind == Ast::NAME)
             && (vsym->declarator->variable_initializer_opt->NameCast()->symbol->Kind()==Symbol::VARIABLE))
    {
        return vsym->declarator->variable_initializer_opt->NameCast()->symbol->VariableCast();
    }
    else if (var_initializer->kind == Ast::CALL)
    {
        AstMethodInvocation *init_call = var_initializer->MethodInvocationCast();
        if (init_call->base_opt && init_call->base_opt->symbol->VariableCast())
        {
            if (((strcmp(init_call->base_opt->symbol->VariableCast()->Type()->fully_qualified_name->value, "java/util/Vector") == 0)
                 && (strcmp(init_call->symbol->MethodCast()->Utf8Name(), "elementAt") == 0))
                || ((strcmp(init_call->base_opt->symbol->VariableCast()->Type()->fully_qualified_name->value, "java/util/ArrayList") == 0)
                    && (strcmp(init_call->symbol->MethodCast()->Utf8Name(), "get") == 0))
                    )
                return init_call->base_opt->symbol->VariableCast();
            else if ((strcmp(init_call->base_opt->symbol->VariableCast()->Type()->fully_qualified_name->value, "java/util/Iterator") == 0)
                     && (strcmp(init_call->symbol->MethodCast()->Utf8Name(), "next") == 0))
            {
                VariableSymbol *iterator = init_call->base_opt->symbol->VariableCast();
                AstMethodInvocation *i_init_call = iterator->declarator->variable_initializer_opt->MethodInvocationCast();
                // iterator initialized at declaration
                if (strcmp(i_init_call->symbol->MethodCast()->Utf8Name(), "iterator") == 0)
                    return i_init_call->base_opt->symbol->VariableCast();
            }
        }

    }
    return NULL;
}

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

vector<Pattern::Ptr> Pattern::FindFactory(Control *control) {
    auto ms_table = control->ms_table;
    auto ast_pool = control->ast_pool;

    vector<Pattern::Ptr> output;

    SymbolSet abstract_factories;
    map<TypeSymbol *, TypeSymbol *> inheritance;
    map<TypeSymbol *, SymbolSet *> concrete_factories;

    for (unsigned i = 0; i < ms_table->size(); i++) {
        MethodSymbol *method = (*ms_table)[i];

        if (!method->containing_type->ACC_ABSTRACT()
            && method->declaration
            //&& method -> declaration -> kind == Ast::METHOD
            && !method->ACC_PRIVATE()
            && !method->Type()->IsArray()
            && method->Type()->file_symbol
            && method->declaration->MethodDeclarationCast()
            && method->declaration->MethodDeclarationCast()->method_body_opt) {
            FactoryAnalysis factory(method, ast_pool);
            MethodSymbol *abstract_factory_method = NULL;
            if ((abstract_factory_method = method->GetVirtual())
                && (factory.IsFactoryMethod())) {
                abstract_factories.AddElement(abstract_factory_method->containing_type);
                inheritance.insert(pair<TypeSymbol *, TypeSymbol *>(method->containing_type,
                                                                    abstract_factory_method->containing_type));
                map<TypeSymbol *, SymbolSet *>::iterator ci = concrete_factories.find(method->containing_type);
                if (ci == concrete_factories.end()) {
                    SymbolSet *set = new SymbolSet();
                    set->Union(factory.types);
                    concrete_factories.insert(pair<TypeSymbol *, SymbolSet *>(method->containing_type, set));
                } else {
                    ci->second->Union(factory.types);
                }

                auto pattern = make_shared<Factory>();
                pattern->factoryMethodClass = abstract_factory_method->containing_type;
                pattern->factoryMethodImpl = method->containing_type;
                pattern->factoryMethod = method;
                pattern->factoryMethodResultBase = method->Type();
                pattern->file = method->containing_type->file_symbol;

                auto type = factory.types.FirstElement();
                do {
                    if (type->TypeCast())
                        pattern->factoryMethodResults.push_back(type->TypeCast());
                } while ((type = factory.types.NextElement()));

                output.push_back(pattern);
            }
            factory.CleanUp();
        }
    }

    return output;

    //check for family of products, Abstract Factory
    /*Symbol *sym = abstract_factories.FirstElement();
    while(sym)
    {
        vector<SymbolSet*> sets;
        TypeSymbol *type = sym->TypeCast();
        map<TypeSymbol*, TypeSymbol*>::iterator ii;
        for (ii = inheritance.begin(); ii != inheritance.end(); ii++)
        {
            if (ii->second == type)
                sets.push_back((concrete_factories.find(ii->first))->second);
        }
        //check mutual isolation between sets
        bool flag = false;
        for (unsigned i=0; i<sets.size() && !flag; i++)
            for (unsigned j=0; j<sets.size() && !flag; j++)
                if (i != j)
                    flag = sets[i]->Intersects(*sets[j]);
        if (!flag)
            nAbstractFactory++;

        sym = abstract_factories.NextElement();
    }*/
}

vector<Pattern::Ptr> Pattern::FindVisitor(Control *control) {
    auto ms_table = control->ms_table;
    vector<Pattern::Ptr> output;

    multimap<TypeSymbol *, TypeSymbol *> cache;

    for (auto method : *ms_table) {
        // Recognizing the Accept(Visitor v) declaration.
        if ((method->declaration->kind == Ast::METHOD)
            && method->ACC_PUBLIC()
                ) {
            bool flag1 = false;
            unsigned i = 0;
            while (!flag1 && (i < method->NumFormalParameters())) {
                if (method->FormalParameter(i)->Type()->ACC_ABSTRACT()
                    && method->FormalParameter(i)->Type()->file_symbol
                    && method->FormalParameter(i)->Type()->file_symbol->IsJava()
                    && !method->containing_type->IsFamily(method->FormalParameter(i)->Type())
                        ) {
                    auto p = cache.begin();
                    while ((p != cache.end())
                           && (!method->containing_type->IsSubtype(p->first) &&
                               !method->FormalParameter(i)->Type()->IsSubtype(p->second)))
                        p++;

                    if (p == cache.end()) {
                        VariableSymbol *vsym = method->FormalParameter(i);
                        AstMethodDeclaration *method_declaration = method->declaration->MethodDeclarationCast();
                        if (method_declaration->method_body_opt) {
                            AstMethodBody *block = method_declaration->method_body_opt;

                            bool flag2 = false;
                            unsigned j = 0;
                            while (!flag2 && (j < block->NumStatements())) {
                                if ((block->Statement(j)->kind == Ast::EXPRESSION_STATEMENT)
                                    &&
                                    (block->Statement(j)->ExpressionStatementCast()->expression->kind == Ast::CALL)) {
                                    // analyze the visitor.Accept(this) invocation
                                    AstMethodInvocation *call = (j < block->NumStatements())
                                                                ? block->Statement(
                                                    j)->ExpressionStatementCast()->expression->MethodInvocationCast()
                                                                : NULL;
                                    if (call
                                        && call->base_opt
                                        && (call->base_opt->kind == Ast::NAME)
                                        && (call->base_opt->NameCast()->symbol->VariableCast() == vsym)) {
                                        bool flag3 = false;
                                        unsigned k = 0;
                                        while (!flag3 && (k < call->arguments->NumArguments())) {
                                            if ((call->arguments->Argument(k)->kind == Ast::THIS_EXPRESSION)
                                                || ((call->arguments->Argument(k)->kind == Ast::NAME)
                                                    &&
                                                    (call->arguments->Argument(k)->NameCast()->symbol->VariableCast())
                                                    && (!call->arguments->Argument(
                                                    k)->NameCast()->symbol->VariableCast()->IsLocal()))) {

                                                flag1 = flag2 = flag3 = true;

                                                auto pattern = make_shared<Visitor>();
                                                pattern->visitor = method->FormalParameter(i)->Type();
                                                pattern->visitee = method->containing_type;

                                                auto super_visitee = method->IsVirtual();
                                                if(super_visitee) {
                                                    pattern->abstractVisitee = super_visitee;
                                                    auto impl = super_visitee->subtypes->FirstElement();
                                                    do {
                                                        if(impl->TypeCast())
                                                            pattern->visiteeImplementations.push_back(impl->TypeCast());
                                                    } while((impl = super_visitee->subtypes->NextElement()));
                                                }

                                                pattern->accept = method;
                                                pattern->visit = call->symbol->MethodCast();
                                                if(call->arguments->Argument(k)->kind == Ast::THIS_EXPRESSION)
                                                    pattern->isThisExposed = true;
                                                else
                                                    pattern->exposed = call->arguments->Argument(k)->NameCast()->symbol->VariableCast();

                                                pattern->file = method->containing_type->file_symbol;

                                                output.push_back(pattern);
                                            }
                                            k++;
                                        }
                                    }
                                }
                                j++;
                            }
                        }
                    }
                }
                i++;
            }
        }
    }

    return output;
}

vector<Pattern::Ptr> Pattern::FindObserver(Control *control)
{
    auto cs_table = control->cs_table;
    auto d_table = control->d_table;
    vector<Pattern::Ptr> output;

    vector<TypeSymbol*> cache;
    unsigned c;
    for (c = 0; c < cs_table ->size(); c++)
    {
        TypeSymbol *unit_type = (*cs_table)[c];
        if (!unit_type -> ACC_INTERFACE())
        {
            for (unsigned i = 0; i < unit_type -> declaration-> NumInstanceVariables(); i++)
            {
                AstFieldDeclaration* field_decl = unit_type -> declaration -> InstanceVariable(i);
                for (unsigned vi = 0; vi < field_decl -> NumVariableDeclarators(); vi++)
                {
                    AstVariableDeclarator* vd = field_decl -> VariableDeclarator(vi);

                    TypeSymbol *generic_type = unit_type -> IsOnetoMany(vd -> symbol, d_table) ;
                    if (generic_type && generic_type -> file_symbol)
                    {
                        for (int j = 0; j < d_table -> size(); j++)
                        {
                            DelegationEntry* entry = d_table -> Entry(j);
                            if ((unit_type == entry -> enclosing -> containing_type) && (generic_type == entry -> to))
                            {
                                /*
                                    if ((unit_type == generic_type) && (entry -> vsym == vd -> symbol) && (entry -> enclosing == entry -> method) && (entry -> enclosing -> callers -> Size() > 1))
                                    {
                                        nObserver++;
                                        Coutput << "Observer Pattern." << endl
                                            << unit_type -> Utf8Name() << " is an observer iterator." << endl
                                            << generic_type -> Utf8Name() << " is the generic type for the listeners." << endl
                                            << entry -> enclosing -> Utf8Name() << " is the notify method." << endl
                                            << entry -> method -> Utf8Name() << " is the update method." << endl;
                                        Coutput << "Subject class(es):";
                                        entry -> enclosing -> callers -> Print();
                                        Coutput << "File Location: " << unit_type->file_symbol->FileName() << endl << endl;
                                    }
                                */
                                if (!entry->enclosing->callers
                                    || (!entry->enclosing->callers -> IsElement(generic_type)
                                            //&& !entry->enclosing->callers -> IsElement(unit_type)
                                    )
                                        )
                                {
                                    VariableSymbol *iterator = 0;
                                    ControlAnalysis controlflow(entry -> call);
                                    if (entry -> enclosing -> declaration -> MethodDeclarationCast()
                                        && entry -> enclosing -> declaration -> MethodDeclarationCast() -> method_body_opt)
                                        entry -> enclosing -> declaration -> MethodDeclarationCast() -> method_body_opt -> Accept(controlflow);

                                    if (controlflow.result
                                        && controlflow.IsRepeated()
                                        && entry -> base_opt
                                        && (iterator = IteratorVar(entry->base_opt))
                                        && ((iterator == vd->symbol)
                                            || (vd->symbol == unit_type -> Shadows(iterator))
                                            || (vd -> symbol == ListVar(iterator))))
                                    {
                                        auto pattern = make_shared<Observer>();
                                        pattern->iterator = unit_type;
                                        pattern->listenerType = generic_type;
                                        pattern->notify = entry->enclosing;
                                        pattern->update = entry->method;
                                        pattern->file = unit_type->file_symbol;

                                        auto subjects = entry->enclosing->callers;
                                        if(subjects) {
                                            auto subject = subjects->FirstElement();
                                            do {
                                                if (subject->TypeCast())
                                                    pattern->subjects.push_back(subject->TypeCast());
                                            } while ((subject = subjects->NextElement()));
                                        }

                                        output.push_back(pattern);
                                    }
                                }
                            }
                            else if ((generic_type == entry -> enclosing -> containing_type) && (unit_type == entry -> to))
                            {
                                unsigned j = 0;
                                for (; (j < cache.size()) && (cache[j] != unit_type) ; j++);
                                if (j == cache.size())
                                {
                                    cache.push_back(unit_type);

                                    auto pattern = make_shared<Mediator>();
                                    pattern->mediator = unit_type;
                                    pattern->colleagues.push_back(generic_type);
                                    pattern->file = unit_type->file_symbol;

                                    auto impl = generic_type->subtypes->FirstElement();
                                    if(impl)
                                        do {
                                            if(impl->TypeCast())
                                                pattern->colleagues.push_back(impl->TypeCast());
                                        } while ((impl = generic_type->subtypes->NextElement()));

                                    output.push_back(pattern);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return output;
}

vector<Pattern::Ptr> Pattern::FindMediator(Control *control)
{
    auto cs_table = control->cs_table;
    vector<Pattern::Ptr> output;

    map<TypeSymbol*, SymbolSet*> cache;
    vector<TypeSymbol*> ordered_cache;
    unsigned c;
    for (c = 0; c < cs_table ->size(); c++)
    {
        TypeSymbol *unit_type = (*cs_table)[c];
        // check if this facade class can be serving as a mediator for some of the hidden classes.
        unsigned i = 0;
        while (i < unit_type->NumMethodSymbols())
        {
            MethodSymbol *msym = unit_type->MethodSym(i);
            if (msym->callers && msym->invokees)
            {
                Symbol *sym1 = msym->callers->FirstElement();
                while (sym1)
                {
                    TypeSymbol *caller = sym1->TypeCast();
                    Symbol *sym2 = msym->invokees->FirstElement();
                    while(sym2)
                    {
                        TypeSymbol *callee = sym2->MethodCast()->containing_type;
                        if (caller->file_symbol
                            && caller->file_symbol->IsJava()
                            && caller->call_dependents
                            && callee->file_symbol
                            && callee->file_symbol->IsJava()
                            && callee->call_dependents
                            && !caller->call_dependents->IsElement(callee)
                            && !callee->call_dependents->IsElement(caller)
                            && (caller != unit_type)
                            && (callee != unit_type)
                                )
                        {
                            if (!unit_type->mediator_colleagues)
                                unit_type->mediator_colleagues = new SymbolSet(0);
                            unit_type->mediator_colleagues->AddElement(caller);
                            unit_type->mediator_colleagues->AddElement(callee);

                            /* trying to get rid of STL map
                         map<TypeSymbol*, SymbolSet*>::iterator m = cache.find(unit_type);
                         if (m == cache.end())
                         {
                             SymbolSet *set = new SymbolSet(0);
                             set->AddElement(caller);
                             set->AddElement(callee);
                             cache.insert(pair<TypeSymbol*, SymbolSet*>(unit_type, set));
                         }
                         else
                         {
                             m->second->AddElement(caller);
                             m->second->AddElement(callee);
                         }
                         */

                        }
                        sym2 = msym->invokees->NextElement();
                    }
                    sym1 = msym->callers->NextElement();
                }
            }
            i++;
        }
    }
    // Print results
    /*
    nMediator += cache.size();
    map<TypeSymbol*, SymbolSet*>::iterator pattern;
    for (pattern = cache.begin(); pattern != cache.end(); pattern++)
    {
        Coutput << "Mediator Pattern." << endl;
        Coutput << "Mediator: " << pattern->first->Utf8Name() << endl;
        Coutput << "Colleagues: ";
        pattern->second->Print();
        Coutput << "FileLocation: " << pattern->first->file_symbol->FileName() << endl << endl;

        mediators.AddElement(pattern->first);
    }
    */

    for (c = 0; c < cs_table ->size(); c++)
    {
        if ((*cs_table)[c]->mediator_colleagues)
        {
            auto mediator = (*cs_table)[c];
            auto pattern = make_shared<Mediator>();
            pattern->mediator = mediator;
            pattern->file = mediator->file_symbol;

            auto colleague = mediator->mediator_colleagues->FirstElement();
            do {
                if(colleague->TypeCast())
                    pattern->colleagues.push_back(colleague->TypeCast());
            } while ((colleague = mediator->mediator_colleagues->NextElement()));

            output.push_back(pattern);
            pattern->Print();

            /*Coutput << "Mediator Pattern." << endl;
            Coutput << "Mediator: " << (*cs_table)[c]->Utf8Name() << endl;
            Coutput << "Colleagues: ";
            (*cs_table)[c]->mediator_colleagues->Print();
            Coutput << "FileLocation: " << (*cs_table)[c]->file_symbol->FileName() << endl << endl;*/

            //mediators.AddElement((*cs_table)[c]);
            // TODO add this statistic?
        }
    }

    return output;
}