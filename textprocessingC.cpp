#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <omp.h>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#endif

// Function to measure memory usage (cross-platform)
size_t getMemoryUsage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS memCounter;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter))) {
        return memCounter.WorkingSetSize / 1024;  // Convert bytes to KB
    }
    return 0;
#else
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;  // Memory in KB
#endif
}

// Sequential Word Count
std::unordered_map<std::string, int> countWordsSequential(const std::string& filename) {
    std::unordered_map<std::string, int> wordCount;
    std::ifstream file(filename);
    std::string word;

    if (!file) {
        std::cerr << "Error opening file!" << std::endl;
        return wordCount;
    }

    while (file >> word) {
        ++wordCount[word];
    }

    return wordCount;
}

// Parallel Word Count using OpenMP
std::unordered_map<std::string, int> countWordsParallel(const std::string& filename) {
    std::unordered_map<std::string, int> wordCount;
    std::ifstream file(filename);
    std::vector<std::string> words;
    std::string word;

    if (!file) {
        std::cerr << "Error opening file!" << std::endl;
        return wordCount;
    }

    // Read all words into a vector
    while (file >> word) {
        words.push_back(word);
    }

    std::cout << "Running Parallel Word Count...\n";

#pragma omp parallel
    {
        // Check number of threads
        int numThreads = omp_get_num_threads();
#pragma omp single
        {
            std::cout << "Number of threads: " << numThreads << std::endl;
        }

        std::unordered_map<std::string, int> localWordCount;
#pragma omp for nowait
        for (size_t i = 0; i < words.size(); ++i) {
            ++localWordCount[words[i]];
        }

#pragma omp critical
        for (const auto& pair : localWordCount) {
            wordCount[pair.first] += pair.second;
        }
    }

    return wordCount;
}

int main() {
    std::string filename = "large_text.txt";

    // Sequential Word Count
    std::cout << "Running Sequential Word Count...\n";
    size_t memBeforeSeq = getMemoryUsage();
    auto startSeq = std::chrono::high_resolution_clock::now();

    auto wordCountSeq = countWordsSequential(filename);

    auto endSeq = std::chrono::high_resolution_clock::now();
    size_t memAfterSeq = getMemoryUsage();

    std::chrono::duration<double> elapsedSeq = endSeq - startSeq;
    std::cout << "Sequential Word Count completed in " << elapsedSeq.count() << " seconds.\n";
    std::cout << "Memory used: " << (memAfterSeq - memBeforeSeq) << " KB\n\n";

    // Print sequential word count results
    for (const auto& pair : wordCountSeq) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }

    std::cout << "\n------------------------------------\n";

    // Parallel Word Count
    size_t memBeforePar = getMemoryUsage();
    auto startPar = std::chrono::high_resolution_clock::now();

    auto wordCountPar = countWordsParallel(filename);

    auto endPar = std::chrono::high_resolution_clock::now();
    size_t memAfterPar = getMemoryUsage();

    std::chrono::duration<double> elapsedPar = endPar - startPar;
    std::cout << "Parallel Word Count completed in " << elapsedPar.count() << " seconds.\n";
    std::cout << "Memory used: " << (memAfterPar - memBeforePar) << " KB\n";

    // Print parallel word count results
    for (const auto& pair : wordCountPar) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }

    return 0;
}
