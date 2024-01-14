cmake -DOpenMP_CXX_FLAG="-Xclang -fopenmp" \
-DOpenMP_CXX_INCLUDE_DIR=/opt/homebrew/opt/libomp/include \
-DOpenMP_CXX_LIB_NAMES=libomp -DOpenMP_C_FLAG="-Xclang -fopenmp" \
-DOpenMP_C_INCLUDE_DIR=/opt/homebrew/opt/libomp/include \
-DOpenMP_C_LIB_NAMES=libomp \
-DOpenMP_libomp_LIBRARY=/opt/homebrew/opt/libomp/lib/libomp.dylib