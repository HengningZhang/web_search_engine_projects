#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <unordered_map>
// #include "mysql/jdbc.h"
using namespace std;

const string SERVER = "wse-document-do-user-10132967-0.b.db.ondigitalocean.com";
const string USERNAME = "doadmin";
const string PASSWORD = "UsuY0FZyThSgOpJb";
const string DB="defaultdb";
const string LEXICON_FILE="lexicon.txt";
chrono::steady_clock::time_point start_time = chrono::steady_clock::now();
const string INDEX_FILE="inverted_index.txt";

//display elapsed time
void display_elapsed_time(chrono::steady_clock::time_point start){
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    cout << "Elapsed time: "
        << chrono::duration_cast<chrono::milliseconds>(end - start).count()
        << " ms" << endl;
}

//read the next byte from buffer
int read_byte(unsigned char buffer[], int& array_pos){
    int result=(int)buffer[array_pos];
    array_pos++;
    return result;
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

unordered_map<string, long long> load_lexicon(string lexicon_file){
    ifstream file;
    file.open(lexicon_file, ios::in);
    string term;
    long long pos;
    unordered_map<string, long long> lexicon;
    while(file>>term>>pos){
        lexicon.insert(make_pair(term,pos));
    }
    return lexicon;
}

struct ListPointer{
    ifstream index;
    long long pos;
    int block_size=0;
    int docID_count;
    int last_in_block;
    int doc_id=-1;
    //index to specify where we are in the current block's decode buffer
    int array_pos;
    vector<int> block;
    int current_block=-1;
    int block_count;
    unsigned char buffer[320];
    int next_pos;

    ListPointer(long long lexiconPos){
        cout<<"initializing list pointer"<<endl;
        pos=lexiconPos;
        cout<<"pos:"<<pos<<endl;
        index.open(INDEX_FILE,ios::binary);
        index.seekg(pos);
        index.read((char*)buffer,8);
        docID_count=varbyte_decode(buffer,array_pos);
        block_count=docID_count/64;
        //if total doc count is exactly multiply of 64, there is no leftover small block
        if(block_count%64==0){
            block_count=block_count-1;
        }
        cout<<"docID count: "<<docID_count<<" block count: "<<block_count<<endl;
        next_pos=pos+array_pos;
        // read 16 bytes because we need at most 4 numbers at the start of each posting
        // # of files that this term, last docID in this block, size of docID block, size of frequency block
        getBlockInfo();
        decompressBlock();
    }

    void getBlockInfo(){
        //getting the info of next array means buffer pos and content for the previous block are no longer needed
        block.clear();
        block_size=0;
        pos=next_pos;
        index.seekg(pos);
        array_pos=0;
        current_block++;
        index.read((char*)buffer,16);
        last_in_block=varbyte_decode(buffer,array_pos);
        // determine how much to decompress
        block_size=block_size+varbyte_decode(buffer,array_pos);
        block_size=block_size+varbyte_decode(buffer,array_pos);
        // take note of the start of next block
        next_pos=pos+array_pos+block_size;
    }

    void decompressBlock(){
        cout<<"decompressing"<<endl;
        index.seekg(pos+array_pos);
        index.read((char*)buffer,block_size);
        int i=0;
        array_pos=0;
        while(array_pos<block_size){
            block.push_back(varbyte_decode(buffer,array_pos));
        }
    }
};

pair<int,int> nextGEQ(ListPointer* lp, int k){
    cout<<"querying k= "<<k<<endl;
    if(lp->last_in_block<k){
        cout<<"last in block = "<<lp->last_in_block<<endl;
        while(lp->current_block<=lp->block_count){
            cout<<"in while"<<endl;
            lp->getBlockInfo();
            cout<<"new block last in block: "<<lp->last_in_block<<endl;
            if(lp->last_in_block>=k){
                lp->decompressBlock();
                break;
            }
        }
        
    }
    cout<<"lp->current_block: "<<lp->current_block<<" lp->block_count:"<<lp->block_count<<endl;
    if(lp->current_block>lp->block_count){
        return make_pair(-1,0);
    }
    for(int i=0;i<lp->block.size()/2;i++){
        cout<<lp->block[i]<<endl;
        if(lp->block[i]>=k){
            return make_pair(lp->block[i],lp->block[i+lp->block.size()/2]);
        }
    }
    return make_pair(-1,0);
}

class FileSys{
    public:
        FileSys(string lexicon_file){
            lexicon=load_lexicon(lexicon_file);
        }
        ListPointer* open_term(string term){
            lp=new ListPointer(lexicon[term]);
            return lp;
        }
    private:
        unordered_map<string, long long> lexicon;
        ListPointer* lp;
};

int main(){
    string input;
    
    string term;
    // sql::mysql::MySQL_Driver *driver;
    // sql::Connection *con;
    // sql::Statement *stmt;
    // sql::ResultSet  *res;
    // driver = sql::mysql::get_mysql_driver_instance();
    // con = driver->connect(SERVER, USERNAME, PASSWORD);
    // stmt = con->createStatement();
    // stmt->execute("USE defaultdb");
    // stmt->execute("DROP TABLE IF EXISTS test");
    // stmt->execute("CREATE TABLE test(id INT, label CHAR(1))");
    // stmt->execute("INSERT INTO test(id, label) VALUES (1, 'a')");
    // res = stmt->executeQuery("SELECT *");

    // while(res->next()){
    //     cout << "id = " << res->getInt(1);
    //     cout << ", label = '" << res->getString("label") << "'" << endl;
    // }


    // unordered_map<string,long long> lexicon=load_lexicon(LEXICON_FILE);
    FileSys filesys("lexicon.txt");
    cout<<"filesys initialized"<<endl;
    display_elapsed_time(start_time);
    ListPointer* example;
    example=filesys.open_term("accepted");
    cout<<"opened term"<<endl;
    display_elapsed_time(start_time);
    pair<int,int> result;
    result=nextGEQ(example,1);
    cout<<"result of nextGEQ(1) is "<<result.first<<" "<<result.second<<endl;
    display_elapsed_time(start_time);
    result=nextGEQ(example,1693);
    cout<<"result of nextGEQ(1693) is "<<result.first<<" "<<result.second<<endl;
    display_elapsed_time(start_time);

    vector<vector<string>> query_terms;
    vector<string> term_group;
    cout<<"loaded lexicon"<<endl;
    display_elapsed_time(start_time);
    // cout<<lexicon["accepted"]<<endl;
    // query processer 
    // while(true){
    //     cout<<"input query:"<<endl;
    //     getline(cin,input);
    //     istringstream query(input);
    //     while(query>>term){
    //         if(term=="quit"){
    //             return 0;
    //         }
    //         else if(term=="OR"){
    //             query_terms.push_back(term_group);
    //             term_group.clear();
    //         }
    //         else if(term!="AND"){
    //             term_group.push_back(term);
    //         }
    //     }
    //     cout<<"read!"<<endl;
    //     query_terms.push_back(term_group);
    //     term_group.clear();
    //     for(int i=0;i<query_terms.size();i++){
    //         cout<<i;
    //         for(int j=0;j<query_terms[i].size();j++){
    //             cout<<" "<<query_terms[i][j];
    //         }
    //         cout<<endl;
    //     }
    //     query_terms.clear();
    // }
};