# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(JERRYSCRIPT_SOURCE "$ENV{MICRONODE}/deps/jerryscript")

# JerryScript setting here
set(JERRY_GLOBAL_HEAP_SIZE "(128)")
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)
set(IDF_TARGET "esp32c3")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
#idf_build_process(esp32c3)
project(micronode)
