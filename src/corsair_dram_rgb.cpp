#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <cstdarg>
#include <dirent.h>
#include <cstring>
#include <linux/limits.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include "LogManager.h"
#include "pci_ids.h"
#include "DetectionManager.h"
#include "Controllers/CorsairDRAMController/CorsairDRAMController.h"
#include "Controllers/CorsairDRAMController/CorsairDRAMDevices.h"
#include "Controllers/CorsairDRAMController/RGBController_CorsairDRAM.h"

// Forward declaration
DetectedControllers DetectCorsairDRAMControllers(std::vector<i2c_smbus_interface *> &buses);

class LinuxSMBus : public i2c_smbus_interface
{
public:
    int handle;

    LinuxSMBus(int fd) : handle(fd) {}
    ~LinuxSMBus() override { if(handle >= 0) close(handle); }

    s32 i2c_smbus_xfer(u8 addr, char read_write, u8 command, int size, union i2c_smbus_data* data) override
    {
        struct i2c_smbus_ioctl_data args;
        ioctl(handle, I2C_SLAVE, addr);
        args.read_write = read_write;
        args.command = command;
        args.size = size;
        args.data = data;
        return ioctl(handle, I2C_SMBUS, &args);
    }

    s32 i2c_xfer(u8 /*addr*/, char /*read_write*/, int* /*size*/, u8* /*data*/) override
    {
        return -1;
    }
};

// Minimal LogManager stubs
static LogManager log_mgr;
LogManager* LogManager::get() { return &log_mgr; }
unsigned int LogManager::GetLogLevel() { return LL_INFO; }
void LogManager::LogEntry(const char* /*filename*/, int /*line*/, unsigned int /*level*/, const char* /*fmt*/, ...) {}
LogManager::LogManager() {}
LogManager::~LogManager() {}

DetectionManager* DetectionManager::get() { return nullptr; }
void DetectionManager::RegisterI2CDeviceDetector(std::string, I2CDeviceDetectorFunction, bool) {}

static std::vector<i2c_smbus_interface*> ScanI2CBuses()
{
    std::vector<i2c_smbus_interface*> buses;
    DIR* dir = opendir("/sys/bus/i2c/devices/");
    if(!dir) return buses;

    struct dirent* ent;
    while((ent = readdir(dir)) != NULL)
    {
        if(strncmp(ent->d_name, "i2c-", 4) == 0)
        {
            char devpath[PATH_MAX];
            snprintf(devpath, sizeof(devpath), "/dev/%s", ent->d_name);
            int fd = open(devpath, O_RDWR);
            if(fd >= 0)
            {
                LinuxSMBus* bus = new LinuxSMBus(fd);
                snprintf(bus->info.device_name, sizeof(bus->info.device_name), "%s", ent->d_name);

                // Get PCI info by walking up realpath
                char full_path[PATH_MAX];
                snprintf(full_path, sizeof(full_path), "/sys/bus/i2c/devices/%s", ent->d_name);
                char* rp = realpath(full_path, NULL);
                if(rp)
                {
                    char path[PATH_MAX];
                    strncpy(path, rp, sizeof(path));
                    free(rp);
                    while(true)
                    {
                        char vpath[PATH_MAX], dpath[PATH_MAX];
                        snprintf(vpath, sizeof(vpath), "%s/vendor", path);
                        snprintf(dpath, sizeof(dpath), "%s/device", path);
                        if(access(vpath, R_OK) == 0 && access(dpath, R_OK) == 0)
                        {
                            FILE* f = fopen(vpath, "r");
                            if(f) { unsigned int v = 0; if(fscanf(f, "%x", &v) == 1) bus->info.pci_vendor = v; fclose(f); }
                            f = fopen(dpath, "r");
                            if(f) { unsigned int d = 0; if(fscanf(f, "%x", &d) == 1) bus->info.pci_device = d; fclose(f); }
                            break;
                        }
                        char* slash = strrchr(path, '/');
                        if(!slash || slash == path) break;
                        *slash = '\0';
                    }
                }
                buses.push_back(bus);
            }
        }
    }
    closedir(dir);
    return buses;
}

static bool ParseColor(const std::vector<std::string>& args, RGBColor& out_color)
{
    if(args.empty()) return false;

    static const std::map<std::string, RGBColor> named_colors = {
        {"red",     ToRGBColor(255, 0, 0)},
        {"rouge",   ToRGBColor(255, 0, 0)},
        {"green",   ToRGBColor(0, 255, 0)},
        {"vert",    ToRGBColor(0, 255, 0)},
        {"blue",    ToRGBColor(0, 0, 255)},
        {"bleu",    ToRGBColor(0, 0, 255)},
        {"white",   ToRGBColor(255, 255, 255)},
        {"blanc",   ToRGBColor(255, 255, 255)},
        {"cyan",    ToRGBColor(0, 255, 255)},
        {"magenta", ToRGBColor(255, 0, 255)},
        {"yellow",  ToRGBColor(255, 255, 0)},
        {"jaune",   ToRGBColor(255, 255, 0)},
        {"orange",  ToRGBColor(255, 128, 0)},
        {"purple",  ToRGBColor(128, 0, 255)},
        {"violet",  ToRGBColor(128, 0, 255)},
        {"pink",    ToRGBColor(255, 105, 180)},
        {"rose",    ToRGBColor(255, 105, 180)},
        {"off",     ToRGBColor(0, 0, 0)},
        {"black",   ToRGBColor(0, 0, 0)},
        {"noir",    ToRGBColor(0, 0, 0)},
    };

    std::string arg0 = args[0];
    std::transform(arg0.begin(), arg0.end(), arg0.begin(), ::tolower);

    // Named color
    auto it = named_colors.find(arg0);
    if(it != named_colors.end())
    {
        out_color = it->second;
        return true;
    }

    // Hex color: #RRGGBB or RRGGBB
    std::string hex_str = arg0;
    if(hex_str.rfind("#", 0) == 0) hex_str = hex_str.substr(1);
    if(hex_str.length() == 6 && hex_str.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos)
    {
        unsigned int val = std::stoul(hex_str, nullptr, 16);
        unsigned char r = (val >> 16) & 0xFF;
        unsigned char g = (val >> 8) & 0xFF;
        unsigned char b = val & 0xFF;
        out_color = ToRGBColor(r, g, b);
        return true;
    }

    // 3 decimal arguments: R G B
    if(args.size() >= 3)
    {
        try {
            int r = std::clamp(std::stoi(args[0]), 0, 255);
            int g = std::clamp(std::stoi(args[1]), 0, 255);
            int b = std::clamp(std::stoi(args[2]), 0, 255);
            out_color = ToRGBColor(r, g, b);
            return true;
        } catch(...) {
            return false;
        }
    }

    return false;
}

static void PrintHelp(const char* prog)
{
    std::cout << "Usage: " << prog << " [COMMANDE | COULEUR]\n\n"
              << "Contrôleur OpenRGB pour RAM Corsair Vengeance RGB DDR5\n\n"
              << "Exemples d'utilisation :\n"
              << "  " << prog << " red                # Met la RAM en rouge\n"
              << "  " << prog << " bleu               # Met la RAM en bleu\n"
              << "  " << prog << " \"#00FFAA\"          # Couleur hexadécimale\n"
              << "  " << prog << " 255 128 0           # Valeurs RGB (0-255)\n"
              << "  " << prog << " off                # Éteint les LEDs\n"
              << "  " << prog << " --status           # Affiche les détails de la RAM détectée\n"
              << "  " << prog << " --help             # Affiche cette aide\n\n"
              << "Couleurs prédéfinies :\n"
              << "  red, green, blue, white, cyan, magenta, yellow, orange, purple, pink, off\n";
}

int main(int argc, char** argv)
{
    if(argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h"))
    {
        PrintHelp(argv[0]);
        return 0;
    }

    std::vector<std::string> args;
    for(int i = 1; i < argc; i++) args.push_back(argv[i]);

    bool info_only = (!args.empty() && (args[0] == "--info" || args[0] == "--status" || args[0] == "-i"));

    std::vector<i2c_smbus_interface*> buses = ScanI2CBuses();
    if(buses.empty())
    {
        std::cerr << "Erreur: Aucun bus I2C accessible. Vérifiez que votre utilisateur fait partie du groupe 'i2c' et que le module 'i2c-dev' est chargé.\n";
        return 1;
    }

    DetectedControllers controllers = DetectCorsairDRAMControllers(buses);
    if(controllers.empty())
    {
        std::cerr << "Erreur: Aucune RAM Corsair DDR5 détectée sur les bus SMBus.\n";
        for(auto b : buses) delete b;
        return 1;
    }

    if(info_only || args.empty())
    {
        std::cout << "=== RAM Corsair Détectée via OpenRGB ===\n";
        for(size_t i = 0; i < controllers.size(); i++)
        {
            auto c = controllers[i];
            std::cout << "Module [" << i + 1 << "] : " << c->GetName() << "\n"
                      << "  Emplacement : " << c->GetLocation() << "\n"
                      << "  Firmware    : " << c->GetVersion() << "\n"
                      << "  LEDs        : " << c->GetLEDCount() << " LEDs\n";
        }
        if(args.empty())
        {
            std::cout << "\nPour changer la couleur, tapez par exemple : " << argv[0] << " cyan\n";
        }
        for(auto c : controllers) delete c;
        for(auto b : buses) delete b;
        return 0;
    }

    RGBColor target_color = 0;
    if(!ParseColor(args, target_color))
    {
        std::cerr << "Erreur: Format de couleur non reconnu.\n\n";
        PrintHelp(argv[0]);
        for(auto c : controllers) delete c;
        for(auto b : buses) delete b;
        return 1;
    }

    unsigned char r = RGBGetRValue(target_color);
    unsigned char g = RGBGetGValue(target_color);
    unsigned char b = RGBGetBValue(target_color);

    for(auto c : controllers)
    {
        c->SetAllColors(target_color);
        c->UpdateLEDs();
        std::cout << "✓ " << c->GetName() << " (" << c->GetLocation() << ") mis à jour vers RGB("
                  << (int)r << ", " << (int)g << ", " << (int)b << ") / #"
                  << std::hex << std::setw(2) << std::setfill('0') << (int)r
                  << std::setw(2) << std::setfill('0') << (int)g
                  << std::setw(2) << std::setfill('0') << (int)b << std::dec << "\n";
    }

    for(auto c : controllers) delete c;
    for(auto b : buses) delete b;
    return 0;
}
