#include "archive.hpp"
int main(int argc, char** argv) {
    try {
        require(argc == 3 || argc == 4, "usage: reriff-native GAME_DIR --verify | --list | --config ASSET_PATH");
        std::string mode=argv[2];
        require((mode=="--config" && argc==4) || ((mode=="--verify" || mode=="--list") && argc==3), "invalid command");
        Archive archive(std::string(argv[1])+"/vfs.zip");
        size_t configs=0;
        for (const auto& entry : archive.entries) {
            if (mode=="--list") std::cout << entry.size << '\t' << entry.name << '\n';
            else if (mode=="--config" && entry.name==argv[3]) {
                auto b=archive.load(entry);
                std::cout << Config(b).json() << '\n';
                return 0;
            } else if (mode=="--verify") {
                auto b=archive.load(entry);
                if (entry.name.size()>=4 && entry.name.substr(entry.name.size()-4)==".cfg") {
                    Config(b).json();
                    ++configs;
                }
            }
        }
        require(mode!="--config", "asset not found");
        if (mode=="--verify") std::cout << "Verified " << archive.entries.size() << " assets and decoded " << configs << " GFCF configs natively.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "reriff-native: " << e.what() << '\n';
        return 1;
    }
}
