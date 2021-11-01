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

int main(){
    chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
    ifstream docs("fulldocs-new.trec",ios::in);
    // docs.seekg(30);
    // display_elapsed_time(start_time);
    // string word;
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
    ofstream doc_lookup("doc_lookup.txt",ios::out);
    string word;
    int i=0;
    int word_count=0;
    long long all_word_count=0;
    int docID=-1;
    int avg;
    long long pos=0;
    string buffer;
    bool flag=false;

    while(docs>>word){
        if(word=="<TEXT>"){
            flag=true;
            word_count=0;
            docID++;
            pos=docs.tellg();
        }
        else if(word=="</TEXT>"){
            buffer.append(to_string(docID)+" "+to_string(pos)+" "+to_string(word_count)+"\n");
            all_word_count=all_word_count+word_count;
            word_count=0;
            flag=false;
        }
        else if(flag==true){
            word_count++;
        }
        if(buffer.size()>=1024*1024){
            doc_lookup.write(buffer.c_str(),buffer.size());
            buffer.clear();
            display_elapsed_time(start_time);
            cout<<"at docID "<<docID<<endl;
        }
    }
    cout<<"at docID "<<docID<<endl;
    doc_lookup.write(buffer.c_str(),buffer.size());
    buffer.clear();
    display_elapsed_time(start_time);
    
    cout<<"end! total word count "<<all_word_count<<" avg: "<<all_word_count/(docID+1)<<endl;
    
    
    return 0;

}