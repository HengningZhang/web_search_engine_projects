#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>


using namespace std;

//code copied from slides, my previous implementation was slower since I was doing division
//write an integer<128 into buffer
void write_byte(unsigned char buffer[], int& array_pos, int num){
    buffer[array_pos]=(unsigned char)num;
    array_pos++;
}

//read the next byte from buffer
int read_byte(unsigned char buffer[], int& array_pos){
    int result=(int)buffer[array_pos];
    array_pos++;
    return result;
}

//write a long long into buffer, write into file if buffer is full
int varbyte_encode(unsigned char buffer[], int& array_pos, long long num, ofstream& file, int size_limit){
    int size=1;
    while(num>127){
        size++;
        if(array_pos>=size_limit){
            file.write((char*) buffer,array_pos);
            array_pos=0;
        }
        write_byte(buffer,array_pos,num&127);
        num=num>>7;
    }

    write_byte(buffer,array_pos,128+num);
    return size;
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

//display time so we there is something to look at while it is running
void display_elapsed_time(chrono::steady_clock::time_point start){
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    cout << "Elapsed time: "
        << chrono::duration_cast<chrono::milliseconds>(end - start).count()
        << " ms" << endl;
}

//costs around 750s to finish, using a sorted intermediate posting file of 20.8 GB
void build_index(){
    chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
    //1 GB input buffer size
    const int BUFFER_SIZE = 1024*1024*1024;
    //output buffer
    const int INVERTED_INDEX_BUFFER_SIZE=1000000;
    //input buffer
    vector<char> buffer (BUFFER_SIZE + 1, 0);
    string lexicon_buffer;
    int  inverted_index_buffer_pos=0;
    unsigned char inverted_index_buffer[INVERTED_INDEX_BUFFER_SIZE];
    ifstream sortedPostingFile;
    ofstream lexicon;
    ofstream inverted_index;
    ofstream docid_debug;
    ofstream frequency_debug;
    //debug files that checks if something gets thrown into the wrong place
    //should be empty if correct
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
    long long curPos=0;
    while(1){
        sortedPostingFile.read(buffer.data(), BUFFER_SIZE);
        streamsize s = ((sortedPostingFile) ? BUFFER_SIZE : sortedPostingFile.gcount());
        buffer[s] = 0;
        for(int i=0;i<s;i++){
            //reach the end of line of intermediate posting
            //means we need to store the curword as frequency
            //reset second to show now we are looking at the first column next, which is the term
            if(buffer[i]=='\n'){
                frequencies.push_back(stoi(curword));
                second=false;
            }
            //not blankspace, means we are reading into curword
            else if(buffer[i]!=' '){
                curword.push_back(buffer[i]);
                continue;
            }
            //at blankspace! the show is on
            else {
                //no curTerm! means we are just starting, set curTerm and proceed
                if(curTerm.compare("")==0){
                    curTerm=curword;
                }
                //if this is the second column, we need to store curword as docID
                else if(second){
                    docIDs.push_back(stoi(curword));
                }
                else{
                    //at the first column
                    //second is set to true because we are looking at the second column next turn.
                    second=true;
                    if(curword.compare(curTerm)!=0){
                        //getting a new word, push docID and frequency buffer contents into file
                        // push term info (term + starting position in inverted index file) in the file into lexicon
                        lexicon_buffer.append(curTerm+" "+to_string(curPos)+"\n");
                        //missed this line in the first run. How did I miss this??????????
                        // life -20min
                        curTerm=curword;
                        if(lexicon_buffer.size()>1024*1024){
                            lexicon.write(lexicon_buffer.c_str(),lexicon_buffer.size());
                            lexicon_buffer.clear();
                        }
                        //write the number of postings into inverted index
                        curPos=curPos+varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,docIDs.size(),inverted_index,INVERTED_INDEX_BUFFER_SIZE);
                        for(int k=0;k<docIDs.size();k++){
                            //write the docids and frequencies stored in vectors into a temporary buffer
                            //because we want to know the compressed sizes
                            varbyte_encode(temp_compressed_docIDs,docID_block_size,docIDs[k],docid_debug,300);
                            varbyte_encode(temp_compressed_frequencies,frequency_block_size,frequencies[k],frequency_debug,300);
                            blocksize++;
                            // using blocksize of 64
                            if(blocksize==64){
                                // leave enough room for last docID in this block(max 4 byte)+docID block size (max 2 byte)+frequency block size (max 2 byte) and the actual blocks
                                if(inverted_index_buffer_pos+docID_block_size+frequency_block_size+8>INVERTED_INDEX_BUFFER_SIZE){
                                    inverted_index.write((char*) inverted_index_buffer,inverted_index_buffer_pos);
                                    inverted_index_buffer_pos=0;
                                    cout<<curword<<endl;
                                    display_elapsed_time(start_time);
                                }
                                //last docID in this block
                                curPos=curPos+varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,docIDs[k],inverted_index,INVERTED_INDEX_BUFFER_SIZE);
                                //size of this docID block
                                curPos=curPos+varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,docID_block_size,inverted_index,INVERTED_INDEX_BUFFER_SIZE);
                                //size of this frequency block
                                curPos=curPos+varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,frequency_block_size,inverted_index,INVERTED_INDEX_BUFFER_SIZE);
                                //loading all docIDs in the block into output buffer
                                for(int j=0;j<docID_block_size;j++){
                                    inverted_index_buffer[inverted_index_buffer_pos+j]=(unsigned char)temp_compressed_docIDs[j];
                                }
                                //move curPos by compressed docID block size
                                inverted_index_buffer_pos=inverted_index_buffer_pos+docID_block_size;
                                curPos=curPos+docID_block_size;
                                docID_block_size=0;
                                //loading all frequencies in the block into output buffer
                                for(int j=0;j<frequency_block_size;j++){
                                    inverted_index_buffer[inverted_index_buffer_pos+j]=(unsigned char)temp_compressed_frequencies[j];
                                }
                                //move curPos by compressed frequency block size
                                inverted_index_buffer_pos=inverted_index_buffer_pos+frequency_block_size;
                                curPos=curPos+frequency_block_size;
                                frequency_block_size=0;
                                blocksize=0;
                            }
                        }
                        //deal with the last block
                        //same code all over again
                        //kinda messy
                        if(blocksize!=0){
                            if(inverted_index_buffer_pos+docID_block_size+frequency_block_size+8>INVERTED_INDEX_BUFFER_SIZE){
                                inverted_index.write((char*) inverted_index_buffer,inverted_index_buffer_pos);
                                inverted_index_buffer_pos=0;
                                cout<<curword<<endl;
                                display_elapsed_time(start_time);
                            }
                            curPos=curPos+varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,docIDs[docIDs.size()-1],inverted_index,INVERTED_INDEX_BUFFER_SIZE);
                            curPos=curPos+varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,docID_block_size,inverted_index,INVERTED_INDEX_BUFFER_SIZE);
                            curPos=curPos+varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,frequency_block_size,inverted_index,INVERTED_INDEX_BUFFER_SIZE);
                            for(int j=0;j<docID_block_size;j++){
                                inverted_index_buffer[inverted_index_buffer_pos+j]=(unsigned char)temp_compressed_docIDs[j];
                            }
                            inverted_index_buffer_pos=inverted_index_buffer_pos+docID_block_size;
                            curPos=curPos+docID_block_size;
                            docID_block_size=0;
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
    //same codes again!
    lexicon_buffer.append(curTerm+" "+to_string(curPos)+"\n");
    lexicon.write(lexicon_buffer.c_str(),lexicon_buffer.size());
    lexicon_buffer.clear();
    varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,docIDs.size(),inverted_index,INVERTED_INDEX_BUFFER_SIZE);
    for(int k=0;k<docIDs.size();k++){
        //difference here is we don't need to take care of curPos anymore because the last entry of lexicon is already done.
        varbyte_encode(temp_compressed_docIDs,docID_block_size,docIDs[k],docid_debug,300);
        varbyte_encode(temp_compressed_frequencies,frequency_block_size,frequencies[k],frequency_debug,300);
        blocksize++;
        if(blocksize==64){
            if(inverted_index_buffer_pos+docID_block_size+frequency_block_size+8>INVERTED_INDEX_BUFFER_SIZE){
                inverted_index.write((char*) inverted_index_buffer,inverted_index_buffer_pos);
                inverted_index_buffer_pos=0;
                cout<<curword<<endl;
                display_elapsed_time(start_time);
            }
            varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,docIDs[k],inverted_index,INVERTED_INDEX_BUFFER_SIZE);
            curPos=curPos+varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,docID_block_size,inverted_index,INVERTED_INDEX_BUFFER_SIZE);
            curPos=curPos+varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,frequency_block_size,inverted_index,INVERTED_INDEX_BUFFER_SIZE);
            for(int j=0;j<docID_block_size;j++){
                inverted_index_buffer[inverted_index_buffer_pos+j]=(unsigned char)temp_compressed_docIDs[j];
            }
            inverted_index_buffer_pos+=docID_block_size;
            docID_block_size=0;
            for(int j=0;j<frequency_block_size;j++){
                inverted_index_buffer[inverted_index_buffer_pos+j]=(unsigned char)temp_compressed_frequencies[j];
            }
            inverted_index_buffer_pos+=frequency_block_size;
            frequency_block_size=0;
            blocksize=0;
        }
    }
    //this is the last occurance of the same code block.
    //to deal with the last block of the last term.
    if(blocksize!=0){
        if(inverted_index_buffer_pos+docID_block_size+frequency_block_size+8>INVERTED_INDEX_BUFFER_SIZE){
            inverted_index.write((char*) inverted_index_buffer,inverted_index_buffer_pos);
            inverted_index_buffer_pos=0;
            //make sure this is actually run.
            cout<<"last block"<<endl;
            cout<<curword<<endl;
            display_elapsed_time(start_time);
        }
        varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,docIDs[docIDs.size()-1],inverted_index,INVERTED_INDEX_BUFFER_SIZE);
        varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,docID_block_size,inverted_index,INVERTED_INDEX_BUFFER_SIZE);
        varbyte_encode(inverted_index_buffer,inverted_index_buffer_pos,frequency_block_size,inverted_index,INVERTED_INDEX_BUFFER_SIZE);
        for(int j=0;j<docID_block_size;j++){  
            inverted_index_buffer[inverted_index_buffer_pos+j]=(unsigned char)temp_compressed_docIDs[j];
        }
        inverted_index_buffer_pos+=docID_block_size;
        docID_block_size=0;
        for(int j=0;j<frequency_block_size;j++){
            inverted_index_buffer[inverted_index_buffer_pos+j]=(unsigned char)temp_compressed_frequencies[j];
        }
        inverted_index_buffer_pos+=frequency_block_size;
        frequency_block_size=0;
        blocksize=0;
    }
    docIDs.clear();
    frequencies.clear();
    //write everything left in the inverted index buffer in
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
