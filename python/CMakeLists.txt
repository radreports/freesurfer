project(fspython)

# build the cxx/python bindings for the below modules
add_subdirectories(fsbindings)
if(NOT INTEGRATE_SAMSEG)
  add_subdirectories(gems/cxx)
endif()

# run 'pip install' and direct package installs to freesurfer/python/packages
# pass INTEGRATE_SAMSEG_OPTION=${INTEGRATE_SAMSEG} environment variable for setup.py to act accordingly
install(CODE "
  message(STATUS \"Installing python packages\")
  execute_process(COMMAND bash -c \"INTEGRATE_SAMSEG_OPTION=${INTEGRATE_SAMSEG} ${PYTHON_EXECUTABLE} -m pip install ${CMAKE_CURRENT_SOURCE_DIR} --no-dependencies --upgrade --target=${CMAKE_INSTALL_PREFIX}/python/packages\" RESULT_VARIABLE retcode)
  if(NOT \${retcode} STREQUAL 0)
    message(FATAL_ERROR \"Could not install python modules\")
  endif()"
  COMPONENT fspy
)

# install the fspython script
if(APPLE_ARM64)
   # Allow script to use local packages with the system or Apple recommended Anaconda python
   install(PROGRAMS fspython.local DESTINATION bin RENAME fspython)
else()
   install(PROGRAMS fspython DESTINATION bin)
endif()


# WIP: Custom packages to modify existing install - install over pre-existing package with --ignore-installed and rebuild (--no-cache-dir)
# surfa
set(SURFA_LOCAL_PATH "/absolute/path/to/local/surfa/code")
# ?? ifdef on ENV setting for SURFA_LOCAL_PATH, else use default PKG_TARGET setting for site-packages ??
# set(PKG_TARGET "--target=${CMAKE_INSTALL_PREFIX}/python/packages")

add_custom_target(setup_surfa)

file(GLOB_RECURSE surfa_dists ${CMAKE_INSTALL_PREFIX}/python/packages/surfa-*.dist-info)

foreach(dist ${surfa_dists})
  file(REMOVE_RECURSE ${dist})
endforeach()

# message(STATUS "Installing Surfa from local path: ${SURFA_LOCAL_PATH}")
add_custom_command(
  TARGET setup_surfa 
  COMMAND 
    ${CMAKE_INSTALL_PREFIX}/python/bin/python3 -m pip install --ignore-installed --no-cache-dir --upgrade -e ${SURFA_LOCAL_PATH} ${PKG_TARGET} 
  WORKING_DIRECTORY 
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# default is least version specific or sparse requirements to allow for latest pkg revisions
set(requirements "-r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt")
set(requirements_extra "-r ${CMAKE_CURRENT_SOURCE_DIR}/requirements-extra.txt")
if(FSPYTHON_BUILD_REQ)
   # pip install current snapshot of (hardcoded) package revisions into fspython
   if(NOT APPLE)
      ## all linux OS
      # set(requirements "-r ${CMAKE_CURRENT_SOURCE_DIR}/requirements-linux.txt")
      set(requirements "-r ${CMAKE_CURRENT_SOURCE_DIR}/requirements-build.txt")
      set(requirements_extra "")
   else()
      if(NOT APPLE_ARM64)
         ## darwin_x86_64
         # set(requirements "-r ${CMAKE_CURRENT_SOURCE_DIR}/requirements-darwin-intel.txt")
	 message(STATUS "For HOST_OS=${HOST_OS} using default requirements files")
      else()
         ## darwin_arm64
	 message(STATUS "For HOST_OS=${HOST_OS} - no requirements files currently in use with mininconda darwin_arm64.")
      endif()
   endif()
endif()
if(NOT APPLE_ARM64)
   message(STATUS "For HOST_OS=${HOST_OS} fspython pip set to use requirement arguments: ${requirements} ${requirements_extra}")
endif() 

# install any package requirements
if(INSTALL_PYTHON_DEPENDENCIES)

  if(NOT DISTRIBUTE_FSPYTHON)
    set(PKG_TARGET "--target=${CMAKE_INSTALL_PREFIX}/python/packages")
  endif()

  install(CODE "
    message(STATUS \"Installing python package dependencies into fspython with cmd:\")
    message(STATUS \"${CMAKE_INSTALL_PREFIX}/python/bin/python3 -m pip install --upgrade --disable-pip-version-check ${EXTRA_PACKAGES} ${requirements} ${requirements_extra} ${PKG_TARGET}\")

    execute_process(COMMAND bash -c \"${CMAKE_INSTALL_PREFIX}/python/bin/python3 -m pip install --upgrade --disable-pip-version-check ${EXTRA_PACKAGES} ${requirements} ${requirements_extra} ${PKG_TARGET}\" RESULT_VARIABLE retcode)

    if(NOT \${retcode} STREQUAL 0)
      message(FATAL_ERROR \"Could not install python dependencies\")
    endif()

    execute_process(COMMAND bash -c \"find ${CMAKE_INSTALL_PREFIX}/python/bin ! -name 'python3*' ! -name 'ds' -type f -exec rm -f {} +\" RESULT_VARIABLE retcode)

    if(NOT \${retcode} STREQUAL 0)
      message(FATAL_ERROR \"Could not remove pip-installed scripts\")
    endif()

    # Create the symbolic link after pip install
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink \"${CMAKE_INSTALL_PREFIX}/python/bin/ds\" \"${CMAKE_INSTALL_PREFIX}/bin/ds\")
  ")

  if(PATCH_FSPYTHON)
     # Use to workaround a python module not installing, e.g., problem with setup.py.
     # Enable with -DPATCH_FSPYTHON=ON as arg to cmake command to copy install from previous build.
     # The path to the previous build must be available/mounted in the build container.
     set(FSPYTHON_PREVIOUS_BUILD_BASE "/space/freesurfer/build/garnet/sandboxes/utility/freesurfer/dev.20221207")
     install(CODE "
       message(STATUS \"patching fspython distribution\")
       execute_process(COMMAND bash -c \" ( cd ${FSPYTHON_PREVIOUS_BUILD_BASE}/python/lib/python3.8/site-packages && tar cpf - surfa* xxhash* ) | ( cd ${CMAKE_INSTALL_PREFIX}/python/lib/python3.8/site-packages && tar xpf - ) \" RESULT_VARIABLE patch)
       if(NOT \${patch} STREQUAL 0)
         message(FATAL_ERROR \"Could not patch fspython\")
       endif()"
     )
  endif()

endif()
