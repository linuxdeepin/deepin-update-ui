#!/bin/bash

display_daemon="/usr/lib/deepin-daemon/greeter-display-daemon"
if [ -x "$display_daemon" ]; then
    "$display_daemon" &
    xsettingsd_conf="/etc/lightdm/deepin/xsettingsd.conf"
    if [ -e "$xsettingsd_conf" ]; then
        exec xsettingsd -c "$xsettingsd_conf"
    else
        trap 'exit 0' TERM INT
        wait 2>/dev/null || true
    fi
fi
