#insmod /mnt/weston/mali_kbase.ko
#insmod /mnt/weston/sstar_drm.ko
export QT_ROOT=/customer/weston
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$QT_ROOT/lib
export QT_QPA_PLATFORM_PLUGIN_PATH=$QT_ROOT/lib/qt/plugins/platforms
export QT_QPA_FONTDIR=$QT_ROOT/lib/qt/fonts
export FONTCONFIG_PATH=$QT_ROOT/lib/qt/fonts
export QT_DEBUG_PLUGINS=1
export QML_IMPORT_PATH=$QT_ROOT/qml
export QML2_IMPORT_PATH=$QT_ROOT/qml
export XDG_RUNTIME_DIR=/tmp/weston
export XDG_CONFIG_HOME=/customer/weston
export QT_QPA_PLATFORM="wayland-egl"
export QT_QPA_EGLFS_KMS_ATOMIC=1
export QT_QPA_EGLFS_INTEGRATION="eglfs_kms"
mkdir /tmp/udev
mkdir /tmp/weston
chmod 700 /tmp/weston
./S10udev start
./bin/weston --backend=drm-backend.so --tty=1 --drm-device=card0 --debug &
