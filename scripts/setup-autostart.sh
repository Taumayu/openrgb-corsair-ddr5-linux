#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
CONFIG_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/corsair-dram-rgb"
SYSTEMD_USER_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user"

echo "=== Configuration du démarrage automatique pour Corsair DDR5 ==="

# 1. Dossier de configuration
mkdir -p "$CONFIG_DIR"
if [ ! -f "$CONFIG_DIR/color.conf" ]; then
    echo "cyan" > "$CONFIG_DIR/color.conf"
    echo "✓ Fichier de configuration créé avec la couleur par défaut (cyan)."
fi

# 2. Permissions sur les scripts
chmod +x "$SCRIPT_DIR/corsair-dram-rgb"
chmod +x "$SCRIPT_DIR/set-ram-color"
chmod +x "$SCRIPT_DIR/apply-ram-color-boot.sh"
echo "✓ Permissions d'exécution vérifiées."

# 3. Installation du service systemd utilisateur
mkdir -p "$SYSTEMD_USER_DIR"
cp "$SCRIPT_DIR/corsair-dram-rgb.service" "$SYSTEMD_USER_DIR/corsair-dram-rgb.service"
echo "✓ Service copié dans $SYSTEMD_USER_DIR/corsair-dram-rgb.service"

# 4. Rechargement et activation de systemd
systemctl --user daemon-reload
systemctl --user enable corsair-dram-rgb.service
echo "✓ Service activé au démarrage !"

# 5. Test immédiat
echo "Lancement du service pour test..."
systemctl --user start corsair-dram-rgb.service
echo "✓ Service exécuté avec succès."

echo ""
echo "=== Tout est prêt ! ==="
echo "Tu peux changer la couleur à tout moment avec :"
echo "  $SCRIPT_DIR/set-ram-color <couleur>"

