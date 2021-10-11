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

void display_elapsed_time(chrono::steady_clock::time_point start){
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    cout << "Elapsed time: "
        << chrono::duration_cast<chrono::milliseconds>(end - start).count()
        << " ms" << endl;
}

void load_terms_into_postings_buffer(vector<string>& buffer, vector<string>& terms, int docID){
    sort(terms.begin(),terms.end());
    string curTerm="";
    int count=0;
    for(string term:terms){
        if(curTerm.compare("")==0){
            count=1;
            curTerm=term;
        }
        else if(term.compare(curTerm)==0){
            count+=1;
        }
        else{
            buffer.push_back(curTerm+" "+to_string(docID)+" "+to_string(count)+" ");
            count=1;
            curTerm=term;
        }
        
    }
    buffer.push_back(curTerm+" "+to_string(docID)+" "+to_string(count)+" ");
    terms.clear();
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

void load_postings_into_output_buffer(string& buffer, vector<string>& postings){

    sort(postings.begin(),postings.end());
    string curTerm="";
    string leftover;
    string term;
    string merged_posting;
    for(int i=0;i<postings.size();i++){
        parse_posting(postings[i],term,leftover);
        if(curTerm.compare("")==0){
            curTerm=term;
            buffer.append(postings[i]);
        }
        else if(term.compare(curTerm)==0){
            buffer.append(leftover);
        }
        else{
            curTerm=term;
            buffer.append(postings[i]);
        }
    }
}


void parse_file(){
    const int BUFFER_SIZE = 1024*1024*20;
    vector<char> buffer (BUFFER_SIZE + 1, 0);
    vector<string> posting_buffer;
    string output_buffer;
    ofstream debugFile;
	
	ifstream ifile("fulldocs-new.trec", ios::binary);
    ifile.read(buffer.data(), BUFFER_SIZE);
    streamsize s = ((ifile) ? BUFFER_SIZE : ifile.gcount());
    buffer[s] = 0;
    string curword;
    string subword;
    vector<string> words;
    int docID=0;
    vector<string> urls;
    bool flag=false;
    bool get_url=true;
    regex rgx("[a-zA-Z]+");
    smatch match;
    int output_file_id=0;
    for(int i=0;i<BUFFER_SIZE;i++){
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
                urls.push_back(curword);
                get_url=false;
            }
            else if(curword.compare("</TEXT>")==0){
                //end of file, time to sort and write into output buffer
                load_terms_into_postings_buffer(posting_buffer,words,docID);
                docID++;
                if(posting_buffer.size()>=1024*400){
                    load_postings_into_output_buffer(output_buffer,posting_buffer);
                    debugFile.open("debug"+to_string(output_file_id)+".txt", ios::out);
                    debugFile.write(output_buffer.c_str(),output_buffer.size());
                    debugFile.close();
                    output_buffer.clear();
                    posting_buffer.clear();
                    cout<<output_file_id<<endl;
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
    cout<<words.size()<<endl;
    cout<<urls.size()<<endl;
    cout<<docID<<endl;
    
    // while(ifile && s == BUFFER_SIZE){
    //     for(int i=0;i<BUFFER_SIZE,i++){
    //         if(buffer[i]!=' ' || buffer[i]!='\n'){
    //             curword.push_back(buffer[i]);
    //         }
    //         else{
    //             if(!flag){
    //                 curword.clear();
    //             }
    //             else if(curword=="<TEXT>"){
    //                 flag=true;
    //                 get_url=true;
    //                 i++;
    //             }
    //             else if(get_url){
    //                 urls.push_back(curword);
    //                 flag=false;
    //             }
    //             else if(curword=="</TEXT>"){
    //                 flag=false;
    //             }
    //             else{
    //                 while(regex_search(s, match, rgx)){
    //                     cout<<match.str()<<endl;
    //                 }
    //             }
    //             curword.clear();

    //         }
    //     }
    // }
    int buffer_size;
    ifile.close();
}
int main() {
    chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
    parse_file();
    display_elapsed_time(start_time);

}