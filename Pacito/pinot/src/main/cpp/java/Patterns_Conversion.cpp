#include "../patterns/Patterns.h"
#include "helpers.h"
#include <vector>

// Define constraint on symbols
template<typename T>
concept HasUtf8 =
    requires(T t) {
        t.Utf8Name();
        requires same_as<const char *, decltype(t.Utf8Name())>;
    };

template<HasUtf8 T> std::vector<const char *> symbolVectorToStringVector(std::vector<T*> in) {
    std::vector<const char *> output;
    std::transform(in.begin(), in.end(), back_inserter(output), [](auto symbol) { return symbol->Utf8Name(); });
    return output;
}

jobject Bridge::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/Bridge");
    setString(env, obj, "Delegator", this->delegator->Utf8Name());
    setString(env, obj, "DelegatorFile", this->delegatorFile->FileName());
    setString(env, obj, "Delegated", this->delegated->Utf8Name());
    setString(env, obj, "DelegatedFile", this->delegatedFile->Utf8Name());

    return obj;
}

jobject CoR::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/CoR");
    setString(env, obj, "Handler", this->handler->Utf8Name());
    setString(env, obj, "File", this->file->FileName());
    setString(env, obj, "HandlerMethod", this->handlerMethod->Utf8Name());
    setString(env, obj, "PropagatorName", this->propagator->Utf8Name());
    setString(env, obj, "PropagatorType", this->propagator->Type()->Utf8Name());

    return obj;
}

jobject Decorator::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/Decorator");
    setString(env, obj, "Decorator", this->decorator->Utf8Name());
    setString(env, obj, "File", this->file->FileName());
    setString(env, obj, "DecorateMethod", this->decorateMethod->Utf8Name());
    setString(env, obj, "DecorateeName", this->decoratee->Utf8Name());
    setString(env, obj, "DecorateeType", this->decoratee->Type()->Utf8Name());

    return obj;
}

jobject StatePattern::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/State");
    setString(env, obj, "Context", this->context->Utf8Name());
    setString(env, obj, "State", this->state->Utf8Name());
    setStringArray(env, obj, "StateImplementations", symbolVectorToStringVector(stateImplementations));

    setString(env, obj, "Delegator", this->delegator->Utf8Name());
    setString(env, obj, "StateChanger", this->stateChanger->Utf8Name());
    setStringArray(env, obj, "ChangeInvokers", symbolVectorToStringVector(changeInvokers));

    setString(env, obj, "ContextFile", this->contextFile->FileName());
    setString(env, obj, "StateFile", this->stateFile->FileName());

    return obj;
}

jobject Strategy::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/Strategy");
    setString(env, obj, "Context", this->context->Utf8Name());
    setString(env, obj, "Strategy", this->strategy->Utf8Name());
    setStringArray(env, obj, "StrategyImplementations", symbolVectorToStringVector(strategyImplementations));

    setString(env, obj, "Delegator", this->delegator->Utf8Name());
    setString(env, obj, "ContextFile", this->contextFile->FileName());
    setString(env, obj, "StrategyFile", this->strategyFile->FileName());

    return obj;
}