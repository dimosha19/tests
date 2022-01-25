#include <iostream>
#include <utility>
#include <vector>
#include <map>
#include <chrono>
#include <fstream>
using namespace std::chrono;

using namespace std;
int fat[400];
bool HDD[400]; // При изменении обратить внимание на конструктор файла и метод открытия
bool RAM[400];
struct Properties;
class Note;
class Directory;
class File;
vector<Properties *> allFiles;
vector<Directory *> movement;
Directory * currentDir;
Directory * rootDir;
unsigned char isReadable;
unsigned char isExecutable;
unsigned char isWriteable;
unsigned char isArchived;
unsigned char isHidden;

int FirstPointerFinderHDD(){
    for (int i = 0; i < 400; i++) {
        if (!HDD[i]) return i;
    }
    return -3;
}

void PusherHDD(int size, int first){
    int originalSize = size;
    int pointers[size];
    int n = 0;
    for (int i = first; i < 400; i++){
        if (!HDD[i] && size > 0){
            HDD[i] = true;
            size--;
            pointers[n++] = i;
        }
    }
    for (int i = 0; i < originalSize - 1; i++){
        fat[pointers[i]] = pointers[i+1];
    }
    fat[pointers[originalSize - 1]] = -1;

}

void CleanerHDD(int first){
    if (first == -1) return;
    while (fat[first] != -1){
        HDD[first] = false;
        first = fat[first];
    }
    HDD[first] = false;
}

struct Properties{
    bool isDir;
    char * name = nullptr;
    int size;
    bool isPtr = false;
    File* parent = nullptr;
    unsigned char mod = 0;
    int firstHDD = -1;
    int firstRAM = -1;

    string GetName() const{
        string sname;
        for (int i = 0; name[i]!= 0; i++){
            sname+=name[i];
        }
        return sname;
    }
};

char * ToChar(const string& str, const string& str2){
    int n = 0;
    char * cname = new char[str.size() + str2.size() + 1] {};
    for (auto i : str){
        cname[n++] = i;
    }
    for (auto i : str2){
        cname[n++] = i;
    }
    return cname;
}

class Note {
public:
    virtual void open() = 0;
    virtual Properties getProperties() = 0;
    virtual Note* copy(Note* file) = 0;
};


class File :public Note {
public:
    Properties properties;
    explicit File(Properties _properties) : properties(_properties){
        properties.mod |= isReadable | isWriteable | isExecutable;
        if (!properties.isPtr){
            properties.firstHDD = FirstPointerFinderHDD();
            PusherHDD(properties.size, properties.firstHDD);
        }
    };
    ~File(){
        CleanerHDD(properties.firstHDD);
    }
    void chmod(){
        properties.mod ^= isExecutable;
    }
    Note * copy(Note* file) override {
        Properties copiedProp = file->getProperties();
        copiedProp.name = ToChar(copiedProp.GetName(), "_copy");
        copiedProp.firstHDD = FirstPointerFinderHDD();
        Note * newfile = new File(copiedProp);
        return newfile;
    }
    void open() override {
        if (static_cast<bool>(properties.mod & isExecutable)){
            int start = 0, end = 0;
            for (auto i : RAM){
                if (end - start + 1 == properties.size) {
                    properties.firstRAM = start;
                    allFiles.push_back(&properties);
                    for (;start <= end; start++){
                        RAM[start] = true;
                    }
                    break;
                }
                else if (!i && end - start < properties.size) end++;
                else if (i && end - start < properties.size) {
                    start = end + 1;
                    end++;
                }
            }
        } else {
            cout << "file not executable" << endl;
        }
    }
    void close() {
        for (int i = properties.firstRAM; i < properties.size + properties.firstRAM; i++){
            RAM[i] = false;
        }
    }
    Properties getProperties() override {
        return properties;
    }

};

class Directory :public Note {

public:
    string name;
    Properties properties{true, ToChar(name, ""), 0};
    vector<Note *> List;

    explicit Directory(string _dirName) : name(move(_dirName)) {};
    ~Directory(){
        for (auto i : List){
            Delete(i->getProperties().GetName());
        }
        currentDir = rootDir;
    }
    Directory(const Directory &directory){

    }
    void open() override {
        cout << "Opening Dir" << endl;
    }
    void Delete(const string& target) {
        for (auto i : List) {
            if (i->getProperties().GetName() == target) {
                for (auto j : allFiles){
                    if ( j->GetName() == i->getProperties().GetName()) allFiles.erase(remove(allFiles.begin(), allFiles.end(), j), allFiles.end());   // erase(remove(allFiles.begin(), allFiles.end(), j), allFiles.end());
                }
                if (i->getProperties().isDir) delete dynamic_cast<Directory *>(i);
                else delete dynamic_cast<File *>(i);
                List.erase(remove(List.begin(), List.end(), i), List.end());
                return;
            }
        }
        cout << "No such file or directory" << endl;
    }
    void createFile(const string& fileName, int size) {
        auto X = new File({false, ToChar(fileName, ""), size});
        List.push_back(X);
    };

    void createFileWeakPtr(const string& ptrName, const string& parentName) {
        for (auto i : List){
            if (i->getProperties().GetName() == parentName) {

                auto X = new File({false, ToChar(ptrName, ""), 0, true, dynamic_cast<File *>(i), 0, -1});
                List.push_back(X);
            }
        }
    };

    Note * copy(Note * file) override{
        // create dir
        // fill dir
        // return * dir
        auto * copy = new Directory(ToChar(properties.GetName(), "_copy"));
        for (auto i : List){
            copy->List.push_back(i->copy(i));
        }
        return copy;
    }
    void createDir(const string& newDirName){
        auto X = new Directory(ToChar(newDirName, ""));
        List.push_back(X);
    }
    Properties getProperties() override {
        return properties;
    }
};

void printDir(Directory * dir) {
    for (auto x : dir->List) {
        cout << x->getProperties().GetName() << endl;
    }
}

void goTo(const string& target){
    for (auto i : currentDir->List) {
        if (i->getProperties().isDir && i->getProperties().GetName() == target) {
            currentDir = dynamic_cast<Directory *>(i);
            movement.push_back(currentDir);
            break;
        }
    }
}
void back(){
    if (movement.size() == 1) return;
    movement.pop_back();
    currentDir = movement.back();
}

void memView(){
    cout << "HDD: [ ";
    for (bool i : HDD){
        cout << i;
    }
    cout << " ]" << endl;
    cout << "RAM: [ ";
    for (bool i : RAM){
        cout << i;
    }
    cout << " ]" << endl;
}

void openFile(const string& target){
    for (auto i : currentDir->List){
        if (i->getProperties().GetName() == target && !i->getProperties().isPtr) {
            dynamic_cast<File *>(i)->open();
            return;
        }
        else if (i->getProperties().GetName() == target && i->getProperties().isPtr) {
            i->getProperties().parent->open();
            return;
        }
    }
    cout << "No such file (or parent file)"<< endl;
}

void closeFile(const string& target){
    for (auto i : currentDir->List){
        if (i->getProperties().GetName() == target && !i->getProperties().isPtr) {
            dynamic_cast<File *>(i)->close();
            break;
        }
        else if (i->getProperties().GetName() == target && i->getProperties().isPtr) {
            i->getProperties().parent->close();
            break;
        }
    }
}

void copy(const string & target){
    for (auto i : currentDir->List){
        if (i->getProperties().GetName() == target) {
            currentDir->List.push_back(i->copy(i));
        }
    }
}

string path(){
    string s;
    for (auto i: movement){
        s += i->getProperties().GetName();
        s+="/";
    }
    return s;
}
// [rwe]
void chmod(const string &target) {
    for (auto i : currentDir->List){
        if (i->getProperties().GetName() == target && !i->getProperties().isDir) {
            dynamic_cast<File *>(i)->chmod();
        }
    }
}
vector<string> find(string& target){
    vector<string> arr;
    char first = target[0];
    if (first == '*'){
        target = target.substr(1);
    }
    for (auto i : currentDir->List){
        if (i->getProperties().GetName().find(target) != string::npos){
            arr.push_back(i->getProperties().GetName());
        }
    }
    return arr;
}

int main()
{
    for (int & i : fat) {
        i = -2;
    }
    rootDir = new Directory("rootDir");
    currentDir = rootDir;
    movement.push_back(currentDir);
    string command, name, parrentName;
    int size = 3;
    isReadable = 0x01;
    isExecutable = 0x02;
    isWriteable = 0x04;
    isArchived = 0x08;
    isHidden = 0x10;

    ofstream file(R"(C:\Users\Dmitriy\CLionProjects\tests\testzone\testFH.txt)");
    unsigned int CreatingFileTiming[100];
    unsigned int DeletingFileTiming[100];
    unsigned int CreatingDirectoryTiming[100];
    unsigned int DeletingDirectoryTiming[50];

    int cf = 0;
    int df = 0;
    int cd = 0;
    int dd = 0;

//testing files
    for (int i = 0; i < 100; i++){
        auto start = steady_clock::now();
        currentDir->createFile("file", 2);
        auto finish = steady_clock::now();
        auto duration = finish - start;
        CreatingFileTiming[cf++] = duration_cast<nanoseconds>(duration).count();
    }
    for (int i = 0; i < 100; i++){
        auto start = steady_clock::now();
        currentDir->Delete("file");
        auto finish = steady_clock::now();
        auto duration = finish - start;
        DeletingFileTiming[df++] = duration_cast<nanoseconds>(duration).count();
    }
    for (unsigned int i : CreatingFileTiming){
        file << i << " ";
    }
    file << endl;
    for (unsigned int i : DeletingFileTiming){
        file << i << " ";
    }
    file << endl;
//testing directory's
    for (int i = 0; i < 100; i++){
        auto start = steady_clock::now();
        currentDir->createDir("dir");
//        goTo("dir");
        auto finish = steady_clock::now();
        auto duration = finish - start;
        CreatingDirectoryTiming[cd++] = duration_cast<nanoseconds>(duration).count();
    }
    for (unsigned int i : CreatingDirectoryTiming){
        file << i << " ";
    }
    file << endl;

    for (int i = 0; i < 100; i++){
        currentDir->Delete("dir");
    }
    for (int i = 0; i < 50; i++){
        currentDir->createDir("dir" + to_string(i));
        goTo("dir" + to_string(i));
        currentDir->createFile("file", 2);
        back();
    }
    for (int i = 0; i < 50; i++){
        auto start = steady_clock::now();
        currentDir->Delete("dir" + to_string(i));
        auto finish = steady_clock::now();
        auto duration = finish - start;
        DeletingDirectoryTiming[dd++] = duration_cast<nanoseconds>(duration).count();
    }

    for (unsigned int i : DeletingDirectoryTiming){
        file << i << " ";
    }
    return 0;
}