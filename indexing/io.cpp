#include <iostream>
#include <fstream>
#include <bitset>
#include <string>
#include <vector>
#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <regex>
#include <sstream>
using namespace std;



void write_compressed(ofstream& ofile, int num){
	vector<uint8_t> result;
	uint8_t b;
	while(num >= 128){
		int a = num % 128;
		bitset<8> byte(a);
		byte.flip(7);
		num = (num - a) / 128;
		b = byte.to_ulong();
		result.push_back(b);
	}
	int a = num % 128;
	bitset<8> byte(a);
	b = byte.to_ulong();
	result.push_back(b);
	for(vector<uint8_t>::iterator it = result.begin(); it != result.end(); it++){
		ofile.write(reinterpret_cast<const char*>(&(*it)), 1);
	}
}

int read_next(ifstream& ifile){
	char c;
	int num;
	int p;
    num = 0;
    p = 0;
    ifile.read(&c,1);
    bitset<8> byte(c);
    while(byte[7] == 1){
        byte.flip(7);
        num += byte.to_ulong()*pow(128, p);
        p++;
        ifile.read(&c,1);
        byte = bitset<8>(c);
    }
    num += (byte.to_ulong())*pow(128, p);
	
	return num;
}

// get the built in lexicon
unordered_map<string, int[]> get_lexicon(string file_path){
    unordered_map<string, int[]> lexicon;
    return lexicon;
} 

//read from file and write into intermediate postings
void get_intermediate_postings(){}

// Linked list object
struct Node {
  int data;
  struct Node *next;
};

// Posting object to be used in comparison
struct Posting{
    string term;
    struct Node* doc_id_head;
    struct Node* doc_id_tail;
    struct Node* frequency_head;
    struct Node* frequency_tail;
    Posting(string input_term){
        term=input_term;
        doc_id_head=nullptr;
        doc_id_tail=nullptr;
        frequency_head=nullptr;
        frequency_tail=nullptr;
    }
    // Free up memory after using!
    ~Posting(){
        Node* nextNode;
        Node* curNode=doc_id_head;
        while(curNode!=0){
            nextNode=curNode->next;
            delete curNode;
            curNode=nextNode;
        }
        curNode=frequency_head;
        while(curNode!=0){
            nextNode=curNode->next;
            delete curNode;
            curNode=nextNode;
        }
    }
    
    bool operator < (const Posting& other) const{
        if(term==other.term){
            return doc_id_head->data<other.doc_id_head->data;
        }
        return (term < other.term);
    }

    void push(int doc_id,int frequency){
        Node* newNode;
        newNode=new Node;
        newNode->data=doc_id;
        newNode->next=nullptr;
        Node* newFrequency;
        newFrequency=new Node;
        newFrequency->data=frequency;
        newFrequency->next=nullptr;
        if(!this->doc_id_head){
            this->doc_id_head=newNode;
        }
        else{
            this->doc_id_tail->next=newNode;
        }
        this->doc_id_tail=newNode;
        if(!this->frequency_head){
            this->frequency_head=newFrequency;
        }
        else{
            this->frequency_tail->next=newFrequency;
        }
        this->frequency_tail=newFrequency;
    }
    void print(){
        if(doc_id_head){
            cout<<term<<" DocIds:";
            Node* curNode=doc_id_head;
            while(curNode){
                cout<<curNode->data<<"->";
                curNode=curNode->next;
            }
            cout<<" Frequencies:";
            curNode=frequency_head;
            while(curNode){
                cout<<curNode->data<<"->";
                curNode=curNode->next;
            }
            cout<<endl;
        }
    }
    void write_intermediate(ofstream& ofs){
        Node* curDoc;
        Node* curFreq;
        curDoc=doc_id_head;
        curFreq=frequency_head;
        while(curDoc){
            ofs<<curDoc->data<<" "<<curFreq->data<<" ";
            curDoc=curDoc->next;
            curFreq=curFreq->next;
        }
    }
};

bool compare_postings(Posting* p1, Posting* p2){
    return *p1<*p2;
}

Posting* merge_postings(Posting* p1, Posting* p2){
    if(p1->term!=p2->term){
            cout<<"Cannot add two postings of different terms"<<endl;
        }
        p1->doc_id_tail->next=p2->doc_id_head;
        p1->doc_id_tail=p2->doc_id_tail;
        p1->frequency_tail->next=p2->frequency_head;
        p1->frequency_tail=p2->frequency_tail;
        return p1;
}

void load_parse_buffer(ifstream& ifs, vector<Posting*>& buffer, ofstream& url_map, int& docID){
    string s;
    regex rgx("[a-zA-Z]+");
    smatch match;
    bool flag=false;
    bool get_url=false;
    Posting* posting;
    int count=0;
    unordered_map<string,int> map;
    while(count<100000){
        while(ifs>>s){
            if(s=="<TEXT>"){
                flag=true;
                get_url=true;
                continue;
            }
            if(s=="</TEXT>"){
                for (auto i : map){
                    posting = new Posting(i.first);
                    posting->push(docID,i.second);
                    buffer.push_back(posting);
                }
                map.clear();
                url_map<<endl;
                docID++;
                flag=false;
            }
            if(!flag){
                break;
            }
            if(get_url){
                url_map<<docID<<" "<<s;
                get_url=false;
                continue;
            }
            while(regex_search(s, match, rgx)){
                if(map.find(match.str())==map.end()){
                    map[match.str()]=1;
                }else{
                    map[match.str()]++;
                }
                s=match.suffix();
                count++;
            }
        }
            
    }
}



void load_merge_buffer(ifstream& reader, vector<Posting*>& buffer){
    string term;
    int doc_id;
    int frequency;
    int count=0;
    //limit the size of each buffer
    cout<<"load buffer called!"<<endl;
    while(count<2 && reader>>term){
        Posting* newPosting;
        newPosting = new Posting(term);
        reader.seekg(1,ios::cur);
        while (isdigit(reader.peek())){
            reader>>doc_id>>frequency;
            newPosting->push(doc_id,frequency);
            reader.seekg(1,ios::cur);
        }
        buffer.push_back(newPosting);
        newPosting->print();
        count++;
    }
} 

void sort_postings_vec(vector<Posting*>& postings){
    sort(postings.begin(),postings.end(),compare_postings);
}

void write_uncompressed(stringstream& ss,vector<Posting*>& buffer){
    Node* curDoc;
    Node* curFreq;
    for(int i=0;i<buffer.size();i++){
        ss<<buffer[i]->term<<" ";
        curDoc=buffer[i]->doc_id_head;
        curFreq=buffer[i]->frequency_head;
        while(curDoc){
            ss<<curDoc->data<<" "<<curFreq->data<<" ";
            curDoc=curDoc->next;
            curFreq=curFreq->next;
        }
        delete curDoc;
        delete curFreq;
    }
}

struct HeapNode{
    struct Posting* posting;
    int origin;
    int pos;
};

bool compare_heap_node(HeapNode* p1, HeapNode* p2){
    return !compare_postings(p1->posting,p2->posting);
}

HeapNode* get_next_heap_node(vector<vector<Posting*>> &buffers,int origin,int pos){
    if(pos+1>=buffers[origin].size()){
        return nullptr;
    }
    HeapNode* newNode;
    newNode=new HeapNode;
    cout<<"getting next heap node:"<<origin<<" "<<pos+1<<endl;
    buffers[origin][pos+1]->print();
    newNode->posting=buffers[origin][pos+1];
    newNode->origin=origin;
    newNode->pos=pos+1;
    return newNode;
}
void up_to_k_way_merge(int start, int k, int round,int outputID){
    vector<ifstream*> files;
    vector<HeapNode*> heap;
    vector<vector<Posting*>> buffers(k);
    string fileName;
    for(int i=0;i<k;i++){
        fileName="temp"+to_string(i+start)+"_round"+to_string(round)+".txt";
        cout<<"loading "<<fileName<<endl;
        ifstream* ifs= new ifstream(fileName,ios::in);
        if(!ifs){
            cout<<"unable to open file in merging"<<endl;
        }
        load_merge_buffer(*ifs,buffers[i]);
        heap.push_back(get_next_heap_node(buffers,i,-1));
        files.push_back(ifs);
    }

    string curTerm,prevTerm;
    HeapNode* curPosting;
    HeapNode* nextPosting;
    fileName="temp"+to_string(outputID)+"_round"+to_string(round+1)+".txt";
    ofstream ofs(fileName,ios::out);
    int origin,pos;
    make_heap(heap.begin(),heap.end(),compare_heap_node);
    prevTerm="";
    while(!heap.empty()){
        cout<<"heap front:"<<endl;
        heap.front()->posting->print();
        curPosting=heap.front();
        
        cout<<"processing"<<endl;
        curPosting->posting->print();
        curTerm=curPosting->posting->term;
        if(prevTerm.compare(curTerm)!=0){
            prevTerm=curTerm;
            ofs<<curTerm<<" ";
        }
        //write doc freq doc freq... into intermediate files
        curPosting->posting->write_intermediate(ofs);
        origin=curPosting->origin;
        pos=curPosting->pos;
        nextPosting=get_next_heap_node(buffers,origin,pos);
        
        pop_heap(heap.begin(),heap.end(),compare_heap_node);
        heap.pop_back();
        if(!nextPosting){
            //exahusted buffer, load new buffer in there!
            buffers[origin].clear();
            load_merge_buffer(*files[origin],buffers[origin]);
            nextPosting=get_next_heap_node(buffers,origin,0);
            //reset the pos to 0 so read from the start of the newly added buffer!
        }
        if(nextPosting){
            heap.push_back(nextPosting);
            push_heap(heap.begin(),heap.end(),compare_heap_node);
        }
    }
    cout<<"end of merging!"<<endl;
    ofs<<endl;
    ofs.close();
}

void write_buffer_to_file(stringstream& ss, ofstream& ofs){
    ofs << ss.rdbuf();
}

int parse_all(string file){
    int docID=0;
    int fileID=-1;
    ifstream ifs(file,ios::in);
    std::stringstream ss;
    if(!ifs){
            cout<<"unable to open input file in parse all."<<endl;
            return 0;
        }
    vector<Posting*> buffer;
    //dictionary
    ofstream ofs("docMap.txt",ios::out);
    //interm posting
    ofstream ofile;
    load_parse_buffer(ifs,buffer,ofs,docID);
    while(buffer.size()!=0){
        sort_postings_vec(buffer);
        fileID++;
        ofile.open("temp"+to_string(fileID)+"_round0.txt",ios::out);
        if(!ofile){
            cout<<"unable to open output file in parse all."<<endl;
            break;
        }
        write_uncompressed(ss,buffer);
        write_buffer_to_file(ss,ofile);
        ofile.close();
        for(int j = 0, i = buffer.size(); j < i ; j++){
            Posting* ptr=buffer.at(j);
            delete ptr;
        }
        buffer.clear();
        load_parse_buffer(ifs,buffer,ofs,docID);
    }
    return fileID;
}

int main() {
    
    // up_to_k_way_merge(0,16,0,0);
    parse_all("fulldocs-new.trec");
    return 0;
}