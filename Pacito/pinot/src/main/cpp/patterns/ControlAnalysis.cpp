#include "ControlAnalysis.h"

void ControlAnalysis::visit(AstBlock *block) {
    unsigned i = 0;
    for (; (i < block->NumStatements()) && !result; i++) {
        visit(block->Statement(i));
    }
}

void ControlAnalysis::visit(AstSynchronizedStatement *synch_statement) {
    visit(synch_statement->block);
    if (!containing_stmt && result)
        containing_stmt = synch_statement;
    if (result)
        rt_stack.push_back(synch_statement);
}

void ControlAnalysis::visit(AstIfStatement *if_statement) {
    flag = false;
    cond = if_statement->expression;
    visit(if_statement->true_statement);
    if (!result && if_statement->false_statement_opt) {
        flag = true;
        visit(if_statement->false_statement_opt);
    }
    if (!result) {
        flag = false;
        cond = 0;
        containing_stmt = 0;
    }
    if (!containing_stmt && result)
        containing_stmt = if_statement;
    if (result)
        rt_stack.push_back(if_statement);
}

void ControlAnalysis::visit(AstConditionalExpression *cond_expression) {
    visit(cond_expression->true_expression);
    if (!result)
        visit(cond_expression->false_expression);

    if (!containing_stmt && result)
        containing_stmt = cond_expression;
    if (result)
        rt_stack.push_back(cond_expression);
}

void ControlAnalysis::visit(AstWhileStatement *while_statement) {
    visit(while_statement->statement);
    if (!containing_stmt && result)
        containing_stmt = while_statement;
    if (result)
        rt_stack.push_back(while_statement);
}

void ControlAnalysis::visit(AstForStatement *for_statement) {
    visit(for_statement->statement);
    if (!containing_stmt && result)
        containing_stmt = for_statement;
    if (result)
        rt_stack.push_back(for_statement);
}

void ControlAnalysis::visit(AstStatement *statement) {
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
        case Ast::EXPRESSION_STATEMENT:
            visit(statement->ExpressionStatementCast()->expression);
            break;
        case Ast::SYNCHRONIZED_STATEMENT:
            visit(statement->SynchronizedStatementCast());
            break;
        case Ast::BLOCK:
            visit(statement->BlockCast());
            break;
        default:
            break;
    }
}

void ControlAnalysis::visit(AstExpression *expression) {
    result = (this->expression == expression);
    if (!result) {
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
            default:
                break;
        }
    }
}

bool ControlAnalysis::IsConditional() {
    for (unsigned i = 0; i < rt_stack.size(); i++) {
        if ((rt_stack[i]->kind == Ast::IF) || (rt_stack[i]->kind == Ast::CONDITIONAL))
            return true;
    }
    return false;
}

bool ControlAnalysis::IsRepeated() {
    for (unsigned i = 0; i < rt_stack.size(); i++) {
        if ((rt_stack[i]->kind == Ast::WHILE) || (rt_stack[i]->kind == Ast::FOR))
            return true;
    }
    return false;
}

bool ControlAnalysis::IsSynchronized() {
    for (unsigned i = 0; i < rt_stack.size(); i++) {
        if (rt_stack[i]->kind == Ast::SYNCHRONIZED_STATEMENT)
            return true;
    }
    return false;
}

void AstExpression::Accept(ControlAnalysis &visitor) { visitor.visit(this); }
void AstBlock::Accept(ControlAnalysis &visitor) { visitor.visit(this); }
void AstIfStatement::Accept(ControlAnalysis &visitor) { visitor.visit(this); }
void AstSynchronizedStatement::Accept(ControlAnalysis &visitor) { visitor.visit(this); }