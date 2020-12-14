#include "Helpers.h"

const char *Role::TagName()
{
    switch(tag)
    {
        case CREATE:
            return "CREATE";
        case REGISTER:
            return "REGISTER";
        case RETRIEVE:
            return "RETRIEVE";
        case ALLOCATE:
            return "ALLOCATE";
        case RETURN:
            return "RETURN";
        case NIL:
            return "NULL";
        default:
            return "N/A";
    }
}

void Flatten::BuildSummary()
{
    visit(method->declaration->MethodDeclarationCast()->method_body_opt);

    for(unsigned i = 0; i < summary.size(); i++)
    {
        set<signed>::iterator p;
        for (p = (summary[i]->next).begin(); p != (summary[i]->next).end(); p++)
            (summary[*p]->previous).insert(i);
    }
}
bool Flatten::Compare(AstExpression *b1, AstExpression *b2)
{
    if (b1->symbol && (b1->symbol == b2->symbol))
        return true;
    else if (b1->kind == b2->kind)
    {
        if (b1->kind == Ast::BINARY)
        {
            return (Compare(b1->BinaryExpressionCast()->left_expression, b2->BinaryExpressionCast()->left_expression)
                    && Compare(b1->BinaryExpressionCast()->right_expression, b2->BinaryExpressionCast()->right_expression));
        }
        else if (b1->kind == Ast::PRE_UNARY)
        {
            return Compare(b1->PreUnaryExpressionCast()->expression, b2->PreUnaryExpressionCast()->expression);
        }
    }
    return false;
}
void Flatten::FlattenBoolean(vector<AstExpression*>& list, AstExpression *expression)
{
    if (expression)
    {
        if (!expression->conjoint)
            list.push_back(expression);
        else
        {
            list.push_back(expression->BinaryExpressionCast()->right_expression);
            FlattenBoolean(list, expression->BinaryExpressionCast()->left_expression);
        }
    }
}
Flatten::TransitionTag Flatten::TransitionFlow(AstExpression *b1, AstExpression *b2)
{
    if (!b1 && !b2)
        return UNCONDITIONAL;
    else if (!b1 && b2)
        return UNCONDITIONAL;
    else if (b1 && !b2)
        return CONDITIONAL;
    else if (Compare(b1, b2))
        return UNCONDITIONAL;
    else
    {
        // pre and post are both not null
        vector<AstExpression*> v1, v2;
        FlattenBoolean(v1, b1);
        FlattenBoolean(v2, b2);

        for (unsigned i = 0; i < v1.size(); i++)
        {
            bool neg1 = ((v1[i]->kind == Ast::PRE_UNARY) && (v1[i]->PreUnaryExpressionCast()->Tag() == AstPreUnaryExpression::NOT));
            AstExpression *expr1 = (neg1) ? v1[i]->PreUnaryExpressionCast()->expression : v1[i];

            for (unsigned j = 0; j < v2.size(); j++)
            {
                bool neg2 = ((v2[j]->kind == Ast::PRE_UNARY) && (v2[j]->PreUnaryExpressionCast()->Tag() == AstPreUnaryExpression::NOT));
                AstExpression *expr2 = (neg2) ? v2[j]->PreUnaryExpressionCast()->expression : v2[j];
                if (Compare(expr1, expr2))
                {
                    if ((neg1 && !neg2) || (!neg1 && neg2))
                        return NOTRANSITION;
                }
            }
        }
        return CONDITIONAL;
    }
}

void Flatten::PushCondition(AstExpression *expression)
{
    if (!condition)
        condition = expression;
    else
    {
        AstExpression *temp = condition;
        condition = ast_pool->NewBinaryExpression(AstBinaryExpression::AND_AND);
        condition->conjoint = true;
        condition->BinaryExpressionCast()->left_expression = temp;
        condition->BinaryExpressionCast()->right_expression = expression;
    }
}
void Flatten::PopCondition()
{
    if (condition)
    {
        if (condition->conjoint)
        {
            if (condition->kind == Ast::BINARY)
            {
                AstExpression *temp = condition->BinaryExpressionCast()->right_expression;
                if (temp->kind == Ast::PRE_UNARY)
                {
                    condition->BinaryExpressionCast()->right_expression = temp->PreUnaryExpressionCast()->expression;
                }
                else
                {
                    condition->BinaryExpressionCast()->right_expression = ast_pool->NewPreUnaryExpression(AstPreUnaryExpression::NOT);
                    condition->BinaryExpressionCast()->right_expression->conjoint = true;
                    condition->BinaryExpressionCast()->right_expression->PreUnaryExpressionCast()->expression = temp;
                }

            }
        }
        else
        {
            AstExpression *temp = condition;
            condition = ast_pool->NewPreUnaryExpression(AstPreUnaryExpression::NOT);
            condition->conjoint = true;
            condition->PreUnaryExpressionCast()->expression = temp;
        }
    }
}
void Flatten::visit(AstBlock* block)
{
    if (block->NumStatements())
    {
        unsigned lstmt = (block->NumStatements() == 1) ? 0 : (block->NumStatements() - 1);
        for (unsigned i = 0; i < lstmt; i++)
            visit(block -> Statement(i));
        visit(block->Statement(lstmt));
        UpdateSummary();
    }
}
void Flatten::visit(AstWhileStatement* while_statement)
{
    UpdateSummary();
    if (summary.size())
    {
        pred.clear();
        pred.insert(summary.size()-1);
    }

    PushCondition(while_statement->expression);
    visit(while_statement->statement);
    UpdateSummary();
    pred.insert(summary.size()-1);

    PopCondition();
}
void Flatten::visit(AstForStatement* for_statement)
{
    UpdateSummary();

    PushCondition(for_statement->end_expression_opt);
    visit(for_statement->statement);
    UpdateSummary();

    PopCondition();
}
void Flatten::visit(AstTryStatement *try_statement)
{
    visit(try_statement->block);
}
void Flatten::visit(AstSynchronizedStatement* synch_statement)
{
    visit(synch_statement->block);
}
void Flatten::visit(AstStatement *statement)
{
    switch(statement -> kind)
    {
        case Ast::IF:
            visit(statement -> IfStatementCast());
            break;
        case Ast::WHILE:
            visit(statement -> WhileStatementCast());
            break;
        case Ast::FOR:
            visit(statement -> ForStatementCast());
            break;
        case Ast::TRY:
            visit(statement -> TryStatementCast());
            break;
        case Ast::EXPRESSION_STATEMENT:
            visit(statement -> ExpressionStatementCast() -> expression);
            break;
        case Ast::SYNCHRONIZED_STATEMENT:
            visit(statement -> SynchronizedStatementCast());
            break;
        case Ast::BLOCK:
            visit(statement -> BlockCast());
            break;
        case Ast::RETURN:
            visit(statement -> ReturnStatementCast());
            break;
        case Ast::LOCAL_VARIABLE_DECLARATION:
            visit(statement -> LocalVariableStatementCast());
            break;
        default:
            statements.push_back(statement);
            break;
    }
}
void Flatten::visit(AstExpression *expression)
{
    switch(expression -> kind)
    {
        case Ast::PARENTHESIZED_EXPRESSION:
            visit(expression->ParenthesizedExpressionCast() -> expression);
            break;
        case Ast::CAST:
            visit(expression->CastExpressionCast() -> expression);
            break;
        case Ast::CONDITIONAL:
            statements.push_back(expression);
            //visit(expression->ConditionalExpressionCast());
            break;
        case Ast::ASSIGNMENT:
            visit(expression->AssignmentExpressionCast());
            break;
        case Ast::CALL:
            visit(expression->MethodInvocationCast());
            break;
        default:
            statements.push_back(expression);
            break;
    }
}

void Flatten::visit(AstMethodInvocation* call)
{
    // might want to check all participants in this method invocation
    // e.g., base_opt, 	call->symbol->MethodCast()>Type(), call->arguments->Argument(i), etc

    if (call->symbol
        && call->symbol->MethodCast()
        && (strcmp(call->symbol->MethodCast()->containing_type->fully_qualified_name->value, "java/security/AccessController") == 0)
        && (strcmp(call->symbol->MethodCast()->SignatureString(), "(Ljava/security/PrivilegedAction;)Ljava/lang/Object;") == 0))
    {
        AstClassCreationExpression *class_creation = Utility::RemoveCasting(call->arguments->Argument(0))->ClassCreationExpressionCast();
        visit(class_creation->symbol->TypeCast()->MethodSym(1)->declaration->MethodDeclarationCast()->method_body_opt);
    }
    else
        statements.push_back(call);
}
void Flatten::visit(AstIfStatement* statement)
{
    UpdateSummary();
    if (summary.size())
    {
        pred.clear();
        pred.insert(summary.size()-1);
    }
    set<signed> pred2(pred);
    AstExpression *top = condition;

    PushCondition(statement->expression);
    //visit(statement->expression);
    visit(statement->true_statement);
    UpdateSummary();

    if (statement->false_statement_opt)
    {
        pred.swap(pred2);
        condition = top;
        AstExpression *else_condition = ast_pool->NewPreUnaryExpression(AstPreUnaryExpression::NOT);
        else_condition->PreUnaryExpressionCast()->expression = statement->expression;
        PushCondition(else_condition);

        visit(statement->false_statement_opt);
        UpdateSummary();
        pred.clear();
        pred.insert(summary.size() - 2); // last block for the true branch
        pred.insert(summary.size() - 1); // last block for the false branch
        condition = top;
    }
    else
    {
        if ((*summary[summary.size() - 1]->statements)[summary[summary.size() - 1]->statements->size() - 1]->kind != Ast::RETURN)
            pred.insert(summary.size() - 1);
        // pred <<Union>> pred2
        set<signed>::iterator p;
        for (p = pred2.begin(); p != pred2.end(); p++)
            pred.insert(*p);
        condition = top;
        AstExpression *else_condition = ast_pool->NewPreUnaryExpression(AstPreUnaryExpression::NOT);
        else_condition->PreUnaryExpressionCast()->expression = statement->expression;
        PushCondition(else_condition);
    }
}
void Flatten::visit(AstAssignmentExpression *expression)
{
    statements.push_back(expression);
    // TODO also check for aliasing
}
void Flatten::visit(AstLocalVariableStatement* local_var)
{
    for (unsigned i=0; i < local_var->NumVariableDeclarators(); i++)
        visit(local_var->VariableDeclarator(i));
}
void Flatten::visit(AstVariableDeclarator* var_declarator)
{
    statements.push_back(var_declarator);
}
void Flatten::visit(AstReturnStatement* statement)
{
    statements.push_back(statement);
    capture_trace = true;
}
void Flatten::UpdateSummary()
{
    if (statements.size())
    {
        Snapshot *snapshot = new Snapshot();
        snapshot->statements = new vector<Ast*>(statements);
        statements.clear();
        if (condition)
        {
            snapshot->condition = condition->Clone(ast_pool)->ExpressionCast();
        }
        snapshot->index = summary.size();
        set<signed>::iterator p;
        for(p = pred.begin(); p != pred.end(); p++)
        {
            if (*p >= 0)
                (summary[*p]->next).insert(snapshot->index);
            else
                (snapshot->previous).insert(-1);
        }
        summary.push_back(snapshot);
        if (capture_trace)
        {
            traces.push_back(snapshot);
            capture_trace = false;
        }
    }
}
void Flatten::DumpSummary()
{
    Coutput << method->Utf8Name() << endl;
    for (unsigned i = 0; i < summary.size(); i++)
    {
        Snapshot *snapshot = summary[i];
        Coutput << "Snapshot[" << snapshot->index << "]" << endl;
        set<signed>::iterator p;
        //incoming edges
        Coutput << "In-coming snapshots: ";
        for (p = (snapshot->previous).begin(); p != (snapshot->previous).end(); p++)
            Coutput << *p << " ";
        Coutput << endl;
        // outgoiong edges
        Coutput << "Out-going snapshots: ";
        for (p = (snapshot->next).begin(); p != (snapshot->next).end(); p++)
            Coutput << *p << " ";
        Coutput << endl;
        Coutput << "STATEMENTS:" << endl;
        unsigned j;
        for (j = 0; j < snapshot->statements->size(); j++)
        {
            Coutput << "---Statement[" << j << "]---" << endl;
            (*snapshot->statements)[j]->Print();
        }
        Coutput << "CONDITIONS:" << endl;

        if (snapshot->condition)
            snapshot->condition->Print();
        Coutput << endl;
    }
}

void AstBlock::Accept(Flatten &visitor) { visitor.visit(this); }