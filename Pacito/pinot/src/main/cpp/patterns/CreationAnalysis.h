#ifndef PACITO_CREATIONANALYSIS_H
#define PACITO_CREATIONANALYSIS_H
#include "../ast.h"

class CreationAnalysis
{
public:
    CreationAnalysis(){}
    void visit(AstBlock*);
    void visit(AstClassCreationExpression*);
    ~CreationAnalysis(){}

    vector<TypeSymbol*> return_types;
private:
    SymbolSet cache; // cache visited method symbols.
};


#endif //PACITO_CREATIONANALYSIS_H
