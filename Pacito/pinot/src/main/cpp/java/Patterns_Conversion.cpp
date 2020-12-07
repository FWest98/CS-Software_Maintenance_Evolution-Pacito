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
    setString(env, obj, "Delegator", delegator->Utf8Name());
    setString(env, obj, "DelegatorFile", delegatorFile->FileName());
    setString(env, obj, "Delegated", delegated->Utf8Name());
    setString(env, obj, "DelegatedFile", delegatedFile->Utf8Name());

    return obj;
}

jobject CoR::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/CoR");
    setString(env, obj, "Handler", handler->Utf8Name());
    setString(env, obj, "File", file->FileName());
    setString(env, obj, "HandlerMethod", handlerMethod->Utf8Name());
    setString(env, obj, "PropagatorName", propagator->Utf8Name());
    setString(env, obj, "PropagatorType", propagator->Type()->Utf8Name());

    return obj;
}

jobject Decorator::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/Decorator");
    setString(env, obj, "Decorator", decorator->Utf8Name());
    setString(env, obj, "File", file->FileName());
    setString(env, obj, "DecorateMethod", decorateMethod->Utf8Name());
    setString(env, obj, "DecorateeName", decoratee->Utf8Name());
    setString(env, obj, "DecorateeType", decoratee->Type()->Utf8Name());

    return obj;
}

jobject Factory::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/Factory");
    setString(env, obj, "FactoryMethodClass", factoryMethodClass->Utf8Name());
    setString(env, obj, "FactoryMethodImplementation", factoryMethodImpl->Utf8Name());
    setString(env, obj, "FactoryMethod", factoryMethod->Utf8Name());
    setString(env, obj, "FactoryMethodResultBase", factoryMethodResultBase->Utf8Name());
    setString(env, obj, "File", file->FileName());
    setStringArray(env, obj, "FactoryMethodResults", symbolVectorToStringVector(factoryMethodResults));

    return obj;
}

jobject Flyweight::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/Flyweight");
    setString(env, obj, "ObjectType", objectType ? objectType->Utf8Name() : "");
    setString(env, obj, "Factory", factory ? factory->Utf8Name() : "");
    setString(env, obj, "FactoryMethod", factoryMethod ? factoryMethod->Utf8Name() : "");
    setString(env, obj, "Pool", pool ? pool->Utf8Name() : "");
    setString(env, obj, "Object", object ? object->Utf8Name() : "");
    setString(env, obj, "File", file->FileName());
    setBool(env, obj, "IsImmutable", isImmutable);

    return obj;
}

jobject Mediator::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/Mediator");
    setString(env, obj, "Mediator", mediator->Utf8Name());
    setStringArray(env, obj, "Colleagues", symbolVectorToStringVector(colleagues));
    setString(env, obj, "File", file->FileName());

    return obj;
}

jobject Observer::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/Observer");
    setString(env, obj, "Iterator", iterator->Utf8Name());
    setString(env, obj, "ListenerType", listenerType->Utf8Name());
    setString(env, obj, "Notify", notify->Utf8Name());
    setString(env, obj, "Update", update->Utf8Name());
    setStringArray(env, obj, "Subjects", symbolVectorToStringVector(subjects));
    setString(env, obj, "File", file->FileName());

    return obj;
}

jobject Proxy::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/Proxy");
    setString(env, obj, "Proxy", proxy->Utf8Name());
    setString(env, obj, "Interface", interface->Utf8Name());
    setString(env, obj, "File", file->FileName());
    setStringArray(env, obj, "Reals", symbolVectorToStringVector(reals));

    return obj;
}

jobject StatePattern::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/State");
    setString(env, obj, "Context", context->Utf8Name());
    setString(env, obj, "State", state->Utf8Name());
    setStringArray(env, obj, "StateImplementations", symbolVectorToStringVector(stateImplementations));

    setString(env, obj, "Delegator", delegator->Utf8Name());
    setString(env, obj, "StateChanger", stateChanger->Utf8Name());
    setStringArray(env, obj, "ChangeInvokers", symbolVectorToStringVector(changeInvokers));

    setString(env, obj, "ContextFile", contextFile->FileName());
    setString(env, obj, "StateFile", stateFile->FileName());

    return obj;
}

jobject Strategy::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/Strategy");
    setString(env, obj, "Context", context->Utf8Name());
    setString(env, obj, "Strategy", strategy->Utf8Name());
    setStringArray(env, obj, "StrategyImplementations", symbolVectorToStringVector(strategyImplementations));

    setString(env, obj, "Delegator", delegator->Utf8Name());
    setString(env, obj, "ContextFile", contextFile->FileName());
    setString(env, obj, "StrategyFile", strategyFile->FileName());

    return obj;
}

jobject Template::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/Template");
    setString(env, obj, "Class", templateClass->Utf8Name());
    setString(env, obj, "Method", templateMethod->Utf8Name());
    setString(env, obj, "Source", templateSource->Utf8Name());
    setString(env, obj, "File", file->FileName());

    return obj;
}

jobject Visitor::ConvertToJava(JNIEnv *env) {
    auto obj = makeObject(env, "Pacito/Patterns/Visitor");
    setString(env, obj, "Visitor", visitor->Utf8Name());
    setString(env, obj, "Visitee", visitee->Utf8Name());
    setString(env, obj, "Accept", accept->Utf8Name());
    setString(env, obj, "Visit", visit->Utf8Name());
    setBool(env, obj, "IsThisExposed", isThisExposed);
    setString(env, obj, "Exposed", exposed ? exposed->Utf8Name() : "");
    setString(env, obj, "File", file->FileName());
    setString(env, obj, "AbstractVisitee", abstractVisitee ? abstractVisitee->Utf8Name() : "");
    setStringArray(env, obj, "VisiteeImplementations", symbolVectorToStringVector(visiteeImplementations));

    return obj;
}