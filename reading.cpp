#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <numeric>
#include <functional>
#include <cmath>
#include <random>
#include <list>
#include <algorithm>
#include <sstream>
#include <map>
#include <algorithm>

#include "params.cpp"

using namespace std;

vector<Batch> ReadBatches(){
    string filePath = TEST_PATH + TEST_PATH_SIZE + "/" + to_string(TEST_NUM) + "/" +
                       TEST_PATH_SIZE + "_" + to_string(TEST_NUM) + "_batch_info.csv";
    ifstream in(filePath);
    vector<Batch> batches;
    
    if(in.is_open()) {
        string str;
        getline(in, str);
        while(!in.eof()){
            getline(in, str);

            istringstream ss(str);
            string batchID;
            getline(ss, batchID, ',');
            string order;
            getline(ss, order, ',');
            
            if (batchID.empty()) break;

            auto it = find_if(batches.begin(),  batches.end(), [batchID](const auto& elem){return elem.ID == batchID;});
            if (it != batches.end()){
                it->orders.push_back(order);
            } else {
                Batch new_batch{batchID, {order}, {}};
                batches.push_back(new_batch);
            }
        }
        in.close();
    }
    else {
        throw runtime_error("Wrong file path");
    }

    return batches;
}

vector<Box> ReadBoxes(){
    string filePath = TEST_PATH + TEST_PATH_SIZE + "/" + to_string(TEST_NUM) + "/" +
                      TEST_PATH_SIZE + "_" + to_string(TEST_NUM) + "_box_info.csv";
    ifstream in(filePath);
    vector<Box> boxes;

    if(in.is_open()) {
        string str;
        getline(in, str);

        while(!in.eof()){
            getline(in, str);
            istringstream ss(str);
            Box new_box;
            getline(ss, new_box.ID, ',');
            getline(ss, new_box.order, ',');
            if(new_box.ID.empty()) break;
            string line; getline(ss, line, ',');
            auto it = find(LINES_CHAR.begin(), LINES_CHAR.end(), line[0]) - 
                      LINES_CHAR.begin();
            new_box.lineType = static_cast<LINES>(it);

            //new_box.volume = stof(volume); не работает из-за 500/1
            boxes.push_back(new_box);
        }
        in.close();
    }
    else {
        throw runtime_error("Wrong file path");
    }

    string filePath2 = TEST_PATH + TEST_PATH_SIZE + "/" + to_string(TEST_NUM) + "/" +
                      TEST_PATH_SIZE + "_" + to_string(TEST_NUM) + "_box_processing_time.csv";
    ifstream in2(filePath2);

    if(in2.is_open()) {
        string str;
        getline(in2, str);

        while(!in2.eof()){
            getline(in2, str);
            istringstream ss(str);

            string boxID;
            int pickTime;
            int packTime;
            int weighingTime;

            getline(ss, boxID, ',');
            if(boxID.empty()) break;
            string temp; float temp2;

            getline(ss, temp, ',');
            temp2 = stof(temp);
            pickTime = static_cast<int>(temp2);

            getline(ss, temp, ',');
            temp2 = stof(temp);
            packTime = static_cast<int>(temp2);

            getline(ss, temp, ',');
            temp2 = stof(temp);
            weighingTime = static_cast<int>(temp2);

            auto it = find_if(boxes.begin(), boxes.end(), [boxID](const auto& elem){return elem.ID == boxID;});
            it -> pickTime = pickTime;
            it -> packTime = packTime;
            it -> weighingTime = weighingTime;
        }
        in2.close();
    }
    else {
        throw runtime_error("Wrong file path");
    }

    return boxes;
}

vector<Item> ReadItems(){
    string filePath = TEST_PATH + TEST_PATH_SIZE + "/" + to_string(TEST_NUM) + "/" +
                      TEST_PATH_SIZE + "_" + to_string(TEST_NUM) + "_item_info.csv";
    ifstream in(filePath);
    vector<Item> items;

    if(in.is_open()) {
        string str;
        getline(in, str);

        while(!in.eof()){
            getline(in, str);
            istringstream ss(str);
            Item new_item;
            getline(ss, new_item.boxID, ',');
            getline(ss, new_item.ID, ',');
            if(new_item.ID.empty()) break;
            ss >> new_item.quantity;
            items.push_back(new_item);
        }
        in.close();
    }
    else {
        throw runtime_error("Wrong file path");
    }

    string filePath2 = TEST_PATH + TEST_PATH_SIZE + "/" + to_string(TEST_NUM) + "/" +
                      TEST_PATH_SIZE + "_" + to_string(TEST_NUM) + "_item_processing_time.csv";
    ifstream in2(filePath2);

    if(in2.is_open()) {
        string str;
        getline(in2, str);

        while(!in2.eof()){
            getline(in2, str);
            istringstream ss(str);
            string itemID;
            int pickTime;
            string zoneID;
            getline(ss, itemID, ',');
            if (itemID.empty()) break;
            ss >> pickTime;
            ss.ignore(1);
            getline(ss, zoneID);
            auto it = find_if(items.begin(), items.end(), [itemID](const auto& elem){return elem.ID == itemID;});
            it -> pickTime = pickTime;
            it -> zoneID = zoneID;
        }
        in2.close();
    }
    else {
        throw runtime_error("Wrong file path");
    }
    return items;
}

vector<Work> getWorks(LINES line){
    vector<Batch> batches = ReadBatches();
    vector<Box> boxes = ReadBoxes();
    vector<Item> items = ReadItems();

    for (auto& box: boxes){
        for (auto& batch: batches){
            auto it = find(batch.orders.begin(), batch.orders.end(), box.order);
            if (it != batch.orders.end()){
                batch.batchBoxes.push_back(box);
                box.batchID = batch.ID;
            }
        }
    }

    vector<Work> lineWorks;
    vector<string> lineBatches;
    for(const auto& box: boxes){
        if (box.lineType == line){
            auto it = find(lineBatches.begin(), lineBatches.end(), box.batchID);
            if (it == lineBatches.end()){
                lineBatches.push_back(box.batchID);
                it = find(lineBatches.begin(), lineBatches.end(), box.batchID);
            }
            Work newWork(lineWorks.size(), box.pickTime, box.packTime, it - lineBatches.begin());
            newWork.linetype = line;
            auto it2 = find_if(multilineBatches.begin(), multilineBatches.end(), [box](const auto& elem){return elem.ID == box.batchID;});

            if (it2 == multilineBatches.end()){
                BatchMultiline newBatch;
                newBatch.ID = box.batchID;
                newBatch.works.push_back(newWork);
                multilineBatches.push_back(newBatch);
            } else {
                it2 -> works.push_back(newWork);
            }
            it2 = find_if(multilineBatches.begin(), multilineBatches.end(), [box](const auto& elem){return elem.ID == box.batchID;});
            newWork.multilineBatchId = it2 - multilineBatches.begin();
            lineWorks.push_back(newWork);

            /*
            for(int i = 0; i < items.size(); ++i){
                if(items[i].boxID == box.ID){
                    newWork.items.push_back(i);
                }
            }
            */
        };
    };

    NUMBER_OF_BATCHES[static_cast<int>(line)] = lineBatches.size();
    NUMBER_OF_WORKS[static_cast<int>(line)] = lineWorks.size();

    return lineWorks;
}

vector<vector<Work>> loadTests(){
    vector<vector<Work>> works;
    for(auto line: LINES()){
        works.push_back(getWorks(line));
    };
    return works;
}
