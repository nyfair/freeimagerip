# Copyright (c) the JPEG XL Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if((SANITIZER STREQUAL "asan") OR (SANITIZER STREQUAL "msan"))
  set(BUILD_TESTING OFF)
endif()

# libpng
if (JPEGLI_BUNDLE_LIBPNG AND EMSCRIPTEN)
  if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libpng/CMakeLists.txt")
    message(FATAL_ERROR "Please provide libpng or "
            "run ${PROJECT_SOURCE_DIR}/deps.sh to fetch the build dependencies.")
  endif()
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/libpng/scripts/pnglibconf.h.prebuilt" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/libpng")
  file(RENAME "${CMAKE_CURRENT_SOURCE_DIR}/libpng/pnglibconf.h.prebuilt" "${CMAKE_CURRENT_SOURCE_DIR}/libpng/pnglibconf.h")
  set(ZLIB_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/zlib/")
  set(ZLIB_LIBRARIES "")
  set(PNG_FOUND YES PARENT_SCOPE)
  set(PNG_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/libpng/" PARENT_SCOPE)
  set(PNG_LIBRARIES "" PARENT_SCOPE)
elseif (JPEGLI_BUNDLE_LIBPNG)
  if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libpng/CMakeLists.txt")
    message(FATAL_ERROR "Please provide libpng or "
            "run ${PROJECT_SOURCE_DIR}/deps.sh to fetch the build dependencies.")
  endif()
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/libpng/scripts/pnglibconf.h.prebuilt" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/libpng")
  file(RENAME "${CMAKE_CURRENT_SOURCE_DIR}/libpng/pnglibconf.h.prebuilt" "${CMAKE_CURRENT_SOURCE_DIR}/libpng/pnglibconf.h")
  add_subdirectory(zlib)
  set(PNG_STATIC ON CACHE BOOL "")
  set(PNG_TOOLS OFF CACHE BOOL "")
  set(ZLIB_INCLUDE_DIR bing_bong_dir)
  set(ZLIB_LIBRARY bing_bong_lib)
  add_library(ZLIB::ZLIB INTERFACE IMPORTED)
  set(PNG_TESTS OFF CACHE BOOL "")
  set(SKIP_INSTALL_ALL ON CACHE BOOL "")
  set(ZLIB_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/zlib/")
  set(ZLIB_LIBRARIES zlibstatic)
  add_subdirectory(libpng EXCLUDE_FROM_ALL)
  set(PNG_FOUND YES PARENT_SCOPE)
  set(PNG_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/libpng/" PARENT_SCOPE)
  target_link_libraries(png_static PUBLIC zlibstatic)
  set(PNG_LIBRARIES png_static PARENT_SCOPE)
  set_property(TARGET png_static PROPERTY POSITION_INDEPENDENT_CODE ON)
  set_property(TARGET zlibstatic PROPERTY POSITION_INDEPENDENT_CODE ON)
  if(JPEGLI_DEP_LICENSE_DIR)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/libpng/LICENSE"
                   ${PROJECT_BINARY_DIR}/LICENSE.libpng COPYONLY)
  endif()
else()
  find_package(PNG)
  if(PNG_FOUND AND JPEGLI_DEP_LICENSE_DIR)
    configure_file("${JPEGLI_DEP_LICENSE_DIR}/zlib1g-dev/copyright"
                   ${PROJECT_BINARY_DIR}/LICENSE.zlib COPYONLY)
    configure_file("${JPEGLI_DEP_LICENSE_DIR}/libpng-dev/copyright"
                   ${PROJECT_BINARY_DIR}/LICENSE.libpng COPYONLY)
  endif()  # JPEGLI_DEP_LICENSE_DIR
endif()

# sjpeg
if (JPEGLI_ENABLE_SJPEG)
  if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sjpeg/CMakeLists.txt")
    message(FATAL_ERROR "Please provide sjpeg or "
            "run ${PROJECT_SOURCE_DIR}/deps.sh to fetch the build dependencies.")
  endif()
  include(sjpeg.cmake)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sjpeg/COPYING"
                 ${PROJECT_BINARY_DIR}/LICENSE.sjpeg COPYONLY)
endif ()
