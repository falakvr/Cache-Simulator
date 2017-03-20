#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <cstring>
#include <iomanip>
#include <cstdlib>

#define debug 0

using namespace std;

//Global declarations
int SIZE = 32;

int L2_on = 1;

int L1_sets, L2_sets, BLOCKSIZE, L1_SIZE, L1_ASSOC, VC_NUM_BLOCKS, L2_SIZE, L2_ASSOC, L1_VC_writebacks, L2_writebacks;
string trace_file, trace_line;

int PC=0;

int swap_requests=0;
int swaps = 0;
int tmf =0;
float srr, MR_L1_VC, MR_L2;


int *vic_arr = (int *) malloc(sizeof(int)*SIZE);

//Class declarations
class Block
{
	public:
		int valid;
		int dirty;
		int tag;
		int blocksize;
		int counter;
		int fulladdress;

		Block()
		{
			counter=0;valid=0;dirty=0; tag=0; fulladdress = 0;
		}

		void setCounter(int counter1)
		{
			counter=counter1;
		}
		
};

class Set
{
	public:
		Block *blocks;
};


class Cache
{
	public:
		string write_policy;
		Set *sets;
		int cache_size;
		int tag_l;
		int index_l;
		int blockoffset_l;
		int write_misses;
		int read_misses;
		int reads;
		int writes;
		
		Cache()
		{
			read_misses=0;write_misses=0;reads=0;writes=0;
		}

};

//Function prototypes		

unsigned int HexToInt(char *inAddress);	
int *IntToBin(unsigned int address, int length);
unsigned int BinToInt(int *, int);
Cache L1_read (Cache L1, int *arr);
Cache L1_write (Cache L1, int *arr);
Cache L2_read (int *arr1);
Cache L2_write (int *arr2);

int compare (const void * a, const void * b)
{
  return ( ((Block*)a)->counter - ((Block*)b)->counter );
}

Cache L2;

int main(int argc, char* argv[])
{
	if(argc != 8)
	{
		printf("ERROR!!! Enter 7 arguments only\n");
		exit(0);
	}

	BLOCKSIZE = atoi(argv[1]);
	L1_SIZE = atoi(argv[2]);
	L1_ASSOC = atoi(argv[3]);
	VC_NUM_BLOCKS = atoi(argv[4]);
	L2_SIZE = atoi(argv[5]);
	L2_ASSOC = atoi(argv[6]);
	trace_file = argv[7];

	if (L2_SIZE == 0 ||  L2_ASSOC == 0 )
	{
		L2_on = 0; 
		L2_SIZE = 8192; 
		L2_ASSOC = 4;
	}

	L1_sets = L1_SIZE / (L1_ASSOC * BLOCKSIZE);
	
	L2_sets = L2_SIZE / (L2_ASSOC * BLOCKSIZE);	

	Cache L1;
	L1.sets = new Set[L1_sets];
	
	for (int i=0; i < L1_sets; i++)
	{
		L1.sets[i].blocks = new Block[L1_ASSOC];

		for(int j=0;j<L1_ASSOC;j++){
			L1.sets[i].blocks[j].setCounter(j);
		}
	}

	for (int k=0; k< L1_sets; k++){
		for (int m=0; m < L1_ASSOC; m++)
		{
			if (debug) cout << L1.sets[k].blocks[m].counter;
		}
	}		

	L1.index_l = log2(L1_sets);
	L1.blockoffset_l = log2(BLOCKSIZE);
	L1.tag_l = SIZE - L1.index_l - L1.blockoffset_l;

	L2.sets = new Set[L2_sets];

	for (int i=0; i < L2_sets; i++)
	{
		L2.sets[i].blocks = new Block[L2_ASSOC];

		for(int j=0;j<L2_ASSOC;j++){
			L2.sets[i].blocks[j].setCounter(j);
		}
	}

	for (int k=0; k< L2_sets; k++){
		for (int m=0; m < L2_ASSOC; m++)
		{
			if (debug) cout << L2.sets[k].blocks[m].counter;
			L2.sets[k].blocks[m].tag = 0;
		}
	}
		
	L2.index_l = log2(L2_sets);
	L2.blockoffset_l = log2(BLOCKSIZE);
	L2.tag_l = SIZE - L2.index_l - L2.blockoffset_l;

	ifstream myfile (trace_file.c_str());

	while (getline (myfile, trace_line))
	{
 	  	PC++;

 	  	if (debug) cout << "PC = "<<std::dec<<PC<<endl;
		std::string str0 = trace_line.substr(0,1);
		std::string str1 = trace_line.substr(2, trace_line.length());
	
		char * str2 = strdup(str1.c_str());

		unsigned int address = HexToInt(str2);

		if (debug) cout << "Hex to Integer address" << address << "\n";

		int *arr = (int *) malloc(sizeof(int)*SIZE);
		int *vic_arr = (int *) malloc(sizeof(int)*SIZE);
		
		arr = IntToBin(address, SIZE);


		if (str0.compare(0,1,"r") == 0)
		{
			L1 = L1_read(L1, arr);		
		}

		else if (str0.compare(0,1,"w") == 0)
		{
			L1 = L1_write(L1, arr);
		}
						
	}

	srr = ((float)(swap_requests) / (float)(L1.reads + L1.writes));

	// cout << endl << "L1 reads = " << L1.reads << endl << "L1 read misses = " << L1.read_misses << endl;
	// cout << "L1 writes = " << L1.writes << endl << "L1 write misses = " << L1.write_misses << endl << endl;
	// cout << "L2 reads = " << L2.reads << endl << "L2 read misses = " << L2.read_misses << endl;
	// cout << "L2 writes = " << L2.writes << endl << "L2 write misses = " << L2.write_misses << endl << endl;
	// cout << "L1/VC writebacks = " << L1_VC_writebacks << endl << endl;

	MR_L1_VC = ((float)(L1.read_misses + L1.write_misses - swaps) / (float)(L1.reads + L1.writes));

	MR_L2 = ((float)(L2.read_misses) / (float)(L2.reads));

	if (L2_on)
		tmf = L2.read_misses + L2.write_misses + L2_writebacks;
	else
		tmf = L1.read_misses + L1.write_misses - swaps + L1_VC_writebacks;
	
	for (int k=0; k < L1_sets; k++){
		qsort (L1.sets[k].blocks, L1_ASSOC, sizeof(Block), compare);
	}
		
	cout << "===== Simulator configuration =====" << endl;
	cout << "  BLOCKSIZE:                  " << BLOCKSIZE << endl;
	cout << "  L1_SIZE:                  " << L1_SIZE << endl;
	cout << "  L1_ASSOC:                    " << L1_ASSOC << endl;
	cout << "  VC_NUM_BLOCKS:               " << VC_NUM_BLOCKS << endl;

	if (L2_on)
		cout << "  L2_SIZE:                  " << L2_SIZE << endl;
	else
		cout << "  L2_SIZE:                  " << "0" << endl;

	if (L2_on)
		cout << "  L2_ASSOC:                    " << L2_ASSOC << endl;
	else
		cout << "  L2_ASSOC:                    " << "0" << endl;

	cout << "  trace_file:      " << trace_file << endl << endl;

	cout << "===== L1 contents =====" << endl;

	for (int k=0; k < L1_sets; k++)
	{
		cout << " set\t" << std::dec << k << ":\t\t\t" ;

		for (int m=0; m < L1_ASSOC; m++)
		{
			cout << std::hex << L1.sets[k].blocks[m].tag << "\t";

			if (L1.sets[k].blocks[m].dirty)
			{
				cout << "D\t";
			}
			else cout << " \t";
		}		
		cout << endl;	
	}

	if (L2_on)
	{
		cout << endl << "===== L2 contents =====" << endl;

 		for (int p=0; p < L2_sets; p++)
		{
			cout << " set  " << std::dec << p << ":	";
		
			for (int q=0; q < L2_ASSOC; q++)
			{
				cout << std::hex << L2.sets[p].blocks[q].tag << "  ";

				if (L2.sets[p].blocks[q].dirty)
					cout << "D 		";
			}
		cout << endl;
		}
	}

	cout << endl << "===== Simulation results =====" << endl;

	cout << "  a. number of L1 reads:                       " << std::dec << L1.reads << endl;

	cout << "  b. number of L1 read misses:                  " << std::dec << L1.read_misses << endl;

	cout << "  c. number of L1 writes:                      " << std::dec << L1.writes << endl;

	cout << "  d. number of L1 write misses:                 " << std::dec << L1.write_misses << endl;

	cout << "  e. number of swap requests:                      " << std::dec << swap_requests << endl;

	cout << "  f. swap request rate:                       "  << fixed << setprecision(4) << srr << endl;

	cout << "  g. number of swaps:                              " << std::dec << swaps << endl;

	cout << "  h. combined L1+VC miss rate:                " << MR_L1_VC << endl;

	cout << "  i. number writebacks from L1/VC:              " << std::dec << L1_VC_writebacks << endl;

	if (L2_on)
		cout << "  j. number of L2 reads:                           " << std::dec << L2.reads << endl;
	else
		cout << "  j. number of L2 reads:                           " << "0" << endl;

	if (L2_on)
		cout <<"  k. number of L2 read misses:                     "<< std::dec << L2.read_misses << endl;
	else
		cout << "  k. number of L2 read misses:                     " << "0" << endl;

	if (L2_on)
		cout << "  l. number of L2 writes:                          " << std::dec << L2.writes << endl;
	else
		cout << "  l. number of L2 writes:                          " << "0" << endl;

	if (L2_on)
		cout << "  m. number of L2 write misses:                    " << std::dec << L2.write_misses << endl;
	else
		cout << "  m. number of L2 write misses:                    " << "0" << endl;

	if (L2_on)
		cout << "  n. L2 miss rate:                            " << fixed << setprecision(4) << MR_L2 << endl;
	else
		cout << "  n. L2 miss rate:                            " << "0.0000" << endl;

	if (L2_on)
		cout << "  o. number of writebacks from L2:                 " << std::dec << L2_writebacks << endl;
	else
		cout << "  o. number of writebacks from L2:                 " << '0' << endl;
	
	cout << "  p. total memory traffic:                     " << std::dec << tmf << endl;

	return 0;
	getchar();
}

/***************************************************************************************************************/

unsigned int HexToInt(char *inAddress) 
{		
	return (unsigned int) strtol(inAddress, NULL, 16);
}

int * IntToBin(unsigned int address, int length = 32)
{

	int *arr = (int *) malloc(sizeof(int)*length);
 	int i=0,r, size=length,k;
 
  	i = size-1;

  	while(address != 0){
  		arr[i--] = address%2;
  		address /= 2;
  	}

  	for(;i>=0;i--)
  		arr[i] = 0;
	
	return arr;
}

unsigned int BinToInt(int *arr1, int length)
{
	unsigned int y =0;
	int i; 
	
	for (i=0; i<length; i++)
	{
		y += (unsigned int)(pow(2, length-i-1)*arr1[i]);
	}

	return y;
};

Cache L1_read (Cache L1, int *arr)
{
	int L1_TAG, L1_INDEX, L1_BLOCKOFFSET, L1_FULLADDRESS;

	int *arr1 = arr;

	L1_FULLADDRESS = BinToInt(arr, SIZE);
	L1_TAG = BinToInt(arr,L1.tag_l);
	L1_INDEX = BinToInt(arr+L1.tag_l,L1.index_l);
	L1_BLOCKOFFSET = BinToInt(arr+L1.tag_l+L1.index_l, L1.blockoffset_l);
	
	L1.reads++;
	
	int x;
	
	for (x=0; x < L1_ASSOC; x++)
	{
		if (L1.sets[L1_INDEX].blocks[x].valid == 1 && L1.sets[L1_INDEX].blocks[x].tag == L1_TAG)
		{

			for (int j=0; j < L1_ASSOC; j++) //LRU update
			{
				if (L1.sets[L1_INDEX].blocks[j].counter < L1.sets[L1_INDEX].blocks[x].counter)

					L1.sets[L1_INDEX].blocks[j].counter++;
			}

			L1.sets[L1_INDEX].blocks[x].counter = 0;

			return L1;	
		}
	}

	if (x == L1_ASSOC)
	{
		
		L1.read_misses++;
					
		for (int j=0; j < L1_ASSOC; j++)
		{
			if ((L1.sets[L1_INDEX].blocks[j].valid == 0) && (L1.sets[L1_INDEX].blocks[j].counter == L1_ASSOC - 1)) // If space available in cache
			{
				if (L2_ASSOC!=0)
					L2_read(arr1); // Read from L2

				L1.sets[L1_INDEX].blocks[j].valid = 1; // Update valid and tag
				L1.sets[L1_INDEX].blocks[j].fulladdress = L1_FULLADDRESS;
				L1.sets[L1_INDEX].blocks[j].tag = L1_TAG;

				for (int k=0; k < L1_ASSOC; k++) //LRU update
				{
					if (L1.sets[L1_INDEX].blocks[k].counter < L1.sets[L1_INDEX].blocks[j].counter)
						L1.sets[L1_INDEX].blocks[k].counter++;
				}

				L1.sets[L1_INDEX].blocks[j].counter = 0;

				L1.sets[L1_INDEX].blocks[j].dirty = 0;

				return L1;
			}	
		}	
		
		for (int j=0; j < L1_ASSOC; j++)
		{	
			if (L1.sets[L1_INDEX].blocks[j].counter == L1_ASSOC - 1) // If set is full
			{		
				if (L1.sets[L1_INDEX].blocks[j].dirty == 1) //evict if dirty
				{					
					vic_arr = IntToBin(L1.sets[L1_INDEX].blocks[j].fulladdress, SIZE);

					L2_write(vic_arr);// evict block from cache and write in L2

					L1_VC_writebacks++;
				}

				L2_read(arr1); //read block from L2
				
				L1.sets[L1_INDEX].blocks[j].fulladdress = L1_FULLADDRESS;
				L1.sets[L1_INDEX].blocks[j].tag = L1_TAG; //update Tag and valid
				L1.sets[L1_INDEX].blocks[j].valid = 1;

				for (int k=0; k < L1_ASSOC; k++) // LRU update
				{
					if (L1.sets[L1_INDEX].blocks[k].counter < L1.sets[L1_INDEX].blocks[j].counter)
					L1.sets[L1_INDEX].blocks[k].counter++;
				}

				L1.sets[L1_INDEX].blocks[j].counter = 0;

				L1.sets[L1_INDEX].blocks[j].dirty = 0;

				return L1;
			}
		}	
	}
	return L1;
};

Cache L1_write (Cache L1, int *arr)
{
	int L1_TAG, L1_INDEX, L1_BLOCKOFFSET, L1_FULLADDRESS;

	int *arr1 = arr;

	L1_FULLADDRESS = BinToInt(arr, SIZE);
	L1_TAG = BinToInt(arr,L1.tag_l);
	L1_INDEX = BinToInt(arr+L1.tag_l,L1.index_l);
	L1_BLOCKOFFSET = BinToInt(arr+L1.tag_l+L1.index_l, L1.blockoffset_l);
	
	L1.writes++;
	
	int y;
	
	for (y=0; y < L1_ASSOC; y++)
	{
		if ((L1.sets[L1_INDEX].blocks[y].valid == 1) && (L1.sets[L1_INDEX].blocks[y].tag == L1_TAG))
		{

			L1.sets[L1_INDEX].blocks[y].dirty = 1; //update dirty
			
			for (int j=0; j<L1_ASSOC; j++) //LRU update
			{
				if (L1.sets[L1_INDEX].blocks[j].counter < L1.sets[L1_INDEX].blocks[y].counter)
					L1.sets[L1_INDEX].blocks[j].counter++;
			}

			L1.sets[L1_INDEX].blocks[y].counter = 0;

			return L1;	
		}
	}

	if (y == L1_ASSOC)
	{
		
		L1.write_misses++;
				
					
		for (int j=0; j < L1_ASSOC; j++)
		{
			if ((L1.sets[L1_INDEX].blocks[j].valid == 0) && (L1.sets[L1_INDEX].blocks[j].counter == L1_ASSOC - 1))  //If space in cache
			{
				
				L2_read(arr1);

				L1.sets[L1_INDEX].blocks[j].valid = 1; //update valid and tag
				L1.sets[L1_INDEX].blocks[j].tag = L1_TAG;
				L1.sets[L1_INDEX].blocks[j].fulladdress = L1_FULLADDRESS;

				for (int k=0; k < L1_ASSOC; k++) //LRU update
				{
					if (L1.sets[L1_INDEX].blocks[k].counter < L1.sets[L1_INDEX].blocks[j].counter)
						L1.sets[L1_INDEX].blocks[k].counter++;
				}

				L1.sets[L1_INDEX].blocks[j].counter = 0;

				L1.sets[L1_INDEX].blocks[j].dirty = 1; //update dirty

				return L1;
			}	
		}	
		
		for (int j=0; j < L1_ASSOC; j++)
		{
			if (L1.sets[L1_INDEX].blocks[j].counter == L1_ASSOC - 1) //if cache is full
			{		

				if (L1.sets[L1_INDEX].blocks[j].dirty == 1)
				{
					vic_arr = IntToBin(L1.sets[L1_INDEX].blocks[j].fulladdress, SIZE);

					L2_write(vic_arr);// evict block from cache and write in L2

					L1_VC_writebacks++;
				}


				L2_read(arr1);
				
				L1.sets[L1_INDEX].blocks[j].fulladdress = L1_FULLADDRESS;
				L1.sets[L1_INDEX].blocks[j].tag = L1_TAG; //Update tag and valid bits
				L1.sets[L1_INDEX].blocks[j].valid = 1;

				for (int k=0; k < L1_ASSOC; k++) //LRU update
				{
					if (L1.sets[L1_INDEX].blocks[k].counter < L1.sets[L1_INDEX].blocks[j].counter)
					L1.sets[L1_INDEX].blocks[k].counter++;
				}

				L1.sets[L1_INDEX].blocks[j].counter = 0;

				L1.sets[L1_INDEX].blocks[j].dirty = 1; //update dirty

				return L1;
			}
		}	
	}
	return L1;
};

Cache L2_read(int *arr1)
{
	int L2_TAG, L2_INDEX, L2_BLOCKOFFSET, L2_FULLADDRESS;

	L2_FULLADDRESS = BinToInt(arr1,SIZE);
	L2_TAG = BinToInt(arr1,L2.tag_l);
	L2_INDEX = BinToInt(arr1+L2.tag_l,L2.index_l);
	L2_BLOCKOFFSET = BinToInt(arr1+L2.tag_l+L2.index_l, L2.blockoffset_l);

	L2.reads++;

	int w;
	
	for (w=0; w < L2_ASSOC; w++)
	{
		if ((L2.sets[L2_INDEX].blocks[w].valid == 1) && (L2.sets[L2_INDEX].blocks[w].tag == L2_TAG))
		{
			
			for (int j=0; j<L2_ASSOC; j++) //LRU update
			{
				if (L2.sets[L2_INDEX].blocks[j].counter < L2.sets[L2_INDEX].blocks[w].counter)
					L2.sets[L2_INDEX].blocks[j].counter++;
			}

			L2.sets[L2_INDEX].blocks[w].counter = 0;

			return L2;	
		}
	}

	if (w == L2_ASSOC)
	{
		
		L2.read_misses++;
				
		for (int j=0; j < L2_ASSOC; j++)
		{
			if ((L2.sets[L2_INDEX].blocks[j].valid == 0) && (L2.sets[L2_INDEX].blocks[j].counter == L2_ASSOC - 1)) //if space in cache
			{
				//read from memory

				L2.sets[L2_INDEX].blocks[j].valid = 1; //update valid and tag

				L2.sets[L2_INDEX].blocks[j].tag = L2_TAG;

				L2.sets[L2_INDEX].blocks[j].fulladdress = L2_FULLADDRESS;

				for (int k=0; k < L2_ASSOC; k++) //update LRU
				{
					if (L2.sets[L2_INDEX].blocks[k].counter < L2.sets[L2_INDEX].blocks[j].counter)
					L2.sets[L2_INDEX].blocks[k].counter++;
				}

				L2.sets[L2_INDEX].blocks[j].counter = 0;

				L2.sets[L2_INDEX].blocks[j].dirty = 0;

				return L2;
			}	
		}	
		
		for (int j=0; j < L2_ASSOC; j++)
		{	
			if (L2.sets[L2_INDEX].blocks[j].counter == L2_ASSOC - 1)
			{		
				//evict block from L2 and write in memory. We need not worry about writing in memory
				
				if (L2.sets[L2_INDEX].blocks[j].dirty == 1)
				L2_writebacks++;
				
				// Read from memory

				L2.sets[L2_INDEX].blocks[j].fulladdress = L2_FULLADDRESS;
				L2.sets[L2_INDEX].blocks[j].tag = L2_TAG; //update tag and valid bits
				L2.sets[L2_INDEX].blocks[j].valid = 1;

				for (int k=0; k < L2_ASSOC; k++) //lru update
				{
					if (L2.sets[L2_INDEX].blocks[k].counter < L2.sets[L2_INDEX].blocks[j].counter)
					L2.sets[L2_INDEX].blocks[k].counter++;

				}

				L2.sets[L2_INDEX].blocks[j].counter = 0;

				L2.sets[L2_INDEX].blocks[j].dirty = 0;

				return L2;
			}
		}	
	}
	return L2;
};

Cache L2_write (int *arr2)
{
	int L2_TAG, L2_INDEX, L2_BLOCKOFFSET, L2_FULLADDRESS;

	L2_FULLADDRESS = BinToInt(arr2,SIZE);
	L2_TAG = BinToInt(arr2,L2.tag_l);
	L2_INDEX = BinToInt(arr2+L2.tag_l,L2.index_l);
	L2_BLOCKOFFSET = BinToInt(arr2+L2.tag_l+L2.index_l, L2.blockoffset_l);
	
	L2.writes++;
	
	int y;
	
	for (y=0; y < L2_ASSOC; y++)
	{
		if ((L2.sets[L2_INDEX].blocks[y].valid == 1) && (L2.sets[L2_INDEX].blocks[y].tag == L2_TAG))
		{
					
			L2.sets[L2_INDEX].blocks[y].dirty = 1; //update dirty
			
			for (int j=0; j<L2_ASSOC; j++) //LRU update
			{
				if (L2.sets[L2_INDEX].blocks[j].counter < L2.sets[L2_INDEX].blocks[y].counter)
					L2.sets[L2_INDEX].blocks[j].counter++;
			}

			L2.sets[L2_INDEX].blocks[y].counter = 0;

			return L2;		
		}
	}

	if (y == L2_ASSOC)
	{
		
		L2.write_misses++;
					
		for (int j=0; j < L2_ASSOC; j++)
		{
			if ((L2.sets[L2_INDEX].blocks[j].valid == 0) && (L2.sets[L2_INDEX].blocks[j].counter == L2_ASSOC - 1))  //If space in cache
			{
				
				//read from memory

				L2.sets[L2_INDEX].blocks[j].valid = 1; //update valid and tag
				L2.sets[L2_INDEX].blocks[j].tag = L2_TAG;
				L2.sets[L2_INDEX].blocks[j].fulladdress = L2_FULLADDRESS;

				//L2.writes++;

				for (int k=0; k < L2_ASSOC; k++) //LRU update
				{
					if (L2.sets[L2_INDEX].blocks[k].counter < L2.sets[L2_INDEX].blocks[j].counter)
					L2.sets[L2_INDEX].blocks[k].counter++;
				}

				L2.sets[L2_INDEX].blocks[j].counter = 0;

				L2.sets[L2_INDEX].blocks[j].dirty = 1; //update dirty

				return L2;
			}	
		}	
		
		for (int j=0; j < L2_ASSOC; j++)
		{
			if (L2.sets[L2_INDEX].blocks[j].counter == L2_ASSOC - 1) //if cache is full
			{		
				//L2 write LRU block
				//L2_read(arr1);
				if (L2.sets[L2_INDEX].blocks[j].dirty == 1)
				L2_writebacks++;

				L2.sets[L2_INDEX].blocks[j].fulladdress = L2_FULLADDRESS;
				L2.sets[L2_INDEX].blocks[j].tag = L2_TAG; //Update tag and valid bits
				L2.sets[L2_INDEX].blocks[j].valid = 1;

				for (int k=0; k < L2_ASSOC; k++) //LRU update
				{
					if (L2.sets[L2_INDEX].blocks[k].counter < L2.sets[L2_INDEX].blocks[j].counter)
					L2.sets[L2_INDEX].blocks[k].counter++;
				}

				L2.sets[L2_INDEX].blocks[j].counter = 0;

				L2.sets[L2_INDEX].blocks[j].dirty = 1; //update dirty

				return L2;
			}
		}	
	}
	return L2;
};
