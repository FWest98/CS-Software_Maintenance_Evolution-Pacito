#include "ChainAnalysis.h"

ChainAnalysis::ResultTag ChainAnalysis::AnalyzeCallChain() {
    flatten.BuildSummary();
    //flatten.DumpSummary();

    for (unsigned t = 0; t < flatten.traces.size(); t++) {
        Snapshot *snapshot = flatten.traces[t];
        path.clear();
        path.push_back(snapshot->index);
        TracePath(snapshot);
    }

    // analyze exec paths HERE

    // for footprints, check for duplicates
    for (unsigned i = 0; i < footprints.size(); i++)
        for (unsigned j = 0; j < footprints.size(); j++)
            if ((i != j) && (footprints[i] == footprints[j]))
                return NONE;

    unsigned occurrances = 0;
    for (unsigned i = 0; i < paths.size(); i++) {
        vector<signed> result;
        Utility::Intersection(footprints, paths[i], result);
        if (result.size() == 1)
            occurrances++;
    }
    if (occurrances == paths.size())
        return DECORATOR;
    else if (occurrances == 1)
        // check for deferral
        return CoR;
    else
        return NONE;
}

void ChainAnalysis::TraceBinaryExpression(AstBinaryExpression *expression, Snapshot *snapshot) {
    if (expression->left_expression->MethodInvocationCast()
        && expression->left_expression->MethodInvocationCast()->base_opt
        && expression->left_expression->MethodInvocationCast()->symbol->MethodCast()
        && (expression->left_expression->MethodInvocationCast()->base_opt->symbol == variable)
        && ((expression->left_expression->MethodInvocationCast()->symbol == method)
            || (strcmp(expression->left_expression->MethodInvocationCast()->symbol->MethodCast()->Utf8Name(),
                       method->Utf8Name()) == 0))
        && (strcmp(expression->left_expression->MethodInvocationCast()->symbol->MethodCast()->SignatureString(),
                   method->SignatureString()) == 0)
            )
        footprints.push_back(snapshot->index);
    else if (expression->right_expression->MethodInvocationCast()
             && expression->right_expression->MethodInvocationCast()->base_opt
             && expression->right_expression->MethodInvocationCast()->symbol->MethodCast()
             && (expression->right_expression->MethodInvocationCast()->base_opt->symbol == variable)
             && ((expression->right_expression->MethodInvocationCast()->symbol == method)
                 || (strcmp(expression->right_expression->MethodInvocationCast()->symbol->MethodCast()->Utf8Name(),
                            method->Utf8Name()) == 0))
             && (strcmp(expression->right_expression->MethodInvocationCast()->symbol->MethodCast()->SignatureString(),
                        method->SignatureString()) == 0)
            )
        footprints.push_back(snapshot->index);
    else if (expression->left_expression->kind == Ast::BINARY)
        TraceBinaryExpression(expression->left_expression->BinaryExpressionCast(), snapshot);
    else if (expression->right_expression->kind == Ast::BINARY)
        TraceBinaryExpression(expression->right_expression->BinaryExpressionCast(), snapshot);
}

void ChainAnalysis::TracePath(Snapshot *snapshot) {
    set<signed> next(snapshot->previous);

    for (unsigned j = snapshot->statements->size() - 1; j < snapshot->statements->size(); j--) {
        Ast *statement = (*snapshot->statements)[j];
        if (statement->kind == Ast::RETURN) {
            if (statement->ReturnStatementCast()->expression_opt) {
                AstExpression *expression = Utility::RemoveCasting(statement->ReturnStatementCast()->expression_opt);
                if (expression->kind == Ast::CALL) {
                    AstMethodInvocation *call = expression->MethodInvocationCast();
                    VariableSymbol *vsym = (call->base_opt) ? call->base_opt->symbol->VariableCast() : NULL;
                    MethodSymbol *msym = call->symbol->MethodCast();
                    if ((vsym == variable)
                        && ((msym == method) || (strcmp(msym->Utf8Name(), method->Utf8Name()) == 0))
                        && (strcmp(msym->SignatureString(), method->SignatureString()) == 0)) {
                        footprints.push_back(snapshot->index);
                    }
                } else if (expression->kind == Ast::BINARY) {
                    TraceBinaryExpression(expression->BinaryExpressionCast(), snapshot);
                }
            }
        } else if (statement->kind == Ast::CALL) {
            AstMethodInvocation *call = statement->MethodInvocationCast();
            VariableSymbol *vsym = (call->base_opt) ? call->base_opt->symbol->VariableCast() : NULL;
            MethodSymbol *msym = call->symbol->MethodCast();
            if ((vsym == variable)
                && ((msym == method) || (strcmp(msym->Utf8Name(), method->Utf8Name()) == 0))
                && (strcmp(msym->SignatureString(), method->SignatureString()) == 0)) {
                footprints.push_back(snapshot->index);
            }
        }
    }
    set<signed>::iterator p;
    for (p = next.begin(); p != next.end(); p++) {
        if (*p >= 0) {
            path.push_back(*p);
            TracePath(flatten.summary[*p]);
            path.pop_back();
        } else {
            paths.push_back(path);
        }
    }
}