#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <math.h>
#include <algorithm>

using namespace std;

void display_elapsed_time(chrono::steady_clock::time_point start){
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    cout << "Elapsed time: "
        << chrono::duration_cast<chrono::milliseconds>(end - start).count()
        << " ms" << endl;
}

bool binary_search(const vector<int>& docIDs, int docID){
    int l,r;
    r=docIDs.size();
    while (l < r) {
        int mid = (l + r) / 2;
        if (docIDs[mid] == docID){
            return true;
        }
        if (docID < docIDs[mid]) {
            r = mid;
        }
        else {
            l = mid + 1;
        }
    }
    return false;
}

float computeMoveGain(const vector<vector<int>>& termEdges, int docID, const vector<int>& Da, const vector<int>& Db, const vector<pair<int,int>>& termDegrees){
    float gain=0;
    float na=Da.size();
    float nb=Db.size();
    for(int i=0;i<termEdges.size();i++){
        if(binary_search(termEdges[i],docID)){
            float da=termDegrees[i].first;
            float db=termDegrees[i].second;
            gain=gain+da*log2(na/(da+1))+db*log2(nb/(db+1));
            gain=gain-(da-1)*log2(na/da)+(db+1)*log2(nb/(db+2));
        }
    }
    return gain;
}
float computeMoveGainBackWard(const vector<vector<int>>& termEdges, int docID, const vector<int>& Da, const vector<int>& Db, const vector<pair<int,int>>& termDegrees){
    float gain=0;
    float na=Db.size();
    float nb=Da.size();
    // cout<<na<<" "<<nb<<endl;
    for(int i=0;i<termEdges.size();i++){
        if(binary_search(termEdges[i],docID)){
            float da=termDegrees[i].second;
            float db=termDegrees[i].first;
            gain=gain+da*log2(na/(da+1))+db*log2(nb/(db+1));
            gain=gain-(da-1)*log2(na/da)+(db+1)*log2(nb/(db+2));
        }
    }
    return gain;
}

vector<pair<int,int>> computeTermDegrees(const vector<vector<int>>& termEdges,const vector<int>& Da, const vector<int>& Db){
    vector<int> mask(25205179);
    for(int i=0;i<Da.size();i++){
        mask[Da[i]]=1;
    }
    vector<pair<int,int>> result;
    for(int i=0;i<termEdges.size();i++){
        pair<int,int> degrees={0,0};
        for(int j=0;j<termEdges[i].size();j++){
            if(mask[termEdges[i][j]]==1){
                degrees.first++;
            }
            else{
                degrees.second++;
            }
        }
        result.push_back(degrees);
    }
    return result;
}

bool custom_greater(pair<float,int> a,pair<float,int> b){
    return a.first>b.first;
}

int main(){
    chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
    

    ifstream ifile;
    ifile.open("term_edges.txt");
    ifstream reassignment;
    reassignment.open("reassignment_bp_3.txt");
    ofstream ofile;
    ofile.open("reassignment_bp_4.txt");
    string term;
    vector<vector<int>> termEdges;
    int amount,docID;
    vector<int> edges;
    int count=0;
    while(ifile>>term>>amount){
        count+=1;
        if(count%1000==0){
            cout<<term<<endl;
        }
        edges.clear();
        ifile>>docID;
        edges.push_back(docID);
        for(int i=1;i<amount;i++){
            ifile>>docID;
            edges.push_back(docID+edges[i-1]);
        }
        termEdges.push_back(edges);
    }
    cout<<count<<" words read"<<endl;
    display_elapsed_time(start_time);
    vector<int> docIDs(25205179);
    for(int i=0;i<docIDs.size();i++){
        docIDs[i]=i;
    }
    display_elapsed_time(start_time);
    vector<int> d1(docIDs.size()/2);
    vector<int> d2(docIDs.size()/2);
    for(int i=0;i<d1.size();i++){
        reassignment>>docID;
        d1[i]=docID;
    }
    for(int i=0;i<d2.size();i++){
        reassignment>>docID;
        d2[i]=docID;
    }
    cout<<"bisection finished"<<endl;
    cout<<d1[d1.size()-1]<<endl;
    cout<<d2[d2.size()-1]<<endl;
    display_elapsed_time(start_time);
    vector<pair<int,int>> termDegrees=computeTermDegrees(termEdges,d1,d2);
    cout<<"term degrees computed"<<endl;
    display_elapsed_time(start_time);
    vector<pair<float,int>> d1gains,d2gains;

    for(int i=0;i<d1.size();i++){
        float d1gain=computeMoveGain(termEdges,d1[i],d1,d2,termDegrees);
        float d2gain=computeMoveGainBackWard(termEdges,d2[i],d1,d2,termDegrees);
        // if(d1gain!=0){
        //     cout<<"d1 gain from moving "<<d1[i]<<" "<<d1gain<<endl;
        // }
        // if(d2gain!=0){
        //     cout<<"d2 gain from moving "<<d2[i]<<" "<<d2gain<<endl;
        // }
        if(i%5000==0){
            float percentage=(float)i/(float)d1.size();
            cout<<"progress:"<<percentage*1000<<" %0"<<endl;
            display_elapsed_time(start_time);
        }
        d1gains.push_back({d1gain,i});
        d2gains.push_back({d2gain,i});
        
    }
    cout<<"move gain computed"<<endl;
    display_elapsed_time(start_time);
    sort(d1gains.begin(),d1gains.end(),custom_greater);
    sort(d2gains.begin(),d2gains.end(),custom_greater);
    cout<<"done sorting"<<endl;
    display_elapsed_time(start_time);
    count=0;
    for(int i=0;i<d1.size();i++){
        if(d1gains[i].first+d2gains[i].first>0){
            int temp;
            temp=d1[d1gains[i].second];
            d1[d1gains[i].second]=d2[d2gains[i].second];
            d2[d2gains[i].second]=temp;
            count++;
        }
    }
    for(int i=0;i<d1.size();i++){
        ofile<<to_string(d1[i])<<"\n";
    }
    for(int i=0;i<d2.size();i++){
        ofile<<to_string(d2[i])<<"\n";
    }
    ofile<<to_string(25205178)<<"\n";
    
    cout<<"first iter done"<<endl;
    cout<<count<<" swaps done"<<endl;
    display_elapsed_time(start_time);
    return 1;
}
