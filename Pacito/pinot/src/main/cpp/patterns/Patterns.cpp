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
                    cor->subject = entry->from;
                    cor->propagator = entry->vsym;
                    cor->handler = entry->enclosing;
                    cor->file = entry->enclosing->containing_type->file_symbol;
                    output.push_back(cor);

                    cache.push_back(entry->enclosing);
                } else if (result == ChainAnalysis::DECORATOR) {
                    auto decorator = std::make_shared<Decorator>();
                    decorator->subject = entry->from;
                    decorator->decorateOp = entry->enclosing;
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