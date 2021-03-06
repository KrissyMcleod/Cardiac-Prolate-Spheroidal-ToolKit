cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
  cmake_policy(VERSION 2.4)
  cmake_policy(SET CMP0005 OLD)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)


project(CPSTK)


# On Visual Studio 8 MS deprecated C. This removes all 1.276E1265 security
# warnings
if(WIN32)
  if(NOT BORLAND)
    if(NOT CYGWIN)
      if(NOT MINGW)
        if(NOT ITK_ENABLE_VISUAL_STUDIO_DEPRECATED_C_WARNINGS)
          add_definitions(
            -D_CRT_FAR_MAPPINGS_NO_DEPRECATE
            -D_CRT_IS_WCTYPE_NO_DEPRECATE
            -D_CRT_MANAGED_FP_NO_DEPRECATE
            -D_CRT_NONSTDC_NO_DEPRECATE
            -D_CRT_SECURE_NO_DEPRECATE
            -D_CRT_SECURE_NO_DEPRECATE_GLOBALS
            -D_CRT_SETERRORMODE_BEEP_SLEEP_NO_DEPRECATE
            -D_CRT_TIME_FUNCTIONS_NO_DEPRECATE
            -D_CRT_VCCLRIT_NO_DEPRECATE
            -D_SCL_SECURE_NO_DEPRECATE
            )
        endif(NOT ITK_ENABLE_VISUAL_STUDIO_DEPRECATED_C_WARNINGS)
      endif(NOT MINGW)
    endif(NOT CYGWIN)
  endif(NOT BORLAND)
endif(WIN32)


## #################################################################
## Required dependencies
## #################################################################

find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

unset(CPSTK_THIRD_PARTY_PACKAGES)
set(CPSTK_THIRD_PARTY_PACKAGES
  VTK
  TTK
)

set(CPSTK_PACKAGES_MISSING 0)

foreach (package ${CPSTK_THIRD_PARTY_PACKAGES})
  find_package(${package} QUIET)
  if(${package}_FOUND)
    include(${${package}_USE_FILE})
  else(${package}_FOUND)
    set(CPSTK_PACKAGES_MISSING 1)
    set(CPSTK_MISSING_PACKAGES 
      ${package} 
      ${CPSTK_MISSING_PACKAGES}
      )
  endif(${package}_FOUND)
endforeach (package) 


## #################################################################
## Configure path
## #################################################################


if(WIN32)
  set(${PROJECT_NAME}_ARCHIVE_OUTPUT_DIRECTORY lib)
  set(${PROJECT_NAME}_RUNTIME_OUTPUT_DIRECTORY bin)
  set(${PROJECT_NAME}_LIBRARY_OUTPUT_DIRECTORY bin)
else(WIN32)
  set(${PROJECT_NAME}_ARCHIVE_OUTPUT_DIRECTORY lib)
  set(${PROJECT_NAME}_RUNTIME_OUTPUT_DIRECTORY bin)
  set(${PROJECT_NAME}_LIBRARY_OUTPUT_DIRECTORY lib)
endif(WIN32)

set(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR}/${${PROJECT_NAME}_LIBRARY_OUTPUT_DIRECTORY})
set(ARCHIVE_OUTPUT_PATH    ${CMAKE_BINARY_DIR}/${${PROJECT_NAME}_ARCHIVE_OUTPUT_DIRECTORY})
set(RUNTIME_OUTPUT_PATH    ${CMAKE_BINARY_DIR}/${${PROJECT_NAME}_RUNTIME_OUTPUT_DIRECTORY})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/${${PROJECT_NAME}_RUNTIME_OUTPUT_DIRECTORY})

set(LIBRARY_INSTALL_OUTPUT_PATH    ${CMAKE_INSTALL_PREFIX}/${${PROJECT_NAME}_LIBRARY_OUTPUT_DIRECTORY})
set(ARCHIVE_INSTALL_OUTPUT_PATH    ${CMAKE_INSTALL_PREFIX}/${${PROJECT_NAME}_ARCHIVE_OUTPUT_DIRECTORY})
set(RUNTIME_INSTALL_OUTPUT_PATH    ${CMAKE_INSTALL_PREFIX}/${${PROJECT_NAME}_RUNTIME_OUTPUT_DIRECTORY})
set(EXECUTABLE_INSTALL_OUTPUT_PATH ${CMAKE_INSTALL_PREFIX}/${${PROJECT_NAME}_RUNTIME_OUTPUT_DIRECTORY})

set(${PROJECT_NAME}_VERSION_MAJOR 1 CACHE STRING "CPSTK major version number.")
set(${PROJECT_NAME}_VERSION_MINOR 1 CACHE STRING "CPSTK minor version number.")
set(${PROJECT_NAME}_VERSION_BUILD 0 CACHE STRING "CPSTK build version number.")
set(${PROJECT_NAME}_VERSION
  "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_BUILD}")

mark_as_advanced(${PROJECT_NAME}_VERSION_MAJOR)
mark_as_advanced(${PROJECT_NAME}_VERSION_MINOR)
mark_as_advanced(${PROJECT_NAME}_VERSION_BUILD)

## ###################################################################
## Some packages options are required for this project. Check them now
## ###################################################################

set(_ITKVersionString "${ITK_VERSION_MAJOR}.${ITK_VERSION_MINOR}.${ITK_VERSION_PATCH}" )
math(EXPR _ITKVersionNum "${ITK_VERSION_MAJOR}*100*100 + ${ITK_VERSION_MINOR}*100 + ${ITK_VERSION_PATCH}")
if( _ITKVersionNum LESS 30800 )
  message(SEND_ERROR "The ITK version you want to use (${_ITKVersionString}) is not supported by this project. Please use a more recent version of ITK. The minimum required version is 3.8.0. The recommended version is 3.20.1.")
endif( _ITKVersionNum LESS 30800 )

# -----------------------------------------------------------------------------
# Header file used to pass CMake settings to the source code
# -----------------------------------------------------------------------------

configure_file(${PROJECT_SOURCE_DIR}/cpstkConfigure.h.in
               ${PROJECT_BINARY_DIR}/cpstkConfigure.h CONFIGURE IMMEDIATE)

if(NOT ${PROJECT_NAME}_INSTALL_NO_DEVELOPMENT)
  install(FILES ${CPSTK_BINARY_DIR}/cpstkConfigure.h
      DESTINATION include)
endif(NOT ${PROJECT_NAME}_INSTALL_NO_DEVELOPMENT)

# To help using CPSTK with other projects
configure_file(${CPSTK_SOURCE_DIR}/UseCPSTK.cmake.in
               ${CPSTK_BINARY_DIR}/UseCPSTK.cmake COPYONLY IMMEDIATE)

#-----------------------------------------------------------------------------
# Add compiler flags CPSTK needs to work on this platform.  This must be
# done AFTER the call to CMAKE_EXPORT_BUILD_SETTINGS.
set(CPSTK_REQUIRED_C_FLAGS)
set(CPSTK_REQUIRED_CXX_FLAGS)
SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${CPSTK_REQUIRED_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CPSTK_REQUIRED_CXX_FLAGS}")

#-----------------------------------------------------------------------------
# Set any extra compilation flags here
if(CMAKE_COMPILER_IS_GNUCXX)
  # Add some useful warning flags
  # Note: -Wall and -Wno-deprecated are already pulled in by
  # UseITK.cmake
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wshadow")
endif(CMAKE_COMPILER_IS_GNUCXX)


#-----------------------------------------------------------------------------
# Set the include directories
# also include the binary directories as some source files are built there
#-----------------------------------------------------------------------------
set(${PROJECT_NAME}_INCLUDE_DIRS
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/Numerics
  ${PROJECT_SOURCE_DIR}/Algorithms
  ${PROJECT_SOURCE_DIR}/Commands
  ${PROJECT_SOURCE_DIR}/IPF
  ${PROJECT_BINARY_DIR}
  ${PROJECT_BINARY_DIR}/Commands
)

include_directories(
  ${${PROJECT_NAME}_INCLUDE_DIRS}
)

#-----------------------------------------------------------------------------
# Include the external configuration file
include (${PROJECT_SOURCE_DIR}/CPSTKConfig.cmake)


## #################################################################
## BUILD TYPE
## #################################################################

set(CPSTK_BUILD_TYPE
STATIC
)

if (NOT WIN32)
  if (ITK_BUILD_SHARED)
    set (CPSTK_BUILD_TYPE SHARED)
  endif (ITK_BUILD_SHARED)
endif (NOT WIN32)


## #################################################################
## Actual CMakeLists code
## #################################################################
  
if(NOT ${CPSTK_PACKAGES_MISSING})
  add_subdirectory(IPF)
  add_subdirectory(Algorithms)
  add_subdirectory(Numerics)
  add_subdirectory(Commands)
  
  add_executable(cpstk
    cpstk.cxx
    )
  target_link_libraries(cpstk
    CPSTKCommands
    CPSTKProgramFactory
    ${ITK_LIBRARIES}
    )
else(NOT ${CPSTK_PACKAGES_MISSING})
  message(WARNING "Some packages are missing to completely build ${PROJECT_NAME} : ${CPSTK_MISSING_PACKAGES}. ${PROJECT_NAME} won't be built")  
endif(NOT ${CPSTK_PACKAGES_MISSING})

## #################################################################
## Installation
## #################################################################

if(NOT ${PROJECT_NAME}_INSTALL_NO_RUNTIME)
  if (NOT ${CPSTK_PACKAGES_MISSING})
    install(TARGETS cpstk
      BUNDLE DESTINATION bin
      RUNTIME DESTINATION bin
      )
  endif()
endif()


#-----------------------------------------------------------------------------
# doxygen documentation

find_package(Doxygen)
if (DOXYGEN)
  option( LINK_EXTERNAL_DOC "Should the documentation be linked with external sources such as ITK?" NO )
  set(MY_LINK_EXTERNAL_DOC ${LINK_EXTERNAL_DOC})
  set(MY_DOXYGEN_BUILD_DIR ${PROJECT_BINARY_DIR}/Doxygen)
  
  
  configure_file (
    ${PROJECT_SOURCE_DIR}/Doxygen/doxygen.config.in
    ${MY_DOXYGEN_BUILD_DIR}/doxygen.config
  )

  if (MY_LINK_EXTERNAL_DOC)
    exec_program(${CMAKE_COMMAND} ${MY_DOXYGEN_BUILD_DIR}
      ARGS -E tar xvz ${PROJECT_SOURCE_DIR}/Doxygen/vtkNightlyDoc.tag.tar.gz
    )
    exec_program(${CMAKE_COMMAND} ${MY_DOXYGEN_BUILD_DIR}
      ARGS -E tar xvz ${PROJECT_SOURCE_DIR}/Doxygen/KWWidgetsNightlyDoc.tag.tar.gz
    )
    exec_program(${CMAKE_COMMAND} ${MY_DOXYGEN_BUILD_DIR}
      ARGS -E tar xvz ${PROJECT_SOURCE_DIR}/Doxygen/InsightDoxygen.tag.tar.gz
    )
  else (MY_LINK_EXTERNAL_DOC)
    exec_program(${CMAKE_COMMAND}
      ARGS -E remove ${MY_DOXYGEN_BUILD_DIR}/vtkNightlyDoc.tag
    )
    exec_program(${CMAKE_COMMAND}
      ARGS -E remove ${MY_DOXYGEN_BUILD_DIR}/KWWidgetsNightlyDoc.tag
    )
    exec_program(${CMAKE_COMMAND}
      ARGS -E remove ${MY_DOXYGEN_BUILD_DIR}/InsightDoxygen.tag
    )
  endif (MY_LINK_EXTERNAL_DOC)

  add_custom_target(doc
    ${DOXYGEN}
    ${MY_DOXYGEN_BUILD_DIR}/doxygen.config
  )
endif(DOXYGEN) 

add_subdirectory(Utils)
