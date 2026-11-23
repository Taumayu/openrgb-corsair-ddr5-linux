#!/bin/sh
# Script exécuté par systemd-suspend au réveil
if [ "$1" = "post" ]; then
    sleep 2
    su tauma -c "/home/tauma/Project/OpenRGB/apply-ram-color-boot.sh"
fi

