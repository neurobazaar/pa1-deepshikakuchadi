#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <utility>
#include "./matplotlib-cpp/matplotlibcpp.h"  // Include the matplotlibcpp.h header
namespace plt = matplotlibcpp;
// Function to read word counts from a file and return them as a vector of pairs
std::vector<std::pair<std::string, int>> readWordCounts(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    std::vector<std::pair<std::string, int>> wordCounts;
    std::string line, word;
    int count;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        if (iss >> word >> count) {
            wordCounts.push_back({word, count});
        }
    }

    return wordCounts;
}

// Function to write sorted word counts to a file
void writeSortedWordCounts(const std::vector<std::pair<std::string, int>>& wordCounts, 
                           const std::filesystem::path& filePath) {
    std::ofstream file(filePath);
    for (const auto& [word, count] : wordCounts) {
        file << word << " " << count << "\n";
    }
}

void processDirectory(const std::filesystem::path& inputDir, 
                      const std::filesystem::path& outputDir,
                      long long& totalWords, 
                      double& totalTime,
                      std::vector<double>& fileSizes, 
                      std::vector<double>& throughputValues) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(inputDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            std::filesystem::path relativePath = std::filesystem::relative(entry.path(), inputDir);
            std::filesystem::path outputPath = outputDir / relativePath;
            std::filesystem::create_directories(outputPath.parent_path());

            auto wordCounts = readWordCounts(entry.path());

            double fileSize = std::filesystem::file_size(entry.path()) / 1024.0 / 1024.0; // Size in MiB
            auto start = std::chrono::high_resolution_clock::now();
            
            // Sort by frequency in descending order
            std::sort(wordCounts.begin(), wordCounts.end(), 
                      [](const auto& a, const auto& b) { return a.second > b.second; });

            auto end = std::chrono::high_resolution_clock::now();

            writeSortedWordCounts(wordCounts, outputPath);

            double timeTaken = std::chrono::duration<double>(end - start).count();
            totalWords += wordCounts.size();
            totalTime += timeTaken;

            fileSizes.push_back(fileSize);
            throughputValues.push_back(wordCounts.size() / timeTaken); // Calculate throughput
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
    long long totalWords = 0;
    double totalTime = 0.0;
    std::vector<double> fileSizes, throughputValues;

    processDirectory(inputDir, outputDir, totalWords, totalTime, fileSizes, throughputValues);

    // Plotting
    plt::plot(fileSizes, throughputValues, "bo"); // "bo" stands for "blue circle" markers
    plt::xlabel("File Size (MiB)");
    plt::ylabel("Throughput (words/second)");
    plt::title("Throughput vs. File Size");
    plt::show();

    return 0;
}
