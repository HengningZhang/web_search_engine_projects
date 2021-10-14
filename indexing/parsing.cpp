#include <iostream>
#include <fstream>
#include <bitset>
#include <string>
#include <vector>
#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <sstream>
#include <chrono>

// sort -S 50% --parallel=4 -k1,1 -s intermediate_postings.txt > sorted_postings.txt
using namespace std;

void display_elapsed_time(chrono::steady_clock::time_point start){
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    cout << "Elapsed time: "
        << chrono::duration_cast<chrono::milliseconds>(end - start).count()
        << " ms" << endl;
}

void parse_posting(string& posting, string& term, string& leftover){
    term.clear();
    leftover.clear();
    bool flag=false;
    for(char c:posting){
        if(!flag && c==' '){
            flag=true;
            continue;
        }
        if(!flag){
            term.push_back(c);
        }
        if(flag){
            leftover.push_back(c);
        }
    }
}

void load_terms_into_output_buffer(string& buffer, vector<string>& terms, int docID){
    sort(terms.begin(),terms.end());
    string curTerm="";
    int count=0;
    for(string term:terms){
        if(curTerm.compare("")==0){
            count=1;
            curTerm=term;
            if(term.compare("")==0){
                cout<<"caught the rat!"<<endl;
            }
        }
        else if(term.compare(curTerm)==0){
            count+=1;
        }
        else{
            if(count==0){
                cout<<"encountered a zero!"<<endl;
            }
            buffer.append(curTerm+" "+to_string(docID)+" "+to_string(count)+"\n");
            count=1;
            curTerm=term;
        }
        
    }
    if(curTerm.compare("")!=0){
        buffer.append(curTerm+" "+to_string(docID)+" "+to_string(count)+"\n");
    }
    
    terms.clear();
}


void parse_file(){
    chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
    const int BUFFER_SIZE = 1024*1024*2000;
    vector<char> buffer (BUFFER_SIZE + 1, 0);
    vector<string> posting_buffer;
    string output_buffer;
    string doc_buffer;
    ofstream intermediatePostingFile;
    ofstream docMap;
	ifstream ifile("fulldocs-new.trec", ios::binary);
    string curword;
    string subword;
    vector<string> words;
    int docID=0;
    vector<string> urls;
    bool flag=false;
    bool get_url=true;
    int output_file_id=0;
    intermediatePostingFile.open("intermediate_postings.txt", ios::out);
    docMap.open("docMap.txt",ios::out);
    while(1){
        ifile.read(buffer.data(), BUFFER_SIZE);
        streamsize s = ((ifile) ? BUFFER_SIZE : ifile.gcount());
        buffer[s] = 0;
        for(int i=0;i<s;i++){
            if(buffer[i]!=' ' && buffer[i]!='\n'){
                curword.push_back(buffer[i]);
                continue;
                
            }
            if(!curword.empty()){
                if(curword.compare("<TEXT>")==0){
                    flag=true;
                    get_url=true;
                }
                else if(!flag){
                    curword.clear();
                }
                else if(get_url){
                    doc_buffer.append(to_string(docID)+" "+curword+"\n");
                    if(doc_buffer.size()>1024*1024*1024){
                        docMap.write(doc_buffer.c_str(),doc_buffer.size());
                        doc_buffer.clear();
                    }
                    get_url=false;
                }
                else if(curword.compare("</TEXT>")==0){
                    //end of file, time to sort and write into output buffer
                    load_terms_into_output_buffer(output_buffer,words,docID);
                    docID++;
                    if(output_buffer.size()>=1024*1024*1024){
                        intermediatePostingFile.write(output_buffer.c_str(),output_buffer.size());
                        output_buffer.clear();
                        output_file_id++;
                    }
                    flag=false;
                }
                else{
                    for(char c:curword){
                        if(!isalpha(c)&&!isdigit(c)){
                            if(!subword.empty()){
                                words.push_back(subword);
                            }
                            subword.clear();
                            break;
                        }
                        else{
                            subword.push_back(c);
                        }
                    }
                    if(!subword.empty()){
                        words.push_back(subword);
                    }
                    subword.clear();
                }
                curword.clear();
            }
        }
        cout<<docID<<endl;
        display_elapsed_time(start_time);
        if(!ifile){
            break;
        }
    }
    docMap.write(doc_buffer.c_str(),doc_buffer.size());
    doc_buffer.clear();
    intermediatePostingFile.write(output_buffer.c_str(),output_buffer.size());
    output_buffer.clear();
    ifile.close();
    intermediatePostingFile.close();
    docMap.close();
}
int main() {
    
    parse_file();
}