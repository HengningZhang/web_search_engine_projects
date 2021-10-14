#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>


using namespace std;


int main(){
    string QUERY_TERM="ability";
    const int BUFFER_SIZE = 1024*1024;
    vector<char> buffer (BUFFER_SIZE + 1, 0);
    ifstream ifile;
    ifile.open("sorted_postings.txt", ios::in);
    string s;
    int docID;
    int frequency;
    long long pos;
    while(ifile>>s>>docID>>frequency){
        if(s==QUERY_TERM){
            cout<<0<<" "<<s<<" "<<docID<<" "<<frequency<<endl;
            for(int i=0;i<200;i++){
                ifile>>s>>docID>>frequency;
                cout<<i+1<<" "<<s<<" "<<docID<<" "<<frequency<<endl;
            }
            break;
        }
    }
    
    return 1;
}
