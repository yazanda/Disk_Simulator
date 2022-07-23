#include <iostream>
#include <utility>
#include <vector>
#include <map>
#include <cassert>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <fcntl.h>

using namespace std;

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"
#define DISK_SIZE 18
// ============================================================================
//void decToBinary(int n, char &c)
//{
//    // array to store binary number
//    int binaryNum[8];
//
//    // counter for binary array
//    int i = 0;
//    while (n > 0)
//    {
//        // storing remainder in binary array
//        binaryNum[i] = n % 2;
//        n = n / 2;
//        i++;
//    }
//
//    // printing binary array in reverse order
//    for (int j = i - 1; j >= 0; j--)
//    {
//        if (binaryNum[j] == 1)
//            c = c | 1u << j;
//    }
//}
/***File System Class***/
class FsFile {
    int file_size;
    int block_in_use;
    int index_block;
    int file_id;
public:
    FsFile() {
        file_size = 0;
        block_in_use = 0;
        index_block = -1;
        file_id = -1;
    }
    ~FsFile() = default;
    int blocksUsage() const{ return block_in_use; }
    int getIndex() const{ return index_block; }
    int getSize() const{ return file_size; }
    int getId() const{ return file_id; }
    void incBlocksUsage(){ block_in_use++; }
    void incFileSize(){ file_size++; }
    void setIndex(int index){ index_block = index; }
    void setId(int id){ file_id = id; }
};

/***File Descriptor Class***/
class FileDescriptor {
    string file_name;
    FsFile* fs_file;
    bool inUse;
public:
    FileDescriptor(string FileName, FsFile* fsi) {
        file_name = move(FileName);
        fs_file = fsi;
        inUse = true;
    }
    ~FileDescriptor(){
        delete this->fs_file;
    }
    //setters and getters.
    string getFileName() { return file_name; }
    bool isInUse() const{ return inUse; }
    FsFile *getFsFile() {return fs_file;}
    void setUsing(bool status){ this->inUse = status; }
};
/***File System Disk Class***/
class fsDisk {
    FILE *sim_disk_fd; //disk file pointer.
    bool is_formated;  //saves if the disk is formated.
    int block_size;    //size of the block.
    int num_of_files;  //used when creating new fd.
    int max_file_size; //maximum number of bytes in a file.
    int freeBlocks;    //number of the unused blocks.
    int BitVectorSize; //number of the blocks in the disk.
    int *BitVector;    //saves for each block if it used.
    map<int,FileDescriptor*> OpenfileDescriptors; //contains the opened files.
    vector<FileDescriptor*> MainDir; //contains all the files on the disk.
    vector<int> fds;   //saves files ids after deleting.
    // ------------------------------------------------------------------------
public:
    fsDisk() { /**constructor**/
        sim_disk_fd = fopen(DISK_SIM_FILE , "w+");
        assert(sim_disk_fd);
        int ret_val;
        for (int i=0; i < DISK_SIZE ; i++) { //filling the disk file with '/0'.
            ret_val = fseek ( sim_disk_fd , i , SEEK_SET);
            assert(ret_val == 0);
            ret_val = (int)fwrite( "\0" ,  1 , 1, sim_disk_fd);
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
        //initializing the parameters with first values.
        BitVector = nullptr;
        num_of_files = 0;
        block_size = 0;
        max_file_size = 0;
        BitVectorSize = 0;
        freeBlocks = 0;
        is_formated = false;
    }
    ~fsDisk(){ /**destructor**/ //closing the file and free all the memory.
        fclose(this->sim_disk_fd);
        if(is_formated) {
            for (auto &file: this->MainDir)
                delete file;
            free(this->BitVector);
            this->MainDir.clear();
            this->OpenfileDescriptors.clear();
        }
    };
    // ------------------------------------------------------------------------ //
    /***Disk's information's print function***/
    void listAll() {
        int i;
        for (i = 0; i < this->MainDir.size(); i++) //printing files' information.
            cout << "index: " << i << ": FileName: " << this->MainDir[i]->getFileName()  <<  " , isInUse: " << this->MainDir[i]->isInUse() << endl;
        int ret_val;
        char bufy;
        cout << "Disk content: '";
        ret_val = fseek(sim_disk_fd, 0, SEEK_SET);
        assert(ret_val == 0);
        for (i = 0; i < DISK_SIZE; i++)//printing disk's content.
        {
            ret_val = (int)fread(&bufy, 1, 1, sim_disk_fd);
            assert(ret_val == 1);
            cout << "(" << bufy << ")";
        }
        cout << "'" << endl;
    }
    // ------------------------------------------------------------------------ //
    /***Disk formatting function***/
    void fsFormat( int blockSize =4 ) {
        if(sim_disk_fd == nullptr){//if the file is not opened.
            fprintf(stderr,"Disk file is not opened!");
            return;
        }
        this->block_size = blockSize;
        this->max_file_size = blockSize*blockSize;
        this->BitVectorSize = DISK_SIZE/this->block_size;
        this->freeBlocks = this->BitVectorSize;
        this->num_of_files = 0;
        int ret_val;
        for (int i=0; i < DISK_SIZE ; i++) {//clearing the disk.
            ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            assert(ret_val == 0);
            ret_val = (int)fwrite( "\0" ,  1 , 1, sim_disk_fd);
            assert(ret_val == 1);
        }
        if(this->BitVector != nullptr) free(BitVector);
        this->BitVector = (int *) calloc(this->BitVectorSize, sizeof(int));
        assert(BitVector != nullptr);
        for(int i = 0; i < BitVectorSize; i++)//initializing the bit vector with zeros.
            BitVector[i] = 0;
        for(auto & file : this->MainDir) //free the used memory if any file is exists.
            delete file;
        this->OpenfileDescriptors.clear();//remove the opened files.
        this->MainDir.clear();//remove the files.
        this->fds.clear();
        this->is_formated = true;
        cout << "Disk formatted!" << endl;
    }
    // ------------------------------------------------------------------------ //
    /*****Creating File Function*****/
    int CreateFile(string fileName) {
        if(!this->is_formated || this->isExists(fileName) != -1)
            return -1;
        FsFile *fs_file;
        FileDescriptor *newFile;
        int fileId = this->createId();
        fs_file = new FsFile();
        newFile = new FileDescriptor(move(fileName), fs_file);
        newFile->getFsFile()->setId(fileId);
        this->MainDir.push_back(newFile);
        this->OpenfileDescriptors.insert({fileId,newFile});
        return fileId;
    }
    // ------------------------------------------------------------------------ //
    /*****Opening File Function*****/
    int OpenFile(string fileName) {
        int fd = this->isExists(fileName);
        if(fd == -1) //if file is not exists.
            cout << "file name " << fileName << " is not exists!" << endl;
        else if(this->isOpened(fd) && this->OpenfileDescriptors.at(fd)->isInUse()){ //if file is opened.
            cout << "file name " << fileName << " is opened!" << endl;
            fd = -1;
        } else {
            MainDir[fd]->setUsing(true);
            int id = MainDir[fd]->getFsFile()->getId();
            this->OpenfileDescriptors.insert({id,MainDir[fd]});
            fd = id;
        }
        return fd;
    }
    // ------------------------------------------------------------------------ //
    /*****Closing File Function*****/
    string CloseFile(int fd){
        if(this->isOpened(fd)){
            this->OpenfileDescriptors.at(fd)->setUsing(false);
            string fileName = this->OpenfileDescriptors.at(fd)->getFileName();
            this->OpenfileDescriptors.erase(fd);
            return fileName;
        }
        return "-1";
    }
    // ------------------------------------------------------------------------ //
    /*****Writing to File Function*****/
    int WriteToFile(int fd, char *buf, int len ) {
        if(!this->is_formated || !this->isOpened(fd))
            return -1;
        int fileSize = this->OpenfileDescriptors.at(fd)->getFsFile()->getSize();
        int lenToWrite, indexBlock;
        if((fileSize + len) > max_file_size) lenToWrite = max_file_size-fileSize; //if file size isn't enough.
        else lenToWrite = len;
        if(fileSize == 0) { //if writing on file for the first time.
            indexBlock = this->getEmptyBlock();
            if (indexBlock == -1) {
                cout << "The Disk is full!" << endl;
                return -1;
            }
            this->OpenfileDescriptors.at(fd)->getFsFile()->setIndex(indexBlock);
        }
        indexBlock = this->OpenfileDescriptors.at(fd)->getFsFile()->getIndex();
        int block, offset, ret_val, writtenCharsNum = 0, inBuf = 0;
        unsigned char buffer;
        while((lenToWrite--) > 0){
            if(fileSize%block_size > 0){// if there's a space in the last block in use.
                offset = this->OpenfileDescriptors.at(fd)->getFsFile()->blocksUsage()-1;
                ret_val = fseek(this->sim_disk_fd,(indexBlock*block_size)+offset,SEEK_SET);
                assert(ret_val == 0);
                ret_val = (int)fread(&buffer,1,1,this->sim_disk_fd);
                assert(ret_val == 1);
                block = (int) buffer-48;
                offset = (fileSize%block_size);
            }
            else{// if a new block is needed.
               if( (block = this->getEmptyBlock()) < 0)
                   break;
               this->addBlockToFile(fd,block);
               offset = 0;
               this->OpenfileDescriptors.at(fd)->getFsFile()->incBlocksUsage();
            }
            // writing a char in the block.
            ret_val = fseek(this->sim_disk_fd,(block*block_size)+offset,SEEK_SET);
            assert(ret_val == 0);
            ret_val = (int) fwrite(&buf[inBuf++],1,1, this->sim_disk_fd);
            assert(ret_val == 1);
            this->OpenfileDescriptors.at(fd)->getFsFile()->incFileSize();
            fileSize++;
            writtenCharsNum++;
        }
        if(writtenCharsNum == 0 && fileSize == 0) { //if not have a space to write in a new file.
            this->OpenfileDescriptors.at(fd)->getFsFile()->setIndex(-1);
            BitVector[indexBlock] = 0;
        }
        if(writtenCharsNum == len) // writing success.
           return writtenCharsNum;
        else return -1;
    }
    // ------------------------------------------------------------------------ //
    /*****Deleting File Function*****/
    int DelFile(string FileName) {
        int fd = -1, index = 0, i = 0;
        for(auto & file : this->MainDir) { //pass on the files to find the file to delete.
            if (file->getFileName() == FileName) {
                int blockIndex = file->getFsFile()->getIndex();
                if(blockIndex != -1) {
                    int len = file->getFsFile()->blocksUsage();
                    int ret_val;
                    unsigned char c;
                    while ((len--) > 0) { //passing on index block.
                        ret_val = fseek(this->sim_disk_fd, (blockIndex * block_size) + (i++), SEEK_SET);
                        assert(ret_val == 0);
                        ret_val = (int) fread(&c, 1, 1, this->sim_disk_fd);
                        assert(ret_val == 1);
                        this->deleteBlock((int) c-48); //clear block.
                    }
                    this->deleteBlock(blockIndex); //clear the index block.
                }
                //remove from MainDir and open files.
                fd = file->getFsFile()->getId();
                if(isOpened(fd)) this->OpenfileDescriptors.erase(fd);
                this->fds.push_back(fd);
                delete file;
                this->MainDir.erase(MainDir.begin()+index);
                return fd;
            }
            index++;
        }
        return fd;
    }
    // ------------------------------------------------------------------------ //
    /*****Reading from File Function*****/
    int ReadFromFile(int fd, char *buf, int len ) {
        if(!this->is_formated || !this->isOpened(fd)) {
            strcpy(buf,"-1");
            return -1;
        }
        int fileSize = this->OpenfileDescriptors.at(fd)->getFsFile()->getSize();
        int lenToRead;
        if(len > fileSize) lenToRead = fileSize;
        else lenToRead = len;
        int indexBlock = this->OpenfileDescriptors.at(fd)->getFsFile()->getIndex();
        int blocksNum = this->OpenfileDescriptors.at(fd)->getFsFile()->blocksUsage();
        int index = 0, blockDir, charsToRead, ret_val, readChars = lenToRead, i = 0;
        unsigned char toRead;
        while(index < blocksNum && lenToRead > 0){//passing on index block.
            ret_val = fseek(this->sim_disk_fd,(indexBlock*block_size)+(index++),SEEK_SET);
            assert(ret_val == 0);
            ret_val = (int) fread(&toRead,1,1, this->sim_disk_fd);
            assert(ret_val == 1);
            blockDir = (int) (toRead - 48);
            charsToRead = min(len,block_size);
            while ((charsToRead--) > 0){ //reading from block.
                ret_val = fseek(this->sim_disk_fd,(blockDir*block_size)+(i%block_size),SEEK_SET);
                assert(ret_val == 0);
                ret_val = (int) fread(&buf[i++],1,1, this->sim_disk_fd);
                assert(ret_val == 1);
                lenToRead--;
            }
        }
        if(readChars > 0) buf[i] = '\0';
        else strcpy(buf,"-1");
        return readChars;
    }
    /****Help Functions****/
    int createId(){
        int fd;
        if(!fds.empty()) {
            fd = *min_element(fds.begin(), fds.end());
            for(int i = 0; i < fds.size(); i++){
                if(fds[i] == fd) fds.erase(fds.begin() + i);
            }
        }
        else fd = num_of_files++;
        return fd;
    }
    int getEmptyBlock(){
        int i;
        for(i = 0; i < this->BitVectorSize && this->BitVector[i] != 0; i++);
        if(i >= this->BitVectorSize) return -1;
        else {
            this->BitVector[i] = 1;
            freeBlocks--;
            return i;
        }
    }
    bool isOpened(int fd){
        return this->OpenfileDescriptors.find(fd) != this->OpenfileDescriptors.end();
    }
    int isExists(string name){
        for(int i = 0; i < this->MainDir.size(); i++){
            if(MainDir[i]->getFileName() == name){
                return i;
            }
        }
        return -1;
    }
    int addBlockToFile(int fd, int block){
        int indexBlockOffset = this->OpenfileDescriptors.at(fd)->getFsFile()->blocksUsage();
        if(indexBlockOffset == this->block_size)
            return -1;
        int index = this->OpenfileDescriptors.at(fd)->getFsFile()->getIndex();
        unsigned char blockNum;
        blockNum = ((unsigned char) block)+48;
        int ret_value;
        ret_value = fseek(this->sim_disk_fd,(index* this->block_size)+indexBlockOffset,SEEK_SET);
        assert(ret_value == 0);
        ret_value = (int)fwrite(&blockNum,1,1, this->sim_disk_fd);
        assert(ret_value == 1);
        return 1;
    }
    void deleteBlock(int block){
        char toWrite = '\0';
        int ret_val;
        for(int i = 0; i < block_size; i++) {
            ret_val = fseek(this->sim_disk_fd,(block*block_size)+i,SEEK_SET);
            assert(ret_val == 0);
            ret_val = (int) fwrite(&toWrite, 1, 1, this->sim_disk_fd);
            assert(ret_val == 1);
        }
        this->BitVector[block] = 0;
        freeBlocks++;
    }
};

int main() {
    int blockSize;
    //int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read;
    int _fd;
    int s;

    fsDisk *fs;
    fs = new fsDisk();
    int cmd_;
    while(true) {
        cin >> cmd_;
        switch (cmd_)
        {
            case 0:   // exit
                delete fs;
                exit(0);

            case 1:  // list-file
                fs->listAll();
                break;

            case 2:    // format
                cin >> blockSize;
                fs->fsFormat(blockSize);
                break;

            case 3:    // creat-file
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 4:  // open-file
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 5:  // close-file
                cin >> _fd;
                fileName = fs->CloseFile(_fd);
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 6:   // write-file
                cin >> _fd;
                cin >> str_to_write;
                s = fs->WriteToFile( _fd , str_to_write , (int)strlen(str_to_write) );
                cout << "write function: " << s << endl;
                break;

            case 7:    // read-file
                cin >> _fd;
                cin >> size_to_read ;
                //str_to_read[0] = '\0';
                _fd = fs->ReadFromFile( _fd , str_to_read , size_to_read );
                cout << "ReadFromFile " << _fd << " characters: " << str_to_read << endl;
                break;

            case 8:   // delete file
                cin >> fileName;
                _fd = fs->DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            default:
                break;
        }
    }
}