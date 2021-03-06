cmake_minimum_required(VERSION 2.6)

project(gekko)

# silence some spam
add_definitions(-Wno-attributes)
add_definitions(-DSINGLETHREADED)

# dependency checking
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMakeTests)
include(FindSDL2 REQUIRED)
include(FindOpenGL REQUIRED)
include(FindGLFW REQUIRED)
include(FindX11 REQUIRED)

if(NOT GLFW_FOUND)
    message(FATAL_ERROR "glfw not found")
else()
    include_directories(${GLFW_INCLUDE_DIRS})
endif()

option(DISABLE_QT4 "Disable Qt4 GUI" OFF)
if(NOT DISABLE_QT4)
    include(FindQt4)
    find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL)

    if(QT4_FOUND AND QT_QTCORE_FOUND AND QT_QTGUI_FOUND AND QT_QTOPENGL_FOUND)
        include(${QT_USE_FILE})
        include_directories(${QT_INCLUDES})
        include_directories(externals/qhexedit)
    else()
        message("Qt4 libraries not found! Disabling Qt4 GUI")
    endif()
endif()

# external includes
include_directories(${SDL2_INCLUDE_DIR})
include_directories(externals/rapidxml-1.13)
include_directories(${OPENGL_INCLUDE_DIR})
include_directories(externals/glm-0.9.3.1)
    
# internal includes
include_directories(src/common/src)
include_directories(src/core/src)
include_directories(src/video_core/src)
include_directories(src/input_common/src)
include_directories(src/win32/src)
include_directories(src/gekko/src)

# process subdirectories
if(QT4_FOUND AND QT_QTCORE_FOUND AND QT_QTGUI_FOUND AND QT_QTOPENGL_FOUND AND NOT DISABLE_QT4)
    add_subdirectory(externals/qhexedit)
endif()
add_subdirectory(src)
