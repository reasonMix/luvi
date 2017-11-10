add_library(cctealib
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/lua-cctea.c
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/xxtea.c
)

set_target_properties(cctealib PROPERTIES
    COMPILE_FLAGS "-DLUA_LIB -DLUA_COMPAT_APIINTCASTS -DVERSION=\\\"2.8.0\\\"")

set(EXTRA_LIBS ${EXTRA_LIBS} cctealib)
