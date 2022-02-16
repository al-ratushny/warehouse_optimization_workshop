#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <map>

using namespace std;

// GEN ALG PARAMS
const int NUMBER_OF_THREADS = 8; // 5
const int NUMBER_OF_ITERATIONS = 40 / NUMBER_OF_THREADS; // 75
const int POPULATIONS_SIZE = 15;
const int GREED_SIZE = 1e+4; // 1e+4

// Параметры остановки
const int STOP_NUMBER_ITERATIONS_SLS = 600; // для swap local search
const int STOP_NUMBER_ITERATIONS_MLS = 600; // для move local search
const int STOP_TIME = 360;
const int STOP_NUMBER_ITERATIONS_GA = 15;

const int TEST_NUM = 1;
std::vector<int> NUMBER_OF_WORKS{0, 0, 0, 0}; // определяется в reading.cpp
std::vector<int> NUMBER_OF_BATCHES{0, 0, 0, 0}; // определяется reading.cpp

// LOCAL SEARCH PARAMS
const int LOCAL_SEARCH_ITERATIONS_SIZE = 600; // 1500
const int LOCAL_SEARCH_POPULATION_SIZE = 15; // 40
const double LOCAL_SEARCH_RATIO = 5;
const int LOCAL_SEARCH_WIDTH = 50; // max расстояния для swap двух работ
                                   // вообще говоря должно зависить от размера партий
                                   // и быть своим для каждой линии

// LINE PARAMS
const std::vector<char> LINES_CHAR{'A', 'B', 'E', 'T'}; // используется в reading.cpp
enum class LINES {A = 0, B = 1, E = 2, T = 3};
LINES operator++(LINES& x) {
    return x = (LINES)(std::underlying_type<LINES>::type(x) + 1); 
}

LINES operator*(LINES c) {
    return c;
}

LINES begin(LINES r) {
    return LINES::A;
}

LINES end(LINES r) {
    LINES l=LINES::T;
    return ++l;
}


const std::map<LINES, int> PICKER_SIZE = {{LINES::A, 1}, {LINES::B, 2}, {LINES::E, 1}, {LINES::T, 3}};
const std::map<LINES, int> PACKER_SIZE = {{LINES::A, 8}, {LINES::B, 3}, {LINES::E, 2}, {LINES::T, 3}};
const std::map<LINES, int> BUFFER_SIZE = {{LINES::A, 8}, {LINES::B, 10}, {LINES::E, 4}, {LINES::T, 4}};
/*
const std::map<LINES, int> PICKER_SIZE = {{LINES::A, 1}, {LINES::B, 1}, {LINES::E, 1}, {LINES::T, 1}};
const std::map<LINES, int> PACKER_SIZE = {{LINES::A, 1}, {LINES::B, 1}, {LINES::E, 1}, {LINES::T, 1}};
const std::map<LINES, int> BUFFER_SIZE = {{LINES::A, 1}, {LINES::B, 1}, {LINES::E, 1}, {LINES::T, 1}};
*/

// PATHS
const std::string TEST_PATH = "benchmark/";
const std::string TEST_PATH_SIZE = "500";
const std::string CSV_PATH = "gant/"; 

// штрафы за нарушение ограничений
const int PENALTY = 150;
const int BATCH_TIME = 6*60*60; //6 hours
const bool PENALTY_FOR_TIME_WINDOW = false;
const bool PENALTY_FOR_BATCH = true;

// параметры для ограничения по количеству машинок
const int PICKER_TIME_WINDOW = 20; // 20
const int WORKS_PER_TIME_WINDOW = 5; // 5

//reading.cpp params
struct Item;
vector<Item> ReadItems();
vector<Item> itemsInWork = ReadItems();
class Work;

struct Item{
    string ID;
    string boxID;
    int quantity;
    int pickTime;
    string zoneID;
};

struct Box{
    string ID;
    string order;
    LINES lineType;
    int pickTime = 0;
    int packTime = 0;
    int weighingTime = 0;
    vector<Item> items;
    string batchID;
};

struct Batch{
    string ID;
    vector<string> orders;
    vector<Box> batchBoxes;
};

struct BatchMultiline{
    string ID;
    vector<Work> works;
    int timeBegin = -1;
    int timeEnd = -1;
};

bool operator==(const BatchMultiline& batch1, const BatchMultiline& batch2){
    return batch1.ID == batch2.ID;
}

class Work{
public:
    Work(const int& id_, const int& durationFirst_, const int& durationSecond_, const int& batchId_): 
        id(id_),
        durationFirst(durationFirst_), 
        durationSecond(durationSecond_),
        batchId(batchId_) {};

    int getTimeEndFirst() const {
        return timeFirst + getDurationFirst();
    };

    int getTimeEndSecond() const {
        return timeSecond + getDurationSecond();
    }

    int getDurationFirst() const {
        return durationFirst;
    }

    int getDurationSecond() const {
        return durationSecond;
    }

    int id = 0;
    int timeFirst = 0;
    int timeSecond = 0;
    int timeBuffer = 0;
    int machineFirstId = 0;
    int machineSecondId = 0;
    int machineBufferId = 0;
    int batchId = 0;

    int multilineBatchId = -1;
    //vector<int> items;
    int durationFirst = 0;
    int durationSecond = 0;
    LINES linetype;
private:
    
};

bool operator==(const Work& work1, const Work& work2){
    return work1.id == work2.id && work1.linetype == work2.linetype;
};

// Партии для мультилиний
vector<BatchMultiline> multilineBatches;