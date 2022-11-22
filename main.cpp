#include <iostream>
#include <vector>
#include <string>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <thread>
#include <boost/filesystem.hpp>
#include <boost/json.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <stdlib.h>
#include "IVirtualMachine.h"

#if defined (MAC_VERSION)

#include <mach-o/dyld.h>

int osx_main(int argc, const char *argv[]);



void ParkEventLoop();

#elif defined (WIN_VERSION)

#endif

void _append_exception_trace_messages(
        JNIEnv&	   ,
        std::string& ,
        jthrowable   ,
        jmethodID	   ,
        jmethodID	   ,
        jmethodID	   ,
        jmethodID	   );

namespace json = boost::json;


// expose java hook functions

static atomic_bool should_exit;

static void         JNICALL shutdownHookCalled(JNIEnv *env, jobject this_object) {
    std::cout << "APP IS EXITING" << std::endl;
    should_exit.store(true);
}


std::mutex m;
std::condition_variable cv;
std::string data;
bool ready = false;
bool processed = false;


static bool registerNativeLinkages(JNIEnv *env, jclass startupClass) {
    bool rval = true;

    static JNINativeMethod interMs[] = {
            {(char *const) "shutdownHookCalled", (char *const) "()V", (void *) &shutdownHookCalled}
    };

    static int numMethods = sizeof(interMs) / sizeof(interMs[0]);

    if (env->RegisterNatives(startupClass, interMs, numMethods) != 0) {
        env->ExceptionDescribe();
        rval = false;
    }
    return rval;
}


static bool endsWith(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

static bool startsWith(const std::string &str, const std::string &prefix) {
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

static bool endsWith(const std::string &str, const char *suffix, unsigned suffixLen) {
    return str.size() >= suffixLen && 0 == str.compare(str.size() - suffixLen, suffixLen, suffix, suffixLen);
}

static bool endsWith(const std::string &str, const char *suffix) {
    return endsWith(str, suffix, std::string::traits_type::length(suffix));
}

static bool startsWith(const std::string &str, const char *prefix, unsigned prefixLen) {
    return str.size() >= prefixLen && 0 == str.compare(0, prefixLen, prefix, prefixLen);
}

static bool startsWith(const std::string &str, const char *prefix) {
    return startsWith(str, prefix, std::string::traits_type::length(prefix));
}


enum ERROR_CODES {
    NO_JLAUNCH_ERROR = 0,
    ERROR_LAUNCH_FILE_NAME = 1,
    ERROR_FILE_SYSTEM,
    ERROR_CONFIG_FILE,
    ERROR_JVM_PRELAUNCH,
    ERROR_JVM_LAUNCH
};

void ErrorAlert(const char *msg, const char *info);

void error_and_exit(ERROR_CODES err, std::string info, std::string detail) {

    std::cout << info << std::endl << detail << std::endl;
    exit(err);

}




/** Error reporting with jboject cleanup)
   pass any jobjects that need to be cleaned out on error
 @parameter env - the JNI Environment
 @parameter numCleanups - the number of open objects to delete
 @parameter ... - cvariable list of objects to delete local reference
 returns true if errors were encountered
 */
static bool exceptionCheck(JNIEnv * env,int numCleanups,...)
{
    // Get the exception and clear as no
    // JNI calls can be made while an exception exists.
    jthrowable exception = env->ExceptionOccurred();
    if (exception != NULL)
    {

        env->ExceptionClear();

        jclass throwable_class = env->FindClass("java/lang/Throwable");
        jmethodID mid_throwable_getCause =
                env->GetMethodID(throwable_class,
                                 "getCause",
                                 "()Ljava/lang/Throwable;");
        jmethodID mid_throwable_getStackTrace =
                env->GetMethodID(throwable_class,
                                 "getStackTrace",
                                 "()[Ljava/lang/StackTraceElement;");
        jmethodID mid_throwable_toString =
                env->GetMethodID(throwable_class,
                                 "toString",
                                 "()Ljava/lang/String;");

        jclass frame_class = env->FindClass("java/lang/StackTraceElement");
        jmethodID mid_frame_toString =
                env->GetMethodID(frame_class,
                                 "toString",
                                 "()Ljava/lang/String;");


        std::string error_msg; // Could use ostringstream instead.

        _append_exception_trace_messages(*env,
                                         error_msg,
                                         exception,
                                         mid_throwable_getCause,
                                         mid_throwable_getStackTrace,
                                         mid_throwable_toString,
                                         mid_frame_toString);
        std::cerr << error_msg << std::endl;


        va_list ap;
        va_start(ap, numCleanups);
        for(int i=0;i<numCleanups;i++)
        {
            env->DeleteLocalRef(va_arg(ap, jobject));
            if(exceptionCheck(env,0))
            {
                //DBG("NFG on Cleanup");
            }
        }
        va_end(ap);
        return true;
    }
    return false;
}

/**
   cleans up jobjects in long jni sequences
 @parameter env - the JNI Environment
 @parameter numCleanups - the number of open objects to delete
 @parameter ... - cvariable list of objects to delete local reference

 */
static void cleanUpObjects(JNIEnv * env,int numCleanups,...)
{
    va_list ap;
    va_start(ap, numCleanups);
    for(int i=0;i<numCleanups;i++)
    {
        env->DeleteLocalRef(va_arg(ap, jobject));
        if(exceptionCheck(env,0))
        {
            //DBG("NFG on Cleanup");
        }
    }
    va_end(ap);

}

static void setJavaSystemProperty(JNIEnv *env,std::string propertyName,std::string propertyValue)
{


    if (env) {


        jclass sysClass = env->FindClass("java/lang/System");
        if(!sysClass){
            exceptionCheck(env, 0);
        }

        jmethodID sysSetProp = env->GetStaticMethodID(sysClass, "setProperty", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
        if(!sysSetProp){
            exceptionCheck(env, 1,sysClass);
            return;
        }

        jstring propName = env->NewStringUTF(propertyName.data());
        jstring propVal = env->NewStringUTF(propertyValue.data());

        jobject sysObj = env->NewObject(sysClass, sysSetProp,propName,propVal);

        if(exceptionCheck(env, 4,sysClass,propName,propVal,sysObj))
        {
            return;
        }

        cleanUpObjects(env, 2, sysObj, sysClass);

    }


}


static void main_thread() {
    // first get our full executable path

    unsigned int buffer_size = 512;
    std::vector<char> buffer(buffer_size + 1);

#if defined (MAC_VERSION)
    if (_NSGetExecutablePath(&buffer[0], &buffer_size)) {
        buffer.resize(buffer_size);
        _NSGetExecutablePath(&buffer[0], &buffer_size);
    } else {

    }

#endif

// determine the parent dir and find the config json

    boost::filesystem::path launch_file = buffer.data();
    std::cout << "launch file path " << &buffer[0] << std::endl;

    boost::filesystem::path resource_folder;
    boost::filesystem::path plugin_folder;
    boost::filesystem::path libraries_folder;
    boost::filesystem::path frameworks_folder;
    boost::filesystem::path configuration_file;
    boost::filesystem::path jre_folder;
    boost::filesystem::path jre_jvmlib_file;
    boost::filesystem::path jre_jlilib_file;

#if defined(MAC_VERSION)
    // for mac we are expected to be inside a bundle .. this brings us to jlaunch.app/Contents
    // root folder


    std::string rootf(launch_file.parent_path().parent_path().string());
    // in case we launch at the command line
    if (endsWith(launch_file.parent_path().string(), ".")) {
        rootf = launch_file.parent_path().parent_path().parent_path().string();
    }
    std::cout << "root " << rootf << std::endl;

    // resources folder
    std::string rf(rootf);
    rf.append("/Resources");
    resource_folder = rf;
    std::cout << "resource folder " << rf << std::endl;

    // Frameworks folder
    std::string ff(rootf);
    ff.append("/Frameworks");
    frameworks_folder = ff;
    std::cout << "frameworks folder " << ff << std::endl;

    // plugin file folder
    std::string pf(rf);
    pf.append("/Plugins");
    plugin_folder = pf;
    std::cout << "plugin folder " << pf << std::endl;

    //configuration file
    std::string cf(rf);
    cf.append("/Config/jlaunch.json");
    configuration_file = cf;
    std::cout << "config file " << cf << std::endl;

    // java home folder
    std::string jf(rf);
    jf.append("/Java/jre");
    jre_folder = jf;
    std::cout << "jre folder " << jf << std::endl;

    // jvm lib
    std::string jvm(jf);
    jvm.append("/lib/server/libjvm.dylib");
    jre_jvmlib_file = jvm;
    std::cout << "jre jvm lib file " << jvm << std::endl;

    // jvm lib
    std::string jli(jf);
    jli.append("/lib/jli/libjli.dylib");
    jre_jlilib_file = jli;
    std::cout << "jre jli lib file " << jli << std::endl;

    // internal library path

    std::string lf(rootf);
    lf.append("/Libraries");
    libraries_folder = lf;
    std::cout << "libraries folder " << lf << std::endl;


#elif defined(WIN_VERSION)
    // for mac we are expected to be inside a bundle .. this brings us to jlaunch.app/Contents
    std::string rf(launch_file.parent_path().string());
    // resources folder
    rf.append("\\]Resources");
    resource_folder = rf;
    // plugin file folder
    std::string pf(rf);
    pf.append("\\Plugins");
    plugin_folder = pf;
    //configuration file
    std::string cf(rf);
    cf.append("\\Config\\jlaunch.json");
    configuration_file = cf;
    // java home folder
    std::string jf(rf);
    jf.append("\\Java\\jre");
    jre_folder = jf;

#endif

    // do the file and folder checks
    std::stringstream file_errors("");

    if (!exists(resource_folder)) {
        file_errors << "Resource folder does not exist" << std::endl;
    }
    if (!exists(frameworks_folder)) {
        file_errors << "Frameworks folder does not exist" << std::endl;
    }
    if (!exists(configuration_file)) {
        file_errors << "Configuration File does not exist" << std::endl;
    }
    if (!exists(jre_folder)) {
        file_errors << "JRE folder does not exist" << std::endl;
    }
    if (!exists(plugin_folder)) {
        file_errors << "Plugin folder does not exist" << std::endl;
    }
    if (!exists(jre_jvmlib_file)) {
        file_errors << "jvm library does not exist" << std::endl;
    }
    if (!exists(jre_jlilib_file)) {
        file_errors << "jli library does not exist" << std::endl;
    }
    if (!exists(libraries_folder)) {
        file_errors << "libraries folder does not exist" << std::endl;
    }


    if (!file_errors.str().empty()) {
        error_and_exit(ERROR_FILE_SYSTEM, "File System errors", file_errors.str().data());
//        ErrorAlert("File System errors", file_errors.str().data());
//        exit(ERROR_FILE_SYSTEM);

    }

    // parse config json and get the base class jar filename and the package and class name to the main function to run



    boost::property_tree::ptree config_properties;
    try {
        read_json(configuration_file.string().data(), config_properties);
    } catch (boost::wrapexcept<boost::property_tree::json_parser::json_parser_error> err) {

        error_and_exit(ERROR_CONFIG_FILE, "Configuration File errors", "could not parse the json config file");
//        ErrorAlert("Configuration File errors", "could not parse the json config file");
//        exit(ERROR_CONFIG_FILE);

    }
#ifdef MAC_VERSION
    if (setenv("EMBEDDED_JVM_LIBRARY_PATH", jre_jvmlib_file.string().data(), 1) != 0 ||
        setenv("EMBEDDED_JLI_LIBRARY_PATH", jre_jlilib_file.string().data(), 1) != 0) {
#else
        if(false) {
#endif
        error_and_exit(ERROR_JVM_PRELAUNCH, "LAUNCH ERROR", "could not set environment variables for jvm launch");
//        ErrorAlert("LAUNCH ERROR", "could not set environment variables for jvm launch");
//        exit(ERROR_JVM_PRELAUNCH);

    }

// json example of our configuration file
//{
// "launch_jar":"appclass.jar",
//            "main_class":"com.example.appclass",
//            "extra_class_paths" : ["hello1","hello2"],
//    "jvm_arguments" : [],
//            "main_arguments" : []
//}

// let's grab all our config from the json file


    std::string launch_jar = config_properties.get<std::string>("launch_jar");
    std::string main_class = config_properties.get<std::string>("main_class");
    std::cout << "launch jar file name :" << launch_jar << std::endl;
    std::cout << "main class name      :" << main_class << std::endl;

    std::vector<std::string> extra_class_paths;
    for (boost::property_tree::ptree::value_type &classpath: config_properties.get_child("extra_class_paths")) {
        extra_class_paths.push_back(classpath.second.data());
        std::cout << "extra class path      :" << classpath.second.data() << std::endl;
    }

    std::vector<std::string> jvm_arguments;
    for (boost::property_tree::ptree::value_type &jvmarg: config_properties.get_child("jvm_arguments")) {
        jvm_arguments.push_back(jvmarg.second.data());
        std::cout << "jvm arg      :" << jvmarg.second.data() << std::endl;
    }

    std::vector<std::string> main_arguments;
    for (boost::property_tree::ptree::value_type &mainarg: config_properties.get_child("main_arguments")) {
        main_arguments.push_back(mainarg.second.data());
        std::cout << "main arg      :" << mainarg.second.data() << std::endl;
    }

    // launch up a VM
    IVirtualMachine *ivm = IVirtualMachine::getInstance();
   // ivm->addLibraryPath(libraries_folder.string());
std::string libfolders("-Djava.library.path=");
libfolders.append (std::string("\""));

    libfolders.append (frameworks_folder.string());
    libfolders.append (std::string(":.\""));

    ivm->addJavaOption(libfolders);

    for (boost::filesystem::directory_entry &entry: boost::filesystem::directory_iterator(plugin_folder)) {
        std::cout << "plugin added " << entry.path() << '\n';
        ivm->addLibraryPath(entry.path().string());
    }

    ivm->startJVM();

    // attach main thread
    JNIEnv *env = ivm->attachJNIThread();
    if (env == nullptr) {
//        ErrorAlert("JNI Error", "error getting JNI environment");
        ivm->killJVM();
        error_and_exit(ERROR_JVM_LAUNCH, "JNI Error", "error getting JNI environment");
//        exit(ERROR_JVM_LAUNCH);

    }

    // add properties using vm setproperties

    setJavaSystemProperty(env,std::string("jlauncher.library.path"), frameworks_folder.string());

    // set up shutdown hook in our jlauncher

    jclass launcher_class = env->FindClass("ca/maplepost/jlauncher/Jlauncher");
    if (launcher_class == nullptr) {
        //ErrorAlert("JNI Launch Error", "can't find Jlauncher class");
        ivm->killJVM();
        error_and_exit(ERROR_JVM_LAUNCH, "JNI Launch Error", "can't find Jlauncher class");
        //exit(ERROR_JVM_LAUNCH);

    }


    should_exit.store(false);  // set our atomic here

    // attach our native bindings to our class
    if (!registerNativeLinkages(env, launcher_class)) {
        //ErrorAlert("JNI Launch Error", "can't attach bindings to launcher class");
        ivm->killJVM();
        error_and_exit(ERROR_JVM_LAUNCH, "JNI Launch Error", "can't attach bindings to launcher class");
        //exit(ERROR_JVM_LAUNCH);

    }
    // launch our launcher
    jmethodID constructor = env->GetMethodID(launcher_class, "<init>", "()V");
    jobject jlauncher = env->NewObject(launcher_class, constructor);

    if (env->ExceptionCheck()) {

        ivm->exceptionCheck(env, 0);
        ivm->killJVM();
        error_and_exit(ERROR_JVM_LAUNCH, "JNI Launch Error", "exception in constructing launcher");

        //exit(ERROR_JVM_LAUNCH);

    }

    // the user main function call

    jclass mainClass = env->FindClass(main_class.data());
    if (mainClass == nullptr) {
        // ErrorAlert("JNI Launch Error", "can't find main class");
        ivm->killJVM();
        error_and_exit(ERROR_JVM_LAUNCH, "JNI Launch Error", "can't find main class");
        // exit(ERROR_JVM_LAUNCH);

    }


    jobjectArray applicationArgs;

    jmethodID mainMethod = env->GetStaticMethodID(mainClass, "main", "([Ljava/lang/String;)V");

    if (mainMethod == nullptr) {
        //ErrorAlert("JNI Launch Error", "can't find main method in main class");
        ivm->killJVM();
        error_and_exit(ERROR_JVM_LAUNCH, "JNI Launch Error", "can't find main method in main class");
        //exit(ERROR_JVM_LAUNCH);
    }


    applicationArgs = env->NewObjectArray(main_arguments.size(), env->FindClass("java/lang/String"), NULL);

    for (int i = 0; i < main_arguments.size(); ++i) {
        env->SetObjectArrayElement(applicationArgs, i, env->NewStringUTF(main_arguments[i].data()));
    }

    env->CallStaticVoidMethod(mainClass, mainMethod, applicationArgs);

    if (env->ExceptionCheck()) {
        ivm->exceptionCheck(env, 2, applicationArgs, jlauncher);
        ivm->killJVM();
        error_and_exit(ERROR_JVM_LAUNCH, "JVM ERROR", "error launching provided main class");
    }


    //ErrorAlert("sittin here","test test");
    // exit normally or with error

}


int main(int argc, const char *argv[]) {

    std::thread java_worker(main_thread);
    java_worker.detach();
#ifdef MAC_VERSION
    ParkEventLoop();
#endif
    std::cout << "Came outa the loop" << std::endl;
    return 0;

}
