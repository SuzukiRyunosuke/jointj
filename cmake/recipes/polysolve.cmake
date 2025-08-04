# PolySolve (https://github.com/polyfem/polysolve)
# License: MIT

if(TARGET polysolve)
    return()
endif()

message(STATUS "Third-party: creating target 'polysolve'")

# TODO: this requires a conflicting version of Eigen. Reenable when Eigen 3.4+ is available.
set(POLYSOLVE_WITH_ACCELERATE OFF CACHE BOOL "Enable Apple Accelerate" FORCE)

include(CPM)

# @kui8shi
# We use the full version of CPMAddPackage to specify polysolve to:
# 1. ease the restrictions of linking order by building shared libraries (*.so)
# 2. forcibly rewrite two variables: `HAVE_BLAS_NO_UNDERSCORE` and `HAVE_BLAS_UNDERSCORE`,
#    which are the cause of 'undefined reference' error, by patching polysolve's suitesparse.cmake
CPMAddPackage(
  NAME polysolve
  GITHUB_REPOSITORY polyfem/polysolve
  GIT_TAG a9b49b825fb5ac12c7c227a450869064268b2594
  PATCHES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/polysolve.patch
  OPTIONS
    "BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS}"
)
# When you revert to the original short-handed version of CPMAddPackage, use this instead.
# CPMAddPackage("gh:polyfem/polysolve#a9b49b825fb5ac12c7c227a450869064268b2594")
