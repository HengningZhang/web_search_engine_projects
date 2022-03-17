#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>

using namespace std;

//display elapsed time
void display_elapsed_time(chrono::steady_clock::time_point start){
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    cout << "Elapsed time: "
        << chrono::duration_cast<chrono::milliseconds>(end - start).count()
        << " ms" << endl;
}

int MAXDOCID=25205179;

int main(){
    chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
    ifstream ifile;
    ofstream ofile;
    ifile.open("reassignment_bp_3.txt",ios::in);
    int docID;
    int newDocID=0;
    string url;
    cout<<"loading new docID map"<<endl;
    vector<int> map(MAXDOCID+1,-1);
    while(ifile>>docID){
        map[docID]=newDocID;
        newDocID++;
        if(newDocID%100000==0){
            cout<<newDocID<<endl;
        }
    }
    cout<<"loaded reassignment map"<<endl;
    display_elapsed_time(start_time);
    string term;
    int freq;
    int count=0;
    string buffer;
    ifile.close();
    ifile.open("sorted_url.txt",ios::in);
    ofile.open("intermediate_postings_bp.txt",ios::out);
    while(ifile>>term>>docID>>freq){
        count++;
        if(map[docID]==-1){
            cout<<docID<<endl;
        }
        else{
            buffer.append(term+" "+to_string(map[docID])+" "+to_string(freq)+"\n");
        }
        
        if(buffer.size()>=500*1024*1024){
            ofile.write(buffer.c_str(),buffer.size());
            buffer.clear();
            display_elapsed_time(start_time);
            cout<<"at docID "<<docID<<endl;
        }
    }
    return 0;
}