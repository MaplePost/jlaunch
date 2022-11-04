#!/bin/bash

if [ $# == 0 ]; then
echo "usage build_osx_app <binary output dir from cmake> <jre zip file> "
exit 1
fi

# this is our most important testexit function used on
# make or break calls  must pass $? as first param then
# the name of the test as teh second param
function testexit () {

    if [ $1 != 0 ]; then
	   echo  "FAILED $2 ..exiting with error code"
	   exit $1
	fi

    echo "PASSED $2"
}

# test is fathe input file exists exit if it doesn't
function testfileexit() {

  if [ -e "$1" ]; then
    echo "File PASSED : $1"
  else
    echo "File FAILED : $1"
    exit 1
  fi

}

# package an artifact
# $1 describe the package
# $2 argument full path to the folder
# $3 the product folder name
package() {
  ditto -c -k --sequesterRsrc --keepParent "${2}" "${2}.zip"
  testexit $? " zipping ${1}"
  local CURRENT_W_DIR_1=`pwd`
  cd ${BASE_FOLDER}

  jar -cf "${3}.jar" "${3}.zip"
  testexit $? " jarring ${1}"
  cd ${CURRENT_W_DIR_1}

}

BASE_APP=jlaunch.app
BASE_BINARY_DIR=${1}
JRE_ZIP_FILE=${2}

rm -f "${BASE_BINARY_DIR}/${BASE_APP}.zip"

echo "hello ${BASE_BINARY_DIR} ${JRE_ZIP_FILE}"
mkdir -pv "${BASE_BINARY_DIR}/${BASE_APP}/Contents/Resources/Java"
testfileexit "${BASE_BINARY_DIR}/${BASE_APP}/Contents/Resources/Java"

# test that the zip file for the desired jre exists
testfileexit "${JRE_ZIP_FILE}"

# create plugins folder
mkdir -pv "${BASE_BINARY_DIR}/${BASE_APP}/Contents/Resources/Plugins"
testfileexit "${BASE_BINARY_DIR}/${BASE_APP}/Contents/Resources/Plugins"

# create libraries folder
mkdir -pv "${BASE_BINARY_DIR}/${BASE_APP}/Contents/Libraries"
testfileexit "${BASE_BINARY_DIR}/${BASE_APP}/Contents/Libraries"

# create configuration folder
mkdir -pv "${BASE_BINARY_DIR}/${BASE_APP}/Contents/Resources/Config"
testfileexit "${BASE_BINARY_DIR}/${BASE_APP}/Contents/Resources/Config"

#create our java folder for jres
ditto -xk "${JRE_ZIP_FILE}" "${BASE_BINARY_DIR}/${BASE_APP}/Contents/Resources/Java"
testfileexit "${BASE_BINARY_DIR}/${BASE_APP}/Contents/Resources/Java/jre"

#test that we at least have our example config file
testfileexit "jlaunch.example.json"

# copy the example to a live version of the config file
cp -fv "jlaunch.example.json" "${BASE_BINARY_DIR}/${BASE_APP}/Contents/Resources/Config/jlaunch.json"

cp -fv "jlauncher/target/jlauncher-1.0-SNAPSHOT.jar" "${BASE_BINARY_DIR}/${BASE_APP}/Contents/Resources/Plugins/"

#packager for osxmac

cp Info.plist "${BASE_BINARY_DIR}/${BASE_APP}/Contents/Info.plist"

testfileexit "${BASE_BINARY_DIR}/${BASE_APP}"
ditto -c -k --sequesterRsrc --keepParent "${BASE_BINARY_DIR}/${BASE_APP}" "${BASE_BINARY_DIR}/${BASE_APP}.zip"
testexit $? " zipping ${BASE_APP}"

mvn install -DBASE_APP="${BASE_APP}" -DBASE_BINARY_DIR="${BASE_BINARY_DIR}"
testexit $? "mvn install"

#jar -cf "${BASE_APP}.jar" "${BASE_APP}"
#testexit $? "jar -cf ${BASE_APP}.jar ${BASE_APP}"

#mvn deploy:deploy-file -DgroupId=<group-id> \
#  -DartifactId=<artifact-id> \
#  -Dversion=<version> \
#  -Dpackaging=<type-of-packaging> \
#  -Dfile=<path-to-file> \
#  -DrepositoryId=<id-to-map-on-server-section-of-settings.xml> \
#  -Durl=<url-of-the-repository-to-deploy>
