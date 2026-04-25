cmake_minimum_required(VERSION 3.14)

project(Blackhole)

include(FetchContent)

# Download imgui core files
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.86
)
FetchContent_MakeAvailable(imgui)

# Download stb_image.h into src/
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/src/stb_image.h")
  file(DOWNLOAD
    "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"
    "${PROJECT_SOURCE_DIR}/src/stb_image.h"
    TLS_VERIFY ON)
endif()

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED HINTS /opt/homebrew/opt/glfw)
find_package(GLEW REQUIRED HINTS /opt/homebrew/opt/glew)
find_package(glm REQUIRED HINTS /opt/homebrew/opt/glm)

file(GLOB SRC_FILES
  "${PROJECT_SOURCE_DIR}/src/*.h"
  "${PROJECT_SOURCE_DIR}/src/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/*.c"
  "${PROJECT_SOURCE_DIR}/src/*.cc")

# imgui core sources
set(IMGUI_SOURCES
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

add_executable(${CMAKE_PROJECT_NAME} ${SRC_FILES} ${IMGUI_SOURCES})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
  ${imgui_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OpenGL::GL)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GLEW::GLEW)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glm::glm)

target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE GLM_ENABLE_EXPERIMENTAL)

add_custom_command(
  TARGET ${CMAKE_PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/assets"
          "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/assets")

add_custom_command(
  TARGET ${CMAKE_PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/shader/"
          "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/shader")
