#!/bin/sh

TOP_DIR=$(pwd)
INSTALL_PREFIX=${TOP_DIR}/install
BUILD_PREFIX=${TOP_DIR}/build
INSTALL_PKGEXE_PATH=${INSTALL_PREFIX}/bin
export INSTALL_PKGEXE_PATH=${INSTALL_PKGEXE_PATH}

DEBUG_LOG_FILE= #"debug.log"

debug_print() {
    cut_time=$(date +%s)

    echo ">>>>>> [${cut_time}] >>>>>> $*"
    if [[ -n "${DEBUG_LOG_FILE}" ]]; then
        echo ">>>>>> [${cut_time}] >>>>>> $*" >> ${DEBUG_LOG_FILE}
    fi
}

build_clean() {
    debug_print "Clean begin ..."
    [[ -d ${INSTALL_PREFIX} ]] && rm -rf ${INSTALL_PREFIX}
    [[ -d ${BUILD_PREFIX} ]] && rm -rf ${BUILD_PREFIX}

    mkdir -p ${INSTALL_PREFIX}
    mkdir -p ${BUILD_PREFIX}
    debug_print "Clean end ..."
}

build_prepare() {
    local build_dir=${BUILD_PREFIX}
    debug_print "Build prepare begin ..."
    [[ -d ${build_dir} ]] && rm -rf ${build_dir}
    mkdir -p ${build_dir}
    debug_print "Build prepare end ..."
}

build_target() {
    local target_dir=${TOP_DIR}
    local build_dir=${BUILD_PREFIX}

    debug_print "Build begin ..."

    if [[ -f ${target_dir}/CMakeLists.txt ]] && [[ -d ${build_dir} ]]; then
        debug_print "Build ..."
        cd ${build_dir}
        cmake -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} ${target_dir}
        make >/dev/null && make install >/dev/null #&& cd ..
        cd ..
    fi
    debug_print "Build end ..."
}

code_lint() {
	find src -iname "*.[ch]" | xargs -r clang-format -i -style=file
	find src \( -iname "*.[ch]" -o -iname Makefile -o -iname CMakeLists.txt -o -iname README.md -o -iname config.in -o -iname "*.patch" -o -iname "*.pc.in" \) -executable | xargs -r chmod -x
}

print_help() {
    local app_name="$1"
    echo "Usage:"
    echo "$app_name <option>"
    echo "Option:"
    echo "    lint"
    echo "    build"
}

option=$1
case "${option}" in
    lint)
        debug_print "code lint begin"
        code_lint
        debug_print "code lint end"
        ;;
    build)
        debug_print "target build begin"
        build_clean
        build_prepare
        build_target
        debug_print "target build end"
        ;;
    *)
        print_help "$0"
        ;;
esac