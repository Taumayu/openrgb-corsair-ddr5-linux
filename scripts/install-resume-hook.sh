#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "$0")")"

echo "Installation du hook de reprise de veille pour OpenRGB..."

cp "$SCRIPT_DIR/openrgb-resume.service" /etc/systemd/system/openrgb-resume.service
cp "$SCRIPT_DIR/openrgb-resume.sh" /lib/systemd/system-sleep/openrgb-resume.sh
chmod +x /lib/systemd/system-sleep/openrgb-resume.sh

systemctl daemon-reload
systemctl enable openrgb-resume.service

echo "✓ Service et hook de reprise de veille installés avec succès !"

