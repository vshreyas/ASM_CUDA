#include <stdio.h>
#include <assert.h>
#include <cuda.h>
#define uint32_t unsigned int
#define MAX_THREADS 512
#define MAX_PATTERN_SIZE 1024
#define MAX_BLOCKS 8
#define MAX_STREAMS 16
#define TEXT_MAX_LENGTH 1000000000
void calculateBBArray(uint32_t** BB,const char* pattern_h,int m,int k , int lc , int J);

void checkCUDAError(const char *msg)
{
        cudaError_t err = cudaGetLastError();
        if( cudaSuccess != err) 
        {   
                fprintf(stderr, "Cuda error: %s: %s.\n", msg, 
                                cudaGetErrorString( err) );
                exit(EXIT_FAILURE);
        }    
}
char* getTextString()
{
   FILE *input, *output;
   char c;
   char * inputbuffer=(char *)malloc(sizeof(char)*TEXT_MAX_LENGTH);
   
   int numchars = 0, index  = 0;

   input = fopen("sequence.fasta", "r");
   c = fgetc(input);
   while(c != EOF)
   {
	inputbuffer[numchars] = c;
  	numchars++;
	c = fgetc(input);
   }
   fclose(input);
   inputbuffer[numchars] = '\0'; 
   return inputbuffer;
}


__global__ void match(uint32_t* BB_d,const char* text_d,int n, int m,int k,int J,int lc,int start_addr,int textBlockSize,int overlap ,int* matched)
{
	__shared__ int D[MAX_THREADS+2];
	__shared__ char Text_S[MAX_PATTERN_SIZE];
	__shared__ int DNew[MAX_THREADS+2];
	__shared__ int BB_S[4][MAX_THREADS];
     //   memset(matched, 0, n*sizeof(int));
	int w=threadIdx.x+1;

	for(int i=0;i<4;i++)
	{
		BB_S[i][threadIdx.x]= BB_d[i*J+threadIdx.x];
	}

	{
		D[threadIdx.x] = 0;
		{
			D[w] = (1<<(k+1)) -1;

			for(int i = 0; i < lc - 1; i++)
			{
				D[w] = (D[w] << k+2) + (1<<(k+1)) -1;
			}
		}
		D[J+1] = (1<<((k+2)*lc)) - 1;
	}
	int startblock=(blockIdx.x == 0?start_addr:(start_addr+(blockIdx.x * (textBlockSize-overlap))));
//	int startblock=start_addr;
 //       int endBlock = (((startblock + textBlockSize) > n )? ((startblock +(n- (startblock)))-1):((startblock + textBlockSize)-1));
        int size= (((startblock + textBlockSize) > n )? ((n- (startblock))):( textBlockSize));

	int copyBlock=(size/J)+ ((size%J)==0?0:1);
//	int text_start_pos=startblock+(threadIdx.x*copyBlock);
	if((threadIdx.x * copyBlock) <= size)
	memcpy(Text_S+(threadIdx.x*copyBlock),text_d+(startblock+threadIdx.x*copyBlock),(((((threadIdx.x*copyBlock))+copyBlock) > size)?(size-(threadIdx.x*copyBlock)):copyBlock));



   // for(w=1; w < J + 2; w++) printf("D[%d] intitialized to %x\n", w, D[w]);
 //   uint32_t* Dnew = malloc((J+2)*32);*
    memcpy(DNew, D, (J+2)*sizeof(int));
    __syncthreads();
    uint32_t initial = D[1];
    uint32_t x;
    uint32_t mask = 1;
    for(int i = 0; i < lc - 1; i++)mask = (mask<<(k+2)) + 1;
//    printf("mask: %x\n", mask);
    for(int i = 0; i < size;i++)
    {
  //      printf("Char read: %c\n\n",  text[i]);
  //      for(w=1;w<=J;w++)
        {
            x =  ((D[w] >> (k+2)) | (D[w - 1] << ((k + 2)* (lc - 1))) | (BB_S[(((int)Text_S[i])/2)%4][w-1])) & ((1 << (k + 2)* lc) - 1);
            DNew[w] = ((D[w]<<1) | mask)
	     	    & (((D[w] << k+3) | mask|((D[w +1] >>((k+2)*(lc - 1)))<<1)))
                    & (((x + mask) ^ x) >> 1)
                    & initial;
        }
	__syncthreads();
        memcpy(D, DNew, (J+2)*sizeof(int));
	if(!(D[J] & 1<<(k + (k + 2)*(lc*J -m + k ))))
	{
		matched[startblock+i] = 1;
		//     D[J] |= (1<<(k+2) - 1);
		D[J] |= ((1<<(k + 1 + (k + 2)*(lc*J -m + k ))) - 1);
	}
    }

}
int main(void)
{
	cudaEvent_t start, stop,stopAfterMatch;
	float time1,time2,time3;
	cudaEventCreate(&start);
	cudaEventCreate(&stop);
	cudaEventCreate(&stopAfterMatch);


	cudaEventRecord(start, 0);
	const char pattern_h[] = "TACACGAGGAGAGGAGAAGAACA";
	//const char pattern_h[] = "ACGACG";
	//const char text_h[] = "TTTACGGCG";
	//        const char text_h[] = "ACGATCGTAGCTAGTCGATGCTAGCTAGCTGATCGTACGTAGCTGTACGTAGCTATCGTAGCTACTGATCGTAGCTAGCTAGCGTAGTATATATTATACGTA";
	char * text_h=getTextString();
	cudaEventRecord(stop, 0);
	cudaEventSynchronize(stop);
	cudaEventElapsedTime(&time1, start, stop);
	
	int k = 13;
	int i;
	int count=0;
	char *pattern_d, *text_d;     // pointers to device memory
	char* text_new_d;
	int* matched_d;
	int* matched_new_d;
	uint32_t* BB_d;
	uint32_t* BB_new_d;
	int* matched_h = (int*)malloc(sizeof(int)* strlen(text_h));
	cudaMalloc((void **) &pattern_d, sizeof(char)*strlen(pattern_h)+1);
	cudaMalloc((void **) &text_d, sizeof(char)*strlen(text_h)+1);
	cudaMalloc((void **) &matched_d, sizeof(int)*strlen(text_h));
	cudaMemcpy(pattern_d, pattern_h, sizeof(char)*strlen(pattern_h)+1, cudaMemcpyHostToDevice);
	cudaMemcpy(text_d, text_h, sizeof(char)*strlen(text_h)+1, cudaMemcpyHostToDevice);
	cudaMemset(matched_d, 0,sizeof(int)*strlen(text_h));

	int m = strlen(pattern_h);
	int n = strlen(text_h);
	if(k>= m)
	{
		printf("Error: Distance must be less than m\n");
		exit(0);
	}
	char revpatt[m];
	int rev=0,rev_start_pos=0;
	for(rev=0;rev<strlen(pattern_h);rev++)
		revpatt[rev]=pattern_h[strlen(pattern_h)-1-rev];
	revpatt[rev]='\0';

	uint32_t* BB_h[4];
        unsigned int maxLc = ((((m-k)*(k+2)) > (31))?(31/(k+2)):(m-k));
	unsigned int lc=2;
	if(lc>maxLc)
	{
		printf("Error: Maximum Diagonals possible is %d but you entered %d\n",maxLc,lc);
		exit(0);
	}
	unsigned int noWordorNfa =((m-k)/lc) + (((m-k)%lc)  == 0?0:1);
	cudaMalloc((void **) &BB_d, sizeof(int)*noWordorNfa*4);
	printf("# of diagonals packed into a word: %d, number of automata/words: %d\n , m %d , k %d  n %d \n", lc,noWordorNfa,m,k,n);
	if(noWordorNfa >= MAX_THREADS)
	{
		printf("Error: max threads\n");
		exit(0);
	}

	calculateBBArray(BB_h,pattern_h,m,k,lc,noWordorNfa);

	for(i=0;i<4;i++)
	{
		cudaMemcpy(BB_d+ i*noWordorNfa, BB_h[i], sizeof(int)*noWordorNfa, cudaMemcpyHostToDevice);
	}
	int overlap=m;
	int textBlockSize=(((m+k+1)>n)?n:(m+k+1));
	cudaStream_t stream[MAX_STREAMS];
	for(i=0;i<MAX_STREAMS;i++)
		cudaStreamCreate( &stream[i] );
//	int maxNoBlocks=((1 + ((n-textBlockSize)/(textBlockSize-overlap)) + (((n-textBlockSize)%(textBlockSize-overlap)) == 0?0:1)));
	int start_addr=0,index=0,maxNoBlocks=0;
	if(textBlockSize>n)
	{
		maxNoBlocks=1;
	}
	else
	{
		 maxNoBlocks=((1 + ((n-textBlockSize)/(textBlockSize-overlap)) + (((n-textBlockSize)%(textBlockSize-overlap)) == 0?0:1)));
	}
	int kernelBlocks = ((maxNoBlocks > MAX_BLOCKS)?MAX_BLOCKS:maxNoBlocks);
 	int blocksRemaining =maxNoBlocks;
	printf(" maxNoBlocks %d kernel Blocks %d \n",maxNoBlocks,kernelBlocks);
	while(blocksRemaining >0)
	{
	kernelBlocks = ((blocksRemaining > MAX_BLOCKS)?MAX_BLOCKS:blocksRemaining);
	printf(" Calling %d Blocks with starting Address %d , textBlockSize %d \n",kernelBlocks,start_addr,textBlockSize);
	match<<<kernelBlocks,noWordorNfa,0,stream[(index++)%MAX_STREAMS]>>>(BB_d,text_d,n,m,k,noWordorNfa,lc,start_addr,textBlockSize,overlap,matched_d);
	start_addr+=kernelBlocks*(textBlockSize-overlap);;
	blocksRemaining -= kernelBlocks;
	}
	cudaMemcpy(matched_h, matched_d, sizeof(int)*strlen(text_h), cudaMemcpyDeviceToHost);
	checkCUDAError("Matched Function");
	for(i=0;i<MAX_STREAMS;i++)
		cudaStreamSynchronize( stream[i] );	
	for(int i=0; i<strlen(text_h); i++)
	{
		if(matched_h[i] == 1){

			rev_start_pos=((i-(m+k)<0)?0:(i-(m+k)));
			char revtext[i-rev_start_pos+1];
			//printf(" size %d %d", startpos, (i-startpos));
			for(rev=0;rev<(i-rev_start_pos);rev++)
				revtext[rev]=text_h[i-rev];
			revtext[rev]='\0';
			int* matched_new_h = (int*)malloc(sizeof(int)* (i-rev_start_pos));
			//matchFn(revtext,revpatt,k,matchednew);
			uint32_t* BB_new_h[4];
			calculateBBArray(BB_new_h,revpatt,m,k,lc,noWordorNfa);
			cudaMalloc((void **) &BB_new_d, sizeof(int)*noWordorNfa*4);
			for(int l=0;l<4;l++)
			{
				cudaMemcpy(BB_new_d+ l*noWordorNfa, BB_new_h[l], sizeof(int)*noWordorNfa, cudaMemcpyHostToDevice);
			}
			cudaMalloc((void **) &text_new_d, sizeof(char)*(i-rev_start_pos+1));
			cudaMalloc((void **) &matched_new_d, sizeof(int)*(i-rev_start_pos));
			cudaMemcpy(text_new_d, revtext, sizeof(char)*(i-rev_start_pos+1), cudaMemcpyHostToDevice);
			cudaMemset(matched_new_d, 0,sizeof(int)*(i-rev_start_pos));

			match<<<1,noWordorNfa>>>(BB_new_d,text_new_d,i-rev_start_pos,m,k,noWordorNfa,lc,0,i-rev_start_pos ,0,matched_new_d);
			cudaMemcpy(matched_new_h, matched_new_d, sizeof(int)*(i-rev_start_pos), cudaMemcpyDeviceToHost);
			for(rev=0;rev<(i-rev_start_pos);rev++){
				if(matched_new_h[rev]== 1)
				{
					// printf(" startpos values %d \n ",rev);
					break; }
			}
			count++;printf("match from : start %d to end %d\n", i-rev, i); }
	}   
	printf("Total matches is %d\n",count);
	cudaEventRecord(stopAfterMatch, 0);

	cudaEventSynchronize(stopAfterMatch);
	cudaEventElapsedTime(&time2, start, stopAfterMatch);
	cudaEventElapsedTime(&time3, stop, stopAfterMatch);
	printf ("Time for the kernel: %f ms for file Op and Total time taken is %f ms, Match time is %f ms  \n", time1,time2,time3);


	free(matched_h);
	cudaFree(pattern_d);cudaFree(text_d);cudaFree(matched_d);

}

void calculateBBArray(uint32_t** BB,const char* pattern,int m,int k , int lc,int J)
{

	uint32_t* B[4];
	int i,w;
	uint32_t mask;
	  
	for(i=0; i<4; i++)
	{
		B[i] = (uint32_t*)malloc(J*sizeof(uint32_t));
		for(w = 0; w < J;w++) {
			B[i][w] = (1<<(k+lc)) - 1;
		}
	}

	i = 0;
	int iter = 0;
	while(i < m)
	{
		for(w = 0;w < k + lc;w++) {
			if(iter < J) {
				if(i + w < m) {
					 mask = ~(1 << w);
					if(pattern[i + w] == 'A')B[0][iter] &= mask;
					if(pattern[i + w] == 'C')B[1][iter] &= mask;
					if(pattern[i + w] == 'T')B[2][iter] &= mask;
					if(pattern[i + w] == 'G')B[3][iter] &= mask;
				}
				else {
					int cnt;
					for(cnt = 0; cnt < 4;cnt++) B[cnt][iter] &= ~(1 << w);
				}
			}
		}
		i += w - k;
		++iter;
	}
/*	for(i = 0;i < 4;i++) {
		//printf("C[%d] is now : ", i);
		for(int w = 0; w < J;w++) {
			printf("%x,\t", B[i][w]);
		}
		printf("\n");
	}
*/

	mask = (1<<(k+1)) - 1;

	for(i = 0; i < 4; i++)
	{
		int j,w;
		int shift;
		BB[i] = (uint32_t* )malloc(J * sizeof(int));
		memset(BB[i], 0, J*sizeof(int));
		for(w =0; w<J; w++)
		{
			for(j = 0; j < lc; j++)
			{
				if((w*lc + j) >= (m-k)) continue;
				shift = (lc-j-1)*(k+2);
				BB[i][w] |= ((((B[i][w]>> j))& mask) << shift);
				//printf("chunk: %x, shifted by %d\n", (B[i]>>j)& mask, shift);
				//printf("iteration %d:  OR'ed with %x,\n BB is now %x\n", j,((B[i]>>j)& mask)<<(m-k-j-1)*(k+2), BB[i]);

			}
			//printf("BB[%d][%d]: %x \t",i, w, BB[i][w]);

		}
	//	printf("\n");
	}

}

