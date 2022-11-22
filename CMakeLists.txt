cmake_minimum_required(VERSION 3.21)
project(jlaunch)

set(CMAKE_CXX_STANDARD 11)

include(${CMAKE_CURRENT_SOURCE_DIR}/modules/MaplePlatformModule.cmake)


set(CUR_TARGET jlaunch)

set(MY_SRC IVirtualMachine.cpp
        main.cpp)
option(SOURCE_JRE_ZIP "JRE SOURCE ZIP FILE" "~/SDKs/JAVA/jre.zip" )

set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
set(Boost_USE_DEBUG_LIBS        OFF)  # ignore debug libs and
set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
set(Boost_NO_BOOST_CMAKE        OFF) # had to set this off on mac using brew to install cmake etc

#for windows set the boost root
if(WIN32)
    #set(BOOST_ROOT "C:\\SDKs\\boost\\boost_1_80_0")
    #set(BOOST_INCLUDEDIR "C:\\SDKs\\boost\\boost_1_80_0")
    #set(BOOST_LIBRARYDIR "C:\\SDKs\\boost\\boost_1_80_0\\lib64-msvc-14.3")
    message("boost root set")


endif()

#find boost on windows
# and give the path to the boost root dir
#and the path to the boost lib dir


find_package(Boost REQUIRED OPTIONAL_COMPONENTS filesystem json)
if(Boost_FOUND)
    message("boost found ok")
    message("boost include dir $ENV{BOOST_INCLUDEDIR}")
    message("boost lib dir $ENV{BOOST_LIBRARYDIR}")
    message("boost filesystem library release ${Boost_FILESYSTEM_LIBRARY} ")
    message("boost json library release ${Boost_JSON_LIBRARY} ")
else()
    message("BOOST NOT FOUND ")
endif()

include_directories(${Boost_INCLUDE_DIRS})




if(DARWIN)
    set(CMAKE_MACOSX_RPATH 1)
    list(APPEND MY_SRC alert_osx.mm osx_launch.mm )
    add_executable(${CUR_TARGET} MACOSX_BUNDLE ${MY_SRC})
    set_target_properties(${CUR_TARGET} PROPERTIES
            BUNDLE TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER my.domain.style.identifier.${CUR_TARGET}
            MACOSX_BUNDLE_BUNDLE_NAME ${CUR_TARGET}
            MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
            #MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/cmake/customtemplate.plist.in
            )

    target_compile_definitions(${CUR_TARGET} PUBLIC MAC_VERSION=1)


    target_link_libraries(${CUR_TARGET} PUBLIC Boost::filesystem "-framework AppKit")

    # this ensures we wipe the app bundle on clean
    set_directory_properties(
            PROPERTIES
            ADDITIONAL_CLEAN_FILES "${CMAKE_CURRENT_BINARY_DIR}/jlaunch.app")
    #add the rpath to the build so we can find the dylibs
    add_custom_command(TARGET ${CUR_TARGET}
            POST_BUILD COMMAND
            ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path/../Frameworks"
            $<TARGET_FILE:jlaunch>)

    add_custom_command(TARGET ${CUR_TARGET} POST_BUILD
            COMMAND mvn clean install
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jlauncher
            COMMENT "Building test app"
            )


    add_custom_command(TARGET ${CUR_TARGET} POST_BUILD
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build_osx_app.sh "${CMAKE_CURRENT_BINARY_DIR}" "${SOURCE_JRE_ZIP}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Building app"
            )


endif(DARWIN)

if(WIN32)
    add_executable(${CUR_TARGET}
            IVirtualMachine.cpp
            main.cpp)
target_compile_definitions(${CUR_TARGET} PUBLIC WIN_VERSION=1)

target_link_directories(${CUR_TARGET} PUBLIC $ENV{BOOST_LIBRARYDIR})
    target_link_libraries(${CUR_TARGET}  )

endif(WIN32)

    if (DARWIN)
    include(FindJNI)
    if (NOT JNI_FOUND)
        message (STATUS "Could not find JNI, retrying for JDK.")
        execute_process(COMMAND /usr/libexec/java_home OUTPUT_VARIABLE JAVA_HOME OUTPUT_STRIP_TRAILING_WHITESPACE)
        # message (STATUS "JAVA_HOME=${JAVA_HOME}")
        if (NOT JAVA_HOME)
            message (FATAL_ERROR "Could not find JDK Java home, fatal.")
            return()
        endif ()

        set(JAVA_JNI_INCLUDE ${JAVA_HOME}/include)
        if (APPLE)
            set(JAVA_JNI_INCLUDE ${JAVA_JNI_INCLUDE} ${JAVA_JNI_INCLUDE}/darwin )
        endif()
        # message (STATUS "JAVA_JNI_INCLUDE=${JAVA_JNI_INCLUDE}")

        # message (STATUS "${JAVA_HOME}/jre/lib/jli")
        # find_library(JAVA_JNI_LIB NAMES jli HINTS "${JAVA_HOME}/lib/jli" "${JAVA_HOME}/jre/lib/jli")
        # message (STATUS "JAVA_JNI_LIB=${JAVA_JNI_LIB}")

        if (NOT JAVA_JNI_INCLUDE) # OR NOT JAVA_JNI_LIB)
            message (FATAL_ERROR "Could not find JDK Java JNI, fatal.")
            return()
        else ()
            message (STATUS "JDK resolved.")
        endif ()
    else()
        # message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
        set(JAVA_JNI_INCLUDE ${JNI_INCLUDE_DIRS})
        # message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
        # set(JAVA_JNI_LIB ${JNI_LIBRARIES})
    endif()
else ()
    include(FindJNI)
    if (NOT JNI_FOUND)
        message (FATAL_ERROR "No JNI found, cannot build")
    else()
        # message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
        set(JAVA_JNI_INCLUDE ${JNI_INCLUDE_DIRS})
        # message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
        # set(JAVA_JNI_LIB ${JNI_LIBRARIES})
    endif()
endif ()

include_directories(${JAVA_JNI_INCLUDE})

