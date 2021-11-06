#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <unordered_map>
#include <math.h>
#include <algorithm>
using namespace std;

const string SERVER = "wse-document-do-user-10132967-0.b.db.ondigitalocean.com";
const string USERNAME = "doadmin";
const string PASSWORD = "UsuY0FZyThSgOpJb";
const string DB="defaultdb";
const string LEXICON_FILE="lexicon.txt";
const string INDEX_FILE="inverted_index.txt";
const string DOCMAP_FILE="docMap.txt";
const string DOC_LOOKUP_FILE="doc_lookup.txt";
const int MAXDOCID=3213834;
const int RESULT_COUNT=10;

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

unordered_map<string, long long> load_lexicon(string lexicon_file){
    ifstream file;
    file.open(lexicon_file, ios::in);
    string term;
    long long pos;
    unordered_map<string, long long> lexicon;
    while(file>>term>>pos){
        lexicon.insert(make_pair(term,pos));
    }
    file.close();
    return lexicon;
}

vector<string> load_docMap(string docMap_file){
    ifstream file;
    file.open(docMap_file, ios::in);
    string url;
    int docID;
    vector<string> docMap;
    while(file>>docID>>url){
        docMap.push_back(url);
    }
    file.close();
    return docMap;
}

vector<pair<long long,int>> load_docLookup(string doc_lookup_file){
    ifstream file;
    file.open(doc_lookup_file, ios::in);
    int word_count;
    long long pos;
    int docID;
    vector<pair<long long,int>> doc_lookup;
    while(file>>docID>>pos>>word_count){
        doc_lookup.push_back(make_pair(pos,word_count));
    }
    file.close();
    return doc_lookup;
}

struct ListPointer{
    string lpterm;
    ifstream index;
    long long pos;
    int block_size=0;
    int docID_count;
    int last_in_block;
    int doc_id=-1;
    //index to specify where we are in the current block's decode buffer
    int array_pos=0;
    vector<int> block;
    int current_block=-1;
    int block_count;
    unsigned char buffer[320];
    pair<int,int> current_posting;

    ListPointer(long long lexiconPos,string term){
        lpterm=term;
        pos=lexiconPos;
        index.open(INDEX_FILE,ios::binary);
        index.seekg(pos);
        index.read((char*)buffer,8);
        docID_count=varbyte_decode(buffer,array_pos);
        block_count=docID_count/64;
        //if total doc count is exactly multiply of 64, there is no leftover small block
        if(block_count%64==0){
            block_count=block_count-1;
        }
        pos=pos+array_pos;
        getBlockInfo();
        decompressBlock();
    }

    void getBlockInfo(){
        //getting the info of next array means buffer pos and content for the previous block are no longer needed
        block.clear();
        pos=pos+block_size;
        block_size=0;
        index.seekg(pos);
        array_pos=0;
        current_block++;
        index.read((char*)buffer,20);
        last_in_block=varbyte_decode(buffer,array_pos);
        // determine how much to decompress
        block_size=block_size+varbyte_decode(buffer,array_pos);
        block_size=block_size+varbyte_decode(buffer,array_pos);
        // take note of the start of next block
        pos=pos+array_pos;
    }

    void decompressBlock(){
        index.seekg(pos);
        index.read((char*)buffer,block_size);
        int i=0;
        array_pos=0;
        while(array_pos<block_size){
            block.push_back(varbyte_decode(buffer,array_pos));
        }
    }
};

pair<int,int> nextGEQ(ListPointer* lp, int k){
    if(lp->last_in_block<k & (lp->current_block)>=(lp->block_count)){
        lp->current_posting=make_pair(MAXDOCID+1,0);
        return lp->current_posting;
    }
    if(lp->last_in_block<k & (lp->current_block)<(lp->block_count)){
        while(lp->current_block<=lp->block_count & (lp->current_block)<(lp->block_count)){
            lp->getBlockInfo();
            if(lp->last_in_block>=k){
                lp->decompressBlock();
                break;
            }
        }
        
    }
    for(int i=0;i<lp->block.size()/2;i++){
        if(lp->block[i]>=k){
            lp->current_posting=make_pair(lp->block[i],lp->block[i+lp->block.size()/2]);
            return lp->current_posting;
        }
    }
    return make_pair(MAXDOCID+1,0);
}

bool comparison(const tuple<int,float,vector<pair<string,int>>>& p1, const tuple<int,float,vector<pair<string,int>>>& p2){
    return get<1>(p1)>get<1>(p2);
}

bool merger_comparison(pair<tuple<int,float,vector<pair<string,int>>>,int> p1, pair<tuple<int,float,vector<pair<string,int>>>,int> p2){
    return get<1>(p1.first)<get<1>(p2.first);
}

string to_lower(string s){
    string result;
    for(int i=0;i<s.size();i++){
        if(s[i]!=',' && s[i]!='.' && s[i]!='!' && s[i]!='?' && s[i]!='"'){
            try{
                result.push_back(tolower(s[i]));
            }
            catch(const exception e){
                return "";
            }
        }
        
    }
    return result;
}

class FileSys{
    public:
        FileSys(string lexicon_file){
            //load lexicon and docMap
            lexicon=load_lexicon(lexicon_file);
            doc_lookup=load_docLookup(DOC_LOOKUP_FILE);
        }
        ListPointer* open_term(string term){
            if(lexicon.find(term) == lexicon.end()){
                return nullptr;
            }
            lp=new ListPointer(lexicon[term],term);
            return lp;
        }
        vector<tuple<int,float,vector<pair<string,int>>>> conjunctive_query(vector<string> query,int k){
            vector<ListPointer*> lps;
            vector<tuple<int,float,vector<pair<string,int>>>> result;
            ListPointer* lp;
            //open listpointers for each query term, return empty result if some of the terms do not exist
            for(int i=0; i<query.size();i++){
                lp=open_term(query[i]);
                if(lp==nullptr){
                    return result;
                }
                lps.push_back(lp);
            }
            int docID=0;
            int d;
            float impact_score=0;
            // holds docID and freq for cur_term
            pair<int,int> cur_term;
            // stores each term's occurance in each doc that contains all query terms
            vector<pair<string,int>> posting_info;
            make_heap(result.begin(),result.end(),comparison);
            while(docID<=MAXDOCID){
                // get the next docID that contains the first query term
                cur_term=nextGEQ(lps[0],docID);
                docID=cur_term.first;
                posting_info.clear();
                posting_info.push_back(make_pair(query[0],cur_term.second));
                impact_score=0;
                for (int i=1; i<query.size(); i++){
                    cur_term=nextGEQ(lps[i], docID);
                    d=cur_term.first;
                    posting_info.push_back(make_pair(query[i],cur_term.second));
                    if(d != docID){
                        break;
                    }
                };
                if(d>docID){
                    docID=d;
                }
                else{
                    if(docID==MAXDOCID+1){
                        sort_heap(result.begin(),result.end(),comparison);
                        return result;
                    }
                    for(int i=0; i<query.size();i++){
                        impact_score+=impactScore(lps[i]);
                    }
                    if(result.size()<k){
                        result.push_back(make_tuple(docID,impact_score,posting_info));
                        push_heap(result.begin(),result.end(),comparison);
                    }
                    else{
                        if(get<1>(result.front())<impact_score){
                            pop_heap(result.begin(),result.end(),comparison);
                            result.pop_back();
                            result.push_back(make_tuple(docID,impact_score,posting_info));
                            push_heap(result.begin(),result.end(),comparison);
                        }
                    }
                    docID++;
                }

            }
            for(int i=0; i<query.size();i++){
                lps[i]->index.close();
            }
            sort_heap(result.begin(),result.end(),comparison);
            return result;
        }
        float impactScore(ListPointer* lp){
            int N=MAXDOCID+1;
            float ft=lp->docID_count;
            float fdt=lp->current_posting.second;
            float d = doc_lookup[lp->current_posting.first].second;
            float davg=1129;
            float k1=1.2;
            float b=0.75;
            float score;
            float K;
            K=k1*((1-b)+b*(d/davg));
            score=log((N-ft+0.5)/(ft+0.5))*(k1+1)*fdt/(K+fdt);
            return score;
        }
        void show_snippets(const vector<tuple<int,float,vector<pair<string,int>>>>& docIDs, const vector<string>& query){
            ofstream ofile("output.txt");
            vector<pair<string,int>> word_freqs;
            pair<string,int> word_freq;
            for(int i=0;i<docIDs.size();i++){
                ofile<<"docID:"<<get<0>(docIDs[i])<<" BM25 Score:"<<get<1>(docIDs[i])<<"\n";
                ofile<<"Word frequencies:\n";
                word_freqs=get<2>(docIDs[i]);
                for(int j=0;j<word_freqs.size();j++){
                    word_freq=word_freqs[j];
                    ofile<<word_freq.first<<" "<<word_freq.second<<" ";
                }
                ofile<<"\n";
                ofile<<generate_snippet(get<0>(docIDs[i]),query)<<"\n";
                ofile<<"\n";
            }
            ofile.close();
        }
        string generate_snippet(int docID, const vector<string>& query){
            vector<string> doc;
            string word;
            ifstream ifile("fulldocs-new.trec",ios::in);
            ifile.seekg(doc_lookup[docID].first);
            //output the url first
            ifile>>word;
            string result;
            result.append(word+"\n");
            //load the other part of the document into a vector
            for(int j=0;j<doc_lookup[docID].second-1;j++){
                ifile>>word;
                doc.push_back(word);
            }
            ifile.close();
            //lets first try to find if there is a small window that contain all the terms in the query
            unordered_map<string,int> occurance;
            for(int i=0;i<query.size();i++){
                if(occurance.find(to_lower(query[i]))==occurance.end()){
                    occurance.insert(make_pair(to_lower(query[i]),0));
                }                
            }
            int distinct_term=occurance.size();
            
            int min_len=INT_MAX;
            pair<int,int> min_interval=make_pair(-1,-1);
            int count=0;
            int lp=0;
            int rp=0;
            if(distinct_term==1){
                min_len=1;
                for(int i=0;i<doc.size();i++){
                    if(to_lower(doc[i])==query[0]){
                        lp=i;
                        rp=i;
                        min_interval=make_pair(i,i);
                    }
                }
            }else{
                string curword;
                while(rp<=doc.size()){
                    if(count==distinct_term){
                        if(rp-lp<min_len){
                            min_interval=make_pair(lp,rp);  
                            min_len=rp-lp;                      
                        }
                        
                        curword=to_lower(doc[lp]);
                        if(occurance.find(curword)!=occurance.end()){
                            occurance[curword]--;
                            if(occurance[curword]==0){
                                count--;
                            } 
                        }
                        lp++;
                        continue;
                    }

                    if(rp==doc.size()){
                        break;
                    }
                    curword=to_lower(doc[rp]);
                    if(occurance.find(curword)!=occurance.end()){
                        if(occurance[curword]==0){
                            count++;
                        }
                        occurance[curword]++;
                    }
                    rp++;
                }
            }
            int maxPos=doc.size();
            //if not found, return empty string
            if(min_interval.first==-1){
                result.append("snippet unavailable");
                return result;
            }
            // if too long, hide the middle part and only get two 20-word string
            else if(min_len>50){
                for(int i=max(0,min_interval.first-10);i<min_interval.first+10;i++){
                    result.append(doc[i]+" ");
                }
                result.append("...... ");
                for(int i=min_interval.second-10;i<min(min_interval.second+10,maxPos);i++){
                    result.append(doc[i]+" ");
                }
            }
            //else get the whole window
            else{
                for(int i=max(0,min_interval.first-10);i<min(min_interval.second+10,maxPos);i++){
                    result.append(doc[i]+" ");
                }
            }
            
            doc.clear();
            return result;
        }
        vector<pair<tuple<int,float,vector<pair<string,int>>>,int>> disjunctive_query(vector<vector<string>>& disjunctive_query_terms){
            vector<vector<tuple<int,float,vector<pair<string,int>>>>> disjunctive_results;
            vector<pair<tuple<int,float,vector<pair<string,int>>>,int>> disjunctive_merger;
            vector<vector<pair<tuple<int,float,vector<pair<string,int>>>,int>>> disjunctive_mergers;
            pair<tuple<int,float,vector<pair<string,int>>>,int> d_result;
            vector<pair<tuple<int,float,vector<pair<string,int>>>,int>> results;
            int disjunctive_result_count=0;
            ofstream ofile("output.txt");
            ofile<<"disjunctive query results"<<endl;
            for(int i=0;i<disjunctive_query_terms.size();i++){
                disjunctive_results.push_back(conjunctive_query(disjunctive_query_terms[i],RESULT_COUNT));
                int count=disjunctive_results[i].size();
                disjunctive_result_count+=count;
                for(int j=0;j<min(RESULT_COUNT,count);j++){
                    disjunctive_merger.push_back(make_pair(disjunctive_results[i][j],i));
                }
                disjunctive_mergers.push_back(disjunctive_merger);
                make_heap(disjunctive_mergers[i].begin(),disjunctive_mergers[i].end(),merger_comparison);
                disjunctive_merger.clear();
            }
            for(int i=0;i<disjunctive_query_terms.size();i++){
                disjunctive_merger.push_back(disjunctive_mergers[i].front());
                pop_heap(disjunctive_mergers[i].begin(),disjunctive_mergers[i].end(),merger_comparison);
                disjunctive_mergers[i].pop_back();
            }
            make_heap(disjunctive_merger.begin(),disjunctive_merger.end(),merger_comparison);
            cout<<"got "<<min(RESULT_COUNT,disjunctive_result_count)<<" results"<<endl;
            for(int i=0;i<min(RESULT_COUNT,disjunctive_result_count);i++){
                //get the results with higher impact scores
                d_result=disjunctive_merger.front();
                results.push_back(d_result);
                ofile<<get<0>(d_result.first)<<" BM25 score="<<get<1>(d_result.first)<<"\n";
                ofile<<"Word frequencies:\n";
                vector<pair<string,int>> word_freqs=get<2>(d_result.first);
                pair<string,int> word_freq;
                for(int j=0;j<word_freqs.size();j++){
                    word_freq=word_freqs[j];
                    ofile<<word_freq.first<<" "<<word_freq.second<<" ";
                }
                ofile<<"\n";
                ofile<<generate_snippet(get<0>(d_result.first),disjunctive_query_terms[d_result.second])<<"\n";
                ofile<<"\n";
                if(i==min(RESULT_COUNT,disjunctive_result_count)-1){
                    break;
                }
                pop_heap(disjunctive_merger.begin(),disjunctive_merger.end(),merger_comparison);
                disjunctive_merger.pop_back();
                disjunctive_merger.push_back(disjunctive_mergers[d_result.second].front());
                push_heap(disjunctive_merger.begin(),disjunctive_merger.end(),merger_comparison);
                pop_heap(disjunctive_mergers[d_result.second].begin(),disjunctive_mergers[d_result.second].end(),merger_comparison);
                disjunctive_mergers[d_result.second].pop_back();
            }
            disjunctive_result_count=0;
            disjunctive_mergers.clear();
            disjunctive_merger.clear();
            disjunctive_results.clear();
            disjunctive_query_terms.clear();            
            
            ofile.close();
            return results;
        }

    private:
        unordered_map<string, long long> lexicon;
        vector<pair<long long,int>> doc_lookup;
        vector<string> docMap;
        ListPointer* lp;
};



int main(){
    chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
    string input;
    string term;
    FileSys filesys("lexicon.txt");
    cout<<"filesys initialized"<<endl;
    display_elapsed_time(start_time);
    // query processer 
    
    vector<string> query_terms;
    vector<tuple<int,float,vector<pair<string,int>>>> results;
    vector<vector<string>> disjunctive_query_terms;
    
    
    while(true){
        cout<<"input query:"<<endl;
        getline(cin,input);
        istringstream query(input);
        bool conjunctive=true;
        while(query>>term){
            if(term=="quit"){
                return 0;
            }
            else if(term=="OR"){
                conjunctive=false;
                disjunctive_query_terms.push_back(query_terms);
                query_terms.clear();
            }
            else if(term!="AND"){
                query_terms.push_back(to_lower(term));
            }
        }
        for(int j=0;j<query_terms.size();j++){
            cout<<query_terms[j]<<" ";
        }
        cout<<endl;
        if(!conjunctive){
            disjunctive_query_terms.push_back(query_terms);
            query_terms.clear();
            for(int i=0;i<disjunctive_query_terms.size();i++){
                cout<<"[";
                for(int j=0;j<disjunctive_query_terms[i].size();j++){
                    cout<<disjunctive_query_terms[i][j]<<" ";
                }
                cout<<"]"<<endl;
            }
        }
        start_time = chrono::steady_clock::now();
        if(conjunctive){
            results=filesys.conjunctive_query(query_terms,RESULT_COUNT);
            cout<<"got "<<results.size()<<" results"<<endl;
            // sort(results.begin(),results.end(),comparison);
            filesys.show_snippets(results,query_terms);
            display_elapsed_time(start_time);
            results.clear();
            query_terms.clear();
        }
        else{
            filesys.disjunctive_query(disjunctive_query_terms);
            display_elapsed_time(start_time);
        }

        
    }
    
};