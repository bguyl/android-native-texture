#include <jni.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <android/log.h>
#include "IUnityGraphics.h"

#define LOC_JAVA_CALSS "com/idealsee/tools/TextureLoader"

static JavaVM *currentVm;
static jclass classTextureLoader;
static jmethodID initTextureID;
static jmethodID loadImageFileID;
static jmethodID loadRawImageID;
static jmethodID updateTextureID;
static int currentStatus = 0; // 0-not init; 1-init failed; 2-idle; 3-loading image; 4-load image success; 5-updating tex; 6-unknow;
static jint* initTexID = NULL;
static int texCount = 1;
static int imageSize = 512;
static int lastSuccessUpdateTexID = 0;

// Plugin function to handle a specific rendering event
static void UNITY_INTERFACE_API OnRenderEvent(int eventID)
{
	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "ReceiveEvent");
	if (currentVm == NULL)
	{
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "JavaVM is null !");
		return;
	}
	JNIEnv * env = NULL;
	bool needDetach = false;

	int getEnvStat = currentVm->GetEnv((void **)&env, JNI_VERSION_1_6);
	if (getEnvStat == JNI_EDETACHED) {

		if (currentVm->AttachCurrentThread(&env, NULL) != 0) {
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "Failed to attach");
			return;
		}
		needDetach = true;
	} else if (getEnvStat == JNI_OK) {
		needDetach = false;
	} else if (getEnvStat == JNI_EVERSION) {
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "GetEnv: version not supported");
		return;
	}

    if (eventID == 0)
    {
    	//Init
    	initTexID = NULL;
    	env->CallStaticVoidMethod(classTextureLoader, initTextureID, texCount, imageSize);
    	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "CallObjectMethod initTextureID finish!");
    }
    else
    {
    	//Update
    	currentStatus = 5;
    	lastSuccessUpdateTexID = 0;
    	env->CallStaticVoidMethod(classTextureLoader, updateTextureID, eventID);
    	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "CallObjectMethod updateTexture");
    }
    if (env->ExceptionCheck())
    {
     env->ExceptionDescribe();
    }
    if (needDetach) currentVm->DetachCurrentThread();
}

// Freely defined function to pass a callback to plugin-specific scripts
extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
    GetRenderEventFunc()
{
    return OnRenderEvent;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
	StartLoadImageFile(const char * path, int size)
{

	if (currentVm == NULL)
	{
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "JavaVM is null !");
		return;
	}

	JNIEnv * env = NULL;

	if (currentVm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
		return;
	}

	currentStatus = 3;

	jstring message = env->NewStringUTF(path);
	if (size == 0) size = imageSize;
	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "StartLoadImageFile!");
	env->CallStaticVoidMethod(classTextureLoader, loadImageFileID, message, size);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
	StartLoadRawImage(const char * path, int size)
{

	if (currentVm == NULL)
	{
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "JavaVM is null !");
		return;
	}

	JNIEnv * env = NULL;

	if (currentVm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
		return;
	}

	currentStatus = 3;

	jstring message = env->NewStringUTF(path);
	if (size == 0) size = imageSize;
	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "StartLoadRawImage!");
	env->CallStaticVoidMethod(classTextureLoader, loadRawImageID, message, size);
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
	GetTextureLoadStatus()
{
	return currentStatus;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
	SetInitTextureCountAndSize(int count, int size)
{
	initTexID = NULL;
	if (count != 0) texCount = count;
	if (size != 0) imageSize = size;
}

extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
	IsInitTextureFinish()
{
	return initTexID != NULL;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
	GetInitTextureID(int *outTexIDList)
{
	if (initTexID == NULL) return;
	for (int i = 0; i < texCount; i++)
	{
		outTexIDList[i] = initTexID[i];
	}
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
	GetLastSuccessUpdateTextureID()
{
	return lastSuccessUpdateTexID;
}

void _native_ImageLoadFinish(JNIEnv * env, jobject thiz, jboolean finish)
{
	if (finish == JNI_TRUE)
	{
		currentStatus = 4;
	}
	else
	{
		currentStatus = 2;
	}
}

void _native_UpdateTextureFinish(JNIEnv * env, jobject thiz, jint texID)
{
	lastSuccessUpdateTexID = texID;
	currentStatus = 2;
}

void _native_InitTexFinish(JNIEnv * env, jobject thiz, jintArray result)
{
	if (result == NULL)
	{
		currentStatus = 1;
		return;
	}
	initTexID = env->GetIntArrayElements(result, NULL);
	if (initTexID == NULL) currentStatus = 1;
	else currentStatus = 2;
}

static JNINativeMethod gMethods[] = {
        { "nativeImageLoadFinish", "(Z)V", (void *)_native_ImageLoadFinish },
		{ "nativeUpdateTextureFinish", "(I)V", (void *)_native_UpdateTextureFinish },
		{ "nativeInitTexFinish", "([I)V", (void *)_native_InitTexFinish },
};

static int register_methods(JNIEnv * env) {
    int ret = env->RegisterNatives(classTextureLoader, gMethods, sizeof(gMethods)/sizeof(gMethods[0]));

    return ret;
}

jint JNI_OnLoad(JavaVM *vm, void * reserved) {
	currentStatus = 1;

	currentVm = vm;

	JNIEnv * env = NULL;

    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    assert(env != NULL);

    __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "JNI on load! ");

    classTextureLoader = env->FindClass(LOC_JAVA_CALSS);
	if (classTextureLoader == NULL)
	{
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "Class TextureLoader is null !");
		return -1;
	}

	if (register_methods(env) < 0) {
		return -1;
	}

	initTextureID = env->GetStaticMethodID(classTextureLoader, "initTexture", "(II)V");
	if (!initTextureID)
	{
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "initTextureID is invalid !");
		return -1;
	}

	loadImageFileID = env->GetStaticMethodID(classTextureLoader, "loadImageFile", "(Ljava/lang/String;I)V");
	if (!loadImageFileID)
	{
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "loadImageFileID is invalid !");
		return -1;
	}

	loadRawImageID = env->GetStaticMethodID(classTextureLoader, "loadRawImage", "([BI)V");
	if (!loadRawImageID)
	{
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "loadRawImageID is invalid !");
		return -1;
	}

	updateTextureID = env->GetStaticMethodID(classTextureLoader, "updateTexture", "(I)V");
	if (!updateTextureID)
	{
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "updateTextureID is invalid !");
		return -1;
	}

	currentStatus = 2;

    return JNI_VERSION_1_6;
}

