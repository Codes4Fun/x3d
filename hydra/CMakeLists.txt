cmake_minimum_required (VERSION 2.6)

project (hydra)

include_directories ("${SIXENSE_PATH}/include")
add_library (hydra Hydra.cpp)


