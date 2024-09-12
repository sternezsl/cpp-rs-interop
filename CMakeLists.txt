cmake_minimum_required(VERSION 3.20)
project(cpp_rs_inteop)

set(FBOSS_VERSION 2024.08.25)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

string(REPLACE "-" "_" CARGO_TARGET_UNDERLINE ${PLATFORM})
string(TOUPPER ${CARGO_TARGET_UNDERLINE} CARGO_TARGET_UNDERLINE_UPPER)

list(APPEND CMAKE_PREFIX_PATH
  "/opt/oss/folly/${FBOSS_VERSION}/lib/${CXXSTL}/${PLATFORM}/cmake")
find_package(folly CONFIG REQUIRED)

string(TOLOWER ${CMAKE_BUILD_TYPE} CARGO_BUILD_PROFILE)
if(CARGO_BUILD_PROFILE STREQUAL debug)
  set(CARGO_BUILD_PROFILE dev)
endif()
set(CARGO_TARGET_DIR ${CMAKE_BINARY_DIR}/rust)

if(APPLE)
  set(CARGO_SYSROOT ${CMAKE_OSX_SYSROOT})
else()
  set(CARGO_SYSROOT ${CMAKE_SYSROOT})
endif()

if(MSVC)
  set(CMAKE_CXX_FLAGS "/EHs -Wno-dollar-in-identifier-extension -Wno-unsafe-buffer-usage ${CMAKE_CXX_FLAGS}")
  set(CARGO_CXX_FLAGS ${CMAKE_CXX_FLAGS})
elseif(APPLE)
  set(CARGO_CXX_FLAGS "--sysroot=${CARGO_SYSROOT} ${CMAKE_CXX_FLAGS}")
else()
  set(CARGO_CXX_FLAGS "--sysroot=${CARGO_SYSROOT} ${CMAKE_CXX_FLAGS}")
endif()

############## build cpp demo ####################
set(BLOBSTORE_DIR ${CMAKE_SOURCE_DIR}/blobstore-rs)
set(BLOBSTORE_SOURCE_FILE ${BLOBSTORE_DIR}/src/lib.rs)
set(BLOBSTORE_LIB ${CARGO_TARGET_DIR}/${PLATFORM}/${CARGO_BUILD_PROFILE}/${CMAKE_STATIC_LIBRARY_PREFIX}blobstore_rs${CMAKE_STATIC_LIBRARY_SUFFIX})
add_custom_command(
  OUTPUT ${BLOBSTORE_LIB}
  COMMAND ${CMAKE_COMMAND} -E env
          CXX_${CARGO_TARGET_UNDERLINE}=${CMAKE_CXX_COMPILER}
          C_${CARGO_TARGET_UNDERLINE}=${CMAKE_C_COMPILER}
          AR_${CARGO_TARGET_UNDERLINE}=${CMAKE_AR}
          CXXFLAGS_${CARGO_TARGET_UNDERLINE}=${CARGO_CXX_FLAGS}
          FOLLY_INCLUDE_DIR=${FOLLY_INCLUDE_DIR}
          cargo build
          --profile=${CARGO_BUILD_PROFILE}
          --target ${PLATFORM}
          --target-dir ${CARGO_TARGET_DIR}
          --manifest-path ${BLOBSTORE_DIR}/Cargo.toml
  DEPENDS ${BLOBSTORE_SOURCE_FILE}
          ${BLOBSTORE_DIR}/blobstore/blobstore.cc
          ${BLOBSTORE_DIR}/blobstore/blobstore.h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  USES_TERMINAL
  COMMENT "building library: ${BLOBSTORE_LIB} ...")

add_executable(cpp_rs_inteop blobstore/main.cc ${BLOBSTORE_LIB})
target_include_directories(
  cpp_rs_inteop
  PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${BLOBSTORE_DIR}
  ${BLOBSTORE_DIR}/cxx-async/cxx-async/include
  ${CARGO_TARGET_DIR}/${PLATFORM}/cxxbridge)
target_link_libraries(cpp_rs_inteop ${BLOBSTORE_LIB} folly::folly)
# Windows-only configuration
if(WIN32)
  target_link_libraries(cpp_rs_inteop userenv ws2_32 bcrypt ntdll)
  set_target_properties(
    cpp_rs_inteop
    PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
elseif(CMAKE_SYSTEM_NAME STREQUAL Linux)
  target_link_libraries(cpp_rs_inteop pthread dl)
endif()
set_target_properties(
  cpp_rs_inteop
  PROPERTIES ADDITIONAL_CLEAN_FILES ${CARGO_TARGET_DIR})

############## build rust demo ##################
if(TARGET folly::folly)
  get_target_property(_folly_lib_location folly::folly IMPORTED_LOCATION_RELWITHDEBINFO)
  get_target_property(_event_lib_location folly::event IMPORTED_LOCATION_RELWITHDEBINFO)
  get_target_property(_boost_thread_lib_location folly::boost_thread IMPORTED_LOCATION_RELWITHDEBINFO)
endif()

set(CARGO_RUSTFLAGS "-Clinker=${CMAKE_CXX_COMPILER} -Clink-arg=-nostdlib++ -Clink-arg=-fuse-ld=${CMAKE_LINKER} -Clink-arg=--sysroot=${CARGO_SYSROOT} -Clink-arg=-L${UCC_TOOLCHAIN_ROOT}/runtime/${UCC_VERSION}/lib/${PLATFORM} -Clink-arg=-Wl,-rpath,${UCC_TOOLCHAIN_ROOT}/runtime/${UCC_VERSION}/lib/${PLATFORM} -Clink-arg=${_folly_lib_location}")
if(PLATFORM STREQUAL aarch64-unknown-linux-gnu)
  set(CARGO_RUSTFLAGS "${CARGO_RUSTFLAGS} -Clink-arg=${CARGO_SYSROOT}/usr/lib/libc_nonshared.a -Clink-arg=--gcc-toolchain=${NATIVE_TOOLCHAIN_ROOT} -Clink-arg=-lc++")
elseif(PLATFORM STREQUAL x86_64-unknown-linux-gnu)
  set(CARGO_RUSTFLAGS "${CARGO_RUSTFLAGS} -Clink-arg=${CARGO_SYSROOT}/usr/lib64/libc_nonshared.a -Clink-arg=--gcc-toolchain=${NATIVE_TOOLCHAIN_ROOT} -Clink-arg=-lc++")
elseif(PLATFORM STREQUAL aarch64-apple-darwin)
  set(CARGO_RUSTFLAGS "${CARGO_RUSTFLAGS} -Clink-arg=-mmacosx-version-min=11.1.0 -Clink-arg=-march=armv8.5-a+crc+crypto -Clink-arg=-lc++abi -Clink-arg=-Wl,-undefined,dynamic_lookup")
elseif(PLATFORM STREQUAL x86_64-pc-windows-msvc)
  set(CARGO_RUSTFLAGS "-Clinker=${UCC_TOOLCHAIN_ROOT}/${UCC_VERSION}/bin/lld-link -Clink-arg=-libpath:\"${CMAKE_BINARY_DIR}/winsdk_lib_symlinks\" -Clink-arg=-libpath:\"${UCC_TOOLCHAIN_ROOT}/toolchains/msvc/WindowsKits/10/Lib/${WINDOWS_KITS_VER}/um/x64/\" -Clink-arg=-libpath:\"${UCC_TOOLCHAIN_ROOT}/toolchains/msvc/VC/Tools/MSVC/${MSVC_VER}/lib/x64\" -Clink-arg=-libpath:\"${UCC_TOOLCHAIN_ROOT}/toolchains/msvc/WindowsKits/10/Lib/${WINDOWS_KITS_VER}/ucrt/x64\" -Clink-arg=${_folly_lib_location} -Clink-arg=${_event_lib_location} -Clink-arg=${_boost_thread_lib_location} -Clink-arg=iphlpapi.lib")
endif()

add_custom_command(
  OUTPUT ${CARGO_TARGET_DIR}//${CARGO_BUILD_PROFILE}/cpp-rs-interop
  COMMAND ${CMAKE_COMMAND} -E env
          CXX_${CARGO_TARGET_UNDERLINE}=${CMAKE_CXX_COMPILER}
          C_${CARGO_TARGET_UNDERLINE}=${CMAKE_C_COMPILER}
          AR_${CARGO_TARGET_UNDERLINE}=${CMAKE_AR}
          CXXFLAGS_${CARGO_TARGET_UNDERLINE}=${CARGO_CXX_FLAGS}
          CARGO_TARGET_${CARGO_TARGET_UNDERLINE_UPPER}_RUSTFLAGS=${CARGO_RUSTFLAGS}
          FOLLY_INCLUDE_DIR=${FOLLY_INCLUDE_DIR}
          cargo build
          --profile=${CARGO_BUILD_PROFILE}
          --target ${PLATFORM}
          --target-dir ${CARGO_TARGET_DIR}
          --manifest-path ${CMAKE_SOURCE_DIR}/Cargo.toml
  DEPENDS ${CMAKE_SOURCE_DIR}/src/main.rs
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  USES_TERMINAL
  COMMENT "building ${CARGO_TARGET_DIR}/${CARGO_BUILD_PROFILE}/cpp-rs-interop ...")
add_custom_target(cpp_rust_interop_rust
  ALL DEPENDS
  ${CARGO_TARGET_DIR}/${CARGO_BUILD_PROFILE}/cpp-rs-interop)
