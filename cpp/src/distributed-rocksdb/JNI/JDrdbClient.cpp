//
// Created by Hutan Baghery Moghaddam on 18.02.22.
//

#include <iostream>
#include "JDrdbClient.h"
#include "../Client/DrdbClient.h"
#include "glog/logging.h"
#include "../Common/ClientServerCommons.h"
#include <filesystem>

std::string convert_jstring(JNIEnv *env, jstring jKey) {
    jboolean isCopy;
    auto converted_string = env->GetStringUTFChars(jKey, &isCopy);
    auto key = std::string{converted_string};
    env->ReleaseStringUTFChars(jKey, converted_string);
    return key;
}

std::string convert_jByteArray(JNIEnv *env, jbyteArray jValue) {
    jsize num_bytes = env->GetArrayLength(jValue);
    jboolean isCopy;
    jbyte *elements = env->GetByteArrayElements(jValue, &isCopy);
    std::string value{reinterpret_cast<char *>(elements), static_cast<size_t>(num_bytes)};
    env->ReleaseByteArrayElements(jValue, elements, JNI_ABORT);
    return value;
}

jlong Java_site_ycsb_db_JDrdbClient_initDrdbClient(JNIEnv *env, jobject, jstring serverAddress, jint serverPort,
                                                   jstring logDir) {
    FLAGS_logtostderr = true;
    FLAGS_v = 5;

    int id = 1;
    std::string server_name = convert_jstring(env, serverAddress);
    std::string log_dir = convert_jstring(env, logDir);
    int server_port = (int) serverPort;
    VLOG(INFO) << "Creating Client to " << server_name << ":" << server_port << " with log_dir " << log_dir;

    // log dir may only be set before init
    FLAGS_logtostderr = false;
    FLAGS_log_dir = log_dir;
    std::filesystem::create_directories(log_dir);
    google::InitGoogleLogging("JDrdbClient");
    FLAGS_minloglevel = 0;
    FLAGS_v = 3;

    auto *pNew = new DrdbClient(id, server_name, server_port);

    VLOG(INFO) << "Created Client at address " << pNew;

    return (long) pNew;
}

void Java_site_ycsb_db_JDrdbClient_destroyDrdbClient(JNIEnv *, jobject) {

}

// Returns Bytes or empty Bytes if not found
jbyteArray Java_site_ycsb_db_JDrdbClient_get(JNIEnv *env, jobject, jlong drdb_p, jstring jKey, jint request) {
    int request_number = (int) request;
    VLOG(VERBOSE) << "GET DrdbClient " << request_number;
    auto *drdbClient_p = (DrdbClient *) drdb_p;
    VLOG(VERBOSE) << "Client ID " << drdbClient_p->id;

    VLOG(VERBOSE) << "Converting String";
    std::string key = convert_jstring(env, jKey);
    VLOG(VERBOSE) << "Converted String " << key;

    VLOG(VERBOSE) << "Requesting";
    auto result = drdbClient_p->get(key);
    VLOG(VERBOSE) << "Request done " << result.drdbStatus << " " << result.value;

    if (result.value.empty()) {
        VLOG(VERBOSE) << "Returning empty ByteArray";
        return env->NewByteArray(0);
    }

    VLOG(VERBOSE) << "Result value length " << result.value.length();
    jbyteArray reply = env->NewByteArray(result.value.length());
    env->SetByteArrayRegion(reply, 0, result.value.length(),
                            reinterpret_cast<const jbyte *>(result.value.c_str())
    );

    return reply;
}

// Returns Status
jbyte
Java_site_ycsb_db_JDrdbClient_put(JNIEnv *env, jobject, jlong drdb_p, jstring jKey, jbyteArray jValue, jint request) {
    int request_number = (int) request;
    VLOG(VERBOSE) << "PUT DrdbClient " << request_number;
    auto *drdbClient_p = (DrdbClient *) drdb_p;
    VLOG(VERBOSE) << "Client ID " << drdbClient_p->id;

    VLOG(VERBOSE) << "Converting jstring";
    std::string key = convert_jstring(env, jKey);
    VLOG(VERBOSE) << "Converted jstring " << key;

    VLOG(VERBOSE) << "Converting jbyteArray";
    std::string value = convert_jByteArray(env, jValue);
    VLOG(VERBOSE) << "Converted jbyteArray " << value;

    VLOG(VERBOSE) << "Requesting";
    auto result = drdbClient_p->put(key, (void *) value.c_str(), value.length());
    VLOG(VERBOSE) << "Request done " << result.drdbStatus;

    return result.drdbStatus;
}

// Returns Status
jbyte Java_site_ycsb_db_JDrdbClient_del(JNIEnv *env, jobject, jlong drdb_p, jstring jKey, jint request) {
    int request_number = (int) request;
    VLOG(VERBOSE) << "DEL DrdbClient " << request_number;
    auto *drdbClient_p = (DrdbClient *) drdb_p;
    VLOG(VERBOSE) << "Client ID " << drdbClient_p->id;

    VLOG(VERBOSE) << "Converting String";
    std::string key = convert_jstring(env, jKey);
    VLOG(VERBOSE) << "Converted String " << key;

    VLOG(VERBOSE) << "Requesting";
    auto result = drdbClient_p->del(key);
    VLOG(VERBOSE) << "Request done " << result.drdbStatus;

    return result.drdbStatus;
}

