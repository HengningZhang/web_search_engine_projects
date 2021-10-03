#include <iostream>
#include <fstream>
#include <bitset>
#include <vector>
#include <cmath>
#include <unordered_map>
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
};

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
        cout<<term<<endl;
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
    ifstream reader("intermediate.txt",ios::in);
    if(!reader){
        cout<<"Error in load_buffer."<<endl;
    }
    vector<Posting*> buffer=load_buffer(reader);
    reader.close();
    for(int i=0;i<buffer.size();i++){
        buffer[i]->print();
    }
    buffer[0]=merge_postings(buffer[0],buffer[4]);
    buffer[0]->print();
    return 0;
}