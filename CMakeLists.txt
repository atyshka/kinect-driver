cmake_minimum_required(VERSION 3.0.0)
project(KinectRecorder VERSION 0.1.0)
# use C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

include(CTest)
enable_testing()

include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 0817715923c9705e68994eb52ef9df3f6845beba) # The commit hash for 1.10.x. Replace with the latest from: https://github.com/libcpr/cpr/releases
FetchContent_MakeAvailable(cpr)

FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

find_package(ALSA REQUIRED)

include_directories(SYSTEM external/speechsdk/include/cxx_api external/speechsdk/include/c_api)
link_directories(external/speechsdk/lib/x64)

add_executable(AudioRecorder src/main.cpp)
target_link_libraries(AudioRecorder PRIVATE Microsoft.CognitiveServices.Speech.core)
target_link_libraries(AudioRecorder PRIVATE cpr::cpr argparse nlohmann_json::nlohmann_json ${ALSA_LIBRARIES})
target_include_directories(AudioRecorder PRIVATE ${ALSA_INCLUDE_DIRS})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
