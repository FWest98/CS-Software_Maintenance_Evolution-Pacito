#include "FactoryAnalysis.h"

bool FactoryAnalysis::IsFactoryMethod() {
    //Coutput << "Analyzing " << method->Utf8Name() << endl;

    visited.AddElement(method);

    auto x = method->declaration->MethodDeclarationCast()->method_body_opt;
    if (x) {
        typedef void (AstMethodBody::*fac)(Flatten &);
        fac y = &AstMethodBody::Accept;

        auto z = *x;
        (z.*y)(flatten);
        //z.Accept(flatten);
    }

    //flatten.DumpSummary();

    for (unsigned t = 0; t < flatten.traces.size(); t++) {
        VariableSymbol *returned_var = NULL;

        for (unsigned i = flatten.traces[t]->index; i < flatten.summary.size(); i--) {
            Snapshot *snapshot = flatten.summary[i];
            for (unsigned j = snapshot->statements->size() - 1; j < snapshot->statements->size(); j--) {
                Ast *stmt = (*snapshot->statements)[j];
                if (stmt->kind == Ast::RETURN) {
                    AstReturnStatement *return_stmt = stmt->ReturnStatementCast();
                    if (return_stmt->expression_opt) {
                        AstExpression *expression = Utility::RemoveCasting(return_stmt->expression_opt);

                        if (expression->symbol->VariableCast())
                            returned_var = expression->symbol->VariableCast();
                        else if (expression->symbol->MethodCast()) {
                            if (expression->kind == Ast::CLASS_CREATION) {
                                types.AddElement(expression->symbol->MethodCast()->Type());
                                break;
                            } else if (expression->kind == Ast::CALL) {
                                // inter-procedural
                                if (!visited.IsElement(expression->symbol)
                                    && expression->symbol->MethodCast()->declaration
                                    && expression->symbol->MethodCast()->declaration->MethodDeclarationCast()
                                    &&
                                    expression->symbol->MethodCast()->declaration->MethodDeclarationCast()->method_body_opt) {
                                    FactoryAnalysis defer(expression->symbol->MethodCast(), ast_pool);
                                    if (defer.IsFactoryMethod())
                                        break;
                                }
                            }
                        }
                            // Jikes does not compile when returning empty_statement
                        else if (expression->symbol->TypeCast()) {
                            if (expression->kind == Ast::NULL_LITERAL)
                                return false;
                        }
                    }
                } else if (stmt->kind == Ast::ASSIGNMENT) {
                    AstAssignmentExpression *assignment = stmt->AssignmentExpressionCast();
                    if (assignment->left_hand_side->symbol->VariableCast()
                        && (assignment->left_hand_side->symbol == returned_var)) {
                        AstExpression *expression = Utility::RemoveCasting(assignment->expression);
                        if (expression->kind == Ast::CLASS_CREATION) {
                            //types.AddElement(expression->symbol->MethodCast()->Type());
                            types.AddElement(expression->ClassCreationExpressionCast()->class_type->symbol->TypeCast());
                            break;
                        } else if (expression->kind == Ast::NULL_LITERAL)
                            return false;
                        else if (expression->kind == Ast::CALL) {
                            // inter-procedural
                            if (!visited.IsElement(expression->symbol)
                                && expression->symbol->MethodCast()->declaration
                                && expression->symbol->MethodCast()->declaration->MethodDeclarationCast()
                                &&
                                expression->symbol->MethodCast()->declaration->MethodDeclarationCast()->method_body_opt) {
                                FactoryAnalysis defer(expression->symbol->MethodCast(), ast_pool);
                                if (defer.IsFactoryMethod())
                                    break;
                            }
                        } else if (expression->symbol->VariableCast()) {
                            // aliasing
                            returned_var = expression->symbol->VariableCast();
                        }
                    }
                } else if (stmt->kind == Ast::VARIABLE_DECLARATOR) {
                    AstVariableDeclarator *var_declarator = stmt->VariableDeclaratorCast();
                    if (var_declarator->symbol == returned_var) {
                        if (var_declarator->variable_initializer_opt
                            && var_declarator->variable_initializer_opt->ExpressionCast()) {
                            AstExpression *expression = Utility::RemoveCasting(
                                    var_declarator->variable_initializer_opt->ExpressionCast());
                            if (expression->kind == Ast::CLASS_CREATION) {
                                //types.AddElement(expression->symbol->MethodCast()->Type());
                                types.AddElement(
                                        expression->ClassCreationExpressionCast()->class_type->symbol->TypeCast());
                                break;
                            } else if (expression->kind == Ast::NULL_LITERAL)
                                return false;
                            else if (expression->kind == Ast::CALL) {
                                // inter-procedural
                                if (!visited.IsElement(expression->symbol)
                                    && expression->symbol->MethodCast()->declaration
                                    && expression->symbol->MethodCast()->declaration->MethodDeclarationCast()
                                    &&
                                    expression->symbol->MethodCast()->declaration->MethodDeclarationCast()->method_body_opt) {
                                    FactoryAnalysis defer(expression->symbol->MethodCast(), ast_pool);
                                    if (defer.IsFactoryMethod())
                                        break;
                                }
                            } else if (expression->symbol->VariableCast()) {
                                // aliasing
                                returned_var = expression->symbol->VariableCast();
                            }
                        }
                            // variable unitialized and never assigned with a value is a Jikes error
                        else if (!var_declarator->variable_initializer_opt)
                            return false;
                    }
                }
            }
        }
    }
    return (types.Size() && !types.IsElement(method->Type()));

}

bool FactoryAnalysis::IsCreationMethod() {
    //Coutput << "Analyzing " << method->Utf8Name() << endl;

    visited.AddElement(method);
    method->declaration->MethodDeclarationCast()->method_body_opt->Accept(flatten);
    //flatten.DumpSummary();

    for (unsigned t = 0; t < flatten.traces.size(); t++) {
        VariableSymbol *returned_var = NULL;

        for (unsigned i = flatten.traces[t]->index; i < flatten.summary.size(); i--) {
            Snapshot *snapshot = flatten.summary[i];
            for (unsigned j = snapshot->statements->size() - 1; j < snapshot->statements->size(); j--) {
                Ast *stmt = (*snapshot->statements)[j];
                if (stmt->kind == Ast::RETURN) {
                    AstReturnStatement *return_stmt = stmt->ReturnStatementCast();
                    if (return_stmt->expression_opt) {
                        AstExpression *expression = Utility::RemoveCasting(return_stmt->expression_opt);

                        if (expression->symbol->VariableCast())
                            returned_var = expression->symbol->VariableCast();
                        else if (expression->symbol->MethodCast()) {
                            if (expression->kind == Ast::CLASS_CREATION) {
                                types.AddElement(expression->symbol->MethodCast()->Type());
                                break;
                            } else if (expression->kind == Ast::CALL) {
                                // inter-procedural
                                if (!visited.IsElement(expression->symbol)
                                    && expression->symbol->MethodCast()->declaration
                                    && expression->symbol->MethodCast()->declaration->MethodDeclarationCast()
                                    &&
                                    expression->symbol->MethodCast()->declaration->MethodDeclarationCast()->method_body_opt) {
                                    FactoryAnalysis defer(expression->symbol->MethodCast(), ast_pool);
                                    if (defer.IsCreationMethod())
                                        break;
                                }
                            }
                        }
                            // Jikes does not compile when returning empty_statement
                        else if (expression->symbol->TypeCast()) {
                            if (expression->kind == Ast::NULL_LITERAL)
                                return false;
                        }
                    }
                } else if (stmt->kind == Ast::ASSIGNMENT) {
                    AstAssignmentExpression *assignment = stmt->AssignmentExpressionCast();
                    if (assignment->left_hand_side->symbol->VariableCast()
                        && (assignment->left_hand_side->symbol == returned_var)) {
                        AstExpression *expression = Utility::RemoveCasting(assignment->expression);
                        if (expression->kind == Ast::CLASS_CREATION) {
                            types.AddElement(expression->symbol->MethodCast()->Type());
                            break;
                        } else if (expression->kind == Ast::NULL_LITERAL)
                            return false;
                        else if (expression->kind == Ast::CALL) {
                            // inter-procedural
                            if (!visited.IsElement(expression->symbol)
                                && expression->symbol->MethodCast()->declaration
                                && expression->symbol->MethodCast()->declaration->MethodDeclarationCast()
                                &&
                                expression->symbol->MethodCast()->declaration->MethodDeclarationCast()->method_body_opt) {
                                FactoryAnalysis defer(expression->symbol->MethodCast(), ast_pool);
                                if (defer.IsCreationMethod())
                                    break;
                            }
                        } else if (expression->symbol->VariableCast()) {
                            // aliasing
                            returned_var = expression->symbol->VariableCast();
                        }
                    }
                } else if (stmt->kind == Ast::VARIABLE_DECLARATOR) {
                    AstVariableDeclarator *var_declarator = stmt->VariableDeclaratorCast();
                    if (var_declarator->symbol == returned_var) {
                        if (var_declarator->variable_initializer_opt
                            && var_declarator->variable_initializer_opt->ExpressionCast()) {
                            AstExpression *expression = var_declarator->variable_initializer_opt->ExpressionCast();
                            if (expression->kind == Ast::CLASS_CREATION) {
                                types.AddElement(expression->symbol->MethodCast()->Type());
                                break;
                            } else if (expression->kind == Ast::NULL_LITERAL)
                                return false;
                            else if (expression->kind == Ast::CALL) {
                                // inter-procedural
                                if (!visited.IsElement(expression->symbol)
                                    && expression->symbol->MethodCast()->declaration
                                    && expression->symbol->MethodCast()->declaration->MethodDeclarationCast()
                                    &&
                                    expression->symbol->MethodCast()->declaration->MethodDeclarationCast()->method_body_opt) {
                                    FactoryAnalysis defer(expression->symbol->MethodCast(), ast_pool);
                                    if (defer.IsCreationMethod())
                                        break;
                                }
                            } else if (expression->symbol->VariableCast()) {
                                // aliasing
                                returned_var = expression->symbol->VariableCast();
                            }
                        }
                            // variable unitialized and never assigned with a value is a Jikes error
                        else if (!var_declarator->variable_initializer_opt)
                            return false;
                    }
                }
            }
        }
    }
    return ((types.Size() == 1) && types.IsElement(method->Type()));

}