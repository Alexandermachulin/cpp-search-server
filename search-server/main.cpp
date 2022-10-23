#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cmath>

#include <map>
#include <numeric>


//double relevance = log(x/y);

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
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
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:
    
    int document_count_ = 0;
    
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    /*
    for (string addressee : holmes_addressees) {
    cout << "Текст открытки:"s << endl;
    cout << "С новым годом, "s << addressee << "!"s << endl;
    cout << endl;
}
    */
    

    
    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inverse_word_count = 1.0 / words.size();  
        
        document_count_ += 1;
        
        for (const string& word : words)
        {
  //cout << word_to_document_freqs_[word][document_id] << endl;
            //Is math formula correct? Yes!!!
            word_to_document_freqs_[word][document_id] += inverse_word_count;            
      //      cout << document_id << " words = "s << word << "  inv_word_count" << inverse_word_count << endl;

        }        
        
    //    cout << "document_count_ =" << document_count_ << endl;
        
        documents_.push_back({document_id, words});
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    struct DocumentContent {
        int id = 0;
        vector<string> words;
    };

map<string, map<int, double>> word_to_document_freqs_;    
    
    
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    vector<DocumentContent> documents_;

    set<string> stop_words_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

      
    
    vector<Document> FindAllDocuments(const Query& query) const {
        
 /*       Для IDF понадобится знать суммарное количество документов. У нас больше нет вектора документов, но количество нам требуется. Для этого мы ввели document_count_. Когда эта переменная будет меняться? Когда мы добавляем новый документ. Где мы добавляем новый документ? В AddDocument
  */      
        
         map<int, double> document_to_relevance;
                
        for (const string& word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
           // const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            const double inverse_document_freq = log((documents_.size()) * 1.0 / word_to_document_freqs_.at(word).size());

//            cout << "inverse_document_freq = " << inverse_document_freq << endl;
            
            
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
            {


                {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
        
                for (const string& word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word))
            {
                document_to_relevance.erase(document_id);
            }
        }
        
        
        vector<Document> matched_documents;
        
                for (const auto [document_id, relevance] : document_to_relevance)
        {
            matched_documents.push_back(
            {document_id, relevance});
                //, documents_.at(document_id).rating});
        }
        
     
        
        return matched_documents;
    }

    static int MatchDocument(const DocumentContent& content, const Query& query) {
        if (query.plus_words.empty()) {
            return 0;
        }
        set<string> matched_words;
        for (const string& word : content.words) {
            if (query.minus_words.count(word) != 0) {
                return 0;
            }
            if (matched_words.count(word) != 0) {
                continue;
            }
            if (query.plus_words.count(word) != 0) {
                matched_words.insert(word);
            }
        }
        return static_cast<int>(matched_words.size());
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}
