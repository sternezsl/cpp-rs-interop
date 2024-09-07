#!/bin/sh
targets="aarch64-unknown-linux-gnu\
         x86_64-unknown-linux-gnu\
         aarch64-apple-darwin"
for target in $targets
do
# cmake -DCMAKE_TOOLCHAIN_FILE=ucc.cmake -DPLATFORM=$target -DTOOLCHAIN_NAME=ucc -DCXXSTL=libcxx -S ./ -B build/$target
    # /opt/ucc/cmake/3.20.0/bin/cmake --build build/$target
    echo $target
done

# /opt/ucc/cmake/3.20.0/bin/cmake -DCMAKE_TOOLCHAIN_FILE=/opt/ucc/tools/cmake/toolchain/ucc.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPLATFORM=x86_64-pc-windows-msvc -DTOOLCHAIN_NAME=ucc -DMSVC_USE_STATIC_RUNTIME=ON -S ./ -B build/x86_64-pc-windows-msvc
# /opt/ucc/cmake/3.20.0/bin/cmake --build build/x86_64-pc-windows-msvc
