#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <random>
#include <chrono>
#include <cctype>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
size_t getPeakMemoryKB() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.PeakWorkingSetSize / 1024;
    }
    return 0;
}
#else
#include <sys/resource.h>
size_t getPeakMemoryKB() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss; // На Linux повертає КБ, на macOS — байти
}
#endif

bool endsWithPunctuation(const std::string& word) {
    if (word.empty()) return false;
    char last = word.back();
    return last == '.' || last == '!' || last == '?';
}

int main() {
    std::string filename = "../text_1000.txt";
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return 1;
    }

    // 1. Зчитування слів
    std::vector<std::string> words;
    std::string word;
    while (file >> word) {
        words.push_back(word);
    }
    file.close();

    if (words.size() < 2) {
        std::cerr << "Text is too short." << std::endl;
        return 1;
    }

    // Початок вимірювання часу
    auto start_time = std::chrono::high_resolution_clock::now();

    // 2. Побудова Марковського ланцюга
    std::unordered_map<std::string, std::vector<std::string>> chain;
    std::vector<std::string> start_words;

    for (size_t i = 0; i < words.size() - 1; ++i) {
        chain[words[i]].push_back(words[i + 1]);
        if (std::isupper(static_cast<unsigned char>(words[i][0]))) {
            start_words.push_back(words[i]);
        }
    }

    if (start_words.empty()) {
        start_words.push_back(words[0]);
    }

    // 3. Генерація речення
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<> start_dist(0, start_words.size() - 1);
    std::string current_word = start_words[start_dist(gen)];
    std::string sentence = current_word;

    int max_words = 30; // Захист від нескінченного циклу
    int word_count = 1;

    while (!endsWithPunctuation(current_word) && word_count < max_words) {
        if (chain.find(current_word) == chain.end() || chain[current_word].empty()) {
            break;
        }

        const auto& next_options = chain[current_word];
        std::uniform_int_distribution<> next_dist(0, next_options.size() - 1);
        current_word = next_options[next_dist(gen)];
        
        sentence += " " + current_word;
        word_count++;
    }

    if (!endsWithPunctuation(current_word)) {
        sentence += ".";
    }

    // Кінець вимірювання часу
    auto end_time = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    size_t memory_kb = getPeakMemoryKB();

    std::cout << "--- Generated Sentence (C++) ---" << std::endl;
    std::cout << sentence << std::endl << std::endl;
    std::cout << "--- Performance Results ---" << std::endl;
    std::cout << "Execution time: " << duration_ms << " ms" << std::endl;
    std::cout << "Peak memory usage: " << memory_kb << " KB" << std::endl;

    return 0;
}