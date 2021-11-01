#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <unordered_map>
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
    ifstream docs("fulldocs-new.trec",ios::in);
    ofstream test("testing.txt",ios::out);
    unsigned char buffer [8];
    docs.seekg(2680483679);
    docs.read((char*) buffer,8);
    int pos=0;
    string word;
    for(int i=0;i<5000;i++){
        docs>>word;
        test<<word<<" ";
    }
    
    display_elapsed_time(start_time);
    // ifstream docs("inverted_index.txt",ios::in);
    // unsigned char buffer [1000];
    // docs.seekg(1486863302);
    // docs.read((char*) buffer,1000);
    // int pos=0;
    // string word;
    // for(int i=0;i<20;i++){
    //     cout<<varbyte_decode(buffer,pos)<<endl;
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