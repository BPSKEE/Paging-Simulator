/*
I the undersigned promise that the submitted assignment is my own work. While I was
free to discuss ideas with others, the work contained is my own. I recognize that
should this not be the case; I will be subject to penalties as outlined in the course
syllabus.
Brandon Skeens
RedID: 826416091
*/

#ifndef PAGETABLE_H
#define PAGETABLE_H

#include <inttypes.h>
#include <vector>
#include <cmath>
#include <cstring>
#include <string>
#include <iostream>
#include <bitset>

using namespace std;

//Max size as defined in the instructions is 28 (28 levels of 1 bit)
#define MAX_SIZE 28

class Map;
class Level;

class PageTable{
    public:
        //Constructor takes the command line input and helps parse some parts
        PageTable(int argc, char* argv[]);
        //Implementation of search function for mapped physical frame
        Map * searchMappedPfn(unsigned int vAddr);
        //Implementation of insertion function
        void insertMapForVpn2Pfn(unsigned int vAddr);
        //Helper function to extract VPNs
        uint32_t* extractVPNS(unsigned int vAddr);
        //Helper function to service tree bitstrings when time for update
        void serviceBitstrings(Level* node, unsigned long long reference);
        //Helper function to find victim frame for replacement
        Map * frameToBeReplaced(Level* node);

        //Stores amount of levels in tree
        int levels;
        //keeps track of how much memory is used
        unsigned long memoryUsed = 0;
        //Arrays to stor important bit information
        uint32_t bitmask[MAX_SIZE];
        uint32_t bitshift[MAX_SIZE];
        int entryCount[MAX_SIZE];
        //Offset of addresses. Initialize to 32, as that is the "starting" value for a 32 bit address offset
        unsigned int offset = 32;
        //Pointer to root node
        Level* root;
        //Initialize page replacement to false (default)
        bool pageReplace = false;
        int availableFrames = 999999;
        //Initialize bitStringUpdate to default value of 10
        int bitStringUpdate = 10;
        int bitStringCounter = 0;
        //Bitstring to keep track of referenced bits during bitstring interval
        unsigned long long referenced = 0b0;
        //If page is replaced, incrememnt
        int replaces = 0;
        //Initialize the frame to keep track of current frame
        unsigned int frame = 0x00000000;
        unsigned int victimBitstring;
        //Victim virtual address mapped to map
        unsigned int victimAddr;
};

class Level{
    public:
        //Initialize level with depth and the pagetable it is a member of
        Level(int level, PageTable* table);

        //Depth of level node
        int depth;
        //Pointer back to the pagetable the level is a member of
        PageTable* pageTable;
        //Array to store values in next level
        Level** nextLevel;
        //Map for leaf node
        Map* map;
        //Stores if the level is a leaf or not
        bool leaf = false;

        //If the tree is only one level, use this array directly
        Map** singleLevelMaps;
};

class Map{
    public:
    //Constructor
        Map(uint32_t pfn, bool valid);
        //Physical frame number
        uint32_t pfn;
        //Vaddr mapped
        uint32_t vAddr;
        //Valid bit, initialize to false
        bool valid = false;
        //String to keep track of aging using 8 bit aging counter
        unsigned int recentUse = 0;
};

#endif