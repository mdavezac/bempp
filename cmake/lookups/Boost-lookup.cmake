set(toolset "")
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set(toolset "toolset=intel-linux")
    file(WRITE "${EXTERNAL_ROOT}/src/user-config.jam"
      "using intel : : \"${CMAKE_CXX_COMPILER}\" ; \n"
    )
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(toolset "toolset=gcc")
    file(WRITE "${EXTERNAL_ROOT}/src/user-config.jam"
      "using gcc : : \"${CMAKE_CXX_COMPILER}\" ; \n"
    )
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(toolset "toolset=clang")
    file(WRITE "${EXTERNAL_ROOT}/src/user-config.jam"
      "using clang : : \"${CMAKE_CXX_COMPILER}\" ; \n"
    )
else()
  message(FATAL_ERROR
     "Unknown compiler ${CMAKE_CXX_COMPILER_ID}."
     "Please install boost manually."
  )
endif()

include(PatchScript)
set(patchdir "${PROJECT_SOURCE_DIR}/cmake/patches/boost")
create_patch_script(Boost patch_script
    CMDLINE "-p0"
    WORKING_DIRECTORY "${EXTERNAL_ROOT}/src/Boost"
    "${patchdir}/unittests_noncopyable.patch"
)

file(WRITE "${PROJECT_BINARY_DIR}/CMakeFiles/external/boost_configure.sh"
    "#!${bash_EXECUTABLE}\n"
    "userjam=\"${EXTERNAL_ROOT}/src/user-config.jam\"\n"
    "[ -e $userjam ] && cp $userjam tools/build/v2\n"
    "\n"
    "./b2 ${toolset} link=static variant=release --with-test \\\n"
    "    cxxflags=\"${CMAKE_CXX_FLAGS}\"\n"
)
set(configure_command "${EXTERNAL_ROOT}/src/boost_configure.sh")
file(COPY "${PROJECT_BINARY_DIR}/CMakeFiles/external/boost_configure.sh"
    DESTINATION "${EXTERNAL_ROOT}/src"
    FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
)

find_program(PATCH_EXECUTABLE patch REQUIRED)
ExternalProject_Add(
    Boost
    PREFIX ${EXTERNAL_ROOT}
    # Downloads boost from url -- much faster than svn
    URL http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.bz2/download
    URL_MD5 d6eef4b4cacb2183f2bf265a5a03a354
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ./bootstrap.sh
    PATCH_COMMAND ${patch_script}
    BUILD_COMMAND ${configure_command}
    INSTALL_COMMAND ./b2 ${toolset} link=static variant=release --with-test
        --prefix=${EXTERNAL_ROOT} install
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
)
# Rerun cmake to capture new boost install
add_recursive_cmake_step(Boost DEPENDEES install)
set(BOOST_ROOT "${EXTERNAL_ROOT}" CACHE INTERNAL "Prefix for Boost install")
# Makes sure those are not in the CACHE, otherwise, new version will not be found
unset(Boost_INCLUDE_DIR CACHE)
unset(Boost_LIBRARY_DIR CACHE)
