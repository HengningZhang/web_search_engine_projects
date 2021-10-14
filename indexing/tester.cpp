#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>


using namespace std;

int read_byte(unsigned char buffer[], int& array_pos){
    int result=(int)buffer[array_pos];
    array_pos++;
    return result;
}

int varbyte_decode(unsigned char buffer[], int& array_pos){
    int val=0, shift=0, b;
    while((b=read_byte(buffer, array_pos))<128){
        val=val+(b<<shift);
        shift=shift+7;
    }
    val=val+((b-128)<<shift);
    return val;
}

void display_elapsed_time(chrono::steady_clock::time_point start){
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    cout << "Elapsed time: "
        << chrono::duration_cast<chrono::milliseconds>(end - start).count()
        << " ms" << endl;
}


int main(){
    string QUERY_TERM="ability";
    const int BUFFER_SIZE = 1024*1024;
    vector<char> buffer (BUFFER_SIZE + 1, 0);
    ifstream lexicon;
    ofstream testing;
    lexicon.open("lexicon.txt", ios::in);
    testing.open("testing.txt",ios::binary);
    string s;
    long long pos;
    while(lexicon>>s){
        if(s==QUERY_TERM){
            lexicon>>pos;
            cout<<"info for "<<QUERY_TERM<<" starts from position "<<pos<<" in inverted index"<<endl;
            break;
        }
        lexicon>>pos;
    }
    unsigned char inverted_index_buffer[1024*5];
    ifstream inverted_index;
    inverted_index.open("inverted_index.txt",ios::binary);
    inverted_index.seekg(pos);
    int decode_pos=0;
    inverted_index.read((char*)inverted_index_buffer,1024*5);
    for(int i=0;i<300;i++){
        cout<<varbyte_decode(inverted_index_buffer,decode_pos)<<endl;
    }
    return 1;
}
