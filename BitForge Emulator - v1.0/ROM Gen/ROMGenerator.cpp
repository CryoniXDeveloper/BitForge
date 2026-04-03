#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

int main() {
    std::ifstream input("bytes.txt");
    if (!input) {
        std::cerr << "Could not open bytes.txt\n";
        return 1;
    }

    std::vector<uint8_t> bytes;
    std::string token;

    while (input >> token) {
        bytes.push_back((uint8_t)std::stoul(token, nullptr, 16));
    }

    input.close();

    const size_t ROM_SIZE = 32 * 1024;

    if (bytes.size() > ROM_SIZE) {
        std::cerr << "Error: bytes.txt exceeds 32KB\n";
        return 1;
    }

    bytes.resize(ROM_SIZE, 0x00);

    std::ofstream output("../rom.bin", std::ios::binary | std::ios::trunc);
    if (!output) {
        std::cerr << "Could not open rom.bin\n";
        return 1;
    }

    output.write((char*)bytes.data(), bytes.size());
    output.close();

    std::cout << "Done. Wrote " << ROM_SIZE << " bytes to rom.bin\n";
    return 0;
}