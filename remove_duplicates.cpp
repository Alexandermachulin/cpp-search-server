#include <set>
#include <string>
#include <iostream>
#include "search_server.h"

using namespace std;
//Вам нужно знать только, что нет дублей, а для этого достаточно проверить,
//что ранее не была добавлена копия документа. Для этого подойдет контейнер set<set<string>>

void RemoveDuplicates(SearchServer& search_server) {
    set<int> duplicates;
    set<set<string_view>> docs;
    for (auto i = search_server.begin(); i != search_server.end(); ++i) {
        const auto &doc = search_server.GetWordFrequencies(*i);
        set<string_view> doc_words;

        for (const auto &[word, freq]: doc) {
            doc_words.insert(word);
        }
        if (docs.count(doc_words)) {
            duplicates.insert(*i);
        }
        else {
            docs.insert(doc_words);
        }
    }
    for (int id : duplicates) {
        search_server.RemoveDocument(id);
        cout << "Found duplicate document id "s << id << endl;
    }
}
