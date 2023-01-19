void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "crocodile in the river"s;
    const vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), " Exclude for Stop Words from Documents does not work"s);
    }
}

void TestFindAddedDocumentByDocumentWord() {
    const int doc_id = 42;
    const string content = "crocodile in the river"s;
    const vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        ASSERT_EQUAL(server.GetDocumentCount(), 0);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_EQUAL(server.GetDocumentCount(), 1);
        server.AddDocument(doc_id + 1, "black monkey cool leg", DocumentStatus::ACTUAL, ratings);
        ASSERT_EQUAL(server.GetDocumentCount(), 2);
        const auto found_docs = server.FindTopDocuments("crocodile"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
}

void TestExcludeDocumentsWithMinusWordsFromResults() {
    SearchServer server;
    server.AddDocument(101, "cool crocodile cool leg"s, DocumentStatus::ACTUAL, { 1,2,3 });
    server.AddDocument(102, "cool red monkey "s, DocumentStatus::ACTUAL, { 1,2,3 });

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
