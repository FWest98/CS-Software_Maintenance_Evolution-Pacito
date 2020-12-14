#ifndef PACITO_FACTORYANALYSIS_H
#define PACITO_FACTORYANALYSIS_H

#include "set.h"
#include "symbol.h"
#include "control.h"
#include "Helpers.h"

class FactoryAnalysis
{
private:
    MethodSymbol *method;
    Flatten flatten;
    thread_local inline static SymbolSet visited; // visited methods, avoid analysis on recursion methods.
    StoragePool *ast_pool;
public:
    thread_local inline static SymbolSet types;
    FactoryAnalysis(MethodSymbol *msym, StoragePool *pool):method(msym), ast_pool(pool){ flatten.init(msym, pool); }
    bool IsFactoryMethod();
    bool IsCreationMethod();
    void CleanUp() { types.SetEmpty(); visited.SetEmpty(); }
    ~FactoryAnalysis()= default;
};


#endif //PACITO_FACTORYANALYSIS_H
