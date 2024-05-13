/*
I the undersigned promise that the submitted assignment is my own work. While I was
free to discuss ideas with others, the work contained is my own. I recognize that
should this not be the case; I will be subject to penalties as outlined in the course
syllabus.
Brandon Skeens
RedID: 826416091
*/

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cstring>
#include <iomanip>
#include "vaddr_tracereader.h"
#include "log_helpers.h"
#include "pagingwithpr.h"

using namespace std;

//Function to extract a VPN from the virtual address, mask, and shift given
unsigned int extractVPNFromVirtualAddress(unsigned int virtualAddress, unsigned int mask, unsigned int shift) {
    //Bitwise and, then right shift
    return (virtualAddress & mask) >> shift;
}



//Helper method to print the offset if given the virtual address and offset bits
unsigned int offsetCalc(unsigned int vAddr, unsigned int offsetBits) {
    //When calculating offset, you want to pull the last n binary bits
    //This could be done by doing a modulo operation on the vAddr, with the 
    //modulo value being 2^n, where n is the number of offset bits.
    return vAddr % static_cast<unsigned int>(pow(2, offsetBits));
}

int main(int argc, char* argv[]) {
   //search input for the trace file by checking each input for a ".tr" file type
   const char* inputName;
   for (int i = 1; i < argc; ++i) {
        //If there is an arg passed longer than 3 characters, check if it ends in .tr
        if (strlen(argv[i]) > 3) {
            if (strcmp(argv[i] + (strlen(argv[i]) - 3), ".tr") == 0) {
                //If found, store name and break out of loop
                inputName = argv[i];
                break;
            }
        }
   }

    //Print error if file is not open
    FILE* tracef_h = fopen(inputName, "r");
    if (tracef_h == nullptr) {
        cout << "Unable to open <<" << inputName << ">>" << endl;
        exit(1);
    } 

    //Initialize pagetable object with command line args
    PageTable* tablePtr = new PageTable(argc, argv);

    if (tablePtr->levels == 1) {
        //If single level tree, immediately allocate and initialize maps to null
        tablePtr->root->singleLevelMaps = new Map*[static_cast<int>(pow(2, tablePtr->entryCount[0]))];
        for (unsigned int i = 0; i < static_cast<unsigned int>(pow(2, tablePtr->entryCount[0])); ++i) {tablePtr->root->singleLevelMaps[i] = nullptr;}
    } else {
        //If multilevel, allocate and initialzie nextLevel to null
        tablePtr->root->nextLevel = new Level*[static_cast<int>(pow(2, tablePtr->entryCount[0]))];
        for (unsigned int i = 0; i < static_cast<unsigned int>(pow(2, tablePtr->entryCount[0])); ++i) {tablePtr->root->nextLevel[i] = nullptr;}
    }

    //Read in other optional arguments and store into values
    int memoryAccesses;
    int Nfound = false;
    int bitstringUpdate = 10;
    const char* mode = "summary";

    //Parse input values and make sure the optional values entered are valid
    for (int i = 0; i < argc - 1; ++i) {
        //If "-n" arg is present, ensure it is a valid number then read in and convert
        if (strcmp(argv[i], "-n") == 0) {
            if (stoi(argv[i + 1]) < 1) {
                cout << "Number of memory accesses must be a number and greater than 0" << endl;
                exit(1);
            }
            memoryAccesses = stoi(argv[i + 1]);
            Nfound = true;
        }
        //If "-f" arg is present, ensure it is a valid number then read in and convert
        else if (strcmp(argv[i], "-f") == 0) {
            if (stoi(argv[i + 1]) < 1) {
                cout << "Number of available frames must be a number and greater than 0" << endl;
                exit(1);
            }
            tablePtr->availableFrames = stoi(argv[i + 1]);
            tablePtr->pageReplace = true;
        }
        //If "-b" arg is present, ensure it is a valid number then read in and convert
        else if (strcmp(argv[i], "-b") == 0) {
            if (stoi(argv[i + 1]) < 1) {
                cout << "Bit string update interval must be a number and greater than 0" << endl;
                exit (1);
            }
            bitstringUpdate = stoi(argv[i + 1]);
        }
        //If "-l" arg is present, read in next argument for mode
        else if (strcmp(argv[i], "-l") == 0) {
            mode = argv[i + 1];
        }
    }


    //Initialize variables used in loop
    p2AddrTr mtrace;
    unsigned int vAddr;

    //If mode is bitmasks, make call to log bitmasks function using stored table values
    if (strcmp(mode, "bitmasks") == 0) {
        log_bitmasks(tablePtr->levels, tablePtr->bitmask);
    } //If mode entered is summary
    else if (strcmp(mode, "summary") == 0) {
        //Variables necessary to keep track in summary
        int hits = 0;
        int framesAllocated = 0;
        int addresses = 0;
        unsigned int replaces = 0;
        unsigned int pagesize = pow(2, tablePtr->offset);

        //if amount of memory accesses is specified
        if (Nfound) {
            for (int i = 0; i < memoryAccesses; ++i) {
                if (NextAddress(tracef_h, &mtrace)) {
                    //Pull address from file
                    vAddr = mtrace.addr;
                    //If the address is already in the tree, it is a hit
                    if (tablePtr->searchMappedPfn(vAddr) != nullptr) {hits++;}
                    //Insert
                    tablePtr->insertMapForVpn2Pfn(vAddr);
                }
            }
            addresses = memoryAccesses;
        } //if memory accesses is not specified
        else {
            while (NextAddress(tracef_h, &mtrace)) {
                vAddr = mtrace.addr;
                if (tablePtr->searchMappedPfn(vAddr) != nullptr) {hits++;}
                tablePtr->insertMapForVpn2Pfn(vAddr);
                addresses++;
            }
        }
        framesAllocated = addresses - hits;
        log_summary(pagesize, replaces, hits, addresses, framesAllocated, tablePtr->memoryUsed);             
    } //If mode entered is va2pa
    else if (strcmp(mode, "va2pa") == 0) {
        unsigned int pAddr;
        //If memory accesses is specified, run for loop
        if (Nfound) {
            for (int i = 0; i < memoryAccesses; ++i) {
                if (NextAddress(tracef_h, &mtrace)) {
                    //Extract virtual address
                    vAddr = mtrace.addr;
                    //Insert virual address
                    tablePtr->insertMapForVpn2Pfn(vAddr);
                    if(tablePtr->searchMappedPfn(vAddr) != nullptr) {
                        pAddr = tablePtr->searchMappedPfn(vAddr)->pfn << tablePtr->offset;
                        pAddr |= offsetCalc(vAddr, tablePtr->offset);
                        log_va2pa(vAddr, pAddr);
                    }
                }
            }
        } //if memory accesses is not specified, run while to iterate through whole file
        else {
            while (NextAddress(tracef_h, &mtrace)) {
                vAddr = mtrace.addr;
                //Insert map for the given VPN
                tablePtr->insertMapForVpn2Pfn(vAddr);
                if(tablePtr->searchMappedPfn(vAddr) != nullptr) {
                    //Search for the mapped pfn and grab its physical frame number, then shift by offset bits
                    pAddr = tablePtr->searchMappedPfn(vAddr)->pfn << tablePtr->offset;
                    //Add offset calculated from virtual address to physical frame number
                    pAddr |= offsetCalc(vAddr, tablePtr->offset);
                    log_va2pa(vAddr, pAddr);
                }
            }
        }
    } //If mode entered is vpns_pfn
    else if (strcmp(mode, "vpns_pfn") == 0) {
        if (Nfound) {
            for(int i = 0; i < memoryAccesses; ++i) {
                if (NextAddress(tracef_h, &mtrace)) {
                    vAddr = mtrace.addr;
                    tablePtr->insertMapForVpn2Pfn(vAddr);
                    if (tablePtr->searchMappedPfn(vAddr) != nullptr) {log_vpns_pfn(tablePtr->levels, tablePtr->extractVPNS(vAddr), tablePtr->searchMappedPfn(vAddr)->pfn);}
                }
            }
        } else {
            while (NextAddress(tracef_h, &mtrace)) {
                vAddr = mtrace.addr;
                tablePtr->insertMapForVpn2Pfn(vAddr);
                //Increment frame number
                uint32_t* vpns = tablePtr->extractVPNS(vAddr);
                log_vpns_pfn(tablePtr->levels, vpns, tablePtr->searchMappedPfn(vAddr)->pfn);
            }
        }
    } //If mode entered is vpn2pfn_pr
    else if (strcmp(mode, "vpn2pfn_pr") == 0) {
        if (Nfound) {
            for (int i = 0; i < memoryAccesses; ++i) {
                if (NextAddress(tracef_h, &mtrace)) {
                    //Address and hit marker
                    bool hit = false;
                    vAddr = mtrace.addr;
                    //If the address was already mapped, it is a hit
                    if (tablePtr->searchMappedPfn(vAddr) != nullptr) {hit = true;}
                    //Temporary value to keep track of the amount of replaces before inserting
                    int currReplace = tablePtr->replaces;
                    tablePtr->insertMapForVpn2Pfn(vAddr);
                    //If a replace occurred since the last address was inserted, format output
                    if (currReplace != tablePtr->replaces) {
                        log_mapping(vAddr, tablePtr->searchMappedPfn(vAddr)->pfn, tablePtr->victimAddr, tablePtr->victimBitstring, hit);
                    } //IF a replace did not occur, print out default
                    else {
                        log_mapping(vAddr, tablePtr->searchMappedPfn(vAddr)->pfn, -1, 0, hit);
                    }
                }
            }
        } //if memory accesses is not specified
        else {
            while (NextAddress(tracef_h, &mtrace)) {
                //Address and hit marker
                bool hit = false;
                vAddr = mtrace.addr;
                //If the address was already mapped, it is a hit
                if (tablePtr->searchMappedPfn(vAddr) != nullptr) {hit = true;}
                //Temporary value to keep track of the amount of replaces before inserting
                int currReplace = tablePtr->replaces;
                tablePtr->insertMapForVpn2Pfn(vAddr);
                //If a replace occurred since the last address was inserted, format output
                if (currReplace != tablePtr->replaces) {
                    log_mapping(vAddr, tablePtr->searchMappedPfn(vAddr)->pfn, tablePtr->victimAddr, tablePtr->victimBitstring, hit);
                } //If a replace did not occur, print out default
                else {
                    log_mapping(vAddr, tablePtr->searchMappedPfn(vAddr)->pfn, -1, 0, hit);
                }
            }
        }
    } //If mode entered is offset, iterate through the specified number of addresses to print out the offset
    else if (strcmp(mode, "offset") == 0) {
        //If a number is specified in the input, iterate through fot loop
        if (Nfound) {
            for (int i = 0; i < memoryAccesses; ++i) {
                if(NextAddress(tracef_h, &mtrace)) {
                    vAddr = mtrace.addr;
                    //Call to printoffset helper func
                    print_num_inHex(offsetCalc(vAddr, tablePtr->offset));
                }
            }
        } // If no amount of accesses is specified, iterate through whole file 
        else {
            while (NextAddress(tracef_h, &mtrace)) {
                vAddr = mtrace.addr;
                //Call to printoffset helper func
                print_num_inHex(offsetCalc(vAddr, tablePtr->offset));
            }
        }
    } //if the mode was entered as something else, print out an error message and exit
    else {
        cout << "Mode not recognized" << endl;
        exit(1);
    }
    return 0;
}