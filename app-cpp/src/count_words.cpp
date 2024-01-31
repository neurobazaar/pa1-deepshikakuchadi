#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include "./matplotlib-cpp/matplotlibcpp.h" // Include matplotlibcpp for plotting

namespace plt = matplotlibcpp;

std::unordered_map<std::string, int> countWordsInFile(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    if (!file) {
        std::cerr << "Error opening file: " << filePath << "\n";
        return {};
    }

    std::unordered_map<std::string, int> wordCounts;
    std::string word;

    while (file >> word) {
        word.erase(std::remove_if(word.begin(), word.end(), 
                                  [](char c) { return !std::isalnum(c); }), word.end());
        std::transform(word.begin(), word.end(), word.begin(), 
                       [](unsigned char c){ return std::tolower(c); });
        
        if (!word.empty()) {
            wordCounts[word]++;
        }
    }

    return wordCounts;
}

void processDirectory(const std::filesystem::path& inputDir, 
                      const std::filesystem::path& outputDir,
                      std::vector<double>& fileSizes,
                      std::vector<double>& processingTimes) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(inputDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            std::filesystem::path relativePath = std::filesystem::relative(entry.path(), inputDir);
            std::filesystem::path outputPath = outputDir / relativePath;
            std::filesystem::create_directories(outputPath.parent_path());

            auto start = std::chrono::high_resolution_clock::now();

            auto wordCounts = countWordsInFile(entry.path());

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            processingTimes.push_back(elapsed.count());

            double fileSize = static_cast<double>(std::filesystem::file_size(entry.path())) / 1024.0 / 1024.0; // Size in MiB
            fileSizes.push_back(fileSize);

            std::ofstream outputFile(outputPath);
            if (!outputFile) {
                std::cerr << "Error opening output file: " << outputPath << "\n";
                continue;
            }

            for (const auto& [word, count] : wordCounts) {
                outputFile << word << " " << count << "\n";
            }
        }
    }
}

void plotThroughput(const std::vector<double>& fileSizes, const std::vector<double>& processingTimes) {
    std::vector<double> throughputs;
    for (size_t i = 0; i < fileSizes.size(); ++i) {
        throughputs.push_back(fileSizes[i] / processingTimes[i]);
    }

    plt::scatter(fileSizes, throughputs);
    plt::xlabel("Dataset Size (MiB)");
    plt::ylabel("Throughput (MiB/second)");
    plt::title("Throughput vs. Dataset Size");
    plt::show();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <inputDir> <outputDir>\n";
        return 1;
    }

    std::filesystem::path inputDir(argv[1]);
    std::filesystem::path outputDir(argv[2]);

    std::vector<double> fileSizes;
    std::vector<double> processingTimes;

    processDirectory(inputDir, outputDir, fileSizes, processingTimes);

    plotThroughput(fileSizes, processingTimes);

    return 0;
}
