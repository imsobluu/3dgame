# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/a/Downloads/3dgame/build-mac/_deps/learnopengl_glad-src")
  file(MAKE_DIRECTORY "/Users/a/Downloads/3dgame/build-mac/_deps/learnopengl_glad-src")
endif()
file(MAKE_DIRECTORY
  "/Users/a/Downloads/3dgame/build-mac/_deps/learnopengl_glad-build"
  "/Users/a/Downloads/3dgame/build-mac/_deps/learnopengl_glad-subbuild/learnopengl_glad-populate-prefix"
  "/Users/a/Downloads/3dgame/build-mac/_deps/learnopengl_glad-subbuild/learnopengl_glad-populate-prefix/tmp"
  "/Users/a/Downloads/3dgame/build-mac/_deps/learnopengl_glad-subbuild/learnopengl_glad-populate-prefix/src/learnopengl_glad-populate-stamp"
  "/Users/a/Downloads/3dgame/build-mac/_deps/learnopengl_glad-subbuild/learnopengl_glad-populate-prefix/src"
  "/Users/a/Downloads/3dgame/build-mac/_deps/learnopengl_glad-subbuild/learnopengl_glad-populate-prefix/src/learnopengl_glad-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/a/Downloads/3dgame/build-mac/_deps/learnopengl_glad-subbuild/learnopengl_glad-populate-prefix/src/learnopengl_glad-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/a/Downloads/3dgame/build-mac/_deps/learnopengl_glad-subbuild/learnopengl_glad-populate-prefix/src/learnopengl_glad-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
