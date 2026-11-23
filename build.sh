#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
OPENRGB_SRC="${1:-$HOME/Project/OpenRGB}"

if [ ! -d "$OPENRGB_SRC" ]; then
    echo "Error: OpenRGB source directory not found at $OPENRGB_SRC"
    echo "Usage: ./build.sh [path_to_openrgb_source]"
    exit 1
fi

mkdir -p "$SCRIPT_DIR/bin"
echo "Compiling corsair-dram-rgb using headers and controllers from $OPENRGB_SRC..."

g++ -std=c++17 -O2 \
  -I"$OPENRGB_SRC" \
  -I"$OPENRGB_SRC/i2c_smbus" \
  -I"$OPENRGB_SRC/dependencies/CRCpp" \
  -I"$OPENRGB_SRC/dependencies/json" \
  -I"$OPENRGB_SRC/RGBController" \
  -I"$OPENRGB_SRC/pci_ids" \
  -I"$OPENRGB_SRC/serial_port" \
  -I"$OPENRGB_SRC/hidapi_wrapper" \
  -I"$OPENRGB_SRC/dependencies/hidapi-hotplug-win/include" \
  -I"$OPENRGB_SRC/SPDAccessor" \
  "$SCRIPT_DIR/src/corsair_dram_rgb.cpp" \
  "$OPENRGB_SRC/Controllers/CorsairDRAMController/CorsairDRAMController.cpp" \
  "$OPENRGB_SRC/Controllers/CorsairDRAMController/CorsairDRAMDevices.cpp" \
  "$OPENRGB_SRC/Controllers/CorsairDRAMController/CorsairDRAMControllerDetect.cpp" \
  "$OPENRGB_SRC/Controllers/CorsairDRAMController/RGBController_CorsairDRAM.cpp" \
  "$OPENRGB_SRC/RGBController/RGBController.cpp" \
  "$OPENRGB_SRC/RGBController/RGBControllerKeyNames.cpp" \
  "$OPENRGB_SRC/StringUtils.cpp" \
  "$OPENRGB_SRC/i2c_smbus/i2c_smbus.cpp" \
  -o "$SCRIPT_DIR/bin/corsair-dram-rgb"

echo "✓ Successfully built: bin/corsair-dram-rgb"
