#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <math.h>
using namespace std;

const int BUFFER_SIZE=1024*1024;

//display elapsed time
void display_elapsed_time(chrono::steady_clock::time_point start){
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    cout << "Elapsed time: "
        << chrono::duration_cast<chrono::milliseconds>(end - start).count()
        << " ms" << endl;
}

//read the next byte from buffer
int read_byte(unsigned char buffer[], int& array_pos){
    int result=(int)buffer[array_pos];
    array_pos++;
    return result;
}

bool comparison(pair<int,int> p1, pair<int,int> p2){
    return p1.second>p2.second;
}

bool normal_comparison(pair<int,int> p1, pair<int,int> p2){
    return p1.second<p2.second;
}


//decompress the next integer from buffer. Using int here because max docID<MAXINT, can switch to long long to scale
int varbyte_decode(unsigned char buffer[], int& array_pos){
    int val=0, shift=0, b;
    while((b=read_byte(buffer, array_pos))<128){
        val=val+(b<<shift);
        shift=shift+7;
    }
    val=val+((b-128)<<shift);
    return val;
}

int main(){
    chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
    ofstream test("testing.txt",ios::out);
    ifstream docs("fulldocs-new.trec",ios::in);
    
    unsigned char buffer [8];
    docs.seekg(21907738149);
    docs.read((char*) buffer,8);
    int pos=0;
    string word;
    for(int i=0;i<204;i++){
        docs>>word;
        test<<word<<" ";
    }
    
    // display_elapsed_time(start_time);
    // vector<pair<int,int>> vec;
    // vec.push_back(make_pair(1,2));
    // make_heap(vec.begin(),vec.end(),comparison);
    // vec.push_back(make_pair(1,6));
    // push_heap(vec.begin(),vec.end(),comparison);
    // vec.push_back(make_pair(1,3));
    // push_heap(vec.begin(),vec.end(),comparison);
    // vec.push_back(make_pair(1,4));
    // push_heap(vec.begin(),vec.end(),comparison);
    // vec.push_back(make_pair(1,8));
    // push_heap(vec.begin(),vec.end(),comparison);
    // vec.push_back(make_pair(1,5));
    // push_heap(vec.begin(),vec.end(),comparison);
    // vec.push_back(make_pair(1,7));
    // push_heap(vec.begin(),vec.end(),comparison);
    // cout<<vec.front().second<<endl;
    // pop_heap(vec.begin(),vec.end(),comparison);
    // vec.pop_back();
    // for(int i=0;i<vec.size();i++){
    //     cout<<vec[i].second<<endl;
    // }
    
    // cout<<"\n";
    // sort_heap(vec.begin(),vec.end(),comparison);
    // for(int i=0;i<vec.size();i++){
    //     cout<<vec[i].second<<endl;
    // }
    
    // ifstream docs("inverted_index.txt",ios::in);
    // unsigned char buffer [1000];
    // docs.seekg(1486863623);
    // docs.read((char*) buffer,1000);
    // int pos=0;
    // string word;
    // for(int i=0;i<140;i++){
    //     test<<varbyte_decode(buffer,pos)<<endl;
    // }
    
    // display_elapsed_time(start_time);
    
    // int i=0;
    // while(i<20){
    //     docs>>word;
    //     cout<<word<<endl;
    //     i++;
    // }
    // display_elapsed_time(start_time);
    // docs.seekg(21009376964);
    // i=0;
    // while(i<20){
    //     docs>>word;
    //     cout<<word<<endl;
    //     i++;
    // }
    // display_elapsed_time(start_time);

}