#include <iostream>
#include <fstream>
#include <sstream>
#include <climits>
#define int long long
using namespace std;


int L1Reads = 0;
int L1ReadMisses = 0;
int L1Writes = 0;
int L1WriteMisses = 0;
int writeBacksL1 = 0;
int L2Reads = 0;
int L2ReadMisses = 0;
int L2Writes = 0;
int L2WriteMisses = 0;
int writeBacksL2 = 0;

int counter = 0;


struct cache{
    int blkszL1;
    int szL1;
    int assocL1;
    int cacheLinesL1;

    int** validL1;
    int** tagArrayL1;
    int** dirtyL1;
    int** lastUsedL1;

    int blkszL2;
    int szL2;
    int assocL2;
    int cacheLinesL2;

    int** validL2;
    int** tagArrayL2;
    int** dirtyL2;
    int** lastUsedL2;

    cache(int blksz1, int sz1, int assoc1, int blksz2, int sz2, int assoc2){
        blkszL1 = blksz1;
        szL1 = sz1;
        assocL1 = assoc1;
        
        cacheLinesL1 = (sz1/assoc1)/blksz1;

        validL1 = new int*[cacheLinesL1];
        tagArrayL1 = new int*[cacheLinesL1];
        lastUsedL1 = new int*[cacheLinesL1];
        dirtyL1 = new int*[cacheLinesL1];

        for (int i = 0; i < cacheLinesL1; i++) {
            validL1[i] = new int[assoc1];
            tagArrayL1[i] = new int[assoc1];
            lastUsedL1[i] = new int[assoc1];
            dirtyL1[i] = new int[assoc1];
        }

        for (int i = 0; i < cacheLinesL1; i++) {
            for (int j=0; j<assocL1; j++){
                validL1[i][j] = -1;
                tagArrayL1[i][j] = -j-1;
                lastUsedL1[i][j] = -1;
                dirtyL1[i][j] = 0;
            }
        }

        blkszL2 = blksz2;
        szL2 = sz2;
        assocL2 = assoc2;
        
        cacheLinesL2 = (sz2/assoc2)/blksz2;

        validL2 = new int*[cacheLinesL2];
        tagArrayL2 = new int*[cacheLinesL2];
        lastUsedL2 = new int*[cacheLinesL2];
        dirtyL2 = new int*[cacheLinesL2];

        for (int i = 0; i < cacheLinesL2; i++) {
            validL2[i] = new int[assoc2];
            tagArrayL2[i] = new int[assoc2];
            lastUsedL2[i] = new int[assoc2];
            dirtyL2[i] = new int[assoc2];
        }

        for (int i = 0; i < cacheLinesL2; i++) {
            for (int j=0; j<assocL2; j++){
                validL2[i][j] = -1;
                tagArrayL2[i][j] = -j-1;
                lastUsedL2[i][j] = -1;
                dirtyL2[i][j] = 0;
            }
        }
    }







    int getEmptyL1(int index){
        int curr = INT_MAX; 
        for (int i = 0; i < assocL1; i++){
            if (lastUsedL1[index][i] == -1) return i;
        }
        return -1;
    }


    int getEmptyL2(int index){
        int curr = INT_MAX; 
        for (int i = 0; i < assocL2; i++){
            if (lastUsedL2[index][i] == -1) return i;
        }
        return -1;
    }




    int getLRUL1(int index){
        int curr = INT_MAX; 
        int innn = -1;
        for (int i = 0; i < assocL1; i++){
            if (lastUsedL1[index][i] == -1) return i;
            if (lastUsedL1[index][i] < curr) {
                innn = i; 
                curr = lastUsedL1[index][i];
            }
        }
        return innn;
    }
    
    int getLRUL2(int index){
        int curr = INT_MAX; 
        int innn = -1;
        for (int i = 0; i < assocL2; i++){
            if (lastUsedL2[index][i] < curr) {
                innn = i; 
                curr = lastUsedL2[index][i];
            }
        }
        return innn;
    }


    int getBlockL1(int index, int tag){
        for (int i = 0; i < assocL1; i++){
            if (tagArrayL1[index][i] == tag){
                lastUsedL1[index][i] = counter;
                return i;
            }
        }
        return  -1;
    }


    int getBlockL2(int index, int tag){
        for (int i = 0; i < assocL2; i++){
            if (tagArrayL2[index][i] == tag){
                lastUsedL2[index][i] = counter;
                return i;
            }
        }
        return  -1;
    }


    void evictFromL2(int indexL1, int tagL1, int indexL2, int tagL2){
        // cout<<"Evicting data to make space in L2"<<endl;
        int blockNo = getBlockL2(indexL2, tagL2);
        if (blockNo == -1) return;
        if (dirtyL2[indexL2][blockNo] == 0){
            return;
        }
        else {
            // cout<<"writeback to memory is required for this"<<endl;
            writeBacksL2++;
        }
    }


    void getIntoL2(int indexL1, int tagL1, int indexL2, int tagL2){
        // L2Reads++;
        // cout<<"L2 read here"<<endl;
        if (getBlockL2(indexL2,tagL2) != -1) return;
        // L2ReadMisses++;
        // cout<<"L2 read miss at "<<counter<<endl;
        int blockNo = getLRUL2(indexL2);
        int add_new = indexL2 + cacheLinesL2*tagArrayL2[indexL2][blockNo];
        evictFromL2(add_new % cacheLinesL1, add_new / cacheLinesL1, indexL2, tagArrayL2[indexL2][blockNo]);
        tagArrayL2[indexL2][blockNo] = tagL2;
    }


    void evictFromL1(int indexL1, int tagL1, int indexL2, int tagL2){
        // cout<<"evicting data to make space in L1"<<endl;
        int blockNo = getBlockL1(indexL1, tagL1);
        if (blockNo == -1) return;
        if (dirtyL1[indexL1][blockNo] == 0) return;
        else{
            if (getBlockL2(indexL2,tagL2) != -1) L2WriteMisses++;

            getIntoL2(indexL1, tagL1, indexL2, tagL2);
            
            int blockNoL2 = getBlockL2(indexL2, tagL2);
            dirtyL2[indexL2][blockNoL2] = 1;
            dirtyL1[indexL1][blockNo] = 0;
            writeBacksL1++;
            L2Writes++;
            // cout<<"writing back the data into L2"<<endl;
        }
    }


    void getIntoL1(int indexL1, int tagL1, int indexL2, int tagL2){
        // L1Reads++;
        if (getBlockL1(indexL1, tagL1) != -1) {
            return;
        }

        // L1ReadMisses++;
        // cout<<"L1 read miss at "<<counter<<endl;

        getIntoL2(indexL1, tagL1, indexL2, tagL2);
        

        int blockNo = getLRUL1(indexL1);

        evictFromL1(indexL1, tagArrayL1[indexL1][blockNo], indexL2, tagL2);
        // cout<<tagArrayL1[0][0]<<" "<<tagArrayL2[0][0]<<" caution!!"<<endl;
        // cout<<lastUsedL1[0][0]<<" "<<lastUsedL2[0][0]<<" caution!!"<<endl;
        tagArrayL1[indexL1][blockNo] = tagL1;
        lastUsedL1[indexL1][blockNo] = counter;
    }











    void evictFromL2Write(int indexL1, int tagL1, int indexL2, int tagL2){
        // cout<<"Evicting data to make space in L2 (write)"<<endl;
        int blockNo = getBlockL2(indexL2, tagL2);
        if (blockNo == -1) return;
        if (dirtyL2[indexL2][blockNo] == 0){
            return;
        }
        else {
            writeBacksL2++;
        }
    }


    void getIntoL2Write(int indexL1, int tagL1, int indexL2, int tagL2){
        // cout<<"fetching data into L2 "<<indexL1<<" "<<tagL2<<" "<<endl;
        // L2Reads++;
        // cout<<"L2 read here l2write"<<endl;
        if (getBlockL2(indexL2,tagL2) != -1) return;
        // cout<<"not already in L2(write)"<<endl;
        // L2ReadMisses;
        int blockNo = getLRUL2(indexL2);
        // cout<<"abcdef "<<blockNo<<endl;
        evictFromL2Write(indexL1, tagL1, indexL2,tagArrayL2[indexL2][blockNo]);
        tagArrayL2[indexL2][blockNo] = tagL2;
        dirtyL2[indexL2][blockNo] = 0;
        lastUsedL2[indexL2][blockNo] = counter;
    }


    void evictFromL1Write(int indexL1, int tagL1, int indexL2, int tagL2){
        // cout<<"Evicting data to make space in L1 (write) "<<indexL1<<" "<<tagL1<<endl;
        int blockNo = getBlockL1(indexL1, tagL1);
        // cout<<blockNo<<endl;
        if (blockNo == -1) return;
        if (dirtyL1[indexL1][blockNo] == 0) return;
        else{
            if (getBlockL2(indexL2,tagL2) != -1) L2WriteMisses++;
            
            // cout<<"here"<<endl;
            getIntoL2Write(indexL1, tagL1, indexL2, tagL2);
            int blockNoL2 = getBlockL2(indexL2, tagL2);
            dirtyL2[indexL2][blockNoL2] = 1;
            dirtyL1[indexL1][blockNo] = 0;
            // cout<<"qwertyuiop"<<endl;
            writeBacksL1++;
            L2Writes++;
        }
    }


    void getIntoL1Write(int indexL1, int tagL1, int indexL2, int tagL2){
        // cout<<"writing into L1"<<endl;
        // cout<<"indexL1 : "<<indexL1<<"   "<<"tagL2 : "<<tagL2<<"    "<<"indexL2 : "<<indexL2<<"   "<<"tagL2 : "<<tagL2<<endl;
        // L1Writes++;
        if (getBlockL1(indexL1, tagL1) != -1) {
            // cout<<"it is an L1 write hit"<<endl;
            dirtyL1[indexL1][getBlockL1(indexL1,tagL1)] = 1;
            return;
        }

        // L1WriteMisses++;

        int blockNo = getLRUL1(indexL1);
        int addr = blkszL1*tagArrayL1[indexL1][blockNo] + indexL1;
        evictFromL1Write(indexL1, tagArrayL1[indexL1][blockNo], addr % blkszL2, addr / blkszL2);

        getIntoL2Write(indexL1, tagL1, indexL2, tagL2);


        // cout<<tagArrayL1[0][0]<<" "<<tagArrayL2[0][0]<<" caution!!"<<endl;
        // cout<<lastUsedL1[0][0]<<" "<<lastUsedL2[0][0]<<" caution!!"<<endl;
        tagArrayL1[indexL1][blockNo] = tagL1;
        lastUsedL1[indexL1][blockNo] = counter;
        dirtyL1[indexL1][blockNo] = 1;
    }




    void commandL1(char rw, int address){
        int indexL1 = (address/(blkszL1)) % cacheLinesL1; 
        int tagL1 = (address/blkszL1)/cacheLinesL1; 

        if (rw == 'r') L1Reads++;
        else L1Writes++;

        if (getBlockL1(indexL1, tagL1) != -1){
            int blockNo = getBlockL1(indexL1, tagL1);
            if (rw == 'w') dirtyL1[indexL1][blockNo] = 1;
        }
        
        else{

            if (rw == 'r') L1ReadMisses++;
            else L1WriteMisses++;

            if (getEmptyL1(indexL1) != -1){
                int blockNo = getEmptyL1(indexL1);
                if (rw == 'w') dirtyL1[indexL1][blockNo] = 1;
                else dirtyL1[indexL1][blockNo] = 0;

                tagArrayL1[indexL1][blockNo] = tagL1;

                lastUsedL1[indexL1][blockNo] = counter;

                commandL2('r',address);
            }
            else{
                int blockNo = getLRUL1(indexL1);

                if (dirtyL1[indexL1][blockNo] == 1){
                    // cout<<"faajfijneodfija"<<(tagL1*cacheLinesL1 + indexL1) * blkszL1<<endl;
                    commandL2('w', ((tagArrayL1[indexL1][blockNo])*cacheLinesL1 + indexL1) * blkszL1);
                    // cout<<tagL1<<" "<<cacheLinesL1<<" "<<indexL1<<" "<<blkszL1<<endl;
                    writeBacksL1++;
                }

                if (rw == 'w') dirtyL1[indexL1][blockNo] = 1;
                else dirtyL1[indexL1][blockNo] = 0;

                tagArrayL1[indexL1][blockNo] = tagL1;

                lastUsedL1[indexL1][blockNo] = counter;


                commandL2('r',address);

            }

        }

    }


    void commandL2(char rw, int address){

        // cout<<"inside commandL2"<<" "<<rw<<" "<<address<<endl;
        int indexL2 = (address/(blkszL2)) % cacheLinesL2; 
        int tagL2 = (address/blkszL2)/cacheLinesL2; 

        
        if (rw == 'r') L2Reads++;
        else L2Writes++;

        if (getBlockL2(indexL2, tagL2) != -1){
            int blockNo = getBlockL2(indexL2, tagL2);
            if (rw == 'w') dirtyL2[indexL2][blockNo] = 1;
        // cout<<"here"<<endl;
        }
        
        else{

            if (rw == 'r') L2ReadMisses++;
            else L2WriteMisses++;

            if (getEmptyL2(indexL2) != -1){
                int blockNo = getEmptyL2(indexL2);
                if (rw == 'w') dirtyL2[indexL2][blockNo] = 1;
                else dirtyL2[indexL2][blockNo] = 0;

                tagArrayL2[indexL2][blockNo] = tagL2;

                lastUsedL2[indexL2][blockNo] = counter;

            }
            else{
                int blockNo = getLRUL2(indexL2);

                if (dirtyL2[indexL2][blockNo] == 1){
                    // commandL1('w', (tagL2*cacheLinesL2 + indexL2) * blkszL2);
                    writeBacksL2++;
                }

                if (rw == 'w') dirtyL2[indexL2][blockNo] = 1;
                else dirtyL2[indexL2][blockNo] = 0;

                tagArrayL2[indexL2][blockNo] = tagL2;

                lastUsedL2[indexL2][blockNo] = counter;

                // commandL1('r',address);



            }

        }

    }



};



int to_int(string s){
    int l = s.size();
    int ans = 0 ;
    for (int i =0 ;  i< l ; i++ ) {
        if ((s[i] - 'a') <= 5 && (s[i] - 'a') >= 0 || (s[i] - '0') <= 9 && (s[i] - '0') >= 0){
            ans = ((ans*16)) ;
            if (s[i]== 'a') ans+=10 ;
            else if (s[i]== 'b') ans+=11 ;
            else if (s[i]== 'c') ans+=12 ;
            else if (s[i]== 'd') ans+=13 ;
            else if (s[i]== 'e') ans+=14 ;
            else if (s[i]== 'f') ans+=15 ;
            else ans+=(s[i]-'0') ;
        }
    }
    return ans ;
}


int32_t main(int argc, char*argv[]){
    // cache cacheL1(1,2,3);
    cache L(atoi(argv[1]),atoi(argv[2]),atoi(argv[3]),atoi(argv[1]),atoi(argv[4]),atoi(argv[5]));
    // cache L(64,1024,2,64,65536,8);

    string myText;
    ifstream MyReadFile(argv[6]);

    int address;

    while (getline(MyReadFile, myText)){
        // cout<<counter<<endl;
        // cout<<myText<<endl;
        counter++;

        address = to_int(myText.substr(1,100));

        L.commandL1(myText[0],address);


    }




    cout<<"The number of L1 reads are "<<L1Reads<<"\n";
    cout<<"The number of L1 read misses are "<<L1ReadMisses<<"\n";
    cout<<"The number of L1 writes are "<<L1Writes<<"\n";
    cout<<"The number of L1 write misses are "<<L1WriteMisses<<"\n";
    cout<<"The L1 miss rate is "<<(double(L1ReadMisses) + double(L1WriteMisses))/(double(L1Writes) + double(L1Reads))<<endl;
    cout<<"The number of L1 write backs are "<<writeBacksL1<<"\n";
    cout<<endl;


    cout<<"The number of L2 reads are "<<L2Reads<<"\n";
    cout<<"The number of L2 read misses are "<<L2ReadMisses<<"\n";
    cout<<"The number of L2 writes are "<<L2Writes<<"\n";
    cout<<"The number of L2 write misses are "<<L2WriteMisses<<"\n";
        cout<<"The L2 miss rate is "<<(double(L2ReadMisses) + double(L2WriteMisses))/(double(L2Writes) + double(L2Reads))<<endl;
    cout<<"The number of L2 write backs are "<<writeBacksL2<<"\n";
    cout<<endl<<endl;
    cout<<"Time to execute the code is "<<1*(L1Reads+L1Writes)+20*(L2Reads+L2Writes)+200*(L2ReadMisses+L2WriteMisses+writeBacksL2)<<" ns"<<endl;


}
