#include "CreationAnalysis.h"

void CreationAnalysis::visit(AstClassCreationExpression *class_creation) {
    if (class_creation->class_type->symbol) {
        return_types.push_back(class_creation->class_type->symbol->TypeCast());
    }
}

void CreationAnalysis::visit(AstBlock *block) {
    // Assumption: isolated entry and exit
    int lstmt = block->NumStatements() - 1;
    // check the last statement and see what type it returns
    if (lstmt >= 0) {
        if (block->Statement(lstmt)->kind == Ast::RETURN) {
            AstReturnStatement *return_stmt = block->Statement(lstmt)->ReturnStatementCast();
            AstExpression *expression = (return_stmt->expression_opt->kind == Ast::CAST)
                                        ? return_stmt->expression_opt->CastExpressionCast()->expression
                                        : return_stmt->expression_opt;

            if (expression->kind == Ast::CLASS_CREATION) {
                expression->ClassCreationExpressionCast()->Accept(*this);
            } else if ((expression->kind == Ast::NAME) &&
                       (expression->NameCast()->symbol->Kind() == Symbol::VARIABLE)) {
                // do the backward analysis on this returned vsym
                VariableSymbol *vsym = expression->symbol->VariableCast();
                if (vsym->declarator && vsym->declarator->variable_initializer_opt) {
                    AstExpression *expr = (vsym->declarator->variable_initializer_opt->kind == Ast::CAST)
                                          ? vsym->declarator->variable_initializer_opt->CastExpressionCast()->expression
                                          : vsym->declarator->variable_initializer_opt->ExpressionCast();
                    if (expr->kind == Ast::CLASS_CREATION)
                        expr->ClassCreationExpressionCast()->Accept(*this);
                } else {
                    signed i = lstmt - 1;
                    for (; i >= 0; i--) {
                        AstExpressionStatement *expression_stmt;
                        AstAssignmentExpression *assignment_stmt;

                        // should also consider variable initialization upon declaration
                        if ((block->Statement(i)->kind == Ast::EXPRESSION_STATEMENT)
                            && ((expression_stmt = block->Statement(i)->ExpressionStatementCast())->expression->kind ==
                                Ast::ASSIGNMENT)
                            &&
                            ((assignment_stmt = expression_stmt->expression->AssignmentExpressionCast())->lhs(vsym))) {
                            AstExpression *expr = (assignment_stmt->expression->kind == Ast::CAST)
                                                  ? assignment_stmt->expression->CastExpressionCast()->expression
                                                  : assignment_stmt->expression;
                            if (expr->kind == Ast::CLASS_CREATION)
                                expr->ClassCreationExpressionCast()->Accept(*this);
                                //else if (expr-> kind == Ast::CALL)
                                //expr->MethodInvocationCast()->Accept(*this);
                            else if (expr->kind == Ast::NULL_LITERAL)
                                return;
                        }
                    }
                }
            } else if (expression->kind == Ast::CALL) {
                AstMethodInvocation *invocation = expression->MethodInvocationCast();
                MethodSymbol *method = (invocation->symbol->Kind() == Symbol::METHOD)
                                       ? invocation->symbol->MethodCast()
                                       : NULL;
                if (method && !cache.IsElement(method)) {
                    AstMethodDeclaration *declaration = (method && method->declaration &&
                                                         method->declaration->kind == Ast::METHOD)
                                                        ? method->declaration->MethodDeclarationCast()
                                                        : NULL;
                    if (declaration && declaration->method_body_opt) {
                        cache.AddElement(method);
                        declaration->method_body_opt->Accept(*this);
                    }
                }
            } else if (expression->kind == Ast::ASSIGNMENT) {
                if (expression->AssignmentExpressionCast()->expression->kind == Ast::CLASS_CREATION)
                    expression->AssignmentExpressionCast()->expression->ClassCreationExpressionCast()->Accept(*this);
            }
        } else if (block->Statement(lstmt)->kind == Ast::TRY) {
            block->Statement(lstmt)->TryStatementCast()->block->Accept(*this);
        }
    }
}

void AstBlock::Accept(CreationAnalysis &visitor) { visitor.visit(this); }
void AstClassCreationExpression::Accept(CreationAnalysis &visitor) { visitor.visit(this); }