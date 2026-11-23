#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
CONFIG_FILE="${XDG_CONFIG_HOME:-$HOME/.config}/corsair-dram-rgb/color.conf"
OPENRGB_DIR="$SCRIPT_DIR/openrgb-1.1"
RAM_BIN="$SCRIPT_DIR/corsair-dram-rgb"
LOG_DIR="${XDG_STATE_HOME:-$HOME/.local/state}"
LOG_FILE="$LOG_DIR/openrgb-boot.log"

mkdir -p "$LOG_DIR" 2>/dev/null || true
echo "[$(date '+%Y-%m-%d %H:%M:%S')] apply-ram-color-boot démarré..." > "$LOG_FILE" 2>/dev/null || true

# 1. Récupérer la couleur configurée
if [ -f "$CONFIG_FILE" ]; then
    COLOR=$(cat "$CONFIG_FILE" | tr -d ' \n\r')
else
    COLOR="50ee25"
fi

HEX_COLOR="${COLOR#"#"}"
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Couleur cible : #$HEX_COLOR" >> "$LOG_FILE" 2>/dev/null || true

# 2. Attendre que le bus i2c-2 soit accessible
for i in $(seq 1 15); do
    if [ -r /dev/i2c-2 ] && [ -w /dev/i2c-2 ]; then
        break
    fi
    sleep 1
done

# 3. Appliquer directement à la RAM via notre pilote SMBus autonome
if [ -x "$RAM_BIN" ]; then
    "$RAM_BIN" "$HEX_COLOR" >> "$LOG_FILE" 2>&1 || true
fi

# 4. Appliquer via OpenRGB aux composants internes (avec réessais pour laisser le GPU et uaccess s'activer)
if [ -f "$OPENRGB_DIR/usr/bin/OpenRGB" ]; then
    export LD_LIBRARY_PATH="$OPENRGB_DIR/usr/lib"

    for attempt in $(seq 1 5); do
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] Tentative OpenRGB #$attempt..." >> "$LOG_FILE" 2>/dev/null || true

        gpu_ok=0
        mb_ok=0

        # Carte Graphique (Gigabyte Radeon RX 9070 GRE)
        if "$OPENRGB_DIR/usr/bin/OpenRGB" -d "Gigabyte Radeon RX 9070 GRE GAMING OC 12G" -m static -c "$HEX_COLOR" >> "$LOG_FILE" 2>&1; then
            gpu_ok=1
        fi

        # Carte Mère (ASRock B850 Pro RS)
        if "$OPENRGB_DIR/usr/bin/OpenRGB" -d "ASRock B850 Pro RS" -m static -c "$HEX_COLOR" >> "$LOG_FILE" 2>&1; then
            mb_ok=1
        fi

        # RAM Corsair DDR5 via OpenRGB
        "$OPENRGB_DIR/usr/bin/OpenRGB" -d "Corsair Vengeance RGB DDR5" -m direct -c "$HEX_COLOR" >> "$LOG_FILE" 2>&1 || true

        # Si le GPU et la Carte Mère ont été appliqués avec succès, on s'arrête
        if [ "$gpu_ok" -eq 1 ] && [ "$mb_ok" -eq 1 ]; then
            echo "[$(date '+%Y-%m-%d %H:%M:%S')] Succès complet pour tous les composants !" >> "$LOG_FILE" 2>/dev/null || true
            break
        fi

        sleep 2
    done
fi

echo "[$(date '+%Y-%m-%d %H:%M:%S')] Fin du script." >> "$LOG_FILE" 2>/dev/null || true
