
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=44000)

#add_subdirectory( parser )
add_subdirectory( drivers )

include_directories( ${CMAKE_SOURCE_DIR}/kexi ${CMAKE_SOURCE_DIR}/kexi/kexidb/parser ${KDE4_INCLUDES}  )

ADD_DEFINITIONS( 
-D__KEXIDB__= 
-DYYERROR_VERBOSE=1
)

########### generate parser/lexer files ###############
# as described at http://public.kitware.com/pipermail/cmake/2002-September/003028.html

# Create target for the parser
ADD_CUSTOM_TARGET(parser echo "Creating parser/lexer files")

# Create custom command for flex/lex (note the outputs)
ADD_CUSTOM_COMMAND(
COMMAND ${CMAKE_SOURCE_DIR}/kexi/kexidb/parser/generate_parser_code.sh
TARGET parser
OUTPUTS ${CMAKE_SOURCE_DIR}/kexi/kexidb/parser/sqlparser.h  ${CMAKE_SOURCE_DIR}/kexi/kexidb/parser/sqlparser.cpp ${CMAKE_SOURCE_DIR}/kexi/kexidb/parser/sqlscanner.h ${CMAKE_SOURCE_DIR}/kexi/kexidb/parser/sqlscanner.cpp)

# mark files as generated
SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/kexi/kexidb/parser/sqlparser.h GENERATED)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/kexi/kexidb/parser/sqlparser.cpp GENERATED)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/kexi/kexidb/parser/sqlscanner.h GENERATED)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/kexi/kexidb/parser/sqlscanner.cpp GENERATED)

set(kexidbparser_STAT_SRCS
   parser/sqlscanner.cpp 
   parser/sqlparser.cpp 
   parser/parser.cpp 
   parser/parser_p.cpp )

set(kexidb_LIB_SRCS 
   ${kexidbparser_STAT_SRCS}
   drivermanager.cpp 
   driver.cpp 
   driver_p.cpp 
   connection.cpp 
   keywords.cpp 
   object.cpp 
   field.cpp 
   utils.cpp 
   expression.cpp 
   connectiondata.cpp 
   RecordData.cpp
   fieldlist.cpp 
   tableschema.cpp 
   cursor.cpp 
   transaction.cpp 
   indexschema.cpp 
   queryschema.cpp 
   queryschemaparameter.cpp
   schemadata.cpp 
   global.cpp 
   relationship.cpp 
   roweditbuffer.cpp 
   msghandler.cpp 
   dbobjectnamevalidator.cpp 
   preparedstatement.cpp 
   dbproperties.cpp 
   admin.cpp 
   lookupfieldschema.cpp 
   simplecommandlineapp.cpp
   fieldvalidator.cpp
   alter.cpp
)

kde4_add_library(kexidb SHARED ${kexidb_LIB_SRCS})
target_link_libraries(kexidb 
   ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KPARTS_LIBS} kexiutils )

set_target_properties(kexidb PROPERTIES VERSION ${GENERIC_KOFFICE_LIB_VERSION} SOVERSION ${GENERIC_KOFFICE_LIB_SOVERSION} )

if(MSVC)
   # avoid LNK1169 errors
   set_target_properties(kexidb PROPERTIES LINK_FLAGS /FORCE:MULTIPLE)
endif(MSVC)

install(TARGETS kexidb  ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES  kexidb_driver.desktop  DESTINATION ${SERVICETYPES_INSTALL_DIR})

if(FALSE) # TODO: install when we move to independent place
   install( FILES  connection.h connectiondata.h cursor.h
    driver.h drivermanager.h error.h expression.h field.h fieldlist.h global.h object.h
    schemadata.h tableschema.h queryschema.h queryschemaparameter.h indexschema.h
    relationship.h parser/parser.h parser/sqlparser.h transaction.h preparedstatement.h
    RecordData.h utils.h
    kexidb_export.h kexidb_global.h DESTINATION ${INCLUDE_INSTALL_DIR}/kexidb COMPONENT Devel)
endif(FALSE)
