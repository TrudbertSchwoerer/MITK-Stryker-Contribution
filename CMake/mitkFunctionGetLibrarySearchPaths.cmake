function(mitkFunctionGetLibrarySearchPaths search_path intermediate_dir)

  set(_dir_candidates ${MITK_VTK_LIBRARY_DIRS} ${MITK_ITK_LIBRARY_DIRS}
                      "${MITK_BINARY_DIR}/bin" "${MITK_BINARY_DIR}/bin/plugins")

  # Determine the Qt4/5 library installation prefix
  set(_qmake_location )
  if(MITK_USE_Qt4)
    set(_qmake_location ${QT_QMAKE_EXECUTABLE})
  elseif(MITK_USE_Qt5 AND TARGET ${Qt5Core_QMAKE_EXECUTABLE})
    get_property(_qmake_location TARGET ${Qt5Core_QMAKE_EXECUTABLE}
                 PROPERTY IMPORT_LOCATION)
  endif()
  if(_qmake_location)
    if(NOT _qt_install_libs)
      if(WIN32)
        execute_process(COMMAND ${_qmake_location} -query QT_INSTALL_BINS
                        OUTPUT_VARIABLE _qt_install_libs
                        OUTPUT_STRIP_TRAILING_WHITESPACE)
      else()
        execute_process(COMMAND ${_qmake_location} -query QT_INSTALL_LIBS
                        OUTPUT_VARIABLE _qt_install_libs
                        OUTPUT_STRIP_TRAILING_WHITESPACE)
      endif()
      file(TO_CMAKE_PATH "${_qt_install_libs}" _qt_install_libs)
      set(_qt_install_libs ${_qt_install_libs} CACHE INTERNAL "Qt library installation prefix" FORCE)
    endif()
    if(_qt_install_libs)
      list(APPEND _dir_candidates ${_qt_install_libs})
    endif()
  elseif(MITK_USE_QT)
    message(WARNING "The qmake executable could not be found.")
  endif()

  get_property(_additional_paths GLOBAL PROPERTY MITK_ADDITIONAL_LIBRARY_SEARCH_PATHS)
  if(_additional_paths)
    list(APPEND _dir_candidates ${_additional_paths})
  endif()

  if(VTK_DIR)
    find_package(VTK QUIET)
    if(VTK_RUNTIME_LIBRARY_DIRS)
      list(APPEND _dir_candidates ${VTK_RUNTIME_LIBRARY_DIRS})
    endif()
  endif()


  # The code below is sub-optimal. It makes assumptions about
  # the structure of the build directories, pointed to by
  # the *_DIR variables. Instead, we should rely on package
  # specific "LIBRARY_DIRS" variables, if they exist.

  if(WIN32)
    if(DCMTK_DIR)
      list(APPEND _dir_candidates "${DCMTK_DIR}/bin")
    endif()
    if(OpenCV_DIR)
      list(APPEND _dir_candidates "${OpenCV_DIR}/bin")
    endif()
    if(SOFA_DIR)
      list(APPEND _dir_candidates "${SOFA_DIR}/bin")
    if(Python_DIR)
      list(APPEND _dir_candidates "${Python_DIR}/bin")
    endif()
    if(SimpleITK_DIR)
      list(APPEND _dir_candidates "${SimpleITK_DIR}/bin")
    endif()
    list(APPEND _dir_candidates "${ITK_DIR}/bin")
  else()
    if(DCMTK_DIR)
      list(APPEND _dir_candidates "${DCMTK_DIR}/lib")
    endif()
    if(OpenCV_DIR)
      list(APPEND _dir_candidates "${OpenCV_DIR}/lib")
    endif()
    if(SOFA_DIR)
      list(APPEND _dir_candidates "${SOFA_DIR}/lib")
    if(Python_DIR)
      list(APPEND _dir_candidates "${Python_DIR}/lib")
    endif()
    if(SimpleITK_DIR)
      list(APPEND _dir_candidates "${SimpleITK_DIR}/lib")
    endif()
    list(APPEND _dir_candidates "${ITK_DIR}/lib")
  endif()

  if(MITK_USE_Python AND CTK_PYTHONQT_INSTALL_DIR)
    list(APPEND _dir_candidates "${CTK_PYTHONQT_INSTALL_DIR}/bin")
  endif()
  if(MITK_USE_Boost AND MITK_USE_Boost_LIBRARIES AND NOT MITK_USE_SYSTEM_Boost)
    list(APPEND _dir_candidates "${Boost_LIBRARY_DIR}")
  endif()
  if(ACVD_DIR)
    list(APPEND _dir_candidates "${ACVD_DIR}/bin")
  endif()
  if(ANN_DIR)
    list(APPEND _dir_candidates "${ANN_DIR}")
  endif()
  if(CppUnit_DIR)
    list(APPEND _dir_candidates "${CppUnit_DIR}")
  endif()
  if(GLUT_DIR)
    list(APPEND _dir_candidates "${GLUT_DIR}")
  endif()
  if(GDCM_DIR)
    list(APPEND _dir_candidates "${GDCM_DIR}/bin")
  endif()
  if(GLEW_DIR)
    list(APPEND _dir_candidates "${GLEW_DIR}")
  endif()
  if(tinyxml_DIR)
    list(APPEND _dir_candidates "${tinyxml_DIR}")
  endif()
  if(Poco_DIR)
    list(APPEND _dir_candidates "${Poco_DIR}/lib")
  endif()
  if(Qwt_DIR)
    list(APPEND _dir_candidates "${Qwt_DIR}")
  endif()
  if(Qxt_DIR)
    list(APPEND _dir_candidates "${Qxt_DIR}")
  endif()
  if(MITK_USE_TOF_PMDO3 OR MITK_USE_TOF_PMDCAMCUBE OR MITK_USE_TOF_PMDCAMBOARD)
    list(APPEND _dir_candidates "${MITK_PMD_SDK_DIR}/plugins" "${MITK_PMD_SDK_DIR}/bin")
  endif()

  if(MITK_USE_CTK)
    list(APPEND _dir_candidates "${CTK_LIBRARY_DIRS}")
    foreach(_ctk_library ${CTK_LIBRARIES})
      if(${_ctk_library}_LIBRARY_DIRS)
        list(APPEND _dir_candidates "${${_ctk_library}_LIBRARY_DIRS}")
      endif()
    endforeach()
  endif()

  if(MITK_USE_BLUEBERRY)
    if(DEFINED CTK_PLUGIN_RUNTIME_OUTPUT_DIRECTORY)
      if(IS_ABSOLUTE "${CTK_PLUGIN_RUNTIME_OUTPUT_DIRECTORY}")
        list(APPEND _dir_candidates "${CTK_PLUGIN_RUNTIME_OUTPUT_DIRECTORY}")
      else()
        list(APPEND _dir_candidates "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CTK_PLUGIN_RUNTIME_OUTPUT_DIRECTORY}")
      endif()
    endif()
  endif()

  if(MITK_LIBRARY_DIRS)
    list(APPEND _dir_candidates ${MITK_LIBRARY_DIRS})
  endif()

  list(REMOVE_DUPLICATES _dir_candidates)

  set(_search_dirs )
  foreach(_dir ${_dir_candidates})
    if(EXISTS "${_dir}/${intermediate_dir}")
      list(APPEND _search_dirs "${_dir}/${intermediate_dir}")
    else()
      list(APPEND _search_dirs "${_dir}")
    endif()
  endforeach()

  # Special handling for "internal" search dirs. The intermediate directory
  # might not have been created yet, so we can't check for its existence.
  # Hence we just add it for Windows without checking.
  set(_internal_search_dirs "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plugins")
  if(WIN32)
    foreach(_dir ${_internal_search_dirs})
      set(_search_dirs "${_dir}/${intermediate_dir}" ${_search_dirs})
    endforeach()
  else()
    set(_search_dirs ${_internal_search_dirs} ${_search_dirs})
  endif()
  list(REMOVE_DUPLICATES _search_dirs)

  set(${search_path} ${_search_dirs} PARENT_SCOPE)
endfunction()
