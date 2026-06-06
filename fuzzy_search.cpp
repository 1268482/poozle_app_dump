#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace std;

static constexpr int MAX_WORD_LENGTH = 64;

static constexpr double FILTER_THRESHOLD = 0.6;

static constexpr int TOP_K = 5;

static constexpr int NUM_THREADS = 4;

using CharFreqTable = array<int, 256>;

CharFreqTable buildFreqTable(string_view text) {
    CharFreqTable freq{};  
    for (unsigned char ch : text)
        freq[ch]++;
    return freq;
}

struct QueryCache {
    string_view   text;
    int           length;
    CharFreqTable charFreq;
};

QueryCache buildQueryCache(string_view query) {
    QueryCache cache;
    cache.text     = query;
    cache.length   = static_cast<int>(query.size());
    cache.charFreq = buildFreqTable(query);
    return cache;
}

double overlapScore(const CharFreqTable& queryFreq,
                    int                  queryLen,
                    string_view          word) {

    CharFreqTable wordFreq = buildFreqTable(word);

    int sharedChars = 0;
    for (int i = 0; i < 256; i++)
        sharedChars += min(queryFreq[i], wordFreq[i]);

    return static_cast<double>(sharedChars)
         / max(queryLen, static_cast<int>(word.size()));
}

double boundaryScore(string_view query, string_view word) {
    if (query.size() < 4 || word.size() < 4)
        return 0.0;

    int matchCount = 0;
    if (query.front() == word.front()) matchCount++;
    if (query.back()  == word.back())  matchCount++;

    return matchCount / 2.0;
}

double combinedFilterScore(const QueryCache& cache, string_view word) {
    double overlap = overlapScore(cache.charFreq, cache.length, word);

    if (cache.length < 4)
        return overlap;

    double boundary = boundaryScore(cache.text, word);
    return 0.8 * overlap + 0.2 * boundary;
}

bool passesLengthFilter(int queryLen, int wordLen) {
    int allowedGap = 1 + max(queryLen, wordLen) / 6;
    return abs(queryLen - wordLen) <= allowedGap;
}

int damerauLevenshteinDistance(string_view source,
                               string_view target,
                               int         bound) {

    int sourceLen = static_cast<int>(source.size());
    int targetLen = static_cast<int>(target.size());

    auto twoRowsBack = make_unique<int[]>(targetLen + 1);
    auto oneRowBack  = make_unique<int[]>(targetLen + 1);
    auto currentRow  = make_unique<int[]>(targetLen + 1);

    for (int j = 0; j <= targetLen; j++)
        oneRowBack[j] = j;

    for (int i = 1; i <= sourceLen; i++) {
        currentRow[0] = i;
        int rowMinimum = i;

        for (int j = 1; j <= targetLen; j++) {
            int substitutionCost = (source[i - 1] == target[j - 1]) ? 0 : 1;

            currentRow[j] = min({
                oneRowBack[j]     + 1,                  
                currentRow[j - 1] + 1,                  
                oneRowBack[j - 1] + substitutionCost    
            });

            if (i > 1 && j > 1
                && source[i - 1] == target[j - 2]
                && source[i - 2] == target[j - 1]) {
                currentRow[j] = min(currentRow[j],
                                    twoRowsBack[j - 2] + 1);
            }

            rowMinimum = min(rowMinimum, currentRow[j]);
        }
        if (rowMinimum > bound)
            return bound + 1;

        swap(twoRowsBack, oneRowBack);
        swap(oneRowBack,  currentRow);
    }

    return oneRowBack[targetLen];
}

bool sameCharMultiset(const CharFreqTable& a, const CharFreqTable& b) {
    for (int i = 0; i < 256; i++) {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

struct Result {
    int    wordIndex;  
    double score;      
};

void scoreWordSlice(const QueryCache&     cache,
                    const vector<string>& words,
                    int                   sliceStart,
                    int                   sliceEnd,
                    vector<Result>&       results) {

    vector<Result> localResults;
    localResults.reserve(64);

    for (int i = sliceStart; i < sliceEnd; i++) {
        string_view word    = words[i];
        int         wordLen = static_cast<int>(word.size());

        if (wordLen > MAX_WORD_LENGTH)
            continue;


        if (!passesLengthFilter(cache.length, wordLen))
            continue;

        double filterScore = combinedFilterScore(cache, word);
        if (filterScore < FILTER_THRESHOLD)
            continue;
        
        int maxLen = max(cache.length, wordLen);

        
        int distanceBound = maxLen - 1;
        int distance = damerauLevenshteinDistance(cache.text, word,
                                                  distanceBound);
        if (distance > distanceBound)
            continue;

        
        double similarityScore = 1.0 - static_cast<double>(distance) / maxLen;

        
        if (distance == 1 && cache.length == wordLen) {
            CharFreqTable wordFreq = buildFreqTable(word);
            if (sameCharMultiset(cache.charFreq, wordFreq)) {
                similarityScore += 0.001;
            }
        }

        localResults.emplace_back(Result{i, similarityScore});
    }

    results = move(localResults);
}

int main(int argc, char* argv[]) {

    if (argc < 3) {
        cerr << "insufficient arguments :(\n";
        return 1;
    }

    string query    = argv[1];
    string filePath = argv[2];

    if (query.empty()) {
        cerr << "Error: query string is empty :(\n";
        return 1;
    }

    if (static_cast<int>(query.size()) > MAX_WORD_LENGTH) {
        cerr << "Error: query exceeds maximum length of "
             << MAX_WORD_LENGTH << " characters\n :(";
        return 1;
    }

    ifstream wordFile(filePath);
    if (!wordFile) {
        cerr << "Error: cannot open word list file: " << filePath << " :(\n";
        return 1;
    }

    vector<string> words;
    string         line;
    while (getline(wordFile, line)) {
        if (!line.empty())
            words.emplace_back(move(line));
    }

    if (words.empty()) {
        cerr << "Error: word list file is empty :( \n";
        return 1;
    }

    QueryCache cache = buildQueryCache(query);

    int totalWords = static_cast<int>(words.size());
    int baseChunk  = totalWords / NUM_THREADS;
    int extraWords = totalWords % NUM_THREADS;  

    vector<thread>         threads;
    vector<vector<Result>> threadResults(NUM_THREADS);
    threads.reserve(NUM_THREADS);

    int sliceStart = 0;
    for (int t = 0; t < NUM_THREADS; t++) {
        int sliceSize = baseChunk + (t < extraWords ? 1 : 0);
        int sliceEnd  = sliceStart + sliceSize;

        threads.emplace_back(scoreWordSlice,
                             cref(cache),
                             cref(words),
                             sliceStart,
                             sliceEnd,
                             ref(threadResults[t]));

        sliceStart = sliceEnd;
    }

    for (auto& thread : threads)
        thread.join();

    vector<Result> allResults;
    allResults.reserve(256);

    for (auto& perThreadResults : threadResults)
        for (auto& result : perThreadResults)
            allResults.emplace_back(result);

    if (static_cast<int>(allResults.size()) > TOP_K) {
        partial_sort(allResults.begin(),
                     allResults.begin() + TOP_K,
                     allResults.end(),
                     [](const Result& a, const Result& b) {
                         return a.score > b.score;
                     });
        allResults.resize(TOP_K);
    } else {
        sort(allResults.begin(), allResults.end(),
             [](const Result& a, const Result& b) {
                 return a.score > b.score;
             });
    }

    if (allResults.empty()) {
        cout << "\nNo matches found :(\n";
        return 0;
    }

    cout << "\nTop " << TOP_K << " matches:\n";
    for (const auto& result : allResults) {
        cout << words[result.wordIndex]
             << " -> "
             << result.score
             << "\n";
    }

    return 0;
}