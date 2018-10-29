find_package(Doxygen)

if (DOXYGEN_FOUND)
  message("-- Found Doxygen")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxygen.conf.in 
                 ${CMAKE_CURRENT_BINARY_DIR}/doxygen.conf
                 @ONLY )
  add_custom_target(doxygen ${DOXYGEN} ${CMAKE_CURRENT_BINARY_DIR}/doxygen.conf
                    WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
  add_custom_target(doc)
  add_dependencies(doc doxygen)
  set_target_properties(doc PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif (DOXYGEN_FOUND)
