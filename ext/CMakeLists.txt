add_subdirectory(imgui)

#file(GLOB_RECURSE YOJIMBO_SODIUM_SRC CONFIGURE_DEPENDS
#        "${YOJIMBO_SOURCE}/sodium/**.c"
#        "${YOJIMBO_SOURCE}/sodium/**.cpp"
#        "${YOJIMBO_SOURCE}/sodium/**.h")
#
#file(GLOB_RECURSE YOJIMBO_SRC CONFIGURE_DEPENDS
#        "${YOJIMBO_SOURCE}/source/**.cpp"
#        "${YOJIMBO_SOURCE}/include/**.h"
#)

#add_library(yojimbo STATIC
#        ${YOJIMBO_SODIUM_SRC}
#        ${YOJIMBO_SOURCE}/netcode/netcode.c
#        ${YOJIMBO_SOURCE}/netcode/netcode.h
#        ${YOJIMBO_SOURCE}/reliable/reliable.c
#        ${YOJIMBO_SOURCE}/reliable/reliable.h
#        ${YOJIMBO_SOURCE}/tlsf/tlsf.c
#        ${YOJIMBO_SOURCE}/tlsf/tlsf.h
#        ${YOJIMBO_SRC}
#)
#
#target_include_directories(yojimbo PRIVATE
#        ${YOJIMBO_SOURCE}
#        ${YOJIMBO_SOURCE}/netcode
#        ${YOJIMBO_SOURCE}/reliable
#        ${YOJIMBO_SOURCE}/sodium)
#
#target_include_directories(yojimbo PUBLIC
#        ${YOJIMBO_SOURCE}/include
#        ${YOJIMBO_SOURCE}/serialize
#)
#
#if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#    target_compile_definitions(yojimbo PUBLIC YOJIMBO_DEBUG NETCODE_DEBUG RELIABLE_DEBUG)
#else ()
#    target_compile_definitions(yojimbo PUBLIC YOJIMBO_RELEASE NETCODE_RELEASE RELIABLE_RELEASE)
#endif ()