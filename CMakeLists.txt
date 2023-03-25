cmake_minimum_required(VERSION 3.0.0)
project(KinectRecorder VERSION 0.1.0)
# use C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

include(CTest)
enable_testing()

include_directories(external/speechsdk/include/cxx_api external/speechsdk/include/c_api)
link_directories(external/speechsdk/lib/x64)

add_executable(AudioRecorder src/main.cpp)
target_link_libraries(AudioRecorder Microsoft.CognitiveServices.Speech.core)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
