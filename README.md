# Corsair Vengeance RGB DDR5 & OpenRGB Linux Controller

Contrôleur et automatisation de l'éclairage RGB sous Linux pour la RAM **Corsair Vengeance RGB DDR5** (`CMH16GX5M1E6000Z36`), la carte mère **ASRock B850 Pro RS** et le GPU **Gigabyte Radeon RX 9070 GRE** via SMBus et OpenRGB.

## 🚀 Fonctionnalités

- **Pilote autonome Corsair DDR5** : Contrôle direct du microcontrôleur RGB Corsair sur le bus SMBus AMD (`/dev/i2c-2`, adresse `0x19`, protocole v4) sans interface lourde.
- **Synchronisation multi-composants** : Pilote simultanément la RAM, la Carte Mère (ASRock Polychrome USB `26ce:01a2`) et le GPU (Gigabyte Radeon RX 9070 GRE).
- **Application au démarrage automatique (Boot)** : Service systemd utilisateur couplé à un autostart KDE Plasma (`X-KDE-autostart-phase=2`) et une boucle de résilience pour garantir l'application une fois le serveur d'affichage (Wayland/X11) prêt.
- **Gestion de la reprise de veille (Resume/Suspend)** : Service systemd (`openrgb-resume.service`) et hook `/lib/systemd/system-sleep/` avec délai de temporisation de 2s pour empêcher la carte graphique de repasser en arc-en-ciel au réveil.
- **Commande en une ligne** : Outil interactif `set-ram-color` qui applique et mémorise la couleur immédiatement.

---

## 💻 Matériel ciblé

- **RAM** : Corsair Vengeance RGB DDR5 16GB (`CMH16GX5M1E6000Z36`)
- **Carte mère** : ASRock B850 Pro RS (AM5, contrôleur SMBus PIIX4 `1022:790B`)
- **Carte graphique** : Gigabyte Radeon RX 9070 GRE GAMING OC 12G
- **Souris** : Logitech G203 Prodigy (`046d:c084`)

---

## 🛠️ Utilisation rapide

Changer la couleur de tout le système et l'enregistrer pour le démarrage :

```bash
# Code hexadécimal
set-ram-color "#20ff15"
set-ram-color "#95ca72"
set-ram-color "#50ee25"

# Noms de couleurs
set-ram-color cyan
set-ram-color red
set-ram-color blue

# Valeurs RGB (0-255)
set-ram-color 32 255 21

# Éteindre toutes les LEDs
set-ram-color off

# Afficher l'état actuel et la RAM détectée
set-ram-color
```

---

## 📦 Structure du projet

```
├── autostart/
│   └── openrgb-apply.desktop      # Déclencheur de session KDE Plasma
├── build.sh                       # Script de compilation C++ du pilote autonome
├── scripts/
│   ├── apply-ram-color-boot.sh    # Script exécuté au boot avec réessais tolérants
│   ├── install-resume-hook.sh     # Installe le service de réveil après veille
│   ├── set-ram-color              # Outil utilisateur CLI
│   └── setup-autostart.sh         # Installation des services systemd utilisateur
├── src/
│   └── corsair_dram_rgb.cpp       # Code source C++ du pilote autonome Corsair DDR5
├── systemd/
│   ├── corsair-dram-rgb.service   # Service systemd utilisateur au démarrage
│   ├── openrgb-resume.service     # Service systemd système à la reprise de veille
│   └── openrgb-resume.sh          # Hook direct systemd-suspend
└── udev/
    └── 60-openrgb.rules           # Règles udev pour l'accès I2C/SMBus et périphériques USB
```

---

## ⚙️ Installation

1. **Rendre les scripts exécutables** :
   ```bash
   chmod +x scripts/* systemd/*.sh build.sh
   ```

2. **Installer les règles udev (accès matériel sans root)** :
   ```bash
   sudo cp udev/60-openrgb.rules /etc/udev/rules.d/60-openrgb.rules
   sudo udevadm control --reload-rules && sudo udevadm trigger
   ```

3. **Installer les services de démarrage et de reprise de veille** :
   ```bash
   ./scripts/setup-autostart.sh
   sudo ./scripts/install-resume-hook.sh
   ```

4. **Installer le raccourci global `set-ram-color`** :
   ```bash
   mkdir -p ~/.local/bin
   ln -sf "$(pwd)/scripts/set-ram-color" ~/.local/bin/set-ram-color
   ```

---

## 📝 Licence

Ce projet est sous licence [GPL-2.0](LICENSE) conformément aux composants du projet OpenRGB.
