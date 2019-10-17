#
# Headers
#
# argos3/plugins/simulator/visualizations/qt-opengl
set(ARGOS3_HEADERS_PLUGINS_SIMULATOR_VISUALIZATIONS_WEBGL
  webgl_render.h
  controllers_container.h
  ring_buffer.h
  message.h
  models/webgl_box.h
  models/webgl_cylinder.h
  models/webgl_prototype.h
  websocket_server.h)

#
# Source files
#
# Common for all builds
set(ARGOS3_SOURCES_PLUGINS_SIMULATOR_VISUALIZATIONS_WEBGL
  ${ARGOS3_HEADERS_PLUGINS_SIMULATOR_VISUALIZATIONS_WEBGL}
  webgl_render.cpp
  message.cpp
  models/webgl_box.cpp
  models/webgl_cylinder.cpp
  models/webgl_prototype.cpp
  websocket_server.cpp)

#
# Create WebGL visualization
#
add_library(argos3plugin_${ARGOS_BUILD_FOR}_webgl SHARED
  ${ARGOS3_SOURCES_PLUGINS_SIMULATOR_VISUALIZATIONS_WEBGL})
target_link_libraries(argos3plugin_${ARGOS_BUILD_FOR}_webgl argos3plugin_${ARGOS_BUILD_FOR}_entities ${LWS_LIBRARY})

install(DIRECTORY www DESTINATION include/argos3/plugins/simulator/visualizations/webgl)
install(FILES ${ARGOS3_HEADERS_PLUGINS_SIMULATOR_VISUALIZATIONS_WEBGL}        DESTINATION include/argos3/plugins/simulator/visualizations/webgl)
install(TARGETS argos3plugin_${ARGOS_BUILD_FOR}_webgl
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib/argos3
  ARCHIVE DESTINATION lib/argos3
)
