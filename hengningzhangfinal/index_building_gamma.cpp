#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <math.h>

using namespace std;

//code copied from slides, my previous implementation was slower since I was doing division
//write an integer<128 into buffer
void write_byte(unsigned char buffer[], int& array_pos, int num){
    buffer[array_pos]=(unsigned char)num;
    array_pos++;
}

void write_byte_varbyte_buffer(vector<bool>& buffer, int num){
    vector<bool> temp;
    while(num>0){
        temp.push_back(num%2);
        num/=2;
    }
    buffer.push_back(0);
    for(int i=temp.size()-1;i>-1;i--){
        buffer.push_back(temp[i]);
    }
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

int varbyte_encode_buffer(vector<bool>& buffer, int num){
    int size=1;
    while(num>127){
        size++;
        write_byte_varbyte_buffer(buffer,num&127);
        num=num>>7;
    }
    write_byte_varbyte_buffer(buffer,128+num);
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

//included from https://stackoverflow.com/questions/29623605/how-to-dump-stdvectorbool-in-a-binary-file
void binary_write(std::ofstream& fout, const std::vector<bool>& x)
{
    std::vector<bool>::size_type n = x.size();
    // fout.write((const char*)&n, sizeof(std::vector<bool>::size_type));
    vector<unsigned char> temp;
    for(std::vector<bool>::size_type i = 0; i < n;)
    {
        unsigned char aggr = 0;
        for(unsigned char mask = 1; mask > 0 && i < n; ++i, mask <<= 1)
            if(x.at(i))
                aggr |= mask;
        temp.push_back(aggr);
        
    }
    fout.write((const char*)temp.data(), temp.size());
    temp.clear();
}

void binary_read(std::ifstream& fin, std::vector<bool>& x, int num)
{
    std::vector<bool>::size_type n=num*8;
    x.resize(n);
    cout<<n<<endl;
    for(std::vector<bool>::size_type i = 0; i < n;)
    {
        unsigned char aggr;
        fin.read((char*)&aggr, sizeof(unsigned char));
        for(unsigned char mask = 1; mask > 0 && i < n; ++i, mask <<= 1)
            x.at(i) = aggr & mask;
    }
}

void write_unary(vector<bool>& buffer, int num){
    for(int i=0;i<num-1;i++){
        buffer.push_back(0);
    }
    buffer.push_back(1);
}

void decode_unary(vector<bool>& buffer, int& pos){

}

void write_binary_gamma(vector<bool>& buffer, int num){
    vector<bool> temp;
    while(num>0){
        temp.push_back(num%2);
        num/=2;
    }
    for(int i=temp.size()-2;i>-1;i--){
        buffer.push_back(temp[i]);
    }
}

void gamma_encode(vector<bool>& buffer, int num){
    write_unary(buffer,log2(num)+1);
    write_binary_gamma(buffer,num);
}

void gamma_fill(vector<bool>& buffer){
    int gap=8-buffer.size()%8;
    if(buffer.size()%8!=0){
        for(int i=0;i<gap;i++){
            buffer.push_back(0);
        }
    }
}

void build_index(){
    chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
    long long counter=0;
    string term;
    int docID,freq;
    long long curPos=0;
    vector<bool> output_buffer;
    vector<int> docIDs;
    vector<int> freqs;
    vector<bool> block_docID;
    vector<bool> block_freq;
    ifstream intermediate_postings;
    intermediate_postings.open("sorted_url.txt");
    ofstream inverted_index;
    ofstream lexicon;
    inverted_index.open("inverted_index_url_gamma.txt");
    lexicon.open("lexicon_url_gamma.txt");
    string lexicon_buffer;
    string curTerm;
    int blocksize;
    intermediate_postings>>curTerm>>docID>>freq;
    docIDs.push_back(docID);
    freqs.push_back(freq);
    lexicon_buffer.append(curTerm+" "+to_string(curPos)+"\n");
    while(intermediate_postings>>term>>docID>>freq){
        if(term==curTerm){
            docIDs.push_back(docID);
            freqs.push_back(freq);
        }else{
            counter++;
            if(counter%10000==0){
                cout<<curTerm<<endl;
                display_elapsed_time(start_time);
                counter=0;
            }
            
            lexicon_buffer.append(curTerm+" "+to_string(curPos)+"\n");
            
            curTerm=term;
            if(lexicon_buffer.size()>1024*1024){
                lexicon.write(lexicon_buffer.c_str(),lexicon_buffer.size());
                lexicon_buffer.clear();
            }
            //write num of postings
            curPos=curPos+varbyte_encode_buffer(output_buffer,docIDs.size());
            
            for(int i=0;i<docIDs.size();i++){
                if(blocksize==0){
                    if(docIDs.size()-i>64){
                        curPos=curPos+varbyte_encode_buffer(output_buffer,docIDs[i+63]);
                        for(int j=i+63;j>i;j--){
                            docIDs[j]=docIDs[j]-docIDs[j-1];
                        }
                    }else{
                        curPos=curPos+varbyte_encode_buffer(output_buffer,docIDs[docIDs.size()-1]);
                        for(int j=docIDs.size()-1;j>i;j--){
                            docIDs[j]=docIDs[j]-docIDs[j-1];
                        }
                    }
                    //first in block
                    varbyte_encode_buffer(block_docID,docIDs[i]);
                }else{
                    varbyte_encode_buffer(block_docID,docIDs[i]);
                }
                gamma_encode(block_freq,freqs[i]);
                blocksize++;
                if(blocksize==64){
                    gamma_fill(block_docID);
                    gamma_fill(block_freq);
                    curPos=curPos+varbyte_encode_buffer(output_buffer,block_docID.size()+block_freq.size());
                    binary_write(inverted_index,output_buffer);
                    output_buffer.clear();
                    binary_write(inverted_index,block_docID);
                    binary_write(inverted_index,block_freq);
                    blocksize=0;
                    block_docID.clear();
                    block_freq.clear();
                }
            }
            docIDs.clear();
            freqs.clear();
            //last block
            gamma_fill(block_docID);
            gamma_fill(block_freq);
            curPos=curPos+varbyte_encode_buffer(output_buffer,block_docID.size()+block_freq.size());
            binary_write(inverted_index,output_buffer);
            output_buffer.clear();
            binary_write(inverted_index,block_docID);
            binary_write(inverted_index,block_freq);
            block_docID.clear();
            block_freq.clear();
        }

    }
    //last term
    lexicon_buffer.append(curTerm+" "+to_string(curPos)+"\n");
    lexicon.write(lexicon_buffer.c_str(),lexicon_buffer.size());
    lexicon_buffer.clear();
    //write num of postings
    curPos=curPos+varbyte_encode_buffer(output_buffer,docIDs.size());
    for(int i=0;i<docIDs.size();i++){
        if(blocksize==0){
            if(docIDs.size()-i>64){
                curPos=curPos+varbyte_encode_buffer(output_buffer,docIDs[i+63]);
                
                for(int j=i+63;j>i;j--){
                    block_docID[j]=block_docID[j]-block_docID[j-1];
                }
            }else{
                curPos=curPos+varbyte_encode_buffer(output_buffer,docIDs[docIDs.size()-1]);
                for(int j=docIDs.size()-1;j>i;j--){
                    block_docID[j]=block_docID[j]-block_docID[j-1];
                }
            }
            
            //first in block
            varbyte_encode_buffer(block_docID,docIDs[i]);
        }else{
            varbyte_encode_buffer(block_docID,docIDs[i]);
        }
        gamma_encode(block_freq,freqs[i]);
        blocksize++;
        if(blocksize==64){
            gamma_fill(block_docID);
            gamma_fill(block_freq);
            curPos=curPos+varbyte_encode_buffer(output_buffer,block_docID.size()+block_freq.size());
            binary_write(inverted_index,output_buffer);
            output_buffer.clear();
            binary_write(inverted_index,block_docID);
            binary_write(inverted_index,block_freq);
            blocksize=0;
            block_docID.clear();
            block_freq.clear();
        }
    }
    docIDs.clear();
    freqs.clear();
    //last block
    gamma_fill(block_docID);
    gamma_fill(block_freq);
    curPos=curPos+varbyte_encode_buffer(output_buffer,block_docID.size()+block_freq.size());
    binary_write(inverted_index,output_buffer);
    output_buffer.clear();
    binary_write(inverted_index,block_docID);
    binary_write(inverted_index,block_freq);
    block_docID.clear();
    block_freq.clear();
}

int main(){
    chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
    build_index();
    // vector<bool> buffer;
    // unsigned char aggr=0;
    // unsigned char mask=1;
    
    // mask<<=1;
    // mask<<=1;
    // mask<<=1;
    // mask<<=1;
    // mask<<=1;
    // mask<<=1;
    // mask<<=1;
    // mask<<=1;
    // aggr|=mask;
    // int binary[8];
    // for(int n = 0; n < 8; n++)
    //     binary[7-n] = (mask >> n) & 1;
    // /* print result */
    // for(int n = 0; n < 8; n++)
    //     printf("%d", binary[n]);
    // printf("\n");
    // for(int n = 0; n < 8; n++)
    //     binary[7-n] = (aggr >> n) & 1;
    // /* print result */
    // for(int n = 0; n < 8; n++)
    //     printf("%d", binary[n]);
    // printf("\n");
    // ofstream ofile;
    // ofile.open("binary.txt",ios::out);
    // gamma_encode(buffer,30);
    // cout<<buffer.size()<<endl;
    // gamma_fill(buffer);
    // binary_write(ofile,buffer);
    // cout<<buffer.size()<<endl;
    // for(int i=0;i<buffer.size();i++){
    //     cout<<buffer[i];
    // }
    // buffer.clear();
    
    // gamma_encode(buffer,20);
    // cout<<buffer.size()<<endl;

    // gamma_fill(buffer);
    // binary_write(ofile,buffer);
    // cout<<buffer.size()<<endl;
    // ofile.close();
    // for(int i=0;i<buffer.size();i++){
    //     cout<<buffer[i];
    // }
    // cout<<endl;
    // buffer.clear();
    // ifstream ifile;
    // ifile.open("binary.txt",ios::in);
    // binary_read(ifile,buffer,4);
    // for(int i=0;i<buffer.size();i++){
    //     cout<<buffer[i];
    // }
    // cout<<endl;
    return 1;
}
