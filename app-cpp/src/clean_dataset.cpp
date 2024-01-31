#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <regex>
#include <algorithm>
#include "matplotlib-cpp/matplotlibcpp.h"  // Include the matplotlibcpp header

namespace plt = matplotlibcpp;


// Function to clean each file
void cleanFile(const std::filesystem::path& inputFilePath, const std::filesystem::path& outputFilePath) {
    std::ifstream inFile(inputFilePath);
    std::ofstream outFile(outputFilePath);

    if (!inFile.is_open() || !outFile.is_open()) {
        std::cerr << "Error opening file(s).\n";
        return;
    }

    std::string line;
    while (std::getline(inFile, line)) {
        // Remove '\r' characters
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        // Replace non-alphanumeric characters (excluding delimiters) with nothing (remove them)
        line = std::regex_replace(line, std::regex("[^0-9a-zA-Z \\t\\n\\r]+"), "");

        // Replace multiple delimiters with the last one in the sequence
        line = std::regex_replace(line, std::regex("([ \\t\\n\\r])\\1+"), "$1");

        outFile << line << "\n";
    }
}

// Function to clean all files in a directory
void cleanDirectory(const std::filesystem::path& inputDir, 
                    const std::filesystem::path& outputDir, 
                    double& totalSize, 
                    std::vector<double>& sizes, 
                    std::vector<double>& times) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(inputDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            std::filesystem::path relativePath = std::filesystem::relative(entry.path(), inputDir);
            std::filesystem::path outputPath = outputDir / relativePath;
            std::filesystem::create_directories(outputPath.parent_path());

            double fileSize = std::filesystem::file_size(entry.path()) / 1024.0 / 1024.0; // Size in MiB

            // Start measuring wall time using high_resolution_clock
            auto start = std::chrono::high_resolution_clock::now();

            cleanFile(entry.path(), outputPath);

            // Stop measuring wall time
            auto end = std::chrono::high_resolution_clock::now();

            // Calculate the elapsed time (wall time) for processing the file
            std::chrono::duration<double> elapsed = end - start;
            double timeTaken = elapsed.count();

            // Print the wall time for the current file
            std::cout << "Time taken for " << entry.path().filename() << ": " << timeTaken << " seconds.\n";

            totalSize += fileSize;
            sizes.push_back(fileSize);
            times.push_back(timeTaken);
        }
    }
}


void cleanDirectory(const std::filesystem::path& inputDir, const std::filesystem::path& outputDir) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(inputDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            std::filesystem::path relativePath = std::filesystem::relative(entry.path(), inputDir);
            std::filesystem::path outputPath = outputDir / relativePath;
            std::filesystem::create_directories(outputPath.parent_path());
            cleanFile(entry.path(), outputPath);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <inputDir> <outputDir>\n";
        return 1;
    }

    std::filesystem::path inputDir(argv[1]);
    std::filesystem::path outputDir(argv[2]);

    double totalSize = 0.0;
    std::vector<double> sizes, times;
    cleanDirectory(inputDir, outputDir, totalSize, sizes, times);

    // Plotting
    std::vector<double> throughput;
    for (size_t i = 0; i < sizes.size(); ++i) {
        throughput.push_back(sizes[i] / times[i]); // Calculate throughput for each dataset
    }

    plt::plot(sizes, throughput, "bo"); // "bo" stands for "blue circle" markers
    plt::xlabel("Dataset Size (MiB)");
    plt::ylabel("Throughput (MiB/second)");
    plt::title("Throughput vs. Dataset Size");
    plt::show();

    return 0;
}
