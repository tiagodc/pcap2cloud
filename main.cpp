#include <fstream>
#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <iterator>
#include <cstddef>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

typedef string hexVector;

string ToHex(const string& s, bool upper_case, string between = " ")
{
    ostringstream ret;

    for (string::size_type i = 0; i < s.length(); ++i)
    {
        int z = s[i]&0xff;
        ret << std::hex << std::setfill('0') << std::setw(2) << (upper_case ? std::uppercase : std::nouppercase) << z << between;
        //if(i > 20) break;
    }

    return ret.str();
}

string hexStringMaker(char* fileName){

    ifstream::pos_type size;
    char * memblock;
    string tohexed;

    ifstream file (fileName, ios::in|ios::binary|ios::ate);
      if (file.is_open())
      {
        size = file.tellg();
        memblock = new char [size];
        file.seekg (0, ios::beg);
        file.read (memblock, size);
        file.close();

        cout << "the complete file content is in memory" << endl;

        tohexed = ToHex(std::string(memblock, size), false);

       }else{
        cout << "could not open file" << endl;
       }

       return tohexed;
}

std::vector<std::string> stringSplitter(string& spacedPcap){
    std::stringstream ss(spacedPcap);
    std::istream_iterator<std::string> begin(ss);
    std::istream_iterator<std::string> end;
    std::vector<std::string> vstrings(begin, end);
    //std::copy(vstrings.begin(), vstrings.end(), std::ostream_iterator<std::string>(std::cout, "\n"));
    return vstrings;
}

string hexToText(string hex){
    int len = hex.length();
    std::string newString;
    for(int i=0; i< len; i+=2)
    {
        string byte = hex.substr(i,2);
        char chr = (char) (int)strtol(byte.c_str(), nullptr, 16);
        newString.push_back(chr);
    }
    return newString;
}

int main(){

    string fileString = hexStringMaker("n24.pcap");

    //cout << fileString << endl;

    std::vector<std::string> pcapBytes = stringSplitter(fileString);

    //cout << pcapBytes[0]+pcapBytes[1] << endl;

    return 0;
}
