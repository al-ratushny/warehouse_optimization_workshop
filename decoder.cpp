#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <numeric>
#include <functional>
#include <cmath>
#include <random>
#include <algorithm>
#include <chrono>

#include "read_seq.cpp"
#include "reading.cpp"
#include "params.cpp"


using namespace std;

class BaseDecoder{
public:
    BaseDecoder(const vector<Work>& workList_, const vector<int>& idxSequence_, LINES line):
        workList(workList_),
        idxSequence(idxSequence_),
        PICKER_SIZE(::PICKER_SIZE.at(line)),
        PACKER_SIZE(::PACKER_SIZE.at(line)),
        BUFFER_SIZE(::BUFFER_SIZE.at(line)), 
        line_type(line) {};
    
    void generateCSV(const int& testNum){
        //cout << "Generating CSV file for " << targetValue << '\n';
        preparingCSV();
    };

    void preparingCSV(){
        string file_path = CSV_PATH + "testNum-" + to_string(TEST_NUM) + "-" + LINES_CHAR[static_cast<int>(line_type)] + ".csv";
        ofstream out(file_path);
        if (out.is_open()){
            out << "work_id,batch_id,start_pick,finish_pick,start_buff,finish_buff,start_pack,";
            out << "finish_pack,pick_id,buff_id,pack_id,real_time" << '\n';
            for (const auto& work: RESULT_WORKS){
                auto it = find_if(multilineBatches.begin(), multilineBatches.end(),
                [work](const auto& batch){
                    auto it2 = find(batch.works.begin(), batch.works.end(), work);
                    return it2 != batch.works.end();
                }) - multilineBatches.begin();

                out << work.id << ',' << it << ',' << work.timeFirst << ',';
                out << work.getTimeEndFirst() << ',';
                out << work.timeBuffer << ',' << work.timeSecond << ',' << work.timeSecond << ',';
                out << work.getTimeEndSecond() << ',' << work.machineFirstId << ',';                
                out << work.machineBufferId << ',' << work.machineSecondId << ',';
                out << work.getDurationFirst() << '\n';
            }
        } else {   
            throw runtime_error("Coudn't open a csv file");
        }
    }

    int getLowGrade() const {
        double sum_1 = 0;
        double sum_2 = 0;
        double min_1 = workList[0].getDurationFirst();
        double min_2 = workList[0].getDurationSecond();
        for(const auto& work: workList){
            sum_1 += work.getDurationFirst();
            sum_2 += work.getDurationSecond();
            if (work.getDurationFirst() < min_1) min_1 = work.getDurationFirst();
            if (work.getDurationSecond() < min_2) min_2 = work.getDurationSecond();
        }
        sum_1 /= PICKER_SIZE;
        sum_2 /= PACKER_SIZE;
        
        int low_grade = ceil(max(sum_1 + min_2, sum_2 + min_1));
        return low_grade;
    };

    Work getMinimumPackage(){
        auto it = min_element(packageStage.begin(), packageStage.end(), [](auto& work1, auto& work2){return work1.getTimeEndSecond() < work2.getTimeEndSecond();});
        return *it;
    }

    Work getMinimumPicking(){
        auto it = min_element(pickStage.begin(), pickStage.end(), [](auto& work1, auto& work2){return work1.getTimeEndFirst() < work2.getTimeEndFirst();});
        return *it;
    }

    void __updatePacking(){
        if (packageStage.empty()) return;
        Work minSecond = getMinimumPackage();
        while (currentTime >= minSecond.getTimeEndSecond()){
            auto minSecondIdx = find_if(packageStage.begin(), packageStage.end(), [minSecond](const Work& elem){ return elem.id == minSecond.id;});
            Work work = packageStage[minSecondIdx - packageStage.begin()];
            packageStage.erase(minSecondIdx);
            
            FREE_PACK_ID.push_back(work.machineSecondId);
            RESULT_WORKS.push_back(work);
            if (packageStage.empty()) return;
            minSecond = getMinimumPackage();
        }
    }

    void __updatePicking(){
        if (!pickStage.empty()){
            Work minFirst = getMinimumPicking();
            while (!pickStage.empty() && currentTime >= minFirst.getTimeEndFirst() && bufferStage.size() < BUFFER_SIZE){
                auto minFirstIdx = find_if(pickStage.begin(), pickStage.end(), [minFirst](const Work& elem){ return elem.id == minFirst.id;});
                Work work = *minFirstIdx;
                pickStage.erase(minFirstIdx);
                work.timeBuffer = currentTime;
                work.machineBufferId = FREE_BUFFED_ID.back();
                FREE_BUFFED_ID.pop_back();
                bufferStage.push_back(work);
                FREE_PICK_ID.push_back(work.machineFirstId);
                if (pickStage.empty()) break;
                minFirst = getMinimumPicking();
            }
        };
        while (idxOfIdxSeq < seqLen && pickStage.size() < PICKER_SIZE){
            int getNextIndex = idxSequence[idxOfIdxSeq];
            idxOfIdxSeq += 1;
            Work work = workList[getNextIndex];
            work.timeFirst = currentTime;
            work.machineFirstId = FREE_PICK_ID.back();
            FREE_PICK_ID.pop_back();
            pickStage.push_back(work);
        };
    };
        
    int getIdxToPopFromBuffer(int Idx){
        // переписать, если используем что-то кроме FIFO
        return 0;
    };

    void __updateBuffer(){
        if (bufferStage.empty()) return;
        while (packageStage.size() < PACKER_SIZE && !bufferStage.empty()){
            Work work = bufferStage[getIdxToPopFromBuffer(1)];
            bufferStage.erase(bufferStage.begin() + getIdxToPopFromBuffer(1));
            work.timeSecond = currentTime;
            work.machineSecondId = FREE_PACK_ID.back();
            FREE_PACK_ID.pop_back();
            FREE_BUFFED_ID.push_back(work.machineBufferId);
            packageStage.push_back(work);
        }
    }

    int start(){
        RESULT_WORKS.reserve(idxSequence.size());
        FREE_PICK_ID.reserve(PICKER_SIZE);
        FREE_PACK_ID.reserve(PACKER_SIZE);
        FREE_BUFFED_ID.reserve(BUFFER_SIZE);
        pickStage.reserve(PICKER_SIZE);
        packageStage.reserve(PACKER_SIZE);
        bufferStage.reserve(BUFFER_SIZE);
        
        for(int i = 0; i < PICKER_SIZE; ++i) FREE_PICK_ID.push_back(i);
        for(int i = 0; i < PACKER_SIZE; ++i) FREE_PACK_ID.push_back(i);
        for(int i = 0; i < BUFFER_SIZE; ++i) FREE_BUFFED_ID.push_back(i);
        idxOfIdxSeq = 0;
        seqLen = idxSequence.size();
        int minEndTime = -1;

        while (true){
            if(!pickStage.empty() && bufferStage.size() < BUFFER_SIZE){
                minEndTime = getMinimumPicking().getTimeEndFirst();
            } else {
                minEndTime = -1;
            };

            if (!packageStage.empty()){
                int minEndPacking = getMinimumPackage().getTimeEndSecond();
                if (minEndTime != -1){
                    minEndTime = min(minEndTime, minEndPacking);
                } else {
                    minEndTime = minEndPacking;
                };
            }

            currentTime = minEndTime == -1 ? 0 : minEndTime;
            
            bool isFirst = true;

            while (isFirst || (FREE_BUFFED_ID.size() < BUFFER_SIZE && FREE_PACK_ID.size() > 0)){
                __updatePacking();
                __updateBuffer();
                if (idxOfIdxSeq < seqLen || !pickStage.empty()){
                    __updatePicking();
                    __updateBuffer();
                };
                isFirst = false;
            };

            if (idxOfIdxSeq >= seqLen && pickStage.empty() && bufferStage.empty() && packageStage.empty()){
                targetValue = minEndTime;

                getBatchWorks(); //используется в crossover.cpp 
                //Charge penalties
                if (PENALTY_FOR_TIME_WINDOW) chargePenaltyForTimeWindow();

                return targetValue;
            };
        }

    }

    void chargePenaltyForTimeWindow(){
        for (const int& work1 : idxSequence){
            int index1 = find(idxSequence.begin(), idxSequence.end(), work1) - idxSequence.begin();
            auto index1_RW_it = find_if(RESULT_WORKS.begin(), RESULT_WORKS.end(), [work1](const auto& elem){return elem.id == work1;});
            int time1 = index1_RW_it -> timeFirst;
            if (index1 < idxSequence.size() - WORKS_PER_TIME_WINDOW){
                int work2 = idxSequence[index1 + WORKS_PER_TIME_WINDOW];
                auto index2_RW_it = find_if(RESULT_WORKS.begin(), RESULT_WORKS.end(), [work2](const auto& elem){return elem.id == work2;});
                int time2 = index2_RW_it -> timeFirst;
                if(time2 - time1 < PICKER_TIME_WINDOW) {
                    targetValue += PENALTY;
                }
            };
        }
    }

    vector<int> getBatchSequence() const {
        vector<int> batchSeq = {};
        for (const auto& elem: idxSequence){
            auto it = find_if(RESULT_WORKS.begin(), RESULT_WORKS.end(), [elem](const auto& elem2){return elem2.id == elem;});
            if(find(batchSeq.begin(), batchSeq.end(), it -> batchId) == batchSeq.end()){
                batchSeq.push_back(it->batchId);
            }
            if(batchSeq.size() == NUMBER_OF_BATCHES[static_cast<int>(line_type)]){
                break;
            }
        }
        return batchSeq;
    };

    void getBatchWorks(){
        for (Work work: RESULT_WORKS){
            batches[work.batchId].push_back(work.id);
        }
    };

    //Information about the works 
    vector<Work> workList;

    //Sequence of works' ids
    vector<int> idxSequence;

    //Dimensions of the stages
    int PICKER_SIZE = 0;
    int BUFFER_SIZE = 0;
    int PACKER_SIZE = 0;

    //Variables for simulation of works
    vector<Work> packageStage;
    vector<Work> pickStage;
    vector<Work> bufferStage;
    vector<int> FREE_PICK_ID;
    vector<int> FREE_PACK_ID;
    vector<int> FREE_BUFFED_ID;
    int idxOfIdxSeq = 0;
    int seqLen = idxSequence.size();
    int currentTime = 0;

    //Output works with counted time of start and end
    vector<Work> RESULT_WORKS;

    int targetValue = 0;
    LINES line_type;

    //How many fines were charged 
    int hasPenalty = 0;

    //используется в crossover.cpp
    vector<vector<int>> batches = vector<vector<int>>(NUMBER_OF_BATCHES[static_cast<int>(line_type)], vector<int>(0));
};

/*
int main(){
    srand (time(NULL));
    LINES line = LINES::A;
    vector<Work> testData = loadTests()[static_cast<int>(line)];
    vector<int> seq(NUMBER_OF_WORKS[static_cast<int>(line)]);
    iota(seq.begin(), seq.end(), 0);
    seq = ReadSeq();
    auto t1 = chrono::high_resolution_clock::now();
    for(int i = 0; i < 100; ++i){
        //random_shuffle(seq.begin(), seq.end());
        BaseDecoder decoder{testData, seq, line};
        int result = decoder.start();
        //decoder.generateCSV(TEST_NUM);
        //cout << "low grade = " << decoder.getLowGrade() << '\n';
        //cout << "result = " << result << '\n';
    }
    auto t2 = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(t2 - t1); 
    cout << "Time: " << duration.count() << '\n';
    return 0;
}
*/