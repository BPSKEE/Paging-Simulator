/*
I the undersigned promise that the submitted assignment is my own work. While I was
free to discuss ideas with others, the work contained is my own. I recognize that
should this not be the case; I will be subject to penalties as outlined in the course
syllabus.
Brandon Skeens
RedID: 826416091
*/
#include <vector>
#include <cmath>
#include <cstring>
#include <inttypes.h>
#include <vector>
#include "PageTable.h"

using namespace std;

//Extracts the virtual page number from the given virtual address
unsigned int extractVPNFromVirtualAddress(unsigned int virtualAddress, unsigned int mask, unsigned int shift); 

//Helper function takes input of virtual address and number of bits in offset, then prints out the calculated offset
unsigned int offsetCalc(unsigned int vAddr, unsigned int offsetBits);

//Helper to convert virtual address to physical address
void va2pa(PageTable *pt, unsigned int vAddr);
