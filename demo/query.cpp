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

//load the lexicon file into an unordered map
//term->inverted index position
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

//load the doc_lookup file into a vector of pairs
//doc_lookup[docID]->document starting position in .trec file
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

//the list pointer object used in nextGEQ and 
struct ListPointer{
    //the term queried
    string lpterm;
    //index file pointer
    ifstream index;
    //position in index pointer
    long long pos;
    //variable used to temporarily store the compressed size of current block
    int block_size=0;
    //number of documents that contains the queried term
    int docID_count;
    int last_in_block;
    //index to specify where we are in the current block's decode buffer
    int array_pos=0;
    //vector to storea compressed block of docID+freq
    vector<int> block;
    //which block the pointer is pointing to
    int current_block=-1;
    //how many blocks does this term have
    int block_count;
    //buffer used to read a block
    unsigned char buffer[320];
    //specify which docID the lp is pointing to, used for nextGEQ
    pair<int,int> current_posting;

    //constructor
    ListPointer(long long lexiconPos,string term){
        lpterm=term;
        pos=lexiconPos;
        index.open(INDEX_FILE,ios::binary);
        //move the file pointer to lexiconPos, start reading information
        index.seekg(pos);
        index.read((char*)buffer,8);
        // first read how many documents contain the queried term
        docID_count=varbyte_decode(buffer,array_pos);
        // calculate the block count
        block_count=docID_count/64;
        //if total doc count is exactly multiply of 64, there is no leftover small block
        if(block_count%64==0){
            block_count=block_count-1;
        }
        //update the pos pointer to specify where to read for the next operation
        pos=pos+array_pos;
        getBlockInfo();
        decompressBlock();
    }

    //only compress the first 3 numbers of a block, last_docID, docID block size and freq block size
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

    //decompress the next whole block and store the result in a vector
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

//get the next docID>=k that has the term represented by lp
pair<int,int> nextGEQ(ListPointer* lp, int k){
    // check if this block doesn't contains k and is the last block, return MAXDOCID+1,0
    if(lp->last_in_block<k & (lp->current_block)>=(lp->block_count)){
        lp->current_posting=make_pair(MAXDOCID+1,0);
        return lp->current_posting;
    }
    //check if this block doesn't contains k and this is not the last block
    if(lp->last_in_block<k & (lp->current_block)<(lp->block_count)){
        //compress 3 numbers from the following blocks until a block containing docIDs>=k is reached
        while(lp->current_block<=lp->block_count & (lp->current_block)<(lp->block_count)){
            lp->getBlockInfo();
            if(lp->last_in_block>=k){
                lp->decompressBlock();
                break;
            }
        }
        
    }
    //if this block contains k, read through the block and return the next docID and the freq related to it
    for(int i=0;i<lp->block.size()/2;i++){
        if(lp->block[i]>=k){
            lp->current_posting=make_pair(lp->block[i],lp->block[i+lp->block.size()/2]);
            return lp->current_posting;
        }
    }
    return make_pair(MAXDOCID+1,0);
}

//comparison used to collect top-k by heap
bool comparison(const tuple<int,float,vector<tuple<string,int,int>>>& p1, const tuple<int,float,vector<tuple<string,int,int>>>& p2){
    return get<1>(p1)>get<1>(p2);
}

//turn a string s into lower case
string to_lower(string s){
    string result;
    for(int i=0;i<s.size();i++){
        try{
            result.push_back(tolower(s[i]));
        }
        catch(const exception e){
            return "";
        }
    }
    return result;
}

// the file accesser
class FileSys{
    public:
        //load lexicon and docMap
        FileSys(string lexicon_file){ 
            lexicon=load_lexicon(lexicon_file);
            doc_lookup=load_docLookup(DOC_LOOKUP_FILE);
        }
        //open the list pointer for a term
        ListPointer* open_term(string term){
            if(lexicon.find(term) == lexicon.end()){
                return nullptr;
            }
            lp=new ListPointer(lexicon[term],term);
            return lp;
        }
        //a conjunctive query on a vector
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
            //initialize the heap used to filter and store the results
            make_heap(result.begin(),result.end(),comparison);
            while(docID<=MAXDOCID){
                // get the next docID that contains the first query term
                cur_term=nextGEQ(lps[0],docID);
                docID=cur_term.first;
                // when we reach a docID that contains query term, store some of the info for display: 
                // the query term itself
                // its freq in this document
                // how many documents contain this term
                posting_info.clear();
                posting_info.push_back(make_tuple(query[0],cur_term.second,lps[0]->docID_count));
                impact_score=0;
                // call nextGEQ for each of the terms in this query, take note of posting info on the run
                
                for (int i=1; i<query.size(); i++){
                    cur_term=nextGEQ(lps[i], docID);
                    d=cur_term.first;
                    posting_info.push_back(make_tuple(query[i],cur_term.second,lps[i]->docID_count));
                    if(d != docID){
                        break;
                    }
                };
                // some of the query terms do not exist in the same document as the first one, continue from there
                if(d>docID){
                    docID=d;
                }
                else{
                    // docID list for one of the terms is exhausted, so no more possible results
                    if(docID==MAXDOCID+1){
                        sort_heap(result.begin(),result.end(),comparison);
                        return result;
                    }
                    // not exhausted, so query terms all exist in the same document
                    for(int i=0; i<query.size();i++){
                        impact_score+=impactScore(lps[i]);
                    }
                    // if heap is not at desired size, just push into it
                    if(result.size()<k){
                        result.push_back(make_tuple(docID,impact_score,posting_info));
                        push_heap(result.begin(),result.end(),comparison);
                    }
                    // if the new one is better than the worst of previous results, pop the worst one and push the new one
                    else{
                        if(get<1>(result.front())<impact_score){
                            pop_heap(result.begin(),result.end(),comparison);
                            result.pop_back();
                            result.push_back(make_tuple(docID,impact_score,posting_info));
                            push_heap(result.begin(),result.end(),comparison);
                        }
                    }
                    // continue with other docIDs
                    docID++;
                }

            }
            //close all the list pointers created
            for(int i=0; i<query.size();i++){
                lps[i]->index.close();
            }
            sort_heap(result.begin(),result.end(),comparison);
            return result;
        }
        //bm25 calculator
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
        //display snippets in an output file
        //it is just going through a vector and do some file writes
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
        //generate snippets by finding the smallest window that contain all query terms
        string generate_snippet(int docID, const vector<string>& query){
            vector<string> doc;
            string word;
            ifstream ifile("fulldocs-new.trec",ios::in);
            ifile.seekg(doc_lookup[docID].first);
            //get the url first
            ifile>>word;
            string result;
            result.append(word+"\n");
            //load the other part of the document into a vector
            for(int j=0;j<doc_lookup[docID].second-1;j++){
                ifile>>word;
                doc.push_back(word);
            }
            ifile.close();
            //create a hashmap to store all query terms
            unordered_map<string,int> occurance;
            for(int i=0;i<query.size();i++){
                if(occurance.find(to_lower(query[i]))==occurance.end()){
                    occurance.insert(make_pair(to_lower(query[i]),0));
                }                
            }
            // get to know how many distinctive terms exist so that we can use it to check if all query terms exist later
            int distinct_term=occurance.size();
            
            int min_len=INT_MAX;
            pair<int,int> min_interval=make_pair(-1,-1);
            int count=0;
            int lp=0;
            int rp=0;
            
            // if there is only one term in the query, find its first appearance
            if(distinct_term==1){
                string curword;
                string subword;
                char c;
                min_len=1;
                for(int i=0;i<doc.size();i++){
                    curword=to_lower(doc[i]);
                    for(char c:curword){
                        if(!isalpha(c)&&!isdigit(c)){
                            if(subword==query[0]){
                                lp=i;
                                rp=i;
                                min_interval=make_pair(i,i);
                                break;
                            }
                            subword.clear();
                        }
                        else{
                            subword.push_back(c);
                        }
                    }
                    if(min_interval.first!=-1){
                        break;
                    }
                    //deal with the leftover
                    if(!isalpha(c)&&!isdigit(c)){
                        if(subword==query[0]){
                            lp=i;
                            rp=i;
                            min_interval=make_pair(i,i);
                        }
                    }
                    subword.clear();
                }
            }else{
                //go through each word
                string curword;
                while(rp<=doc.size()){
                    //if we currently have all terms in the window, we can move the left pointer
                    if(count==distinct_term){
                        if(rp-lp<min_len){
                            min_interval=make_pair(lp,rp);  
                            min_len=rp-lp;                      
                        }
                        string subword;
                        curword=to_lower(doc[lp]);
                        // each string read in from ifstream may contain some puncuation marks
                        // we want to check all the subwords divided by them
                        for(char c:curword){
                            if(!isalpha(c)&&!isdigit(c)){
                                if(!subword.empty()){
                                    if(occurance.find(subword)!=occurance.end()){
                                        occurance[subword]--;
                                        //if removing a query term makes it nonexisting in the window, flag it by subtracting from count
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
                        //deal with the leftover
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
                    //if right pointer is at end of doc, no more word to process
                    if(rp==doc.size()){
                        break;
                    }
                    string subword;
                    curword=to_lower(doc[rp]);
                    //if not all query terms are in the window, move right pointer
                    for(char c:curword){
                        if(!isalpha(c)&&!isdigit(c)){
                            if(!subword.empty()){
                                if(occurance.find(subword)!=occurance.end()){
                                    // if this is the only one of this query term, flag it found by incrementing count
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
                    //deal with the leftover
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
                result.append("snippet unavailable\n");
                for(int i=0;i<doc.size();i++){
                    result.append(doc[i]+" ");
                }
                result.append("\n");
                doc.clear();
                return result;
            }
            // if too long, hide the middle part and only get two 20-word string at the start and end
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
        //done by aggregation and collection
        vector<tuple<int,float,vector<tuple<string,int,int>>>> disjunctive_query(vector<string>& query_terms, int k){
            vector<tuple<string,int,int>> term_freqs;
            //initialize an array of empty impact scores and empty term info
            vector<pair<float,vector<tuple<string,int,int>>>> accumulator(MAXDOCID+1,make_pair(0,term_freqs));
            ListPointer* lp;
            // for each appearance of each term, add the impact score into the corresponding index of the accumulator
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
            //read through the accumulator and use a heap to collect the top k most relevant ones
            vector<tuple<int,float,vector<tuple<string,int,int>>>> heap;
            for(int i=0;i<accumulator.size();i++){
                if(accumulator[i].first!=0){
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
    vector<tuple<int,float,vector<tuple<string,int,int>>>> results;
    
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
            }
            else if(term!="AND"){
                query_terms.push_back(to_lower(term));
            }
        }
        start_time = chrono::steady_clock::now();
        if(conjunctive){
            results=filesys.conjunctive_query(query_terms,RESULT_COUNT);
            cout<<"got "<<results.size()<<" results"<<endl;
            filesys.show_snippets(results,query_terms);
            display_elapsed_time(start_time);
        }
        else{
            results=filesys.disjunctive_query(query_terms,RESULT_COUNT);
            cout<<"got "<<results.size()<<" results"<<endl;
            filesys.show_snippets(results,query_terms);
            display_elapsed_time(start_time);
            
        }

        results.clear();
        query_terms.clear();
    }
    
};