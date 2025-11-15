cmake_minimum_required(VERSION 3.18)
project(cuda_transcoder LANGUAGES C CXX CUDA)

# --------------------------------------------------------------
# CUDA configuration
# --------------------------------------------------------------
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# --------------------------------------------------------------
# Set Video Codec SDK location
# --------------------------------------------------------------
set(SDK_INCLUDE "${CMAKE_SOURCE_DIR}/Interface")
set(SDK_LIB "${CMAKE_SOURCE_DIR}/Lib/linux/stubs/x86_64")

include_directories(${SDK_INCLUDE})
link_directories(${SDK_LIB})

# --------------------------------------------------------------
# Search for NVENC/NVDEC library
# Include:
#   - libnvidia-encode.so
#   - libnvcuvid.so
# --------------------------------------------------------------
find_library(NVENC_LIB nvidia-encode PATHS ${SDK_LIB})
find_library(NVCUVID_LIB nvcuvid PATHS ${SDK_LIB})


# --------------------------------------------------------------
# Set executable files
# --------------------------------------------------------------
add_executable(cuda-transcoder
    main.cpp
    decoder.cu
)

# CUDA compilation file setting
set_target_properties(cuda-transcoder PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)

# --------------------------------------------------------------
# Link
# --------------------------------------------------------------
target_link_libraries(cuda-transcoder
    ${NVENC_LIB}
    ${NVCUVID_LIB}
    cuda
    cudart
)
