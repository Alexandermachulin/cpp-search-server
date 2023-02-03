#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
            : id(id)
            , relevance(relevance)
            , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
            : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for (auto& x : stop_words_) {
            if (!IsValidWord(x)) {
             
                throw invalid_argument("Bug in constructor1"s);
            }
        }
    }

    explicit SearchServer(const string& stop_words_text)
            : SearchServer(SplitIntoWords(stop_words_text))
    {
        for (auto& x : stop_words_) {
            if (!IsValidWord(x)) {              
                throw invalid_argument("Bug in constructor2"s);
            }
        }
    }


    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings) {

        if (documents_.count(document_id)) { throw invalid_argument("Bug in AddDocument"s); }

        if (document_id < 0) { throw invalid_argument("In AddDocument ID is less 0"s); }
        vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
        docIDs.push_back(document_id);

    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query,
                                      DocumentPredicate document_predicate) const {
        const auto parsed_query = ParseQuery(raw_query);

        {
            Query query = parsed_query;
            vector<Document> matched_documents = FindAllDocuments(query, document_predicate);

            sort(matched_documents.begin(), matched_documents.end(),
                 [](const Document& lhs, const Document& rhs) {
                     if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                         return lhs.rating > rhs.rating;
                     }
                     else {
                         return lhs.relevance > rhs.relevance;
                     }
                 });
            if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
                matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }
            return matched_documents;
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {

        return FindTopDocuments(
                raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                    return document_status == status;
                });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {

        if (const auto parsed_query = ParseQuery(raw_query); parsed_query.minus_words.empty() and parsed_query.plus_words.empty())
        {throw invalid_argument("Bug in MatchDocument"s); }

        else {
            Query query = parsed_query;
            vector<string> matched_words;
            for (const string& word : query.plus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    matched_words.push_back(word);
                }
            }
            for (const string& word : query.minus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    matched_words.clear();
                    break;
                }
            }

            tuple<vector<string>, DocumentStatus> result = { matched_words, documents_.at(document_id).status };
            return result;
        }
    }

    static bool IsValidWord(const string& word) {
      
        return none_of(word.begin(), word.end(), [](char c)
        {
            return c >= '\0' && c < ' ';
        });
    }

    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    int GetDocumentId(int index) const {
        if (index<0 || index>docIDs.size())
        { throw out_of_range("Error"s); }
        return docIDs[index];
    }


private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> docIDs;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) { throw invalid_argument("Bug in Split Into Words"s); }
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int Sum_rating = 0;
        for (const int rating : ratings) {
            Sum_rating += rating;
        }
        return Sum_rating / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
       
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) { throw invalid_argument("Bug in ParseQuery"s); }
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    if (query_word.data[0] == '-' || query_word.data[0] == '\0')
                    { throw invalid_argument("Bug in ParseQuery"s); }

                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
                                      DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                    { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

int main() {
     
    return 0;
}

    {
        const auto found_docs = server.FindTopDocuments("cool -monkey"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL(found_docs[0].id, 101);
    }

    {
        const auto found_docs = server.FindTopDocuments("cool -crocodile"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL(found_docs[0].id, 102);
    }

}

void TestMatchedDocuments() {
    SearchServer server;
    server.SetStopWords("and in on"s);
    server.AddDocument(100, "cool crocodile and black monkey in a collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });

    {
        const auto [matched_words, status] = server.MatchDocument("monkey and crocodile"s, 100);
        const vector<string> expected_result = { "crocodile"s, "monkey"s };
        ASSERT_EQUAL(expected_result, matched_words);
    }

    {
        const auto [matched_words, status] = server.MatchDocument("monkey and -crocodile"s, 100);
        const vector<string> expected_result = {}; // The result is empty due to minusword
        ASSERT_EQUAL(expected_result, matched_words);
        ASSERT(matched_words.empty());
    }
}

void TestSortResultsByRelevance() {
    SearchServer server;
    server.AddDocument(100, "cool crocodile cool leg"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(101, "cool monkey"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(102, "monkey leather collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });

    {
        const auto found_docs = server.FindTopDocuments("cool crocodile"s);
        ASSERT_EQUAL(found_docs.size(), 2u);
        for (size_t i = 1; i < found_docs.size(); i++) {
            ASSERT(found_docs[i - 1].relevance >= found_docs[i].relevance);
        }
    }
}

void TestCalculateDocumentRating() {
    SearchServer server;
    const vector<int> ratings = { 10, 11, 3 };
    const int average = (10 + 11 + 3) / 3;
    server.AddDocument(0, "cool crocodile cool leg"s, DocumentStatus::ACTUAL, ratings);

    {
        const auto found_docs = server.FindTopDocuments("cool crocodile"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].rating, average);
    }
}

void TestDocumentSearchByPredicat() {
    SearchServer server;
    server.AddDocument(100, "crocodile in the river"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(101, "monkey in the town"s, DocumentStatus::IRRELEVANT, { -1, -2, -3 });
    server.AddDocument(102, "monkey and rabbit in the town"s, DocumentStatus::ACTUAL, { -4, -5, -6 });
    const auto found_docs = server.FindTopDocuments("in the crocodile"s, [](int document_id, DocumentStatus status, int rating) {
        status = status;
        document_id = document_id;
        return rating > 0; });

    {
        ASSERT_HINT(found_docs[0].id == 100, "Bad predicat !!!");
    }
}

void TestDocumentSearchByStatus() {
    const int doc_id1 = 42;
    const int doc_id2 = 43;
    const int doc_id3 = 44;
    const string content1 = "crocodile in the river"s;
    const string content2 = "crocodile in the town"s;
    const string content3 = "crocodile in the town or river"s;
    const vector<int> ratings = { 1, 2, 3 };
    SearchServer server;
    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::IRRELEVANT, ratings);
    server.AddDocument(doc_id3, content3, DocumentStatus::IRRELEVANT, ratings);
    const auto found_docs = server.FindTopDocuments("in the crocodile"s, DocumentStatus::IRRELEVANT);

    {
        ASSERT_HINT(found_docs[0].id == doc_id2, "Bad status for doc");
        ASSERT_HINT(found_docs[1].id == doc_id3, "Bad status for doc");
        ASSERT_HINT(found_docs.size() == 2, "Bad status request");
    }
}

void TestCalculateRelevance() {
    SearchServer server;
    server.AddDocument(100, "white crocodile with new ring"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(101, "cool crocodile cool leg"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(102, "good monkey big eyes"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    const auto found_docs = server.FindTopDocuments("cool good crocodile"s);
    double relevance_query = log((3 * 1.0) / 1) * (2.0 / 4.0) + log((3 * 1.0) / 2) * (1.0 / 4.0);

    {
        ASSERT_HINT(fabs(found_docs[0].relevance - relevance_query) < 1e-6, "Bad relevance calculation ! ");
    }
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestFindAddedDocumentByDocumentWord);
    RUN_TEST(TestExcludeDocumentsWithMinusWordsFromResults);
    RUN_TEST(TestMatchedDocuments);
    RUN_TEST(TestSortResultsByRelevance);
    RUN_TEST(TestCalculateDocumentRating);
    RUN_TEST(TestDocumentSearchByPredicat);
    RUN_TEST(TestDocumentSearchByStatus);
    RUN_TEST(TestCalculateRelevance);
}
