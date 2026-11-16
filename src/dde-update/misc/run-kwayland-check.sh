#!/bin/bash
# from /usr/bin/kwin_wayland_helper

dde-dconfig --get -a org.deepin.dde.lightdm-deepin-greeter -r org.deepin.dde.lightdm-deepin-greeter -k defaultGreeterSession | grep "wayland"
if [ $? -eq 0 ];then
    export KWIN_COMPOSE=O2ES
fi

export GDK_BACKEND=x11
export KWIN_USE_BUFFER_AGE=1

if [ ! -f "$HOME/.config/kglobalshortcutsrc" ]; then
    cp -n /etc/xdg/kglobalshortcutsrc $HOME/.config/kglobalshortcutsrc
fi

export QT_LOGGING_RULES="kwin*=true;kwin_libinput=false;"
# TODO 调整缩放系数并注销后需要通过scale-factor计算xft-dpi值，后续需要定位部分设备和Qt版本无法通过Qt获取正确DPI的问题
#CALCULATE_DPI_FROM_X=$(echo "scale=0;`gsettings get com.deepin.xsettings scale-factor` * 96 / 1" | bc)
#if [[ -n "$CALCULATE_DPI_FROM_X" ]]; then
#        export QT_WAYLAND_FORCE_DPI="$CALCULATE_DPI_FROM_X"
#fi
export QT_MESSAGE_PATTERN="[%{time yyyy-MM-dd hh:mm:ss.zzz}] %{function}:%{line} - %{message}"

if [ -f "$HOME/.config/locale.conf" ]; then
    source $HOME/.config/locale.conf
fi
export QT_SCALE_FACTOR=$(gsettings get com.deepin.xsettings scale-factor)
export QT_QPA_PLATFORM=wayland
export QT_QPA_PLATFORM_PLUGIN_PATH=/usr/plugins/platforms
export QT_PLUGIN_PATH=/usr/lib/x86_64-linux-gnu/qt5/plugins
export QML2_IMPORT_PATH=/usr/lib/x86_64-linux-gnu/qt5/qml
export XDG_DATA_DIRS=/usr/share
export XDG_CURRENT_DESKTOP=Deepin
export QT_DEBUG_PLUGINS=1

#崩溃后重新登录会冲掉崩溃时候的kwin日志，此处加入一次拷贝，保证可以获取到kwin崩溃时候的那次日志
mv $HOME/.kwin.log $HOME/.kwin-old.log
# wrapper 同步显示环境；显示服务由 launch-dde-update 在 KWin 就绪后启动。
/usr/bin/kwin_wayland_wrapper --exit-with-session=/usr/bin/launch-dde-update -platform wayland-org.kde.kwin.qpa --xwayland --drm --no-lockscreen 1> $HOME/.kwin.log 2>&1
