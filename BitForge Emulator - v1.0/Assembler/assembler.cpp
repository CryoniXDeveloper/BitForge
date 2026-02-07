#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include "../rom.h"

void checkFileExtension(std::string fileName, std::string fileExtension) {
    size_t dotPosition = fileName.rfind('.');

    if (dotPosition == std::string::npos) {
        std::cerr << "\nError: File has no extension.";
        exit(0);
    }

    std::string extractedString = fileName.substr(dotPosition + 1);

    if (extractedString != fileExtension) {
        std::cerr << "\nError: File extension is incorrect. (" + fileName + "), extension expected: ." + fileExtension;
        exit(0);
    }
}

void getBinaryFromline();

int main() {
    std::cout << "Enter the path of the assembly file to be converted into binary: ";

    std::string assemblyFile;
    std::getline(std::cin, assemblyFile);

    std::ifstream inFile(assemblyFile);
    if (!inFile) {
        std::cerr << "\nError: Assembly file does not exist or cannot be opened.\n";
        exit(0);
    }

    checkFileExtension(assemblyFile, "trasm");

    std::cout << std::endl << "Enter the path of the output file where the binary should be saved: ";

    std::string outputFile;
    std::getline(std::cin, outputFile);

    checkFileExtension(outputFile, "bin");

    std::cout << "Turning assembly into binary..." << std::endl;
    return 0;
}