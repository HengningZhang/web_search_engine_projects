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
#include <chrono>


using namespace std;

void write_compressed(ofstream& ofile, long long num){
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


//write an integer
void write_byte(unsigned char buffer[], int& array_pos, int num){
    buffer[array_pos]=(unsigned char)num;
    array_pos++;
}

int read_byte(unsigned char buffer[], int& array_pos){
    int result=(int)buffer[array_pos];
    array_pos++;
    return result;
}

int varbyte_encode(unsigned char buffer[], int& array_pos, int num, ofstream& file, int size_limit){
    int size;
    while(num>127){
        if(!array_pos<size_limit){
            file.write((char*) buffer,array_pos);
            array_pos=0;
        }
        write_byte(buffer,array_pos,num&127);
        num=num>>7;
    }
    write_byte(buffer,array_pos,128+num);
    return size;
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
    const int BUFFER_SIZE = 1024*1024;
    vector<char> buffer (BUFFER_SIZE + 1, 0);
    string lexicon_buffer;
    string output_buffer;
    ifstream lexicon;
    ofstream testing;
    lexicon.open("lexicon.txt", ios::in);
    testing.open("testing.txt",ios::binary);
    string s;
    long long pos;
    while(lexicon>>s){
        if(s=="accepted"){
            lexicon>>pos;
            cout<<pos<<endl;
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
