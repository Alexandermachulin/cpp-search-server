#pragma once
#include "string_processing.h"
#include "document.h"
#include <string>
#include <vector>
#include <cmath>
#include <set>
#include <map>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <deque>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

/**
 * класс поисковой машины SearchServer
 */
class SearchServer {
public:

    template <typename StringContainer>

    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw std::invalid_argument("Некоторые стоп-слова недействительны");
        }
    }


    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  
    {
    }

    void AddDocument(int document_id, const std::string& document, DocumentStatus status,
                     const std::vector<int>& ratings);

   	/**
	 * В данном методе результат работы метода FindAllDocuments сохраняется в коллекцию matched_documents типа vector<Document>
	 * коллекция сортируется по релевантности по убыванию
	 * Метод возвращает коллекцию, размер которой не превышает значение глобальной константы MAX_RESULT_DOCUMENT_COUNT
	 */ 
    
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    /**
	 * Получить количество документов
	 */
    int GetDocumentCount() const;

    int GetDocumentId(int index) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

private:
    	/**
	 * Данные документа
	 */
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const std::set<std::string> stop_words_;
    
/**
	 * Документы хранятся в коллекции, содержащей информацию о словах, идентификаторах и частотности слов
	 */    
    
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> document_ids_;

    	/**
	 * Проверяет, является ли данное слово стоп-словом
	 */
    
    bool IsStopWord(const std::string& word) const;

    static bool IsValidWord(const std::string& word);
/**
	 * Разделение строки на отдельные слова с исключением стоп-слов
	 */
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    	/**
	 * Вычисление среднего рейтинга
	 */
    static int ComputeAverageRating(const std::vector<int>& ratings);

        	/**
	 * Структура хранит слово и его признаки
	 */
    
    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const std::string& text) const;

    	/**
	 * Структура хранит наборы плюс и минус слов
	 */
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    Query ParseQuery(const std::string& text) const;

    /**
	 *  Вычисление частоты слова в инвертируемом индексе (Inverce document frequency)
	 */
    double ComputeWordInverseDocumentFreq(const std::string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    /**
	 * Метод подсчета релевантности всех документов, в которых встречаются слова запроса
	 * Вторым аргументом метода FindAllDocuments является пользовательская функция фильтрации документов
	 */
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        std::map<int, double> document_to_relevance;
        for (const std::string& word : query.plus_words) {
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

        for (const std::string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};