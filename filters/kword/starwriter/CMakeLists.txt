




########### next target ###############

set(starwriterimport_PART_SRCS pole.cpp starwriterimport.cc )


kde4_add_plugin(starwriterimport ${starwriterimport_PART_SRCS})



target_link_libraries(starwriterimport  ${KDE4_KDECORE_LIBS} )

install(TARGETS starwriterimport  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES  kword_starwriter_import.desktop  DESTINATION ${SERVICES_INSTALL_DIR})


