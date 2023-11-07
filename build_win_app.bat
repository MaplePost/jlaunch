echo off
echo #////////////////////////////////////////////////////////////////////////////
echo #
echo #                       Written by Peter J Slack
echo #               Copyright 2022 Maplepost All Rights Reserved
echo #
echo #////////////////////////////////////////////////////////////////////////////
echo .
echo .
echo .
echo .
REM set
REM
REM set program envirospace
REM

set exitCommand=exit


REM
REM *** List variables that have to be passed in **
REM
REM get the command line arguments
set PRODUCT_FOLDER=%1
set JRE_ZIP_FILE=%2
set SOURCE_FOLDER=%3
set BINARY_FOLDER=%4


echo "########### PROJECT SETTINGS ################"
echo "PRODUCT_BUILD_FOLDER          %PRODUCT_FOLDER%"
echo "JRE_ZIP_FILE                    %JRE_ZIP_FILE%"
echo "SOURCE_FOLDER               %SOURCE_FOLDER%"
echo "BINARY_FOLDER               %BINARY_FOLDER%"
echo off


if %errorlevel% neq 0 exit 1

  echo "########### PACKAGE PROJECT ################"

pushd
cd %SOURCE_FOLDER%\jlauncher
call mvn clean install
if %errorlevel% neq 0 exit 1
popd


   pushd

    cd %PRODUCT_FOLDER%
    REM check if jlaunch.exe exists
    if exist jlaunch.exe (
        echo "jlaunch.exe exists"
        mkdir Libraries
        mkdir Resources
        mkdir Resources\Java
        mkdir Resources\Plugins
        mkdir Resources\Config

        copy "%SOURCE_FOLDER%\jlaunch.example.json" Resources\Config\jlaunch.json
       copy "%SOURCE_FOLDER%\jlauncher\target\jlauncher-1.0-SNAPSHOT.jar" Resources\Plugins\
    ) else (
        echo "jlaunch.exe does not exist"
        exit 1
    )

    REM check if jre exists
    if exist %JRE_ZIP_FILE% (
        echo "jre zip file exists"
        cd Resources\Java
        tar -xf %JRE_ZIP_FILE% -C .


    ) else (
        echo "jre does not exist"
        exit 1
    )

REM make a product jar file

mkdir "%BINARY_FOLDER%\productjar"

jar -Mcvf "%BINARY_FOLDER%\productjar\jlaunch.jar" -C %PRODUCT_FOLDER% .
if %errorlevel% neq 0 exit 1

cd "%SOURCE_FOLDER%"
mvn clean install -DBASE_BINARY_DIR="%BINARY_FOLDER%\productjar"
if %errorlevel% neq 0 exit 1

REM

popd


if %errorlevel% neq 0 exit 1

:endNormal
%exitCommand% 0

