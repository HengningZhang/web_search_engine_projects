#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "zlib.h"
 #include <chrono>

using namespace std;

//display elapsed time
void display_elapsed_time(chrono::steady_clock::time_point start){
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    cout << "Elapsed time: "
        << chrono::duration_cast<chrono::milliseconds>(end - start).count()
        << " ms" << endl;
}

string to_lower(string s){
    string result;
    for(int i=0;i<s.size();i++){
        result.push_back(tolower(s[i]));
    }
    return result;
}

//load vector of terms into output buffer
int load_terms_into_output_buffer(string& buffer, vector<string>& terms, int docID){
    int distinct_words=0;
    //sort it so that it is easier to count the frequency of each word in one file
    sort(terms.begin(),terms.end());
    string curTerm="";
    int count=0;
    for(string term:terms){
        if(curTerm.compare("")==0){
            count=1;
            curTerm=term;
            distinct_words=1;
        }
        //count the current term
        else if(term==curTerm){
            count+=1;
        }
        //if get a different one, write the previous one and its count into buffer
        else{
            distinct_words++;
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
    return distinct_words;
}

int main() {
    chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
    
    ofstream ofile;
    ofstream docInfo;
    ofile.open("intermediate_postings.txt",ios::out);
    
    docInfo.open("docInfo.txt",ios::out);
    int docID=0;
    
    
    display_elapsed_time(start_time);

    bool flag=true;
    string curword;
    string url;
    vector<string> words;
    string output_buffer;
    char curchar;
    int i;
    string folderidStr;
    string fileidStr;
    string fileStr;
    int word_count;
    int distinct_word_count;
    for(int folderid=0;folderid<273;folderid++){
        if(folderid==0){
            folderidStr="000";
        }else if(folderid<10){
            folderidStr="00"+to_string(folderid);
        }else if(folderid<100){
            folderidStr="0"+to_string(folderid);
        }else{
            folderidStr=to_string(folderid);
        }
        for(int fileid=0;fileid<100;fileid++){
            //decompress a file
            if(fileid==0){
                fileidStr="00";
            }else if(fileid<10){
                fileidStr="0"+to_string(fileid);
            }else{
                fileidStr=to_string(fileid);
            }
            fileStr="GOV2/GX"+folderidStr+"/"+fileidStr+".gz";
            gzFile inFileZ = gzopen(fileStr.c_str(), "rb");
            if (inFileZ == NULL) {
                cout<<"Error: Failed to gzopen "+fileStr<<endl;
                break;
            }
            
            unsigned char unzipBuffer[8192];
            unsigned int unzippedBytes;
            std::vector<char> unzippedData;
            while (true) {
                unzippedBytes = gzread(inFileZ, unzipBuffer, 8192);
                if (unzippedBytes > 0) {
                    unzippedData.insert(unzippedData.end(), unzipBuffer, unzipBuffer + unzippedBytes);
                }
                else {
                    break;
                }
            }
            gzclose(inFileZ);
            cout<<"succesfully decompressed"+fileStr<<endl;
            cout<<unzippedData.size()<<" "<<docID<<endl;

            curword.clear();
            url.clear();
            words.clear();
            output_buffer.clear();
            i=0;
            while(i<unzippedData.size()){
                curword.clear();
                if(!flag){
                    while(unzippedData[i]!='<' && i<unzippedData.size()){
                        i++;
                    }
                }
                else{
                    while(unzippedData[i]!='<'&&!isalpha(unzippedData[i])&&!isdigit(unzippedData[i]) && i<unzippedData.size()){
                        i++;
                    }
                }
                
                
                //get a full tag if the next item is a tag
                if(unzippedData[i]=='<'){
                    curword.push_back(unzippedData[i]);
                    i++;
                    while(unzippedData[i]!='>' && i<unzippedData.size()){
                        curword.push_back(unzippedData[i]);
                        i++;
                    }
                    curword.push_back(unzippedData[i]);
                    i++;
                }
                //extract url
                if(curword=="<DOCHDR>"){
                    i++;
                    while(unzippedData[i]!='\n' && i<unzippedData.size()){
                        url.push_back(unzippedData[i]);
                        i++;
                    }
                    i++;
                }else if(curword=="<HTML>" | curword=="<html>"){
                    flag=true;
                    continue;
                }else if(curword=="</HTML>" | curword=="</html>"){
                    flag=false;
                }else if(curword=="</DOC>"){
                    word_count=words.size();
                    distinct_word_count=load_terms_into_output_buffer(output_buffer,words,docID);
                    docInfo<<docID<<" "<<url<<" "<<word_count<<" "<<distinct_word_count<<endl;
                    url.clear();
                    ofile<<output_buffer;
                    output_buffer.clear();
                    docID++;
                }else if(curword.size()>=7){
                    string checker=curword.substr(1,6);
                    if(checker=="script" | checker=="SCRIPT"){
                        //skip to end of script's closing tag
                        while(unzippedData[i]!='<' && i<unzippedData.size()){
                            i++;
                        }
                        while(unzippedData[i]!='>' && i<unzippedData.size()){
                            i++;
                        }
                        i++;
                    } 
                }
                if(flag){
                    curword.clear();
                    curchar=unzippedData[i];
                    while(isalpha(curchar)|isdigit(curchar) && i<unzippedData.size()){
                        curword.push_back(curchar);
                        i++;
                        curchar=unzippedData[i];
                    }
                    if(curchar!='<'){
                        i++;
                    }
                    if(!curword.empty()){
                        words.push_back(to_lower(curword));
                    }
                    curword.clear();
                }
            }
            cout<<folderid<<"|"<<fileid<<endl;
            display_elapsed_time(start_time);
        }
    }
    
    return 0;
}