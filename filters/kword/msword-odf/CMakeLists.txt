add_subdirectory(wv2)
INCLUDE_DIRECTORIES( ${GLIB2_INCLUDE_DIR} ${LIBGSF_INCLUDE_DIR} ${ICONV_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ../../libmso )

set(mswordodf_import_PART_SRCS
   mswordodfimport.cpp
   document.cpp
   conversion.cpp
   texthandler.cpp
   paragraph.cpp
   tablehandler.cpp
   graphicshandler.cpp )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

kde4_add_plugin(mswordodf_import ${mswordodf_import_PART_SRCS})

target_link_libraries(mswordodf_import kowv2 komain mso ${ZLIB_LIBRARIES})

install(TARGETS mswordodf_import  DESTINATION ${PLUGIN_INSTALL_DIR})

install( FILES  kword_msword-odf_import.desktop  DESTINATION ${SERVICES_INSTALL_DIR})

