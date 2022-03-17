#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <math.h>

using namespace std;

void display_elapsed_time(chrono::steady_clock::time_point start){
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    cout << "Elapsed time: "
        << chrono::duration_cast<chrono::milliseconds>(end - start).count()
        << " ms" << endl;
}

int main(){
    chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
    ifstream ifile;
    ifile.open("sorted_url.txt",ios::in);
    long long curPos=0;
    ofstream ofile;
    ofstream lexicon;
    ofile.open("term_edges.txt",ios::out);
    lexicon.open("term_edges_lexicon.txt",ios::out);
    string term;
    int docID,freq;
    string curTerm;
    string buffer;
    vector<int> docIDs;
    ifile>>curTerm>>docID>>freq;
    docIDs.push_back(docID);
    int count=0;
    long long edgeCount=0;

    while(ifile>>term>>docID>>freq){
        if(term!=curTerm){
            if(docIDs.size()>8192){
                lexicon<<curTerm<<" "<<to_string(curPos)<<"\n";
                count++;
                if(count%100==0){
                    cout<<curTerm<<endl;
                    display_elapsed_time(start_time);
                }
                buffer.append(curTerm);
                buffer.append(" "+to_string(docIDs.size()));
                buffer.append(" "+to_string(docIDs[0]));
                for(int i=1;i<docIDs.size();i++){
                    buffer.append(" "+to_string(docIDs[i]-docIDs[i-1]));
                }
                buffer.append("\n");
                curPos=curPos+buffer.size();
                ofile<<buffer;
                edgeCount=edgeCount+docIDs.size();
            }
            docIDs.clear();
            curTerm=term;
            docIDs.push_back(docID);
            buffer.clear();
        }
        else{
            docIDs.push_back(docID);
        }
    }
    if(docIDs.size()>8192){
        buffer.append(curTerm);
        for(int i=0;i<docIDs.size();i++){
            buffer.append(" "+to_string(docIDs[i]));
        }
        ofile<<buffer<<"/n";
    }
    docIDs.clear();
    buffer.clear();
    cout<<count<<endl;
    cout<<edgeCount<<endl;
    return 1;
}
