/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include "JniReferences.h"

#include <core/CHIPCallback.h>
#include <jni.h>
#include <lib/support/Span.h>
#include <sstream>

/// Exposes the underlying UTF string from a jni string
class JniUtfString
{
public:
    JniUtfString(JNIEnv * env, jstring string) : mEnv(env), mString(string) { mChars = env->GetStringUTFChars(string, 0); }
    ~JniUtfString() { mEnv->ReleaseStringUTFChars(mString, mChars); }

    const char * c_str() const { return mChars; }

private:
    JNIEnv * mEnv;
    jstring mString;
    const char * mChars;
};

/// Exposes the underlying binary data from a jni byte array
class JniByteArray
{
public:
    JniByteArray(JNIEnv * env, jbyteArray array) :
        mEnv(env), mArray(array), mData(env->GetByteArrayElements(array, nullptr)), mDataLength(env->GetArrayLength(array))
    {}
    ~JniByteArray() { mEnv->ReleaseByteArrayElements(mArray, mData, 0); }

    const jbyte * data() const { return mData; }
    chip::ByteSpan byteSpan() const { return chip::ByteSpan(reinterpret_cast<const uint8_t *>(data()), size()); }
    jsize size() const { return mDataLength; }

private:
    JNIEnv * mEnv;
    jbyteArray mArray;
    jbyte * mData;
    jsize mDataLength;
};

/// wrap a c-string as a jni string
class UtfString
{
public:
    UtfString(JNIEnv * env, const char * data) : mEnv(env) { mData = data != nullptr ? mEnv->NewStringUTF(data) : nullptr; }
    UtfString(JNIEnv * env, chip::ByteSpan data) : mEnv(env)
    {
        std::ostringstream os;
        for (size_t i = 0; i < data.size(); i++)
        {
            os << data.data()[i];
        }
        mData = env->NewStringUTF(os.str().c_str());
    }
    ~UtfString() { mEnv->DeleteLocalRef(mData); }

    jstring jniValue() { return mData; }

private:
    JNIEnv * mEnv;
    jstring mData;
};

/// Manages an pre-existing global reference to a jclass.
class JniClass
{
public:
    explicit JniClass(jclass mClassRef) : mClassRef(mClassRef) {}
    ~JniClass() { chip::Controller::JniReferences::GetInstance().GetEnvForCurrentThread()->DeleteGlobalRef(mClassRef); }

    jclass classRef() { return mClassRef; }

private:
    jclass mClassRef;
};

/**
 *  Wraps a CHIP callback with a Java callback.
 *
 *  TODO: use this in CHIPClusters-JNI.cpp
 */
template <class T>
class JniCallback : public chip::Callback::Callback<T>
{
public:
    JniCallback(T callback, jobject javaCallback) : chip::Callback::Callback<T>(callback, this)
    {
        JNIEnv * env = chip::Controller::JniReferences::GetInstance().GetEnvForCurrentThread();
        VerifyOrReturn(env != nullptr, ChipLogError(Controller, "Could not get JNIEnv for current thread"));
        javaCallbackRef = env->NewGlobalRef(javaCallback);
        if (javaCallbackRef == nullptr)
        {
            ChipLogError(Controller, "Could not create global reference for Java callback");
        }
    }

    ~JniCallback()
    {
        JNIEnv * env = chip::Controller::JniReferences::GetInstance().GetEnvForCurrentThread();
        VerifyOrReturn(env != nullptr, ChipLogError(Controller, "Could not get JNIEnv for current thread"));
        env->DeleteGlobalRef(javaCallbackRef);
    };

    jobject javaCallback() { return javaCallbackRef; }

private:
    jobject javaCallbackRef;
};
