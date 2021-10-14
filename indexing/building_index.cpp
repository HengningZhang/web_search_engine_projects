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
    int size=1;
    while(num>127){
        size++;
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

void build_index(){
    chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
    const int BUFFER_SIZE = 1024*1024*1024;
    const int INVERTED_INDEX_BUFFER_SIZE=1000000;
    vector<char> buffer (BUFFER_SIZE + 1, 0);
    string lexicon_buffer;
    int  inverted_index_buffer_pos=0;
    unsigned char inverted_index_buffer[INVERTED_INDEX_BUFFER_SIZE];
    ifstream sortedPostingFile;
    ofstream lexicon;
    ofstream inverted_index;
    ofstream docid_debug;
    ofstream frequency_debug;
    docid_debug.open("docid_debug.txt",ios::binary);
    frequency_debug.open("frequency_debug.txt",ios::binary);
    sortedPostingFile.open("sorted_postings.txt", ios::in);
    lexicon.open("lexicon.txt",ios::out);
    inverted_index.open("inverted_index.txt",ios::binary);
    string curTerm="";
    string curword;
    string subword;
    vector<int> docIDs;
    vector<int> frequencies;
    int docID_block_size=0;
    int frequency_block_size=0;
    unsigned char temp_compressed_docIDs[300];
    unsigned char temp_compressed_frequencies[300];
    int blocksize=0;
    //to flag we are getting the second one of each line
    bool second=false;
    unsigned long long curPos=0;
    while(1){
        sortedPostingFile.read(buffer.data(), BUFFER_SIZE);
        streamsize s = ((sortedPostingFile) ? BUFFER_SIZE : sortedPostingFile.gcount());
        buffer[s] = 0;
        for(int i=0;i<s;i++){
            if(buffer[i]=='\n'){
                frequencies.push_back(stoi(curword));
                second=false;
            }
            else if(buffer[i]!=' '){
                curword.push_back(buffer[i]);
                continue;
            }
            else {
                if(curTerm.compare("")==0){
                    curTerm=curword;
                }
                else if(second){
                    docIDs.push_back(stoi(curword));
                }
                else{
                    second=true;
                    if(curword.compare(curTerm)!=0){
                        //getting a new word, push docID and frequency buffer contents into file
                        // push word info starting position in the file into lexicon
                        lexicon_buffer.append(curTerm+" "+to_string(curPos)+"\n");
                        //missed this line in the first run. How did I miss this??????????
                        // life -20min
                        curTerm=curword;
                        if(lexicon_buffer.size()>1024*1024){
                            lexicon.write(lexicon_buffer.c_str(),lexicon_buffer.size());
                            lexicon_buffer.clear();
                        }
                        
                        curPos=curPos+varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,docIDs.size(),inverted_index,INVERTED_INDEX_BUFFER_SIZE);
                        for(int k=0;k<docIDs.size();k++){
                            varbyte_encode(temp_compressed_docIDs,docID_block_size,docIDs[k],docid_debug,300);
                            varbyte_encode(temp_compressed_frequencies,frequency_block_size,frequencies[k],frequency_debug,300);
                            blocksize++;
                            if(blocksize==64){
                                // leave enough room for docID block size (max 2 byte)+frequency block size (max 2 byte) and the actual blocks
                                if(inverted_index_buffer_pos+docID_block_size+frequency_block_size+4>INVERTED_INDEX_BUFFER_SIZE){
                                    inverted_index.write((char*) inverted_index_buffer,inverted_index_buffer_pos);
                                    inverted_index_buffer_pos=0;
                                    cout<<curword<<endl;
                                    display_elapsed_time(start_time);
                                }
                                
                                //end position for this docID block
                                curPos=curPos+varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,docID_block_size,inverted_index,INVERTED_INDEX_BUFFER_SIZE);
                                for(int j=0;j<docID_block_size;j++){
                                    inverted_index_buffer[inverted_index_buffer_pos+j]=(unsigned char)temp_compressed_docIDs[j];
                                }
                                inverted_index_buffer_pos=inverted_index_buffer_pos+docID_block_size;
                                curPos=curPos+docID_block_size;
                                docID_block_size=0;
                                //end position for this frequency block
                                curPos=curPos+varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,frequency_block_size,inverted_index,INVERTED_INDEX_BUFFER_SIZE);
                                for(int j=0;j<frequency_block_size;j++){
                                    inverted_index_buffer[inverted_index_buffer_pos+j]=(unsigned char)temp_compressed_frequencies[j];
                                }
                                inverted_index_buffer_pos=inverted_index_buffer_pos+frequency_block_size;
                                curPos=curPos+frequency_block_size;
                                frequency_block_size=0;
                                blocksize=0;
                            }
                        }
                        //deal with the last block
                        if(blocksize!=0){
                            if(inverted_index_buffer_pos+docID_block_size+frequency_block_size+4>INVERTED_INDEX_BUFFER_SIZE){
                                inverted_index.write((char*) inverted_index_buffer,inverted_index_buffer_pos);
                                inverted_index_buffer_pos=0;
                                cout<<curword<<endl;
                                display_elapsed_time(start_time);
                            }
                            //end position for this docID block
                            curPos=curPos+varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,docID_block_size,inverted_index,INVERTED_INDEX_BUFFER_SIZE);
                            for(int j=0;j<docID_block_size;j++){
                                inverted_index_buffer[inverted_index_buffer_pos+j]=(unsigned char)temp_compressed_docIDs[j];
                            }
                            inverted_index_buffer_pos=inverted_index_buffer_pos+docID_block_size;
                            curPos=curPos+docID_block_size;
                            docID_block_size=0;
                            //end position for this frequency block
                            curPos=curPos+varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,frequency_block_size,inverted_index,INVERTED_INDEX_BUFFER_SIZE);
                            for(int j=0;j<frequency_block_size;j++){
                                inverted_index_buffer[inverted_index_buffer_pos+j]=(unsigned char)temp_compressed_frequencies[j];
                            }
                            inverted_index_buffer_pos=inverted_index_buffer_pos+frequency_block_size;
                            curPos=curPos+frequency_block_size;
                            frequency_block_size=0;
                            blocksize=0;
                        }
                        docIDs.clear();
                        frequencies.clear();
                    }
                }
            }
            curword.clear();
        }
        if(!sortedPostingFile){
            break;
        }
    }
    //deal with the last term
    lexicon_buffer.append(curTerm+" "+to_string(curPos)+"\n");
    lexicon.write(lexicon_buffer.c_str(),lexicon_buffer.size());
    lexicon_buffer.clear();
    //push docID and frequency buffer contents into file
    varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,docIDs.size(),inverted_index,INVERTED_INDEX_BUFFER_SIZE);
    for(int k=0;k<docIDs.size();k++){
        varbyte_encode(temp_compressed_docIDs,docID_block_size,docIDs[k],docid_debug,300);
        varbyte_encode(temp_compressed_frequencies,frequency_block_size,frequencies[k],frequency_debug,300);
        blocksize++;
        if(blocksize==64){
            // leave enough room for docID block size (max 2 byte)+frequency block size (max 2 byte) and the actual blocks
            if(inverted_index_buffer_pos+docID_block_size+frequency_block_size+4>INVERTED_INDEX_BUFFER_SIZE){
                inverted_index.write((char*) inverted_index_buffer,inverted_index_buffer_pos);
                inverted_index_buffer_pos=0;
                cout<<curword<<endl;
                display_elapsed_time(start_time);
            }
            //end position for this docID block
            varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,docID_block_size,inverted_index,INVERTED_INDEX_BUFFER_SIZE);
            for(int j=0;j<docID_block_size;j++){
                inverted_index_buffer[inverted_index_buffer_pos+j]=(unsigned char)temp_compressed_docIDs[j];
            }
            inverted_index_buffer_pos+=docID_block_size;
            docID_block_size=0;
            //end position for this frequency block
            varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,frequency_block_size,inverted_index,INVERTED_INDEX_BUFFER_SIZE);
            for(int j=0;j<frequency_block_size;j++){
                inverted_index_buffer[inverted_index_buffer_pos+j]=(unsigned char)temp_compressed_frequencies[j];
            }
            inverted_index_buffer_pos+=frequency_block_size;
            frequency_block_size=0;
            blocksize=0;
        }
    }
    //deal with the last block
    if(blocksize!=0){
        if(inverted_index_buffer_pos+docID_block_size+frequency_block_size+4>INVERTED_INDEX_BUFFER_SIZE){
            inverted_index.write((char*) inverted_index_buffer,inverted_index_buffer_pos);
            inverted_index_buffer_pos=0;
            cout<<"last block"<<endl;
            cout<<curword<<endl;
            display_elapsed_time(start_time);
        }
        //end position for this docID block
        varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,docID_block_size,inverted_index,INVERTED_INDEX_BUFFER_SIZE);
        for(int j=0;j<docID_block_size;j++){  
            inverted_index_buffer[inverted_index_buffer_pos+j]=(unsigned char)temp_compressed_docIDs[j];
        }
        inverted_index_buffer_pos+=docID_block_size;
        docID_block_size=0;
        //end position for this frequency block
        varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,frequency_block_size,inverted_index,INVERTED_INDEX_BUFFER_SIZE);
        for(int j=0;j<frequency_block_size;j++){
            inverted_index_buffer[inverted_index_buffer_pos+j]=(unsigned char)temp_compressed_frequencies[j];
        }
        inverted_index_buffer_pos+=frequency_block_size;
        frequency_block_size=0;
        blocksize=0;
    }
    docIDs.clear();
    frequencies.clear();
    //write everything in
    if(inverted_index_buffer_pos!=0){
        cout<<"last one"<<endl;
        cout<<curword<<endl;
        display_elapsed_time(start_time);
        
        inverted_index.write((char*) inverted_index_buffer,inverted_index_buffer_pos);
        inverted_index_buffer_pos=0;
    }
}

int main(){
    build_index();
    return 1;
}
