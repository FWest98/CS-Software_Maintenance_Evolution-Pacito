#include "SingletonAnalysis.h"
#include "FactoryAnalysis.h"

bool SingletonAnalysis::ReturnsSingleton() {
    visited.AddElement(method);
    flatten.BuildSummary();
    //flatten.DumpSummary();

    for (unsigned t = 0; t < flatten.traces.size(); t++) {
        Snapshot *snapshot = flatten.traces[t];
        path.clear();
        path.push_back(snapshot->index);
        TracePath(snapshot);
    }
    // footprints, remove duplicates
    Utility::RemoveDuplicates(footprints);

    if (fingerprints.size() == 0)
        return false;
    else if ((fingerprints.size() == 1) && (footprints.size() == 0)) {
        return (variable->declarator->variable_initializer_opt &&
                (variable->declarator->variable_initializer_opt->kind == Ast::CLASS_CREATION));
    } else if (footprints.size() != 1)
        return false;
    else {
        unsigned occurrances = 0;
        for (unsigned i = 0; i < exec_paths.size(); i++) {
            vector<signed> result;
            Utility::Intersection(footprints, exec_paths[i], result);
            if (result.size() == 1)
                occurrances++;
        }
        if (occurrances == exec_paths.size())
            return false;

        Snapshot *snapshot = flatten.summary[footprints[0]];
        vector<AstExpression *> conjoints;
        map<VariableSymbol *, AstExpression *> constraints;
        flatten.FlattenBoolean(conjoints, snapshot->condition);
        for (unsigned i = 0; i < conjoints.size(); i++) {
            // check whether there are other static variables to track
            // but if "instance == null" is in conjoints, then stop checking
            // otherwise, check if these additional variables are
            // 1. modified so that this snapshot will never be entered again, and
            // 2. these vars are not changed anywhere besides snapshot (flow-insensitive)

            // consider BINARY and PRE_UNARY expressions
            if (conjoints[i]->kind == Ast::BINARY) {
                AstBinaryExpression *expression = (*&conjoints[i])->BinaryExpressionCast();
                if (expression->left_expression->symbol == variable) {
                    if ((expression->Tag() == AstBinaryExpression::EQUAL_EQUAL)
                        && (expression->right_expression->kind == Ast::NULL_LITERAL))
                        return true;
                    else
                        return false;
                } else if (expression->left_expression->symbol->VariableCast()) {
                    VariableSymbol *vsym = expression->left_expression->symbol->VariableCast();
                    if (vsym->ACC_PRIVATE()
                        && vsym->ACC_STATIC()
                        && (strcmp(vsym->Type()->Utf8Name(), "boolean") == 0))
                        constraints.insert(pair<VariableSymbol *, AstExpression *>(vsym, expression));
                }
            } else if (conjoints[i]->kind == Ast::PRE_UNARY) {
                AstPreUnaryExpression *pre_unary = (*&conjoints[i])->PreUnaryExpressionCast();
                if (pre_unary->expression->symbol->VariableCast()) {
                    VariableSymbol *vsym = pre_unary->expression->symbol->VariableCast();
                    if (vsym->ACC_PRIVATE()
                        && vsym->ACC_STATIC()
                        && (strcmp(vsym->Type()->Utf8Name(), "boolean") == 0))
                        constraints.insert(pair<VariableSymbol *, AstExpression *>(vsym, pre_unary));
                } else if ((pre_unary->Tag() == AstPreUnaryExpression::NOT)
                           && (pre_unary->expression->kind == Ast::BINARY)
                           && (pre_unary->expression->BinaryExpressionCast()->left_expression->symbol == variable)) {
                    if ((pre_unary->expression->BinaryExpressionCast()->Tag() == AstBinaryExpression::NOT_EQUAL)
                        && (pre_unary->expression->BinaryExpressionCast()->right_expression->kind == Ast::NULL_LITERAL))
                        return true;
                    else
                        return false;
                }
            } else if (conjoints[i]->symbol->VariableCast()) {
                VariableSymbol *vsym = conjoints[i]->symbol->VariableCast();
                if (vsym->ACC_PRIVATE()
                    && vsym->ACC_STATIC()
                    && (strcmp(vsym->Type()->Utf8Name(), "boolean") == 0))
                    constraints.insert(pair<VariableSymbol *, AstExpression *>(vsym, conjoints[i]));
            }
        }
        if (constraints.size() == 0)
            return false;
        else {
            // analyze statements in snapshot, making sure that these control variables close the entrance to this snapshot
            for (unsigned j = (*snapshot->statements).size() - 1; j < (*snapshot->statements).size(); j--) {
                Ast *statement = (*snapshot->statements)[j];
                if (statement->kind == Ast::ASSIGNMENT) {
                    AstAssignmentExpression *assignment = statement->AssignmentExpressionCast();
                    if (assignment->left_hand_side->symbol->VariableCast()) {
                        VariableSymbol *vsym = assignment->left_hand_side->symbol->VariableCast();
                        map<VariableSymbol *, AstExpression *>::iterator p = constraints.find(vsym);
                        if (p != constraints.end()) {
                            // analyze right_hand_side expression
                            if (assignment->expression->kind == Ast::TRUE_LITERAL) {
                                if (((p->second->kind == Ast::PRE_UNARY)
                                     && p->second->PreUnaryExpressionCast()->expression->symbol->VariableCast())
                                    || ((p->second->kind == Ast::BINARY)
                                        && p->second->BinaryExpressionCast()->left_expression->symbol->VariableCast()
                                        && (p->second->BinaryExpressionCast()->right_expression->kind ==
                                            Ast::FALSE_LITERAL)))
                                    goto Ugly;
                            } else if (assignment->expression->kind == Ast::FALSE_LITERAL) {
                                if (p->second->symbol->VariableCast()
                                    || ((p->second->kind == Ast::BINARY)
                                        && p->second->BinaryExpressionCast()->left_expression->symbol->VariableCast()
                                        && (p->second->BinaryExpressionCast()->right_expression->kind ==
                                            Ast::TRUE_LITERAL)))
                                    goto Ugly;
                            }
                        }
                    }
                }
            }
            return false;

            // flow-insensitive analysis in summary
            Ugly:
            SymbolSet modified;
            for (unsigned i = 0; i < flatten.summary.size(); i++) {
                Snapshot *snapshot = flatten.summary[i];
                if (snapshot->index != footprints[0]) {
                    for (unsigned j = 0; j < (*snapshot->statements).size(); j++) {
                        Ast *statement = (*snapshot->statements)[j];
                        if (statement->kind == Ast::ASSIGNMENT) {
                            AstAssignmentExpression *assignment = statement->AssignmentExpressionCast();
                            if (assignment->left_hand_side->symbol->VariableCast()
                                && (constraints.find(assignment->left_hand_side->symbol->VariableCast()) !=
                                    constraints.end()))
                                modified.AddElement(assignment->left_hand_side->symbol->VariableCast());
                            // skip analysis on the right_hand_side expression
                        }
                    }
                }
            }
            return (modified.Size() < constraints.size());
        }
    }
}

void SingletonAnalysis::TracePath(Snapshot *snapshot) {
    set<signed> next(snapshot->previous);

    for (unsigned j = snapshot->statements->size() - 1; j < snapshot->statements->size(); j--) {
        Ast *statement = (*snapshot->statements)[j];
        if (statement->kind == Ast::RETURN) {
            AstReturnStatement *return_statement = statement->ReturnStatementCast();
            if (return_statement->expression_opt) {
                AstExpression *expression = Utility::RemoveCasting(return_statement->expression_opt);
                if (expression->symbol->VariableCast()
                    && (expression->symbol->VariableCast() == variable)) {
                    fingerprints.push_back(snapshot->index);
                }
            }
        } else if (statement->kind == Ast::ASSIGNMENT) {
            AstAssignmentExpression *assignment = statement->AssignmentExpressionCast();
            if (assignment->left_hand_side->symbol->VariableCast()
                && (assignment->left_hand_side->symbol == variable)) {
                AstExpression *expression = Utility::RemoveCasting(assignment->expression);
                if (expression->kind == Ast::CLASS_CREATION) {
                    for (unsigned i = 0; i < expression->ClassCreationExpressionCast()->arguments->NumArguments(); i++)
                        if (expression->ClassCreationExpressionCast()->arguments->Argument(i)->symbol == variable)
                            goto pass;
                    // check if this class creation negates the dominator condition

                    footprints.push_back(snapshot->index);
                    pass:;
                } else if (expression->kind == Ast::CALL) {
                    // Check: are we currnetly under the scope where condition says instance == null?
                    // How to check a segment of code is only executed once, regardless of flag?

                    AstMethodInvocation *call = expression->MethodInvocationCast();
                    if ((strcmp(call->symbol->MethodCast()->Utf8Name(), "newInstance") == 0)
                        && (((call->base_opt->kind == Ast::NAME) &&
                             (strcmp(call->base_opt->symbol->VariableCast()->Type()->Utf8Name(), "Class") == 0))
                            || ((call->base_opt->kind == Ast::CALL)
                                && (strcmp(call->base_opt->symbol->MethodCast()->Utf8Name(), "forName") == 0)
                                && (call->base_opt->MethodInvocationCast()->base_opt->kind == Ast::NAME)
                                &&
                                (strcmp(call->base_opt->MethodInvocationCast()->base_opt->symbol->TypeCast()->Utf8Name(),
                                        "Class") == 0)))
                            ) {
                        footprints.push_back(snapshot->index);
                    } else {
                        FactoryAnalysis factory(call->symbol->MethodCast(), ast_pool);
                        if (factory.IsCreationMethod()) {
                            footprints.push_back(snapshot->index);
                        }
                    }
                }
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
            //paths.push_back('E');
            exec_paths.push_back(path);
        }
    }
}

bool SingletonAnalysis::ReturnsSingleton1() {
#ifdef DONT_BOTHER
    visited.AddElement(method);
    method->declaration->MethodDeclarationCast()->method_body_opt->Accept(flatten);

    // TODO: The following should be included in Flatten.
    for(unsigned i = 0; i < flatten.summary.size(); i++)
    {
        set<signed>::iterator p;
        for (p = (flatten.summary[i]->next).begin(); p != (flatten.summary[i]->next).end(); p++)
            (flatten.summary[*p]->previous).insert(i);
    }
    flatten.DumpSummary();

    for (unsigned t = 0; t < flatten.traces.size(); t++)
    {
        Snapshot *return_snapshot = NULL;
        int return_path = -1;

        vector<unsigned> snapshots;
        snapshots.push_back(flatten.traces[t]->index);
        while(!snapshots.empty())
        //for (unsigned i = flatten.traces[t]->index; i < flatten.summary.size(); i--)
        {
            Snapshot *snapshot = flatten.summary[snapshots[snapshots.size() - 1]];
            snapshots.pop_back();
            /*
            if ((i == flatten.traces[t]->index)
            || ((i < flatten.traces[t]->index)
                && (flatten.TransitionFlow(snapshot->condition, flatten.summary[i + 1]->condition) != Flatten::NOTRANSITION)))
            {
            */
            for (unsigned j = snapshot->statements->size() - 1; j < snapshot->statements->size(); j--)
            {
                Ast *statement = (*snapshot->statements)[j];
                if (statement->kind == Ast::RETURN)
                {
                    AstReturnStatement *return_statement = statement->ReturnStatementCast();
                    if (return_statement->expression_opt)
                    {
                        AstExpression *expression = Utility::RemoveCasting(return_statement->expression_opt);
                        if (expression->kind == Ast::NULL_LITERAL)
                            goto stop_trace;
                        else if (expression->symbol->VariableCast()
                            && (expression->symbol->VariableCast() == variable))
                        {
                            return_snapshot = snapshot;
                            return_path = paths.size();
                            paths.push_back('E');
                        }
                        else
                            return false;
                    }
                }
                else if (return_snapshot && (statement->kind == Ast::ASSIGNMENT))
                {
                    AstAssignmentExpression *assignment = statement->AssignmentExpressionCast();
                    if (assignment->left_hand_side->symbol->VariableCast()
                    && (assignment->left_hand_side->symbol == variable))
                    {
                        AstExpression *expression = Utility::RemoveCasting(assignment->expression);
                        if (expression->kind == Ast::CLASS_CREATION)
                        {
                            // check if this class creation negates the dominator condition
                            vector<AstExpression*> conjoints;
                            flatten.FlattenBoolean(conjoints, snapshot->condition);
                            for (unsigned i = 0; i < conjoints.size(); i++)
                            {
                                if ((conjoints[i]->kind == Ast::BINARY)
                                && ((*&conjoints[i])->BinaryExpressionCast()->Tag() == AstBinaryExpression::EQUAL_EQUAL)
                                && ((*&conjoints[i])->BinaryExpressionCast()->left_expression->symbol == variable)
                                && ((*&conjoints[i])->BinaryExpressionCast()->right_expression->kind == Ast::NULL_LITERAL))
                                {
                                    paths[return_path] = 'N';
                                    goto stop_trace;
                                }
                            }
                        }
                        else if (expression->kind == Ast::CALL)
                        {
                            AstMethodInvocation *call = expression->MethodInvocationCast();
                            if ((strcmp(call->symbol->MethodCast()->Utf8Name(), "newInstance") == 0)
                            //&& (call->base_opt->kind == Ast::CALL)
                            && (strcmp(call->base_opt->symbol->MethodCast()->Utf8Name(), "forName") == 0)
                            //&& (call->base_opt->MethodInvocationCast()->base_opt ->kind== Ast::NAME)
                            && (strcmp(call->base_opt->MethodInvocationCast()->base_opt->symbol->TypeCast()->Utf8Name(), "Class") == 0))
                            {
                                paths[return_path] = 'N';
                                goto stop_trace;
                            }
                        }
                        else if (expression->kind == Ast::NULL_LITERAL)
                        {
                            paths[return_path] = 'X';
                            goto stop_trace;
                        }
                    }
                }
            }
            /*
            }
            */
            // check if it's possible to go to the previous snapshot.
            set<signed>::iterator p;
            for(p = (snapshot->previous).begin(); p != (snapshot->previous).end(); p++)
                snapshots.push_back(*p);
        }
        stop_trace: return_snapshot = NULL;
        return_path = -1;
    }

    int nc = 0, ne = 0;
    for (unsigned i = 0; i < paths.size(); i++)
    {
        if (paths[i] == 'N')
            nc++;
        else if (paths[i] == 'E')
            ne++;
    }
    if ((nc==1) && (ne == 1))
        return true;
    else if ((ne == 1)
        && variable->declarator->variable_initializer_opt
        && (variable->declarator->variable_initializer_opt->kind == Ast::CLASS_CREATION))
        return true;
    else
        return false;
#endif
    return false;
}