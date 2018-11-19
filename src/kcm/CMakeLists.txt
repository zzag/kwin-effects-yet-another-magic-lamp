set (kcm_SRCS
    YetAnotherMagicLampEffectKCM.cc
    plugin.cc
)

kconfig_add_kcfg_files (kcm_SRCS
    ../YetAnotherMagicLampConfig.kcfgc
)

include (PkgConfigGetVar)
pkgconfig_getvar (dbus-1 interfaces_dir INTERFACES_DIR)
qt5_add_dbus_interface (kcm_SRCS ${INTERFACES_DIR}/org.kde.kwin.Effects.xml kwineffects_interface)
qt5_wrap_ui (kcm_SRCS YetAnotherMagicLampEffectKCM.ui)

add_library (kwin_yaml_config MODULE ${kcm_SRCS})

target_link_libraries (kwin_yaml_config
    Qt5::Core
    Qt5::DBus
    Qt5::Gui
    KF5::ConfigCore
    KF5::ConfigGui
    KF5::ConfigWidgets
)

install (
    TARGETS
        kwin_yaml_config

    DESTINATION
        ${PLUGIN_INSTALL_DIR}/kwin/effects/configs
)
