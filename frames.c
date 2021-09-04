#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define MAX_LINE_LENGTH 16
#define VPN_MASK 0xfffff000UL 
#define OFFSET_MASK 0x00000fffUL
#define SHIFT 12
#define HASH_SIZE 1050000


struct Page_entry{
  int VPN;
  int valid_bit : 1;
  int protect_bit;
  int dirty_bit;
  int use_bit;
  int time_stamp;
} ;

struct queries{
	int num_accesses;
	int num_misses;
	int num_writes;
	int num_drops;
};

struct hash_entry{
	int curr_pointer;
	int count;
	int* occurences;
};

struct queries GlobalCounts;
struct Page_entry PAGE_TABLE[1001]; // the size of the page table is the number of the available frames in the system.
struct hash_entry HASH_MAP[HASH_SIZE];
int FRAMES = 0;
int CountPageFaults = 0;
int time = 0;
int CounterClock = 0;
char replaced_page_name[7];
int VerboseBool = 0;
int line_num=0;


long long DecToBin(int n) {
	long long bin = 0;
	int rem, i = 1, step = 1;
	while (n != 0) {
		rem = n % 2;
		n /= 2;
		bin += rem * i;
		i *= 10;
	}
	return bin;
}


char* split(char* input, int type) {
    char* output;
    output = (char*)malloc(MAX_LINE_LENGTH*sizeof(char));
    int i;
    for(i=0;input[i]==' ';i++);
    for(i=i;input[i]!= ' ';i++) output[i] = input[i];
    output[i] = '\0';
    if(type) {
        for(i=i;input[i]==' ';i++); 
        output[0] = input[i];  output[1] = '\0';
    }
    return output;
}


void PrintGlobalAccesses(){
	printf("Number of memory accesses: %d\nNumber of misses: %d\nNumber of writes: %d\nNumber of drops: %d\n", GlobalCounts.num_accesses, GlobalCounts.num_misses, GlobalCounts.num_writes, GlobalCounts.num_drops);	
}

void FreeList(){
	for(int i=0;i<FRAMES;i++) 	printf("VPN: %d PFN:%d Time Stamp:%d Use bit: %d Dirty bit: %d\n", PAGE_TABLE[i].VPN, i, PAGE_TABLE[i].time_stamp, PAGE_TABLE[i].use_bit, PAGE_TABLE[i].dirty_bit);
	PrintGlobalAccesses();
}


void PrintVerbose(long long int decimalnum, int index){
	long long int quotient, remainder;
	int i, j = 0;
	char hexadecimalnum[100];
	quotient = decimalnum;
	while (quotient != 0) {
		remainder = quotient % 16;
		if (remainder < 10)
			hexadecimalnum[j++] = 48 + remainder;
		else
			hexadecimalnum[j++] = 87 + remainder;
		quotient = quotient / 16;
	}

	
	
	printf("Page %s was read from disk, page 0x", replaced_page_name);
	int count = j;
	while(count<5){
		printf("0");
		count++;
	}
	
	// Print Hexadecimal string
	for (i = j-1; i >= 0; i--) printf("%c", hexadecimalnum[i]);
	if(PAGE_TABLE[index].dirty_bit) printf(" was written to the disk.\n");
	else printf(" was dropped (it was not dirty).\n");
}



long long int HexadecimalToDecimal(char* hexVal)
	{
		int len = strlen(hexVal);
		int base = 1;
		int dec_val = 0;
		for (int i = len - 1; i >= 0; i--) {
			if (hexVal[i] >= '0' && hexVal[i] <= '9') {
				dec_val += (hexVal[i] - 48) * base; 
				base = base * 16;
			}
 
			else if (hexVal[i] >= 'A' && hexVal[i] <= 'F') {
				dec_val += (hexVal[i] - 55) * base; 
				base = base * 16;
			}

			else if (hexVal[i] >= 'a' && hexVal[i] <= 'f') {
				dec_val += (hexVal[i] - 87) * base; 
				base = base * 16;
			}

		}
		return dec_val;
	}


void PrintHashMaps(){
	for(int i=0;i<HASH_SIZE;i++){
		if(HASH_MAP[i].count){
			printf("Index: %d, Count: %d, Occurences: ", i, HASH_MAP[i].count);
			for(int j=0;j<HASH_MAP[i].count;j++) printf("%d ", HASH_MAP[i].occurences[j]);
			printf("\n");
		}
	}
	
}

int RandomReplacement(int page_num){
	int frame = rand()%FRAMES;
	if(VerboseBool) PrintVerbose(PAGE_TABLE[frame].VPN, frame);
	return frame;
}

int CLOCK(int page_num){
	int i;
	while(PAGE_TABLE[CounterClock].use_bit !=0) {
            PAGE_TABLE[CounterClock].use_bit = 0;
            CounterClock = (CounterClock + 1)%FRAMES;
    }

	if(VerboseBool) PrintVerbose(PAGE_TABLE[CounterClock].VPN, CounterClock);
	
	PAGE_TABLE[CounterClock].use_bit =  1;
	int ans = CounterClock;
	CounterClock = (CounterClock + 1)%FRAMES; 				
	return ans;
}


int GetMinimum(){
	int min_time = 10000000;
	int output;
	for(int i = 0; i < FRAMES; i++){
		if(PAGE_TABLE[i].time_stamp < min_time){
			min_time = PAGE_TABLE[i].time_stamp;
			output = i;
		}
	}
	return output;
}

int LRU_FIFO(int page_num){
	int replace_index = GetMinimum();
	if(VerboseBool) PrintVerbose(PAGE_TABLE[replace_index].VPN, replace_index);
	PAGE_TABLE[replace_index].time_stamp = time++;
	return replace_index;
    
}


int upper_bound(int arr[], int total_size, int search_key){
    int mid;
    int low = 0;
    int high = total_size;
    while (low < high) {
        mid = low + (high - low) / 2;
        if (search_key >= arr[mid]) low = mid + 1;
        else high = mid;
    }  
    return low;
}


int OPT(int page_num, int start_index, char** addresses, int line_count){
	int evicted_frame_index = 0;
	int maxi = -1;

	for(int i=0;i<FRAMES;i++){
		PAGE_TABLE[i].time_stamp = line_count+1;
	}

	for(int i=0;i<FRAMES;i++){
		int vpn = PAGE_TABLE[i].VPN;
		int j = upper_bound(HASH_MAP[vpn].occurences, HASH_MAP[vpn].count, start_index);
		if(j == HASH_MAP[vpn].count) PAGE_TABLE[i].time_stamp = line_count+1;
		else PAGE_TABLE[i].time_stamp = HASH_MAP[vpn].occurences[j];
	}

	for(int i=0;i<FRAMES;i++){
		int curr_time = PAGE_TABLE[i].time_stamp;
		if(curr_time>maxi){ // Return index with minimum frame number
			evicted_frame_index = i;
			maxi = curr_time;
		}
	}
	if(VerboseBool) PrintVerbose(PAGE_TABLE[evicted_frame_index].VPN, evicted_frame_index);
	return evicted_frame_index;
}


int ReplacementPolicy(int page_num, int strategy, int start_index, char** addresses, int line_count){
	int i = 0, flag=0;
	int output_frame;
	if(strategy == 0 ) output_frame = OPT(page_num, start_index, addresses, line_count);
	if(strategy==1 || strategy == 3) output_frame = LRU_FIFO(page_num);
	else if(strategy==4) output_frame = RandomReplacement(page_num);
	else if(strategy==2) output_frame = CLOCK(page_num);
		
	if(PAGE_TABLE[output_frame].dirty_bit) GlobalCounts.num_writes++;
	else GlobalCounts.num_drops++;                      
	PAGE_TABLE[output_frame].VPN =  page_num;
	PAGE_TABLE[output_frame].dirty_bit = 0;   // When a block is replaced, set dirty bit to 0 now !!!!
	return output_frame;

}


int translate_address(int VirtualAddress, int strategy_num, int start_index, char** addresses, int line_count, char* access_type){
	int VPN = (VirtualAddress & VPN_MASK) >> SHIFT;
	int offset = VirtualAddress & OFFSET_MASK;
	// printf("Original: %d, VPN: %d, Offset: %d\n", VirtualAddress, VPN, offset);
	int success = 0; int PFN; int protect_bit;
	int PhysAddr;
	int i = 0;
	   
	for(i=0;i<FRAMES ;i++){
		if(PAGE_TABLE[i].VPN == VPN){
			
			if(!strcmp(access_type, "W")) PAGE_TABLE[i].dirty_bit = 1;    
			// printf("Page Table Hit!\n");
			PAGE_TABLE[i].use_bit = 1;
			if(strategy_num!=1) PAGE_TABLE[i].time_stamp = time++;
			return PhysAddr = (i << SHIFT) | offset; 
		}
		else if(PAGE_TABLE[i].VPN == -1) break;
	}

	
	struct Page_entry page = PAGE_TABLE[i];
	if(i<FRAMES){
		PAGE_TABLE[i].VPN = VPN;
		PAGE_TABLE[i].use_bit = 1;
		PAGE_TABLE[i].time_stamp = time++;
		if(!strcmp(access_type, "W")) PAGE_TABLE[i].dirty_bit = 1; 
		GlobalCounts.num_misses++;               

		if(!page.valid_bit){
			perror("This translation is not valid!\n");
			exit(EXIT_FAILURE);
		}

		else if(page.protect_bit){
			perror("No rights to access the Page Entry!\n");
			exit(EXIT_FAILURE);
		}
		PhysAddr = (i << SHIFT) | offset;

	}
	
	else{
		GlobalCounts.num_misses++;  
		int frame_num = ReplacementPolicy(VPN, strategy_num, start_index, addresses, line_count);
		if(!strcmp(access_type, "W")) PAGE_TABLE[frame_num].dirty_bit = 1; 
		PhysAddr = (frame_num << SHIFT) | offset; 
	} 
	return PhysAddr;
	
}


void initialise(int line_count){
	// Initialise Inverted Page Table
	for(int i=0;i<FRAMES ;i++){
		PAGE_TABLE[i].VPN = -1;
		PAGE_TABLE[i].valid_bit = 1;
		PAGE_TABLE[i].protect_bit = 0;
		PAGE_TABLE[i].use_bit = 0;
		PAGE_TABLE[i].dirty_bit = 0;
		PAGE_TABLE[i].time_stamp = line_count+1;
	}


	// Initialise global variables
	GlobalCounts.num_accesses = 0;
	GlobalCounts.num_drops = 0;
	GlobalCounts.num_misses = 0;
	GlobalCounts.num_writes = 0;

	for(int i=0;i<HASH_SIZE;i++){
		HASH_MAP[i].count=0;
		HASH_MAP[i].curr_pointer = -1;
	}
}


int main(int argc, char *argv[] )  {  
  
/*Its a 8 bit hexadecimal system means 32 bit binary system.
Hence if page size if 4 KB, Number of pages = 2^20
Hence 20 bits for VPN and 12 bits for offset
*/
	// Perform basic error checking
	if (argc < 4 || argc > 5) { //  including the name of the program
		fprintf(stderr,"Usage: [input file] NumberFrames TypesOfStrategy \n");
		return 0;
	}
	char* file_name = argv[1];
	FRAMES = atoi(argv[2]); 
	char* strategy = argv[3];

	if(argc == 5){
		if(!strcmp(argv[4],"-verbose")) VerboseBool=1;
		else{
			fprintf(stderr,"Usage: -verbose and NOT %s\n", argv[4]);
			return 0;
		}
	}   

	// Set Random seed for RANDOM() Optimal policy
	srand(5635);
	int strategy_num;
	if(!strcmp(strategy,"OPT")) strategy_num = 0;
	else if(!strcmp(strategy,"FIFO")) strategy_num = 1;
	else if(!strcmp(strategy,"CLOCK")) strategy_num = 2;
	else if(!strcmp(strategy,"LRU")) strategy_num = 3;
	else if(!strcmp(strategy,"RANDOM")) strategy_num = 4;

	

	FILE * fp;
	fp = fopen(file_name, "r");

	if (fp == NULL) {
	  perror("Error while opening the file.\n");
	  exit(EXIT_FAILURE);
	}


	char line[MAX_LINE_LENGTH] = {0};
	int line_count = 0;


	// Read number of lines in Input
	while(fgets(line, MAX_LINE_LENGTH, fp))
		line_count ++;


	// Initialise everything
	initialise(line_count);								  

	char **addresses = (char **)malloc(line_count * sizeof(char *));
    for (int i=0; i<line_count; i++)
         addresses[i] = (char *)malloc(MAX_LINE_LENGTH * sizeof(char));

	fseek(fp, 0, SEEK_SET);
	int findex = 0;

	if(strategy_num == 0){
		char* init_add;
		int VPN;
		findex = 0;
		while(fgets(line, MAX_LINE_LENGTH, fp)){ 
			strcpy(addresses[findex], line); 
	        init_add = split(addresses[findex], 0);
	        VPN = (HexadecimalToDecimal(init_add) & VPN_MASK) >> SHIFT;
	        HASH_MAP[VPN].count++;
	        findex++;
	        
	    }

	    for(int i=0;i<HASH_SIZE;i++){							
	    	if(HASH_MAP[i].count){
	    		HASH_MAP[i].curr_pointer = 0;
	    		HASH_MAP[i].occurences = (int*)malloc(HASH_MAP[i].count * sizeof(int));
	    	}
	    }

	    fseek(fp, 0, SEEK_SET);
	    findex = 0;
	    while(fgets(line, MAX_LINE_LENGTH, fp)){
	        init_add = split(addresses[findex], 0);
	        VPN = (HexadecimalToDecimal(init_add) & VPN_MASK) >> SHIFT;
	        int temp = HASH_MAP[VPN].curr_pointer;
	        (HASH_MAP[VPN].occurences)[temp] = findex++;
	        HASH_MAP[VPN].curr_pointer ++;
	    }

	    for(int i=0;i<HASH_SIZE;i++){
	    	if(HASH_MAP[i].count)
	    		HASH_MAP[i].curr_pointer = 0;
	    }
	}

	else{
		findex = 0;
		while(fgets(line, MAX_LINE_LENGTH, fp)){ 
			strcpy(addresses[findex++], line); 
		}
	}

    findex = 0;
	fseek(fp, 0, SEEK_SET);
	GlobalCounts.num_accesses = line_count;   

	for (int i=0;i<line_count;i++) {
		line_num++;
		char* hex_address = split(addresses[i], 0);
		char* access_type = split(addresses[i], 1);
		strncpy(replaced_page_name, &hex_address[0], 7);
		int decimal_address = HexadecimalToDecimal(hex_address);
		int physical_address = translate_address(decimal_address, strategy_num, i, addresses, line_count, access_type);
	}
	PrintGlobalAccesses(); 
	if (fclose(fp)){ return EXIT_FAILURE; perror(file_name);}

	return 0;

}