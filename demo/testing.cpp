#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <unordered_map>
#include <math.h>
#include <algorithm>
#include <utility>
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

bool comparison(const tuple<int,float,vector<tuple<string,int,int>>>& p1, const tuple<int,float,vector<tuple<string,int,int>>>& p2){
    return get<1>(p1)>get<1>(p2);
}

bool collect_comparison(tuple<int,float,vector<tuple<string,int,int>>> p1, tuple<int,float,vector<tuple<string,int,int>>> p2){
    return get<1>(p1)<get<1>(p2);
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
        vector<tuple<int,float,vector<tuple<string,int,int>>>> conjunctive_query(vector<string> query,int k){
            vector<ListPointer*> lps;
            vector<tuple<int,float,vector<tuple<string,int,int>>>> result;
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
            vector<tuple<string,int,int>> posting_info;
            make_heap(result.begin(),result.end(),comparison);
            while(docID<=MAXDOCID){
                // get the next docID that contains the first query term
                cur_term=nextGEQ(lps[0],docID);
                docID=cur_term.first;
                posting_info.clear();
                posting_info.push_back(make_tuple(query[0],cur_term.second,lps[0]->docID_count));
                impact_score=0;
                for (int i=1; i<query.size(); i++){
                    cur_term=nextGEQ(lps[i], docID);
                    d=cur_term.first;
                    posting_info.push_back(make_tuple(query[i],cur_term.second,lps[i]->docID_count));
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
            cout<<N<<" "<<ft<<" "<<fdt<<" "<<d<<" "<<davg<<" "<<score;
            K=k1*((1-b)+b*(d/davg));
            score=log((N-ft+0.5)/(ft+0.5))*(k1+1)*fdt/(K+fdt);
            return score;
        }
        void show_snippets(const vector<tuple<int,float,vector<tuple<string,int,int>>>>& docIDs, const vector<string>& query){
            ofstream ofile("output.txt");
            vector<tuple<string,int,int>> word_freqs;
            tuple<string,int,int> word_freq;
            for(int i=0;i<docIDs.size();i++){
                ofile<<"docID:"<<get<0>(docIDs[i])<<" BM25 Score:"<<get<1>(docIDs[i])<<"\n";
                ofile<<"docLength:"<<doc_lookup[get<0>(docIDs[i])].second<<"\n";
                ofile<<"Word frequencies:\n";
                word_freqs=get<2>(docIDs[i]);
                for(int j=0;j<word_freqs.size();j++){
                    word_freq=word_freqs[j];
                    ofile<<get<0>(word_freq)<<" freq:"<<get<1>(word_freq)<<" total_posting:"<<get<2>(word_freq)<<" ";
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
                        string subword;
                        curword=to_lower(doc[lp]);
                        for(char c:curword){
                            if(!isalpha(c)&&!isdigit(c)){
                                if(!subword.empty()){
                                    if(occurance.find(subword)!=occurance.end()){
                                        occurance[subword]--;
                                        if(occurance[subword]==0){
                                            count--;
                                        } 
                                    }
                                }
                                subword.clear();
                            }
                            else{
                                subword.push_back(c);
                            }
                        }
                        if(!subword.empty()){
                            if(occurance.find(subword)!=occurance.end()){
                                occurance[subword]--;
                                if(occurance[subword]==0){
                                    count--;
                                } 
                            }
                        }
                        subword.clear();
                        
                        lp++;
                        continue;
                    }

                    if(rp==doc.size()){
                        break;
                    }
                    string subword;
                    curword=to_lower(doc[rp]);
                    for(char c:curword){
                        if(!isalpha(c)&&!isdigit(c)){
                            if(!subword.empty()){
                                if(occurance.find(subword)!=occurance.end()){
                                    if(occurance[subword]==0){
                                        count++;
                                    }
                                    occurance[subword]++;
                                }
                            }
                            subword.clear();
                        }
                        else{
                            subword.push_back(c);
                        }
                    }
                    if(!subword.empty()){
                        if(occurance.find(subword)!=occurance.end()){
                            if(occurance[subword]==0){
                                count++;
                            }
                            occurance[subword]++;
                        }
                    }
                    subword.clear();
                    
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
        vector<tuple<int,float,vector<tuple<string,int,int>>>> disjunctive_query(vector<string>& query_terms, int k){
            vector<tuple<string,int,int>> term_freqs;
            vector<pair<float,vector<tuple<string,int,int>>>> accumulator(MAXDOCID,make_pair(0,term_freqs));
            ListPointer* lp;
            for(int i=0;i<query_terms.size();i++){
                lp=open_term(query_terms[i]);
                int d=0;
                pair<int,int> cur_term;
                while(d<=MAXDOCID){
                    cur_term=nextGEQ(lp,d);
                    d=cur_term.first;
                    accumulator[d].first=accumulator[d].first+impactScore(lp);
                    accumulator[d].second.push_back(make_tuple(query_terms[i],cur_term.second,lp->docID_count));
                    d++;
                }
            }

            vector<tuple<int,float,vector<tuple<string,int,int>>>> heap;
            for(int i=0;i<accumulator.size();i++){
                if(accumulator[i].first>0){
                    if(heap.size()==k && accumulator[i].first>get<1>(heap.front())){
                        pop_heap(heap.begin(),heap.end(),comparison);
                        heap.pop_back();
                    }
                    if(heap.size()<k){
                        heap.push_back(make_tuple(i,accumulator[i].first,accumulator[i].second));
                        push_heap(heap.begin(),heap.end(),comparison);
                    }
                }
                
            }
            sort_heap(heap.begin(),heap.end(),comparison);
            return heap;
        }
    unordered_map<string, long long> lexicon;
    private:
        
        vector<pair<long long,int>> doc_lookup;
        vector<string> docMap;
        ListPointer* lp;
};



int main(){
    chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
    string input;
    string term;
    // FileSys filesys("lexicon.txt");
    // cout<<"filesys initialized"<<endl;
    // display_elapsed_time(start_time);
    // // query processer 
    
    // ListPointer* lp=filesys.open_term("to");
    // cout<<lp->docID_count<<endl;
    // cout<<lp->block_size<<endl;
    // cout<<filesys.impactScore(lp)<<endl;
    // ifstream ifile("inverted_index.txt");
    // unsigned char buffer [10000];
    // ifile.seekg(filesys.lexicon["a"]);
    // ifile.read((char*)buffer,10000);
    // int pos=0;
    // vector<int> a;
    // for(int i=0;i<132;i++){
    //     a.push_back(varbyte_decode(buffer,pos));
    // };
    // for(int i=4;i<68;i++){
    //     cout<<a[i]<<" "<<a[i+64]<<endl;
    // }

    string a;
    int b;
    int c;
    ifstream ifile("intermediate_postings.txt");
    for(int i=0;i<1000;i++){
        ifile>>a>>b>>c;
        cout<<a<<" "<<b<<" "<<c<<endl;
    }

    // int b;
    // int c;
    // ofstream test("testing.txt");
    // while(1){
    //     ifile>>a>>b>>c;
    //     if(a=="a"){
    //         test<<a<<" "<<b<<" "<<c<<"\n";
    //         break;
    //     }
    // }
    // while(a=="a"){
    //     ifile>>a>>b>>c;
    //     test<<a<<" "<<b<<" "<<c<<"\n";
    // }
    
};