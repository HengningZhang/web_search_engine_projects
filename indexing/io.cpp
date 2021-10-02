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

// Linked list object
struct Node {
  int data;
  struct Node *next;
};

// Posting object to be used in comparison
struct Posting{
    string term;
    struct Node* head;
    struct Node* tail;
    Posting(string input_term){
        term=input_term;
        head=nullptr;
        tail=nullptr;
    }
    // Free up memory after using!
    ~Posting(){
        Node* nextNode;
        delete &term;
        while(head){
            nextNode=head->next;
            delete head;
            head=nextNode;
        }
    }
    bool operator < (const Posting& other) const{
        if(term==other.term){
            return head->data<other.head->data;
        }
        return (term < other.term);
    }
    void push(int pos){
        Node* newNode;
        newNode=new Node;
        newNode->data=pos;
        newNode->next=nullptr;
        if(!this->head){
            this->head=newNode;
        }
        else{
            this->tail->next=newNode;
        }
        this->tail=newNode;
    }
    void print(){
        if(head){
            cout<<term<<":";
            Node* curNode=head;
            while(curNode){
                cout<<curNode->data<<"->";
                curNode=curNode->next;
            }
            cout<<endl;
        }
    }
};

vector<Posting*> load_buffer(string filename){
    ifstream reader(filename,ios::in);
    if(!reader){
        cout<<"Error in load_buffer."<<endl;
    }
    string term;
    
    int pos;
    vector<Posting*> buffer;
    
    while(reader>>term){
        cout<<term<<endl;
        Posting* newPosting;
        newPosting = new Posting(term);
        reader.seekg(1,ios::cur);
        while (isdigit(reader.peek())){
            reader>>pos;
            cout<<pos<<endl;
            newPosting->push(pos);
            reader.seekg(1,ios::cur);
        }
        newPosting->print();
        cout<<"ending print"<<endl;
        buffer.push_back(newPosting);
        cout<<"ending push"<<endl;
    }
    cout<<"ending buffer"<<endl;
    reader.close();
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
    vector<Posting*> buffer=load_buffer("intermediate.txt");
    for(int i=0;i<buffer.size();i++){
        buffer[i]->print();
    }
    return 0;
}