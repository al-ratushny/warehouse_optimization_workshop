#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

#include "reading.cpp"
#include "decoder.cpp"

using namespace std;

void testSequence(const vector<int>& seq){
    for(int i = 0; i < seq.size(); ++i){
        for(int j = 0; j < seq.size(); ++j){
            if (i != j && seq[i] == seq[j]) throw runtime_error("Cross error");
        };
    };
};

vector<int> simpleCross(vector<int> parent1, vector<int> parent2){
    srand (time(NULL));
    return parent1;
};

vector<int> OX(const vector<int>& parent1, const vector<int>& parent2){
    srand (time(NULL));
    vector<int> p1 = parent1;
    vector<int> p2 = parent2;
    vector<int> resSeq(p1.size(), -1);
    int r1 = rand() % p1.size();
    int r2 = rand() % p1.size();
    while (r1 == r2){
        r1 = rand() % p1.size();
        r2 = rand() % p1.size();
    };
    if (r1 > r2) swap(r1, r2);
    for (int i = r1; i < r2; ++i){
        resSeq[i] = p1[i];
        auto it = find(p2.begin(), p2.end(), p1[i]);
        p2.erase(it);
    };

    for (const auto& elem: p2){
        auto it = find(resSeq.begin(), resSeq.end(), -1);
        *it = elem;
    }

    testSequence(resSeq);
    return resSeq;
};

vector<int> OX_batch(const BaseDecoder& parent1, const BaseDecoder& parent2){
    srand(time(NULL));
    vector<int> p1 = parent1.idxSequence;
    vector<int> p2 = parent2.idxSequence;
    vector<int> resSeq = {};

    vector<int> batches1 = parent1.getBatchSequence();
    vector<int> batches2 = parent2.getBatchSequence();
    vector<int> resBatches(batches1.size(), -1);

    int r1 = rand() % batches1.size();
    int r2 = rand() % batches2.size();
    while (r1 == r2){
        r1 = rand() % batches1.size();
        r2 = rand() % batches2.size();
    };

    if (r1 > r2) swap(r1, r2);
    for (int i = r1; i < r2; ++i){
        resBatches[i] = batches1[i];
        auto it = find(batches2.begin(), batches2.end(), batches1[i]);
        batches2.erase(it);
    };

    for (const auto& elem: batches2){
        auto it = find(resBatches.begin(), resBatches.end(), -1);
        *it = elem;
    }

    for (const int& elem: resBatches){
        for(const int& elem2: parent1.batches[elem]){
            resSeq.push_back(elem2);
        };
    };

    testSequence(resSeq);
    testSequence(resBatches);
    return resSeq;
}

vector<int> OX_batch_2(const BaseDecoder& parent1, const BaseDecoder& parent2){
    vector<int> batch_begins = {};
    vector<int> batch_ends = {};
    vector<int> p2 = parent2.idxSequence;

    //ищем начало и конец для каждой партии
    for(const vector<int>& elem: parent1.batches){
        auto work_first = min_element(elem.begin(), elem.end(), [parent1](const int& work1, const int& work2){
            auto it1 = find_if(parent1.RESULT_WORKS.begin(), parent1.RESULT_WORKS.end(), [work1](const auto& _){return _.id == work1;});
            auto it2 = find_if(parent1.RESULT_WORKS.begin(), parent1.RESULT_WORKS.end(), [work2](const auto& _){return _.id == work2;});
            return it1->timeFirst < it2->timeFirst;
        });
        auto work_last = max_element(elem.begin(), elem.end(), [parent1](const int& work1, const int& work2){
            auto it1 = find_if(parent1.RESULT_WORKS.begin(), parent1.RESULT_WORKS.end(), [work1](const auto& _){return _.id == work1;});
            auto it2 = find_if(parent1.RESULT_WORKS.begin(), parent1.RESULT_WORKS.end(), [work2](const auto& _){return _.id == work2;});
            return it1->timeFirst < it2->timeFirst;
        });
        batch_begins.push_back(*work_first);
        batch_ends.push_back(*work_last);
    }

    int Idx1 = batch_begins[rand() % batch_begins.size()];
    int Idx2 = batch_ends[rand() % batch_ends.size()];
    int index1 = find(parent1.idxSequence.begin(), parent1.idxSequence.end(), Idx1) - parent1.idxSequence.begin();
    int index2 = find(parent1.idxSequence.begin(), parent1.idxSequence.end(), Idx2) - parent1.idxSequence.begin();
    while (index1 > index2){
        Idx1 = batch_begins[rand() % batch_begins.size()];
        Idx2 = batch_ends[rand() % batch_ends.size()];
        index1 = find(parent1.idxSequence.begin(), parent1.idxSequence.end(), Idx1) - parent1.idxSequence.begin();
        index2 = find(parent1.idxSequence.begin(), parent1.idxSequence.end(), Idx2) - parent1.idxSequence.begin();
    }



    vector<int> resSeq(parent1.idxSequence.size() ,-1);
    for(int i = index1; i <= index2; i++){
        resSeq[i] = parent1.idxSequence[i];
        auto it = find(p2.begin(), p2.end(), resSeq[i]);
        p2.erase(it);
    }

    for (const auto& elem: p2){
        auto it = find(resSeq.begin(), resSeq.end(), -1);
        *it = elem;
    }

    testSequence(resSeq);
    return resSeq;
}

