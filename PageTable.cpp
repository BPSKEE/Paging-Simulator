/*
I the undersigned promise that the submitted assignment is my own work. While I was
free to discuss ideas with others, the work contained is my own. I recognize that
should this not be the case; I will be subject to penalties as outlined in the course
syllabus.
Brandon Skeens
RedID: 826416091
*/

#include "PageTable.h"

//PageTable constructor
PageTable::PageTable(int argc, char* argv[]) {
    bool found = false;
    for (int i = 1; i < argc; ++i) {
        //For loop to go through every member of argv
        for (int i = 0; i < argc; ++i) {
            //If the bit levels have already been found, exit the loop
            if (found) {break;}
            

            //Convert argv[i] into a string for easier manipulation
            string arg = argv[i];
            
            //If the first character of argv[0] is a digit, you can begin checking
            if (isdigit(arg[0]) && i == 0) {
                //Set found to true
                found = true;
                //For each subsequent argument, check if it is also a digit
                for (int j = i; j < argc; ++j) {
                    arg = argv[j];
                    //If it is a digit, you can increment levels and add the digit to entrycount
                    if (isdigit(arg[0])) {
                        entryCount[levels] = stoi(argv[j]);
                        levels++;
                    } // Once finished, break out of for loop
                    else {break;}
                }
            } //If digit is found, but not at the first spot, handle it slightly different
            else if (isdigit(arg[0])) {
                //If the previous argument is an optional argument starting in '-', ignore it
                string prevArg = argv[i - 1];
                if (prevArg[0] != '-') {
                    //Set found to be true
                    found = true;
                    //Go through digits starting at current and increment levels for each digit
                    for (int j = i; j < argc; ++j) {
                        arg = argv[j];
                        if (isdigit(arg[0])) {
                            entryCount[levels] = stoi(argv[j]);
                            levels++;
                        } //Break once there are no more digits
                        else {break;}
                    }
                }
            }
        }
    }

    //Ensure bit values meet standards
    for (int i = 0; i < levels; ++i) {
        //If any member of entryCount is invalid, print and exit
        if (entryCount[i] < 1) {
            cout << "Level " << i << " page table must have at least 1 bit" << endl;
            exit(1);
        }
        //Decrement offset by the amount of bits entered
        offset -= entryCount[i];
        //If offset drops below 4, it means that all other bits added up to over 28, so exit
        if (offset < 4) {
            cout << "Too many bits used for the page table" << endl;
            exit(1);
        }
    }

    //temporary shift value to use for bitmask calculations
    int tempShift = 32;
    //calculate the bitmasks
    for (int i = 0; i < levels; ++i) {
        bitmask[i] = 0x00000000;
        for (int j = 0; j < entryCount[i]; ++j) {
            bitmask[i] <<= 1;
            bitmask[i] |= 1;
        }
        bitmask[i] <<= (tempShift -= entryCount[i]);
    }

    //bitshift calculations
    bitshift[0] = 32 - entryCount[0];
    for (int i = 1; i < levels; ++i) {bitshift[i] = bitshift[i - 1] - entryCount[i];}
    
    //Finally initialize the root
    root = new Level(0, this);
}

//Searches tree for the mapped pfn for the given VA
Map * PageTable::searchMappedPfn(unsigned int vAddr) {
    //Extract vpns to use as index
    uint32_t* vpns = extractVPNS(vAddr);
    //set curr to root to traverse tree
    Level* curr = root;

    //If tree is one level, simply look through the single level array at index calculated
    if (levels == 1) {
        unsigned int index = vpns[0];
        //if it exists, return it
        if (curr->singleLevelMaps != nullptr) {return curr->singleLevelMaps[index];} 
        //If not, return a nullptr
        else {return nullptr;}
    }

    //Iterate through the levels
    for (int i = 0; i < levels; ++i) {
        unsigned int index = vpns[i];
        if (curr->nextLevel[index] != nullptr) {curr = curr->nextLevel[index];}
        else {return nullptr;}
    }
    //Return the current leaf node's map
    if (curr->map != nullptr) {return curr->map;}
    return nullptr;
}

//Inserts the mapped pfn for given VA
void PageTable::insertMapForVpn2Pfn(unsigned int vAddr) {
    uint32_t* vpns = extractVPNS(vAddr);
    Level* curr = root;

    //If there is no page replacement
    if (!pageReplace) {
        for (int i = 0; i < levels; ++i) {
            //Initialize and calculate index
            unsigned int index = vpns[i];

            //If tree is single level, try to insert into singlelevelmaps
            if (levels == 1) {
                //If there is no existing map at the index, initialize the map with frame and valid bit
                if (root->singleLevelMaps[index] == nullptr) {
                    root->singleLevelMaps[index] = new Map(frame, frame != -1);
                    //Increment frame number and return
                    frame++;
                    return;
                } //If it already exists, there is no need to do anything, and you can just return
                else {return;}
            } //If the tree is multi level
            else {
                //If next level doesn't exist, instantiate and fill with nullptr
                if (curr->nextLevel == nullptr) {
                    curr->nextLevel = new Level*[1 << entryCount[i]];
                    for (int j = 0; j < (1 << entryCount[i]); ++j) {curr->nextLevel[j] = nullptr;}
                    //If new memory is allocated, update the memoryCount with the size of newly allocated level array
                    memoryUsed += entryCount[i] * (1 << entryCount[i]);
                }
                //if nextlevel[index] doesn't exist, instantiate
                if (curr->nextLevel[index] == nullptr) {
                    curr->nextLevel[index] = new Level(curr->depth + 1, this);
                }
                //Continue down tree
                curr = curr->nextLevel[index];
            }
        }

        //If the map does not exist, map it and increment the frame
        if (curr->map == nullptr) {
            curr->map = new Map(frame, frame != -1);
            curr->map->vAddr = vAddr;
            frame++;
        }
    }
    //If page replacement is enabled
    else {
        if (bitStringCounter == bitStringUpdate) {
            serviceBitstrings(root, referenced);
            referenced = 0;
        }
        //If the address being searched is mapped, there is no need to do anything
        if (searchMappedPfn(vAddr) != nullptr) {
            bitStringCounter++;
            return;
        }
        if (frame < availableFrames) {
            for (int i = 0; i < levels; ++i) {
                //Initialize and calculate index
                unsigned int index = vpns[i];

                //If tree is single level, try to insert into singlelevelmaps
                if (levels == 1) {
                    //If there is no existing map at the index, initialize the map with frame and valid bit
                    if (root->singleLevelMaps[index] == nullptr) {
                        root->singleLevelMaps[index] = new Map(frame, frame != -1);
                        //Increment frame number and return
                        frame++;
                        return;
                    } //If it already exists, there is no need to do anything, and you can just return
                    else {return;}
                } //If the tree is multi level
                else {
                    //If next level doesn't exist, instantiate and fill with nullptr
                    if (curr->nextLevel == nullptr) {
                        curr->nextLevel = new Level*[1 << entryCount[i]];
                        for (int j = 0; j < (1 << entryCount[i]); ++j) {curr->nextLevel[j] = nullptr;}
                        //If new memory is allocated, update the memoryCount with the size of newly allocated level array
                        memoryUsed += entryCount[i] * (1 << entryCount[i]);
                    }
                    //if nextlevel[index] doesn't exist, instantiate
                    if (curr->nextLevel[index] == nullptr) {
                        curr->nextLevel[index] = new Level(curr->depth + 1, this);
                    }
                    //Continue down tree
                    curr = curr->nextLevel[index];
                }
            }

            //If the map does not exist, map it and increment the frame
            if (curr->map == nullptr) {
                curr->map = new Map(frame, frame != -1);
                frame++;
                //Right shift to add recentUse reference
                curr->map->recentUse >> 1;
                //using an 8-bit counter, so left shift 1 by 7 spaces to add reference
                curr->map->recentUse |= (1 << 31);
            }
            referenced |= (1 << frame - 1);
        } 
        //If the bounds go outside of the available frame
        else {
            for (int i = 0; i < levels; ++i) {
                //Initialize and calculate index
                unsigned int index = vpns[i];

                //If tree is single level, try to insert into singlelevelmaps
                if (levels == 1) {
                    
                } //If the tree is multi level
                else {
                    //If next level doesn't exist, instantiate and fill with nullptr
                    if (curr->nextLevel == nullptr) {
                        curr->nextLevel = new Level*[1 << entryCount[i]];
                        for (int j = 0; j < (1 << entryCount[i]); ++j) {curr->nextLevel[j] = nullptr;}
                        //If new memory is allocated, update the memoryCount with the size of newly allocated level array
                        memoryUsed += entryCount[i] * (1 << entryCount[i]);
                    }
                    //if nextlevel[index] doesn't exist, instantiate
                    if (curr->nextLevel[index] == nullptr) {
                        curr->nextLevel[index] = new Level(curr->depth + 1, this);
                    }
                    //Continue down tree
                    curr = curr->nextLevel[index];
                }
            }
            //Find victim map
            Map* victim = frameToBeReplaced(root);
            victimAddr = victim->vAddr;
            //If the current node is unmapped
            if (curr->map == nullptr) {
                //Set current node's map to the victim
                curr->map = victim;
                //Update recently used string
                curr->map->recentUse = victim->recentUse >> 1;
                curr->map->recentUse |= (1 << 31);
                //Set the map's vaddr to current node's vaddr
                curr->map->vAddr = vAddr;
                replaces++;
            }
        }
        //Increment bistringCounter for accessed address
        bitStringCounter++;
    }
}

//Helper function to extract VPN(s) for the given VA
uint32_t* PageTable::extractVPNS(unsigned int vAddr) {
    //initialize return array of vpns, set the first value to be the address shifted
    uint32_t* vpns = new uint32_t(levels);
    vpns[0] = vAddr >> bitshift[0];

    //For remaining addresses, shift by as many bits and then use modulo to extract last n appropriate bits
    for (int i = 1; i < levels; ++i) {
        vpns[i] = (vAddr >> bitshift[i]) % static_cast<int>(pow(2, entryCount[i]));
    }
    return vpns;
}

//Helper function to service bitstrings after interval
void PageTable::serviceBitstrings(Level* node, unsigned long long reference) {
    //Check if node is valid
    if (node == nullptr) {return;}
    //If a leaf node, access map and modify recentUse bitstring
    if (node->leaf) {
        cout << "leaf" << endl;
        //If the map is not null, modify the recent use values
        if (node->map != nullptr) {
            //Right shift one bit
            node->map->recentUse >>= 1;
            //Prepend with a bit
            if ((reference >> node->map->pfn) % 2 == 1) {
                node->map->recentUse |= (1 << 31);
            }
        } 
    } //If not a leaf node, make a recursive call to the service bitstrings function until leaf node is reached
    else {
        if (node->nextLevel != NULL) {
            for (unsigned int i = 0; i < (1 << entryCount[node->depth + 1]); ++i) {
                if (node->nextLevel[i] != NULL) {
                    cout << "Recursive call" << endl;
                    serviceBitstrings(node->nextLevel[i], reference);
                }
            }
        }
    }
}

//Function recursively traverses tree and finds the least recently accesses map
Map * PageTable::frameToBeReplaced(Level* node) {
    Map * leastRecent = nullptr;
    //If the node is not a leaf
    if (!node->leaf) {
        //Traverse through all of node's children recursively
        for (int i = 0; i < (1 << entryCount[node->depth + 1]); ++i) {
            //If the child exists
            if (node->nextLevel[i] != nullptr) {
                //Recursive call
                Map* candidate = frameToBeReplaced(node->nextLevel[i]);
                //If candidate was used less recently (or is the first candidate), set to be the victim
                if (leastRecent == nullptr || (candidate != nullptr && candidate->recentUse < leastRecent->recentUse)) {
                    leastRecent = candidate;
                }
            }
        }
    }// If the node is a leaf
    else {
        //If the node is mapped
        if (node->map != nullptr) {
            //If there is no set least recent, or if recentUse is lower, set to be victim
            if (leastRecent == nullptr || node->map->recentUse < leastRecent->recentUse) {
                leastRecent = node->map;
            }
        }//If no map, just return nullptr
        else {return nullptr;}
    }
    return leastRecent;
}

//Map constructor
Map::Map(uint32_t pfn, bool valid) {
    this->pfn = pfn;
    this->valid = valid;
}

//Level constructor
Level::Level(int depth, PageTable* table) {
    pageTable = table;
    this->depth = depth;
    //If the depth matches the lowest level index, set leaf to true and initialize an empty map
    if (depth == (table->levels - 1)) {
        map = new Map(-1, false);
        leaf = true;
    }
    nextLevel = nullptr;
    map = nullptr;
}