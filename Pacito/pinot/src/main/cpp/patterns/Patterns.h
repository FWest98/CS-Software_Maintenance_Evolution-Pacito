#ifndef PACITO_PATTERNS_H
#define PACITO_PATTERNS_H

#include <memory>
#include <ast.h>
#include <jni.h>

class Pattern {
public:
    typedef std::shared_ptr<Pattern> Ptr;

    Pattern() = default;

    TypeSymbol *subject;
    FileSymbol *file;

    static vector<Pattern::Ptr> FindChainOfResponsibility(Control *);

    virtual void Print() = 0;
    virtual jobject ConvertToJava(JNIEnv *) = 0;
    virtual ~Pattern() = default;
};

class CoR : public Pattern {
public:
    MethodSymbol *handler;
    VariableSymbol *propagator;

    jobject ConvertToJava(JNIEnv *) override;
    void Print() {
        Coutput << "Chain of Responsibility Pattern" << endl;
        Coutput << subject->Utf8Name() << " is a Chain of Responsibility Handler class" << endl;
        Coutput << handler->Utf8Name() << " is a handle operation" << endl;
        Coutput << propagator->Utf8Name() << " of type " << propagator->Type()->Utf8Name() << " propagates the request"
                << endl;
        Coutput << "File location: " << file->FileName() << endl << endl;
    }
};

class Decorator : public Pattern {
public:
    MethodSymbol *decorateOp;
    VariableSymbol *decoratee;

    jobject ConvertToJava(JNIEnv *) override;
    void Print() {
        Coutput << "Decorator Pattern" << endl;
        Coutput << subject->Utf8Name() << " is a Decorator class" << endl;
        Coutput << decorateOp->Utf8Name() << " is a decorate operation" << endl;
        Coutput << decoratee->Utf8Name() << " of type " << decoratee->Type()->Utf8Name() << " is the Decoratee class"
                << endl;
        Coutput << "File location: " << file->FileName() << endl << endl;
    }
};

#endif //PACITO_PATTERNS_H
