#include <iostream>
#include <fstream>
#include <bitset>
#include <string>
#include <vector>
#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <functional>
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
        delete &term;
        while(doc_id_head){
            nextNode=doc_id_head->next;
            delete doc_id_head;
            doc_id_head=nextNode;
        }
        delete doc_id_tail;
        while(frequency_head){
            nextNode=frequency_head->next;
            delete frequency_head;
            frequency_head=nextNode;
        }
        delete frequency_tail;
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

vector<Posting*> load_buffer(ifstream& reader){
    string term;
    int doc_id;
    int frequency;
    int count=0;
    vector<Posting*> buffer;
    //limit the size of each buffer
    while(count<200000 && reader>>term){
        Posting* newPosting;
        newPosting = new Posting(term);
        reader.seekg(1,ios::cur);
        while (isdigit(reader.peek())){
            reader>>doc_id>>frequency;
            newPosting->push(doc_id,frequency);
            reader.seekg(1,ios::cur);
        }
        buffer.push_back(newPosting);
        count++;
    }
    return buffer;
} 

void sort_postings_vec(vector<Posting*>& postings){
    sort(postings.begin(),postings.end(),compare_postings);
}

void write_uncompressed(ofstream& ofile,vector<Posting*>& buffer){
    for(int i=0;i<buffer.size();i++){
        ofile<<buffer[i]->term<<" ";
        Node* curDoc=buffer[i]->doc_id_head;
        Node* curFreq=buffer[i]->frequency_head;
        while(curDoc){
            ofile<<curDoc->data<<" "<<curFreq->data<<" ";
            curDoc=curDoc->next;
            curFreq=curFreq->next;
        }
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
    vector<vector<Posting*>> buffers;
    string fileName;
    for(int i=0;i<k;i++){
        fileName="temp"+to_string(i+start)+"_round"+to_string(round)+".txt";
        cout<<"loading "<<fileName<<endl;
        ifstream ifs(fileName,ios::in);
        if(!ifs){
            cout<<"unable to open file in merging"<<endl;
        }
        buffers.push_back(load_buffer(ifs));
        heap.push_back(get_next_heap_node(buffers,i,-1));
        files.push_back(&ifs);
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
        cout<<"current heap content"<<endl;
        heap[0]->posting->print();
        heap[1]->posting->print();
        cout<<compare_heap_node(heap[0],heap[1])<<endl;
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
            buffers[origin]=load_buffer(*files[origin]);
            nextPosting=get_next_heap_node(buffers,origin,pos);
        }
        if(nextPosting){
            heap.push_back(nextPosting);
            push_heap(heap.begin(),heap.end(),compare_heap_node);
        }
    }
    ofs<<endl;
    ofs.close();
}

int main() {
    // ofstream wf("student.txt", ios::out | ios::binary);
    // if(!wf) {
    //     cout << "Cannot open file!" << endl;
    //     return 1;
    // }
    // write_compressed(wf,5);
    // write_compressed(wf,100);
    // write_compressed(wf,56710565);
    // wf.close();

    // if(!wf.good()) {
    //     cout << "Error occurred at writing time!" << endl;
    //     return 1;
    // }

    // ifstream rf("student.txt", ios::out | ios::binary);
    // if(!rf) {
    //     cout << "Cannot open file!" << endl;
    //     return 1;
    // }

    // rf.seekg(1);
    // cout<<read_next(rf)<<endl;
    // rf.close();

    // if(!rf.good()) {
    //     cout << "Error occurred at reading time!" << endl;
    //     return 1;
    // }
    // ifstream reader("intermediate.txt",ios::in);
    // if(!reader){
    //     cout<<"Error in load_buffer."<<endl;
    // }
    // vector<Posting*> buffer=load_buffer(reader);
    // vector<Posting*> buffer2=load_buffer(reader);
    // reader.close();
    // cout<<"buffer2 size:"<<buffer2.size()<<endl;
    // for(int i=0;i<buffer.size();i++){
    //     buffer[i]->print();
    // }
    // sort_postings_vec(buffer);
    // for(int i=0;i<buffer.size();i++){
    //     buffer[i]->print();
    // }
    // ofstream writer("temp0.txt",ios::out);
    // if(!writer){
    //     cout<<"Error when trying to write."<<endl;
    // }
    // write_uncompressed(writer,buffer);
    // writer.close();
    up_to_k_way_merge(0,2,0,0);
    return 0;
}