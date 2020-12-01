#include <jni.h>
#include "../headers/Pacito_App.h"
#include <iostream>
using namespace std;

JNIEXPORT void JNICALL Java_Pacito_App_test(JNIEnv *env, jobject thisObj) {
    cout << "Test" << endl;
    return;
}