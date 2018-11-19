find_package (Qt5 REQUIRED COMPONENTS
    Core
    DBus
    Gui
)

find_package (KF5 REQUIRED COMPONENTS
    Config
    ConfigWidgets
    CoreAddons
    WindowSystem
)

find_package (KWinEffects REQUIRED)

add_subdirectory (kcm)

set (effect_SRCS
    plugin.cc
    YetAnotherMagicLampEffect.cc
)

kconfig_add_kcfg_files (effect_SRCS
    YetAnotherMagicLampConfig.kcfgc
)

add_library (kwin4_effect_yaml SHARED ${effect_SRCS})

target_link_libraries (kwin4_effect_yaml
    PUBLIC
        Qt5::Core
        Qt5::Gui
        KF5::ConfigCore
        KF5::ConfigGui
        KF5::CoreAddons
        KF5::WindowSystem
        KWinEffects::KWinEffects
)

install (
    TARGETS
        kwin4_effect_yaml

    DESTINATION
        ${PLUGIN_INSTALL_DIR}/kwin/effects/plugins/
)
