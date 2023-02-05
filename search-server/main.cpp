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
const double ACCURACY = 1e-6;

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
             //cout <<  "stop_words_" << x << endl;
                throw invalid_argument("Bug in constructor1"s);
            }
        }
    }

    explicit SearchServer(const string& stop_words_text)
            : SearchServer(SplitIntoWords(stop_words_text)){}


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
                     if (abs(lhs.relevance - rhs.relevance) < ACCURACY){
                         return lhs.rating > rhs.rating;
                     }
                     //else 
                     {
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

//Для ревьюера! Я перенес проверку и выброс исключения в ParseQuery (строка 290-291), 
//но этом случае не проходит проверка. 
//Возможно это связано с тем, что в тексте задания требуется сделать исключения именно в этой части:
//"Метод MatchDocument должен возвращать tuple<vector<string>, DocumentStatus>, выбрасывая исключение //invalid_argument"         
        
        if (const auto parsed_query = ParseQuery(raw_query); parsed_query.minus_words.empty() and parsed_query.plus_words.empty())
        {throw invalid_argument("Bug in MatchDocument"s); }
        else 
        {
           // const auto parsed_query = ParseQuery(raw_query);
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
        // A valid word must not contain special characters
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
        // Word shouldn't be empty
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
             
   //     cout << "query.minus_words.empty() ="  << query.minus_words.empty() << endl; 
  //      cout << "query.plus_words.empty() ="  << query.plus_words.empty() << endl;         
        if ( query.minus_words.empty() and query.plus_words.empty()) 
        {throw invalid_argument("Empty words"s); }      
        return query;
    }

    // Existence required
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

// ==================== для примера =========================

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}


int main() {
    
    SearchServer search_server("и в на"s);
    // Явно игнорируем результат метода AddDocument, чтобы избежать предупреждения
    // о неиспользуемом результате его вызова
    (void) search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    (void) search_server.AddDocument(2, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2}); {
        cout << "Документ 2 был добавлен"s << endl;
    };
   (void) search_server.AddDocument(3, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2}); {
        cout << "Документ 3 был добавлен"s << endl;
    }
 //   (void) search_server.AddDocument(4, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, {1, 3, 2}); {
 //       cout << "Документ не был добавлен, так как содержит спецсимволы"s << endl;
 //   }

    (void) search_server.AddDocument(4, "в"s, DocumentStatus::ACTUAL, {1, 3, 2}); {
        cout << "Документ -пес не был добавлен, так как содержит спецсимволы"s << endl;
    }
    auto result = search_server.FindTopDocuments("в и"s);
    cout << "Документы с ключевыми словами \"черный дракон\" : "s << endl;
    
    /*
    for (auto &doc : result) {
        cout << doc << endl;
    }
*/
    //(void) (const auto documents = search_server.FindTopDocuments("--пушистый"s));
    // std::cout << "Hello, Students and supervisors!" << std::endl;
    return 0;
}

