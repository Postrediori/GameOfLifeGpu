make_executable()

target_precompile_headers(${PROJECT} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h)

target_include_directories(${PROJECT} PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    ${GLAD_INCLUDE_DIR}
    ${IMGUI_INCLUDE_DIR}
    )

target_link_libraries(${PROJECT}
    ${OPENGL_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${GLAD_LIBRARIES}
    ${IMGUI_LIBRARIES}
    ${PLOG_LIBRARY}
    ${HMM_LIBRARY}
    GraphicsLib
    UtilsLib
    )

# Data files
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/life.vert
    ${CMAKE_CURRENT_BINARY_DIR}/data/life.vert COPYONLY)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/life.frag
    ${CMAKE_CURRENT_BINARY_DIR}/data/life.frag COPYONLY)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/life-init.frag
    ${CMAKE_CURRENT_BINARY_DIR}/data/life-init.frag COPYONLY)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/screen-plane.vert
    ${CMAKE_CURRENT_BINARY_DIR}/data/screen-plane.vert COPYONLY)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/screen-plane.frag
    ${CMAKE_CURRENT_BINARY_DIR}/data/screen-plane.frag COPYONLY)
