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
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "laswriter.hpp"

#define PI 3.14159265358979323846264338327950288

using namespace std;

class dataBlock{
    public:
        int timeStamp;
        float azimuth;
        float azimuth2;
        vector<float> distances;
        vector<int> reflectivities;
        vector<int> verticalAngles;
        vector<int> blocks;
        vector<float> X;
        vector<float> Y;
        vector<float> Z;
};

string ToHex(const string& s, bool upper_case, string between = ""){
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

string hexToText(string& hex){
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

dataBlock packetParser(string& packet){

    vector<float> distances;
    vector<int> reflectivities;
    vector<int> vAngles;
    vector<int> blocks;

    int timeStamp = -1;

    if(packet.length() > 196){
        string revBytes = packet.substr(202,2) + packet.substr(200,2) + packet.substr(198,2) + packet.substr(196,2);
        timeStamp = strtol(revBytes.c_str(), nullptr, 16);
    }

    float azimuth = (float)strtol( (packet.substr(2,2) + packet.substr(0,2)).c_str() , nullptr, 16) / 100;

    int laserId = 0;
    int block = 1;
    for(int i = 4; i < 196; i+=6){
        string hexDist = packet.substr(i+2,2) + packet.substr(i,2);
        float dist = (float)(2 * std::strtol(hexDist.c_str(), nullptr, 16)) / 1000;

        distances.push_back(dist);

        int reflect = std::strtol( packet.substr(i+4,2).c_str() , nullptr, 16);
        reflectivities.push_back(reflect);

        int vAng = (laserId % 2 == 0) ? (laserId - 15) : laserId;
        vAngles.push_back(vAng);

        blocks.push_back(block);

        if(++laserId > 15){
            laserId = 0;
            block++;
        }
    }

    dataBlock temp;
    temp.azimuth = azimuth;
    temp.timeStamp = timeStamp;
    temp.distances = distances;
    temp.reflectivities = reflectivities;
    temp.verticalAngles = vAngles;
    temp.blocks = blocks;

    return temp;

}

void calcXYZ(dataBlock& db, float clipOut = 0.5){

    db.X.resize(0);
    db.Y.resize(0);
    db.Z.resize(0);

    for(int i = 0; i < db.distances.size(); ++i){
        float az = ( (db.blocks[i] == 1) ? db.azimuth : db.azimuth2 );
        //float zRange = ( (db.verticalAngles[i] < 0) ? -1 : 1 );

        db.X.push_back( db.distances[i] * cos( (float)db.verticalAngles[i] * PI / 180 ) * sin( az * PI / 180 ) );
        db.Y.push_back( db.distances[i] * cos( (float)db.verticalAngles[i] * PI / 180 ) * cos( az * PI / 180 ) );
        db.Z.push_back( db.distances[i] * sin( (float)db.verticalAngles[i] * PI / 180 ) );
    }

    for(int i = db.X.size()-1; i >=0; --i){
        if(abs(db.X[i]) < clipOut || abs(db.Y[i]) < clipOut || abs(db.Z[i]) < clipOut){
            db.blocks.erase(db.blocks.begin()+i);
            db.distances.erase(db.distances.begin()+i);
            db.reflectivities.erase(db.reflectivities.begin()+i);
            db.verticalAngles.erase(db.verticalAngles.begin()+i);
            db.X.erase(db.X.begin()+i);
            db.Y.erase(db.Y.begin()+i);
            db.Z.erase(db.Z.begin()+i);
        }
    }

}

class VLPcloud{
    public:
        vector<dataBlock> dataBlocks;

        void build(char* fileName){
            dataBlocks.resize(0);
            string fileString = hexStringMaker(fileName);
            boost::replace_all(fileString, "ffee", " ");

            std::vector<std::string> packets = stringSplitter(fileString);

            for(int p = 1; p < packets.size(); ++p){
                dataBlock pack = packetParser(packets[p]);
                dataBlocks.push_back(pack);
            }
        }

        void calculate(){

            int stampIndex = -1;
            int firstUntimed = 0;

            for(int i = 0; i < dataBlocks.size(); ++i){
                dataBlock& temp = dataBlocks[i];

                //timestamps for all (without interpolation)
                if(temp.timeStamp > -1 && temp.timeStamp != stampIndex){
                    for(int j = firstUntimed ; j < i; ++j){
                        dataBlock& temp2 = dataBlocks[j];
                        temp2.timeStamp = temp.timeStamp;
                    }
                }else if(temp.timeStamp == -1 && temp.timeStamp != stampIndex){
                    firstUntimed = i;
                }

                //interpolate azimuths and calculate coordinates
                if(i > 0){
                    dataBlock& temp0 = dataBlocks[i-1];
                    float az0 = temp0.azimuth;
                    float az1 = temp.azimuth;

                    temp0.azimuth2 = (az0 + az1) / 2;

                    calcXYZ(temp);
                }

                stampIndex = temp.timeStamp;
            }

            calcXYZ(dataBlocks[0]);
        }


        void write(char* fileName, bool shiftZ = false){

            string file(fileName);
            string extension = file.substr(file.size()-3, 3);
            boost::algorithm::to_lower(extension);

            if(extension == "las" || extension == "laz"){

                int format_macro = ((extension == "laz") ? LAS_TOOLS_FORMAT_LAZ : LAS_TOOLS_FORMAT_LAS);

                LASwriteOpener laswriteopener;
                laswriteopener.set_file_name(fileName);
                laswriteopener.set_format(format_macro);

                LASheader lasheader;
                lasheader.point_data_format = 0;
                lasheader.point_data_record_length = 20;
                LASpoint laspoint;
                laspoint.init(&lasheader, lasheader.point_data_format, lasheader.point_data_record_length, &lasheader);
                LASwriter* laswriter = laswriteopener.open(&lasheader);

                vector<dataBlock>* coordinates = &dataBlocks;
                vector<dataBlock>::iterator packet;

                packet = coordinates->begin();
                while (packet != coordinates->end()){

                    for(int pt = 0; pt < packet->blocks.size(); ++pt){
                        laspoint.set_x( packet->X[pt] );
                        laspoint.set_y( ((shiftZ) ? packet->Z[pt] : packet->Y[pt]) );
                        laspoint.set_z( ((shiftZ) ? packet->Y[pt] : packet->Z[pt]) );
                        laspoint.set_intensity( packet->reflectivities[pt] );
                        laspoint.set_gps_time( packet->timeStamp );
                        //laspoint.set_user_data( packet->timeStamp );

                        laswriter->write_point(&laspoint);
                        laswriter->update_inventory(&laspoint);
                    }
                    packet++;
                }

                laswriter->update_header(&lasheader, TRUE);
                laswriter->close(TRUE);

                delete laswriter;

            }else{

                ofstream myfile;
                myfile.open (fileName);
                myfile << "timeStamp block azimuth verticalAngle distance x y z reflectivity\n";
                for(int p = 1; p < dataBlocks.size(); ++p){
                    dataBlock pack = dataBlocks[p];
                    for(int i = 0; i < pack.distances.size(); ++i){
                        myfile <<
                        pack.timeStamp << " " <<
                        pack.blocks[i] << " " <<
                        ((pack.blocks[i] == 1) ? pack.azimuth : pack.azimuth2) << " " <<
                        pack.verticalAngles[i] << " " <<
                        pack.distances[i] << " " <<
                        pack.X[i] << " " <<
                        ((shiftZ) ? pack.Z[i] : pack.Y[i]) << " " <<
                        ((shiftZ) ? pack.Y[i] : pack.Z[i]) << " " <<
                        pack.reflectivities[i] << "\n";
                    }
                }
                myfile.close();
            }
        }
};

int main(){

    VLPcloud nuvem;
    nuvem.build("n200.pcap");
    nuvem.calculate();
    nuvem.write("pcap.txt");

    return 0;
}
