#include "FlyweightAnalysis.h"

void FlyweightAnalysis::visit(AstBlock *block) {
    if (block->NumStatements()) {
        unsigned lstmt = (block->NumStatements() == 1) ? 0 : (block->NumStatements() - 1);
        for (unsigned i = 0; i < lstmt; i++) visit(block->Statement(i));
        visit(block->Statement(lstmt));
        UpdateSummary();
    }
}

void FlyweightAnalysis::visit(AstWhileStatement *while_statement) {
    visit(while_statement->statement);
}

void FlyweightAnalysis::visit(AstForStatement *for_statement) {
    UpdateSummary();
    conditions.push_back(for_statement->end_expression_opt);
    visit(for_statement->statement);
    UpdateSummary();
    conditions.pop_back();
}

void FlyweightAnalysis::visit(AstTryStatement *try_statement) {
    visit(try_statement->block);
}

void FlyweightAnalysis::visit(AstStatement *statement) {
    switch (statement->kind) {
        case Ast::IF:
            visit(statement->IfStatementCast());
            break;
        case Ast::WHILE:
            visit(statement->WhileStatementCast());
            break;
        case Ast::FOR:
            visit(statement->ForStatementCast());
            break;
        case Ast::TRY:
            visit(statement->TryStatementCast());
            break;
        case Ast::EXPRESSION_STATEMENT:
            visit(statement->ExpressionStatementCast()->expression);
            break;
        case Ast::SYNCHRONIZED_STATEMENT:
            visit(statement->SynchronizedStatementCast());
            break;
        case Ast::BLOCK:
            visit(statement->BlockCast());
            break;
        case Ast::RETURN:
            visit(statement->ReturnStatementCast());
            break;
        case Ast::LOCAL_VARIABLE_DECLARATION:
            visit(statement->LocalVariableStatementCast());
            break;
        default:
            break;
    }
}

void FlyweightAnalysis::visit(AstExpression *expression) {
    switch (expression->kind) {
        case Ast::PARENTHESIZED_EXPRESSION:
            visit(expression->ParenthesizedExpressionCast()->expression);
            break;
        case Ast::CAST:
            visit(expression->CastExpressionCast()->expression);
            break;
        case Ast::CONDITIONAL:
            visit(expression->ConditionalExpressionCast());
            break;
        case Ast::ASSIGNMENT:
            visit(expression->AssignmentExpressionCast());
            break;
        case Ast::CALL:
            visit(expression->MethodInvocationCast());
            break;
        default:
            break;
    }
}

void FlyweightAnalysis::visit(AstMethodInvocation *call) {
    // might want to check all participants in this method invocationo
    // e.g., base_opt, 	call->symbol->MethodCast()>Type(), call->arguments->Argument(i), etc

    if (call->NumArguments() > 1) {
        AstExpression *expression = *&call->arguments->Argument(1);
        expression = (expression->kind == Ast::CAST) ? expression->CastExpressionCast()->expression : expression;
        if (expression->symbol->VariableCast() && (expression->symbol->VariableCast()->Type() == flyweight)) {
            statements.push_back(call);
        }
    }
}

void FlyweightAnalysis::visit(AstIfStatement *statement) {
    UpdateSummary();
    conditions.push_back(statement->expression);
    visit(statement->expression);
    visit(statement->true_statement);
    UpdateSummary();
    conditions.pop_back();
    if (statement->false_statement_opt)
        visit(statement->false_statement_opt);
}

void FlyweightAnalysis::visit(AstAssignmentExpression *expression) {
    if (!expression->left_hand_side->symbol) return;
    if (expression->left_hand_side->symbol->VariableCast()
        && (expression->left_hand_side->symbol->VariableCast()->Type() == flyweight))
        statements.push_back(expression);
    else if (expression->left_hand_side->symbol->TypeCast()
             && (expression->left_hand_side->symbol->TypeCast() == flyweight))
        statements.push_back(expression);
    // TODO also check for aliasing
}

void FlyweightAnalysis::visit(AstLocalVariableStatement *local_var) {
    if (local_var->type->symbol == flyweight) {
        for (unsigned i = 0; i < local_var->NumVariableDeclarators(); i++)
            visit(local_var->VariableDeclarator(i));
    }
}

void FlyweightAnalysis::visit(AstVariableDeclarator *var_declarator) {
    if (var_declarator->variable_initializer_opt && (var_declarator->symbol->Type() == flyweight))
        statements.push_back(var_declarator);
}

void FlyweightAnalysis::visit(AstReturnStatement *statement) {
    if (statement->expression_opt && statement->expression_opt->symbol) {
        if (statement->expression_opt->symbol->VariableCast()
            && (statement->expression_opt->symbol->VariableCast()->Type() == flyweight))
            statements.push_back(statement);
        else if (statement->expression_opt->symbol->TypeCast()
                 && (statement->expression_opt->symbol->TypeCast() == flyweight))
            statements.push_back(statement);
    }
}

void FlyweightAnalysis::UpdateSummary() {
    if (statements.size()) {
        Snapshot *snapshot = new Snapshot();
        snapshot->statements = new vector<Ast *>(statements);
        statements.clear();
        if (conditions.size()) {
            snapshot->conditions = new vector<AstExpression *>(conditions);
        }
        snapshot->index = summary.size();
        summary.push_back(snapshot);
    }
}

void FlyweightAnalysis::DumpSummary() {
    Coutput << GetFlyweight->Utf8Name() << endl;
    for (unsigned i = 0; i < summary.size(); i++) {
        Snapshot *snapshot = summary[i];
        Coutput << "Snapshot[" << i << "]" << endl;
        Coutput << "STATEMENTS:" << endl;
        unsigned j;
        for (j = 0; j < snapshot->statements->size(); j++) {
            if ((*snapshot->roles)[j]->vsym)
                Coutput << (*snapshot->roles)[j]->vsym->Utf8Name();
            else
                Coutput << (*snapshot->roles)[j]->array_access->base->symbol->VariableCast()->Utf8Name()
                        << "[" << (*snapshot->roles)[j]->array_access->expression->symbol->VariableCast()->Utf8Name()
                        << "]";
            Coutput << ": " << (*snapshot->roles)[j]->TagName() << endl;
            (*snapshot->statements)[j]->Print();
        }
        Coutput << "CONDITIONS:" << endl;
        if (snapshot->conditions) {
            for (j = 0; j < snapshot->conditions->size(); j++)
                (*snapshot->conditions)[j]->Print();
        }
        Coutput << endl;
    }
}

void FlyweightAnalysis::AssignRoles() {
    for (unsigned i = 0; i < summary.size(); i++) {
        Snapshot *snapshot = summary[i];
        snapshot->roles = new vector<Role *>();
        for (unsigned j = 0; j < snapshot->statements->size(); j++) {
            Ast *stmt = (*snapshot->statements)[j];
            if (stmt->kind == Ast::VARIABLE_DECLARATOR) {
                AstVariableDeclarator *var_declarator = stmt->VariableDeclaratorCast();
                if (var_declarator->variable_initializer_opt) {
                    if (var_declarator->variable_initializer_opt->kind == Ast::CLASS_CREATION) {
                        //TODO: check parameters as well.
                        snapshot->roles->push_back(new Role(var_declarator->symbol, Role::CREATE));
                    } else if ((var_declarator->variable_initializer_opt->kind == Ast::PARENTHESIZED_EXPRESSION)
                               &&
                               (var_declarator->variable_initializer_opt->ParenthesizedExpressionCast()->expression->kind ==
                                Ast::CAST)
                               &&
                               (var_declarator->variable_initializer_opt->ParenthesizedExpressionCast()->expression->CastExpressionCast()->expression->kind ==
                                Ast::CALL)) {
                        AstMethodInvocation *call = var_declarator->variable_initializer_opt->ParenthesizedExpressionCast()->expression->CastExpressionCast()->expression->MethodInvocationCast();
                        if (call->base_opt && call->base_opt->symbol->VariableCast()) {
                            if (!container_type)
                                container_type = Utility::IdentifyContainerType(call->base_opt->symbol->VariableCast());
                            if (container_type && container_type->IsGetMethod(call->symbol->MethodCast()))
                                snapshot->roles->push_back(new Role(var_declarator->symbol, Role::RETRIEVE));
                        }
                    } else if (var_declarator->variable_initializer_opt->kind == Ast::ARRAY_ACCESS) {
                        if (!container_type)
                            container_type = new ArrayContainer(
                                    var_declarator->variable_initializer_opt->ArrayAccessCast()->base->symbol->VariableCast());

                        //TODO: check for method invocation from a hashtable/collection.
                        snapshot->roles->push_back(new Role(var_declarator->symbol, Role::RETRIEVE));
                    } else if (var_declarator->variable_initializer_opt->kind == Ast::NULL_LITERAL) {
                        snapshot->roles->push_back(new Role(var_declarator->symbol, Role::NIL));
                    }
                }
            } else if (stmt->kind == Ast::ASSIGNMENT) {
                AstAssignmentExpression *assignment = stmt->AssignmentExpressionCast();
                if (assignment->left_hand_side->kind == Ast::ARRAY_ACCESS) {
                    if (!container_type)
                        container_type = new ArrayContainer(
                                assignment->left_hand_side->ArrayAccessCast()->base->symbol->VariableCast());

                    if (assignment->expression->symbol->VariableCast()) {
                        snapshot->roles->push_back(
                                new Role(assignment->expression->symbol->VariableCast(), Role::REGISTER));
                    } else if (assignment->expression->kind == Ast::CLASS_CREATION) {
                        snapshot->roles->push_back(
                                new Role(assignment->left_hand_side->ArrayAccessCast(), Role::ALLOCATE));
                    }
                } else if (assignment->left_hand_side->symbol->VariableCast()) {
                    if (assignment->expression->kind == Ast::CLASS_CREATION) {
                        //TODO: check parameters as well.
                        snapshot->roles->push_back(
                                new Role(assignment->left_hand_side->symbol->VariableCast(), Role::CREATE));
                    } else if (assignment->expression->kind == Ast::ARRAY_ACCESS) {
                        if (!container_type)
                            container_type = new ArrayContainer(
                                    assignment->expression->ArrayAccessCast()->base->symbol->VariableCast());
                        snapshot->roles->push_back(
                                new Role(assignment->left_hand_side->symbol->VariableCast(), Role::RETRIEVE));
                    } else if ((assignment->expression->kind == Ast::PARENTHESIZED_EXPRESSION)
                               && (assignment->expression->ParenthesizedExpressionCast()->expression->kind == Ast::CAST)
                               &&
                               (assignment->expression->ParenthesizedExpressionCast()->expression->CastExpressionCast()->expression->kind ==
                                Ast::CALL)) {
                        AstMethodInvocation *call = assignment->expression->ParenthesizedExpressionCast()->expression->CastExpressionCast()->expression->MethodInvocationCast();
                        if (call->base_opt && call->base_opt->symbol->VariableCast()) {
                            if (!container_type)
                                container_type = Utility::IdentifyContainerType(call->base_opt->symbol->VariableCast());
                            if (container_type && container_type->IsGetMethod(call->symbol->MethodCast()))
                                snapshot->roles->push_back(
                                        new Role(assignment->left_hand_side->symbol->VariableCast(), Role::RETRIEVE));
                        }
                    }
                }
            } else if (stmt->kind == Ast::CALL) {
                AstMethodInvocation *call = stmt->MethodInvocationCast();
                if (call->base_opt && call->base_opt->symbol->VariableCast()) {
                    if (!container_type)
                        container_type = Utility::IdentifyContainerType(call->base_opt->symbol->VariableCast());
                    if (container_type && container_type->IsPutMethod(call->symbol->MethodCast()))
                        snapshot->roles->push_back(new Role(container_type->GetPutValue(call), Role::REGISTER));
                }
            } else if (stmt->kind == Ast::RETURN) {
                AstReturnStatement *return_stmt = stmt->ReturnStatementCast();
                if (return_stmt->expression_opt) {
                    if (return_stmt->expression_opt->symbol->VariableCast()) {
                        snapshot->roles->push_back(
                                new Role(return_stmt->expression_opt->symbol->VariableCast(), Role::RETURN));
                    } else if (return_stmt->expression_opt->kind == Ast::ARRAY_ACCESS) {
                        snapshot->roles->push_back(
                                new Role(return_stmt->expression_opt->ArrayAccessCast(), Role::RETURN));
                    }
                }
                traces.push_back(snapshot);
            }
        }
    }

}

bool FlyweightAnalysis::IsFlyweightFactory() {
    AssignRoles();
    n = 0;

    for (unsigned t = 0; t < traces.size(); t++) {
        VariableSymbol *returned_var = NULL;
        AstArrayAccess *returned_ref = NULL;
        Snapshot *val_recorded = NULL;
        bool create_pending = false;

        for (unsigned i = traces[t]->index; i < summary.size(); i--) {
            Snapshot *snapshot = summary[i];
            vector<Role *> *roles = snapshot->roles;
            unsigned j = roles->size() - 1;
            for (; j < roles->size(); j--) {
                Role *role = (*roles)[j];
                if ((!returned_var && !returned_ref) && (role->tag == Role::RETURN)) {
                    if (role->vsym)
                        returned_var = role->vsym;
                    else {
                        returned_ref = role->array_access;
                        bitmap[n] = 'E';
                    }
                } else if (role->tag == Role::ALLOCATE) {
                    if ((returned_ref->base->symbol == role->array_access->base->symbol)
                        && (returned_ref->expression->symbol == returned_ref->expression->symbol)) {
                        bitmap[n] = 'N';
                        /*
                        Coutput << "returns new flyweight object in "
                            << returned_ref->base->symbol->VariableCast()->Utf8Name() << "["
                            << returned_ref->expression->symbol->VariableCast()->Utf8Name() << "]" << endl;
                        */
                    }
                } else if ((returned_var == role->vsym) && (role->tag == Role::REGISTER)) {
                    create_pending = true;
                } else if (create_pending && (returned_var == role->vsym) && (role->tag == Role::CREATE)) {
                    // the algorithm should reject if a CREATE occurs w/o create_pending
                    bitmap[n] = 'N';
                    //Coutput << "returns new flyweight object in " << returned_var->Utf8Name() << endl;
                    create_pending = false;
                    val_recorded = snapshot;
                } else if ((returned_var == role->vsym) && (role->tag == Role::RETRIEVE)) {
                    if (val_recorded && val_recorded->conditions) {
                        for (unsigned k = 0; k < val_recorded->conditions->size(); k++) {
                            if ((*val_recorded->conditions)[k]->kind == Ast::BINARY) {
                                AstBinaryExpression *expression = (*val_recorded->conditions)[k]->BinaryExpressionCast();
                                if ((expression->left_expression->symbol == role->vsym)
                                    && (expression->right_expression->kind == Ast::NULL_LITERAL)
                                    && (expression->Tag() == AstBinaryExpression::EQUAL_EQUAL)) {
                                    bitmap[++n] = 'E';
                                    //Coutput << "returns existing flyweight object in " << returned_var->Utf8Name() << endl;
                                    break;
                                }
                            }
                        }
                    } else {
                        bitmap[n] = 'E';
                        //Coutput << "returns existing flyweight object in " << returned_var->Utf8Name() << endl;
                        val_recorded = snapshot;
                    }
                }
            }
        }
        n++;
    }
    return (n == 2) && (((bitmap[0] == 'E') && (bitmap[1] == 'N')) || ((bitmap[0] == 'N') && (bitmap[1] == 'E')));
}

void AstBlock::Accept(FlyweightAnalysis &visitor) { visitor.visit(this); }