#include "../../headers/Pacito_Pinot.h"
#include "../Pacito.h"
#include "helpers.h"
#include "../patterns/Patterns.h"

jobjectArray findPatternHelper(JNIEnv *env, std::vector<Pattern::Ptr> result) {
    auto cls = env->FindClass("Pacito/Patterns/Pattern");
    auto array = env->NewObjectArray(result.size(), cls, nullptr);

    for(auto i = 0; i < result.size(); i++) {
        env->SetObjectArrayElement(array, i, result.at(i)->ConvertToJava(env));
    }

    return array;
}

JNIEXPORT void JNICALL Java_Pacito_Pinot_initialize(JNIEnv *env, jobject thisObj, jstring classPath) {
    auto classPathString = env->GetStringUTFChars(classPath, 0);
    auto pacito = new Pacito(const_cast<char *>(classPathString));

    // Set pacito object to Java
    setInstance(env, thisObj, pacito);
}

JNIEXPORT void JNICALL Java_Pacito_Pinot_clean(JNIEnv *env, jobject thisObj) {
    auto pacito = getInstance<Pacito>(env, thisObj);
    delete pacito;
}

JNIEXPORT jint JNICALL Java_Pacito_Pinot_run(JNIEnv *env, jobject thisObj, jobjectArray files) {
    int fileCount = env->GetArrayLength(files);
    const char* fileNames[fileCount + 1]; // NULL at end

    for (int i = 0; i < fileCount; i++) {
        auto file = (jstring) (env->GetObjectArrayElement(files, i));
        const char* fileString = env->GetStringUTFChars(file, 0);
        fileNames[i] = fileString;
    }

    fileNames[fileCount] = nullptr;

    auto pacito = getInstance<Pacito>(env, thisObj);
    return pacito->run(const_cast<char **>(fileNames));
}

JNIEXPORT jobjectArray JNICALL Java_Pacito_Pinot_findCoR(JNIEnv *env, jobject thisObj) {
    auto pacito = getInstance<Pacito>(env, thisObj);
    auto result = Pattern::FindChainOfResponsibility(pacito->getControl());

    return findPatternHelper(env, result);
}

JNIEXPORT jobjectArray JNICALL Java_Pacito_Pinot_findBridge(JNIEnv *env, jobject thisObj) {
    auto pacito = getInstance<Pacito>(env, thisObj);
    auto result = Pattern::FindBridge(pacito->getControl());

    return findPatternHelper(env, result);
}

JNIEXPORT jobjectArray JNICALL Java_Pacito_Pinot_findStrategy(JNIEnv *env, jobject thisObj) {
    auto pacito = getInstance<Pacito>(env, thisObj);
    auto result = Pattern::FindStrategy(pacito->getControl());

    return findPatternHelper(env, result);
}

JNIEXPORT jobjectArray JNICALL Java_Pacito_Pinot_findFlyweight(JNIEnv *env, jobject thisObj) {
    auto pacito = getInstance<Pacito>(env, thisObj);
    auto result = Pattern::FindFlyweight(pacito->getControl());

    return findPatternHelper(env, result);
}

JNIEXPORT jobjectArray JNICALL Java_Pacito_Pinot_findTemplateMethod(JNIEnv *env, jobject thisObj) {
    auto pacito = getInstance<Pacito>(env, thisObj);
    auto result = Pattern::FindTemplateMethod(pacito->getControl());

    return findPatternHelper(env, result);
}

JNIEXPORT jobjectArray JNICALL Java_Pacito_Pinot_findFactory(JNIEnv *env, jobject thisObj) {
    auto pacito = getInstance<Pacito>(env, thisObj);
    auto result = Pattern::FindFactory(pacito->getControl());

    return findPatternHelper(env, result);
}

JNIEXPORT jobjectArray JNICALL Java_Pacito_Pinot_findVisitor(JNIEnv *env, jobject thisObj) {
    auto pacito = getInstance<Pacito>(env, thisObj);
    auto result = Pattern::FindVisitor(pacito->getControl());

    return findPatternHelper(env, result);
}

JNIEXPORT jobjectArray JNICALL Java_Pacito_Pinot_findObserver(JNIEnv *env, jobject thisObj) {
    auto pacito = getInstance<Pacito>(env, thisObj);
    auto result = Pattern::FindObserver(pacito->getControl());

    return findPatternHelper(env, result);
}

JNIEXPORT jobjectArray JNICALL Java_Pacito_Pinot_findMediator(JNIEnv *env, jobject thisObj) {
    auto pacito = getInstance<Pacito>(env, thisObj);
    auto result = Pattern::FindMediator(pacito->getControl());

    return findPatternHelper(env, result);
}

JNIEXPORT jobjectArray JNICALL Java_Pacito_Pinot_findProxy(JNIEnv *env, jobject thisObj) {
    auto pacito = getInstance<Pacito>(env, thisObj);
    auto result = Pattern::FindProxy(pacito->getControl());

    return findPatternHelper(env, result);
}