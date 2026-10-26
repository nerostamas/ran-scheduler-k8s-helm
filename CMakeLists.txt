cmake_minimum_required(VERSION 3.16)
project(ran_sim_service LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_compile_options(-Wall -Wextra)

find_package(Threads REQUIRED)

# ---- Vendored simulation engine (from the ran-mac-scheduler project) ----
add_library(ran_core
  engine/src/phy/LinkAdaptation.cpp
  engine/src/mac/HarqProcess.cpp
  engine/src/mac/UserEquipment.cpp
  engine/src/mac/Schedulers.cpp
  engine/src/metrics/Kpi.cpp
  engine/src/Simulator.cpp
)
target_include_directories(ran_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine/include)

# ---- cpp-httplib (single-header HTTP server) ----
include(FetchContent)
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(httplib)

# ---- HTTP service ----
add_executable(ran_sim_service
  service/main.cpp
  service/RequestParams.cpp
)
target_include_directories(ran_sim_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/service)
target_link_libraries(ran_sim_service PRIVATE ran_core httplib::httplib Threads::Threads)
