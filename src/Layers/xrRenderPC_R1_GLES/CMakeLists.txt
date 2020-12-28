project(xrRender_GLES)

list(APPEND DIRS
	"."
	"../xrRender"
	"../xrRender/blenders"
	"../xrRenderGLES"
	)


add_dir("${DIRS}")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../xrRender
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Include/xrRender
    ${CMAKE_CURRENT_SOURCE_DIR}/../xrRenderPC_R1
	${CMAKE_CURRENT_SOURCE_DIR}/../../../Externals/luabind
	${CMAKE_CURRENT_SOURCE_DIR}/../../../sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Externals/gli
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Externals/gli/external/glm
    ${SDL_INCLUDE_DIRS}
	)

list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/HW.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/SH_RT.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/stats_manager.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/Texture.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/SH_Texture.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/xr_effgamma.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/ResourceManager_Scripting.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/Light_Render_Direct_ComputeXFS.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/light_gi.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/light_vis.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/light_smapvis.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/r__pixel_calculator.cpp")

#list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/stats_manager.h")
#list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/xr_effgamma.h")

#list(APPEND ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/r__dsgraph_render.cpp")
#list(APPEND ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/Shader.cpp")
#list(APPEND ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRender/R_Backend.cpp")

#list(APPEND     ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../xrRenderPC_R1/FStaticRender.h")

add_definitions(-DXRRENDER_R1_EXPORTS -DUSE_GLES -DNO_XR_VDECLARATOR)
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore xrCDB xrEngine xrParticles xrScriptEngine xrAPI GL GLEW xrLuabind GLESv2 ${LUA_LIBRARIES})
xr_install(${PROJECT_NAME})

