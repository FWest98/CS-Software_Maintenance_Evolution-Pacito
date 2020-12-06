#include "../../headers/Pacito_Pinot.h"
#include "../Pacito.h"
#include "helpers.h"
#include "../patterns/Patterns.h"

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

    auto cls = env->FindClass("Pacito/Patterns/Pattern");
    auto array = env->NewObjectArray(result.size(), cls, NULL);

    for(auto i = 0; i < result.size(); i++) {
        env->SetObjectArrayElement(array, i, result.at(i)->ConvertToJava(env));
    }

    return array;
}