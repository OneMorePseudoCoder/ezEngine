ez_cmake_init()



ez_requires(EZ_3RDPARTY_DUKTAPE_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ez_create_target(LIBRARY ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Core
  RendererCore
  GameEngine
)
