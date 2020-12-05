#include "Pacito.h"

jfieldID getPointerField(JNIEnv *env, jobject obj) {
    // Get JniObject java class
    auto javaJniObject = env->FindClass("Pacito/JniObject");
    if(javaJniObject == nullptr) return nullptr; // Then the object is not a jni object so we can't manage it
    if(!env->IsInstanceOf(obj, javaJniObject)) return nullptr;

    // Get objPtr field id
    auto result = env->GetFieldID(javaJniObject, "objPtr", "J");
    env->DeleteLocalRef(javaJniObject);

    return result;
}

template <class T> void setInstance(JNIEnv *env, jobject obj, T *instance) {
    env->SetLongField(obj, getPointerField(env, obj), (jlong) instance);
}

template <class T> T* getInstance(JNIEnv *env, jobject obj) {
    return (T*) env->GetLongField(obj, getPointerField(env, obj));
}

Pacito::Pacito(char *classPath) {
    option = new Option();
    option->setClasspath(classPath);
}

Pacito::~Pacito() {
    delete option;
    delete control;
}

int Pacito::run(char **files) {
    control = new Control(files, *option);
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

JNIEXPORT void JNICALL Java_Pacito_Pinot_run(JNIEnv *env, jobject thisObj, jobjectArray files) {
    int fileCount = env->GetArrayLength(files);
    const char* fileNames[fileCount + 1]; // NULL at end

    for (int i = 0; i < fileCount; i++) {
        auto file = (jstring) (env->GetObjectArrayElement(files, i));
        const char* fileString = env->GetStringUTFChars(file, 0);
        fileNames[i] = fileString;
    }

    fileNames[fileCount] = nullptr;

    auto pacito = getInstance<Pacito>(env, thisObj);
    pacito->run(const_cast<char **>(fileNames));
}