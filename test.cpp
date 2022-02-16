#include <algorithm>
#include <vector>
#include <iostream>

using namespace std;

int main(){
    vector<int> v = {1,2,3,4,5,6};
    auto it = v.begin();
    rotate(it+2, it+3, it+4);
    for(auto elem: v){
        cout << elem << ", ";
    }
    return 0;
}