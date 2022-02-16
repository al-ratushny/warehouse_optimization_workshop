#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>

using namespace std;


vector<int> ReadSeq(){
    string filePath = "sequence_A.txt";
    ifstream in(filePath);
    vector<int> Seq;
    
    if(in.is_open()) {
        while(!in.eof()){
            int a;
            string str;
            getline(in, str);
            istringstream ss(str);
            ss >> a;
            Seq.push_back(a);
        }
        in.close();
    }
    else {
        throw runtime_error("Wrong file path");
    }
    return Seq;
}
