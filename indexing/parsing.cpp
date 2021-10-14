#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>

using namespace std;

//display elapsed time
void display_elapsed_time(chrono::steady_clock::time_point start){
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    cout << "Elapsed time: "
        << chrono::duration_cast<chrono::milliseconds>(end - start).count()
        << " ms" << endl;
}

//load vector of terms into output buffer
void load_terms_into_output_buffer(string& buffer, vector<string>& terms, int docID){
    //sort it so that it is easier to count the frequency of each word in one file
    sort(terms.begin(),terms.end());
    string curTerm="";
    int count=0;
    for(string term:terms){
        if(curTerm.compare("")==0){
            count=1;
            curTerm=term;
        }
        //count the current term
        else if(term.compare(curTerm)==0){
            count+=1;
        }
        //if get a different one, write the previous one and its count into buffer
        else{
            buffer.append(curTerm+" "+to_string(docID)+" "+to_string(count)+"\n");
            count=1;
            curTerm=term;
        }
        
    }
    //write the last one
    //don't write anything into buffer if word is empty string
    if(curTerm.compare("")!=0){
        buffer.append(curTerm+" "+to_string(docID)+" "+to_string(count)+"\n");
    }
    terms.clear();
}


void parse_file(){
    chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
    //input buffer size roughly 2GB
    const int BUFFER_SIZE = 1024*1024*2000;
    vector<char> buffer (BUFFER_SIZE + 1, 0);
    vector<string> posting_buffer;
    //posting output buffer
    string output_buffer;
    //document map output buffer
    string doc_buffer;
    ofstream intermediatePostingFile;
    ofstream docMap;
	ifstream ifile("fulldocs-new.trec", ios::binary);
    //I know it should probably be named term
    //but naming it this helps me think:)
    string curword;
    //used to decompose word separated by symbols or non ASCII stuff
    string subword;
    //store the words in one doc
    vector<string> words;
    //take note of current document id
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
            //my own parsing
            //if not blankspace or changing line
            //read char into curword
            if(buffer[i]!=' ' && buffer[i]!='\n'){
                curword.push_back(buffer[i]);
                continue;
            }
            if(!curword.empty()){
                //at a blankspace or changing line
                if(curword.compare("<TEXT>")==0){
                    //if starting of the document
                    //now every term counts
                    flag=true;
                    //ready to get url
                    get_url=true;
                }
                else if(!flag){
                    //not in document, don't count anything
                    curword.clear();
                }
                else if(get_url){
                    //at the start of document, read url in
                    doc_buffer.append(to_string(docID)+" "+curword+"\n");
                    if(doc_buffer.size()>1024*1024*1024){
                        docMap.write(doc_buffer.c_str(),doc_buffer.size());
                        doc_buffer.clear();
                    }
                    //already read the url, not reading it anymore in this document
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
                    //decompose the curword if there is weird characters separating it to multiple subwords
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