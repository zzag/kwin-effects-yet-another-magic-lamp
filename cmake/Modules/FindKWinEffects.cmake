# - Try to find kwineffects
#
# Once done this will define
#  KWINEFFECTS_FOUND - System has kwineffects
#  KWINEFFECTS_INCLUDE_DIRS - The kwineffects include directories
#  KWINEFFECTS_LIBRARIES - The libraries needed to use kwineffects
#
# Also, once done this will define the following targets
#  KWinEffects::KWinEffects
#

find_path (KWINEFFECTS_INCLUDE_DIRS
    NAMES kwineffects.h
)

find_library (KWINEFFECTS_LIBRARY
    NAMES kwineffects
)

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (KWinEffects DEFAULT_MSG
    KWINEFFECTS_LIBRARY KWINEFFECTS_INCLUDE_DIRS
)
mark_as_advanced (KWINEFFECTS_LIBRARY)

set (KWINEFFECTS_LIBRARIES ${KWINEFFECTS_LIBRARY})

if (KWINEFFECTS_FOUND)
    add_library (KWinEffects::KWinEffects UNKNOWN IMPORTED)
    set_target_properties (KWinEffects::KWinEffects PROPERTIES
        IMPORTED_LOCATION "${KWINEFFECTS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${KWINEFFECTS_INCLUDE_DIRS}"
    )
endif ()
