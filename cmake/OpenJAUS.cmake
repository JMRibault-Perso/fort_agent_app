# cmake/OpenJAUS.cmake
# Defines imported targets for OpenJAUS SDK
# Links statically on aarch64, dynamically elsewhere

function(_oj_collect_includes out_var)
    set(_incs "")
    foreach(dir IN LISTS ARGN)
        if(EXISTS "${dir}")
            list(APPEND _incs "${dir}")
        endif()
    endforeach()
    set(${out_var} "${_incs}" PARENT_SCOPE)
endfunction()

# Detect umbrella include dir
set(OJ_TOP_INCLUDE "${OJ_SDK_ROOT}/include")
if(NOT EXISTS "${OJ_TOP_INCLUDE}/openjaus/Components/Base.h")
    set(OJ_TOP_INCLUDE "${OJ_SDK_LIB_DIR}/../include")
endif()

# Determine link type and file extension
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(OJ_LINK_TYPE STATIC)
    set(OJ_LIB_SUFFIX ".a")
else()
    set(OJ_LINK_TYPE SHARED)
    set(OJ_LIB_SUFFIX ".so")
endif()

# ---------------------------------------------------------------------------
# Core
# ---------------------------------------------------------------------------
_oj_collect_includes(_core_includes
    "${OJ_SDK_ROOT}/org.openjaus.core-v1.1.cpp/include"
    "${OJ_SDK_ROOT}/org.openjaus.core-v1.1.cpp/generated/include"
    "${OJ_TOP_INCLUDE}"
)
find_library(OJ_CORE_LIB
    NAMES "libopenjaus-core_v1_1${OJ_LIB_SUFFIX}"
    PATHS "${OJ_SDK_LIB_DIR}/core-v1.1" "${OJ_SDK_LIB_DIR}/lib"
    NO_DEFAULT_PATH REQUIRED
)
add_library(OpenJAUS::core ${OJ_LINK_TYPE} IMPORTED)
set_target_properties(OpenJAUS::core PROPERTIES
    IMPORTED_LOCATION "${OJ_CORE_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${_core_includes}"
)

# ---------------------------------------------------------------------------
# Mobility
# ---------------------------------------------------------------------------
_oj_collect_includes(_mob_includes
    "${OJ_SDK_ROOT}/org.openjaus.mobility-v1.1.cpp/include"
    "${OJ_SDK_ROOT}/org.openjaus.mobility-v1.1.cpp/generated/include"
    "${OJ_TOP_INCLUDE}"
)
find_library(OJ_MOB_LIB
    NAMES "libopenjaus-mobility_v1_1${OJ_LIB_SUFFIX}"
    PATHS "${OJ_SDK_LIB_DIR}/mob-v1.1" "${OJ_SDK_LIB_DIR}/lib"
    NO_DEFAULT_PATH REQUIRED
)
add_library(OpenJAUS::mobility ${OJ_LINK_TYPE} IMPORTED)
set_target_properties(OpenJAUS::mobility PROPERTIES
    IMPORTED_LOCATION "${OJ_MOB_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${_mob_includes}"
)

# ---------------------------------------------------------------------------
# Base
# ---------------------------------------------------------------------------
_oj_collect_includes(_base_includes
    "${OJ_SDK_ROOT}/org.openjaus.cpp/include"
    "${OJ_TOP_INCLUDE}"
)
find_library(OJ_BASE_LIB
    NAMES "libopenjaus${OJ_LIB_SUFFIX}"
    PATHS "${OJ_SDK_LIB_DIR}/openjaus" "${OJ_SDK_LIB_DIR}/lib"
    NO_DEFAULT_PATH REQUIRED
)
add_library(OpenJAUS::base ${OJ_LINK_TYPE} IMPORTED)
set_target_properties(OpenJAUS::base PROPERTIES
    IMPORTED_LOCATION "${OJ_BASE_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${_base_includes}"
)