#ifdef __unix__                   
    #include <thread>
    #include <mutex>

#elif defined(_WIN32) || defined(WIN32)   

    #define _WIN32_WINNT 0x0501
    // it's better to use <thread> and <mutex> on linux
    #include "mingw-std-threads-master/mingw.thread.h" 
    #include "mingw-std-threads-master/mingw.mutex.h"  

#endif

#include <vector>
#include <iostream>
#include <algorithm>
#include <map>
#include <random>
#include <cassert>
#include <utility>
#include <iomanip>

#include "reading.cpp"
#include "params.cpp"
#include "decoder.cpp"
#include "tqdm/tqdm.hpp"
#include "crossover.cpp"
#include "read_seq.cpp"

using namespace std;

//чтобы не считывать заново каждый раз
vector<vector<Work>> testData = loadTests();

BaseDecoder getWorstResult(const vector<BaseDecoder>& decoders){
    return *max_element(decoders.begin(), decoders.end(), 
            [](const BaseDecoder& elem1, const BaseDecoder& elem2){
                return elem1.targetValue < elem2.targetValue;});
}

int getWorstResultID(const vector<BaseDecoder>& decoders){
    BaseDecoder dec = getWorstResult(decoders);
    return find_if(decoders.begin(), decoders.end(),
                [dec](const BaseDecoder& elem){
                    return elem.idxSequence == dec.idxSequence;
                    }) - decoders.begin();
}

BaseDecoder getBestResult(const vector<BaseDecoder>& decoders){
    return *min_element(decoders.begin(), decoders.end(), 
            [](const BaseDecoder& elem1, const BaseDecoder& elem2){
                return elem1.targetValue < elem2.targetValue;});
}

int getBestResultID(const vector<BaseDecoder>& decoders){
    BaseDecoder dec = getBestResult(decoders);
    return find_if(decoders.begin(), decoders.end(),
                [dec](const BaseDecoder& elem){
                    return elem.idxSequence == dec.idxSequence;
                    }) - decoders.begin();
}

vector<int> getNewSequence(const int& firstDecoderIdx, const int& secondDecoderIdx,const vector<BaseDecoder>& decoders){
    //return simpleCross(decoders[firstDecoderIdx].idxSequence, decoders[secondDecoderIdx].idxSequence);
    //return OX(decoders[firstDecoderIdx].idxSequence, decoders[secondDecoderIdx].idxSequence);
    return OX_batch(decoders[firstDecoderIdx], decoders[secondDecoderIdx]);
    //return OX_batch_2(decoders[firstDecoderIdx], decoders[secondDecoderIdx]);
}

vector<BatchMultiline> countMultilineBatches(const BaseDecoder& decoder){
    vector<BatchMultiline> multilineBatches_save = multilineBatches;
    for (const auto& work: decoder.RESULT_WORKS){
        /*auto it = find_if(multilineBatches_save.begin(), multilineBatches_save.end(),
                [work](const auto& batch){
                    auto it2 = find(batch.works.begin(), batch.works.end(), work);
                    return it2 != batch.works.end();
                });*/
        
        BatchMultiline& batch = multilineBatches_save[work.multilineBatchId];

        if(batch.timeBegin == -1 || batch.timeBegin > work.timeFirst){
            batch.timeBegin = work.timeFirst;
        }
        if(batch.timeEnd == -1 || batch.timeEnd < work.getTimeEndSecond()){
            batch.timeEnd = work.getTimeEndSecond();
        }

        assert(multilineBatches_save[work.multilineBatchId].timeBegin == batch.timeBegin);
    }
    return multilineBatches_save;
}

BaseDecoder getNewDecoder(const vector<int>& NewSequence, LINES line){
    BaseDecoder decoder(testData[static_cast<int>(line)], NewSequence, line);
    decoder.start();

    if (PENALTY_FOR_BATCH){
        decoder.hasPenalty = 0;
        vector<BatchMultiline> multilineBatches_save = countMultilineBatches(decoder);
        for (const auto& batch: multilineBatches_save){
            if(batch.timeEnd - batch.timeBegin > BATCH_TIME){
                decoder.hasPenalty++;
                decoder.targetValue += PENALTY;
            };
        };
    };
    return decoder;
}

pair<int, int> getTwoSequenceId(const vector<BaseDecoder>& decoders){
    int s1 = 0;
    int s2 = 0;
    while (s1 == s2){
        s1 = rand() % POPULATIONS_SIZE;
        s2 = rand() % POPULATIONS_SIZE;
    }
    return make_pair(s1, s2);
}

BaseDecoder swapLocalSearch(const BaseDecoder& decoder1){
    int iterationsCounter = 0;

    srand (time(NULL));
    BaseDecoder bestDecoder = decoder1;
    for (int i = 0; i < LOCAL_SEARCH_ITERATIONS_SIZE; ++i){
        vector<BaseDecoder> populationDecoders;
        while (populationDecoders.size() < LOCAL_SEARCH_POPULATION_SIZE){
            int Idx1 = rand() % bestDecoder.idxSequence.size();
            int Idx2 = rand() % bestDecoder.idxSequence.size();
            
            while (abs(Idx1 - Idx2) > LOCAL_SEARCH_WIDTH || Idx1 == Idx2){
                Idx1 = rand() % bestDecoder.idxSequence.size();
                Idx2 = rand() % bestDecoder.idxSequence.size();
            };

            vector<int> newSeq = bestDecoder.idxSequence;
            iter_swap(newSeq.begin() + Idx1, newSeq.begin() + Idx2);
            BaseDecoder newDec = getNewDecoder(newSeq, bestDecoder.line_type);
            if (newDec.targetValue < bestDecoder.targetValue*LOCAL_SEARCH_RATIO){
                populationDecoders.push_back(move(newDec));
            }
        }
        BaseDecoder newBestDecoder = getBestResult(populationDecoders);
        if (newBestDecoder.targetValue == bestDecoder.targetValue){
            iterationsCounter++;
        } else {
            iterationsCounter = 0;
        }
        if (iterationsCounter >= STOP_NUMBER_ITERATIONS_SLS) {
            break;
        }
        bestDecoder = bestDecoder.targetValue < newBestDecoder.targetValue ? 
                                                bestDecoder : newBestDecoder; 
    }
    return bestDecoder;
}

BaseDecoder moveLocalSearch(const BaseDecoder& decoder1){
    int iterationsCounter = 0;

    srand (time(NULL));
    BaseDecoder bestDecoder = decoder1;
    for (int i = 0; i < LOCAL_SEARCH_ITERATIONS_SIZE; ++i){
        vector<BaseDecoder> populationDecoders;

        while (populationDecoders.size() < LOCAL_SEARCH_POPULATION_SIZE){

            int Idx1 = rand() % bestDecoder.idxSequence.size();
            int Idx2 = rand() % 40;
            
            while(Idx1 + Idx2 >= bestDecoder.idxSequence.size() || Idx2 == 0){
                Idx1 = rand() % bestDecoder.idxSequence.size();
                Idx2 = rand() % 40;
            };

            vector<int> newSeq = bestDecoder.idxSequence;
            //iter_swap(newSeq.begin() + Idx1, newSeq.begin() + Idx2);
            auto it_move = newSeq.begin() + Idx1;
            rotate(it_move, it_move+1, it_move+Idx2);
            BaseDecoder newDec = getNewDecoder(newSeq, bestDecoder.line_type);

            if (newDec.targetValue < bestDecoder.targetValue*LOCAL_SEARCH_RATIO){
                populationDecoders.push_back(move(newDec));
            }
        }
        BaseDecoder newBestDecoder = getBestResult(populationDecoders);
        if (newBestDecoder.targetValue == bestDecoder.targetValue){
            iterationsCounter++;
        } else {
            iterationsCounter = 0;
        }
        
        bestDecoder = bestDecoder.targetValue < newBestDecoder.targetValue ? 
                                                bestDecoder : newBestDecoder; 

        if (iterationsCounter >= STOP_NUMBER_ITERATIONS_MLS) {
            break;
        }
    }
    return bestDecoder;
}

/*BaseDecoder altLocalSearch(const BaseDecoder& decoder1){
    int check = 0;
    srand (time(NULL));
    BaseDecoder bestDecoder = decoder1;
    for (int i = 0; i < LOCAL_SEARCH_ITERATIONS_SIZE; ++i){
        if (check == 0){
            vector<BaseDecoder> populationDecoders = {};
            while (populationDecoders.size() < LOCAL_SEARCH_POPULATION_SIZE){
                
                int Idx1 = rand() % bestDecoder.idxSequence.size();
                int Idx2 = rand() % 40;
                
                while(Idx1 + Idx2 >= bestDecoder.idxSequence.size() || Idx2 == 0){
                    Idx1 = rand() % bestDecoder.idxSequence.size();
                    Idx2 = rand() % 40;
                };

                vector<int> newSeq = bestDecoder.idxSequence;
                auto it_move = newSeq.begin() + Idx1;
                rotate(it_move, it_move+1, it_move+Idx2);
                BaseDecoder newDec = getNewDecoder(newSeq, bestDecoder.line_type);

                if (newDec.targetValue < bestDecoder.targetValue*LOCAL_SEARCH_RATIO){
                    populationDecoders.push_back(newDec);
                }
            }
            BaseDecoder newBestDecoder = getBestResult(populationDecoders);
            if (newBestDecoder.targetValue == bestDecoder.targetValue) check = (check + 1) % 2;
            bestDecoder = bestDecoder.targetValue < newBestDecoder.targetValue ? 
                                                    bestDecoder : newBestDecoder; 
        } else if(check == 1) {
            vector<BaseDecoder> populationDecoders = {};
            while (populationDecoders.size() < LOCAL_SEARCH_POPULATION_SIZE){
                
                int Idx1 = rand() % bestDecoder.idxSequence.size();
                int Idx2 = rand() % bestDecoder.idxSequence.size();
                
                while (abs(Idx1 - Idx2) > LOCAL_SEARCH_WIDTH || Idx1 == Idx2){
                    Idx1 = rand() % bestDecoder.idxSequence.size();
                    Idx2 = rand() % bestDecoder.idxSequence.size();
                };

                vector<int> newSeq = bestDecoder.idxSequence;
                iter_swap(newSeq.begin() + Idx1, newSeq.begin() + Idx2);
                BaseDecoder newDec = getNewDecoder(newSeq, bestDecoder.line_type);

                if (newDec.targetValue < bestDecoder.targetValue*LOCAL_SEARCH_RATIO){
                    populationDecoders.push_back(newDec);
                }
            }
            BaseDecoder newBestDecoder = getBestResult(populationDecoders);
            if (newBestDecoder.targetValue == bestDecoder.targetValue) check = (check + 1) % 2;
            bestDecoder = bestDecoder.targetValue < newBestDecoder.targetValue ? 
                                                    bestDecoder : newBestDecoder; 
            }
    }
    return bestDecoder;
}*/

BaseDecoder useLocalSearch(const BaseDecoder& decoder){
    BaseDecoder decoder1 = swapLocalSearch(decoder);
    decoder1 = moveLocalSearch(decoder1);
    return decoder1;
};

//генерация решений с заранее распределенным расположением партий
vector<BaseDecoder> genBasePopulation(LINES line){
    vector<vector<int>> populations;
    populations.reserve(GREED_SIZE); //память для популяции
    srand(time(NULL)); 
    vector<BaseDecoder> decoders;
    decoders.reserve(POPULATIONS_SIZE);

    //for(int i = 0; i < GREED_SIZE; ++i){
    cout << "Generating initial population: \n";
    for(int i : tq::trange(GREED_SIZE)){
        vector<vector<int>> batches(NUMBER_OF_BATCHES[static_cast<int>(line)]);

        for (const Work& work: testData[static_cast<int>(line)]){
            batches[work.batchId].push_back(work.id);
        };

        vector<int> seq = {};
        while (!batches.empty()){
            int r1 = rand() % batches.size();
            for(const auto& elem: batches[r1]){
                seq.push_back(elem);
            }
            batches.erase(find(batches.begin(), batches.end(), batches[r1]));
        }
        populations.push_back(seq);
    }

    for (const auto& elem: populations){
        BaseDecoder decoder = getNewDecoder(elem, line); 
        auto r1 = max_element(decoders.begin(), decoders.end(), [](const BaseDecoder& elem1, const BaseDecoder& elem2){return elem1.targetValue < elem2.targetValue;});
        if(decoders.size() < POPULATIONS_SIZE){
            decoders.push_back(decoder);
        } else if (r1->targetValue > decoder.targetValue){
            decoders.erase(r1);
            decoders.push_back(decoder);
        };
    };

    return decoders;
}

void GenAlgorithmOneLineIteration(vector<BaseDecoder>& decoders, mutex& m_add){
    pair<int, int> s1 = getTwoSequenceId(decoders);
    vector<int> newSequence = move(getNewSequence(s1.first, s1.second, decoders));
    BaseDecoder newDecoder = move(getNewDecoder(newSequence, decoders[0].line_type));
    newDecoder = move(useLocalSearch(newDecoder));

    const lock_guard<mutex> lock(m_add);
    int worstResultID = getWorstResultID(decoders);
    if (newDecoder.targetValue < decoders[worstResultID].targetValue){
        decoders[worstResultID] = move(newDecoder);
    }
}

BaseDecoder GenAlgorithmOneLine(LINES line){
    vector<BaseDecoder> decoders = genBasePopulation(line);
    //decodersOut(decoders);

    cout << "\nApplying genetic algorithm: \n";
    for(int i: tq::trange(NUMBER_OF_ITERATIONS)){
        mutex m_add;
        vector<thread> threads;
        for(int j = 0; j < NUMBER_OF_THREADS; ++j){
            threads.push_back(thread(GenAlgorithmOneLineIteration, ref(decoders), ref(m_add)));
        }

        for(int j = 0; j < NUMBER_OF_THREADS; ++j){
            threads[j].join();
        }
    };
    cout << '\n'; //newline after tqdm output

    BaseDecoder bestDecoder = getBestResult(decoders);
    //cout << "Before local search " << bestDecoder.targetValue << '\n';
    bestDecoder = useLocalSearch(bestDecoder);
    bestDecoder.generateCSV(TEST_NUM);
    //cout << bestDecoder.targetValue << '\n';

    return bestDecoder;
}

void decoderPrint(const BaseDecoder& decoder){
    cout << setw(20) << "LINE: " << LINES_CHAR[static_cast<int>(decoder.line_type)] << '\n';
    cout << setw(20) << "Low grade: " << decoder.getLowGrade() << '\n';
    cout << setw(20) << "Target value: " << decoder.targetValue << '\n';
    cout << setw(20) << "GAP: " << static_cast<double>(decoder.targetValue - decoder.getLowGrade()) / static_cast<double>(decoder.getLowGrade()) << '\n';
    cout << setw(20)<< "Has penalty: " << decoder.hasPenalty << '\n';
    cout << "-------------------------------- \n";
}

void getSolution(LINES line){
    BaseDecoder decoderA = GenAlgorithmOneLine(line);
    decoderPrint(decoderA);
    multilineBatches = countMultilineBatches(decoderA);
}

vector<LINES> getLinesOrder(){
    vector<pair<LINES, int>> low_grades;
    for (const auto& line: LINES()){
        BaseDecoder decoder(testData[static_cast<int>(line)], {0}, line);
        low_grades.push_back(make_pair(line, decoder.getLowGrade()));
        sort(low_grades.begin(), low_grades.end(), 
            [](const auto& elem1, const auto& elem2){
                return elem1.second < elem2.second;
            });
    }
    vector<LINES> lines_order;
    for(const auto& elem: low_grades){
        lines_order.push_back(elem.first);
    };


    return lines_order;
}

int main(){
    vector<LINES> lines_order = getLinesOrder();
    //lines_order = {LINES::A, LINES::B, LINES::E, LINES::T};
    lines_order = {LINES::A};
    for(const auto& line: lines_order){
        getSolution(line);
    }

    return 0;
}
