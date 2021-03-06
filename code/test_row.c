#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

uint32_t* lookup(uint32_t** table, char c)
{
    if(c == 'a')return table[0];
    if(c == 'n')return table[1];
    if(c == 'u')return table[2];
    if(c == 'l')return table[3];
    else return NULL;
}

void match(const char* text, const char* pattern, int k, int* matched, int lc)
{
    int m = strlen(pattern);
    int n = strlen(text);
    uint32_t* B[4];
    uint32_t* BB[4];
    int i,w;
    //unsigned int lc = ((((m-k)*(k+2)) > (31))?(31/(k+2)):(m-k));
    //lc = 2;
    unsigned int J =((m-k)/lc) + (((m-k)%lc)  == 0?0:1);
    //printf("# of diagonals packed into a word: %d, number of automata/words: %d , m %d , k %d \n", lc,J,m,k);
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
                    if(pattern[i + w] == 'a')B[0][iter] &= mask;
                    if(pattern[i + w] == 'n')B[1][iter] &= mask;
                    if(pattern[i + w] == 'u')B[2][iter] &= mask;
                    if(pattern[i + w] == 'l')B[3][iter] &= mask;
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
    for(i = 0;i < 4;i++) {
        for(w = 0; w < J;w++) {
            //printf("%x,\t", B[i][w]);
        }
        //printf("\n");
    }

    mask = (1<<(k+1)) - 1;
    for(i = 0; i < 4; i++)
    {
        int j;
        int shift;
        BB[i] = (uint32_t*)malloc(J*sizeof(uint32_t));
        memset(BB[i], 0, J*sizeof(uint32_t));
        for(w = 0;w < J;w++){
            for(j = 0;j < lc;j++) {
                shift = (lc-j-1)*(k+2);
                BB[i][w] |= ((((B[i][w]>>j) & mask)) << shift);
            }
        //printf("BB[%d][%d] = %x\t", i, w, BB[i][w]);
        }
        //printf("\n");
    }


    uint32_t* D = malloc((J+2)*4);
    D[0] = 0;
    for(w=1; w < J + 1; w++)
    {
        D[w] = (1<<(k+1)) -1;
        for(i = 0; i < lc - 1; i++)
        {
            D[w] = (D[w] << k+2) + (1<<(k+1)) -1;
        }
    }
    D[J+1] = (1<<((k+2)*lc)) - 1;
//    for(w=1; w < J + 2; w++) printf("D[%d] intitialized to %x\n", w, D[w]);
    uint32_t* Dnew = malloc((J+2)*4);
    memcpy(Dnew, D, (J+2)*4);
    uint32_t initial = D[1];
    uint32_t x;
    mask = 1;
    for(i = 0; i < lc - 1; i++)mask = (mask<<(k+2)) + 1;
//    printf("mask: %x\n", mask);
    for(i = 0; i < n; i++)
    {
        //printf("Char read: %c, text pos: %d\n\n",  text[i], i);
        for(w=1;w<=J;w++)
        {
            x =  ((D[w] >> (k+2)) | (D[w - 1] << ((k + 2)* (lc - 1))) | ((lookup(BB, text[i]))[w-1])) & ((1 << (k + 2)* lc) - 1);
            //printf("lookup %x, D[%d]>>%d=%x, D[w - 1] << %d=%x, x:%x\n", (lookup(BB, text[i]))[w-1], w, k+2, D[w]>>(k+2), ((k + 2)* (lc - 1)), (D[w - 1] << ((k + 2)* (lc - 1))), x);
            // printf("Substitution: %x, insertion: %x, matches: %x, initial: %x\n", ((D[w]<<1) | mask), (((D[w] << k+3) | mask| D[w +1] >>((k+2)*(lc - 1) - 1))), (((x + mask) ^ x) >> 1), initial);
            Dnew[w] = ((D[w]<<1) | mask)
                    & (((D[w] << k+3) | mask|((D[w +1] >>((k+2)*(lc - 1)))<<1)))
                    & (((x + mask) ^ x) >> 1)
                    & initial;
            //printf("Dnew[%d] : %x\t", w, Dnew[w]);
            int c;
            x = Dnew[w];
            for(c = 0;c < lc;c++) {
                uint32_t diag = x&((1<<(k+2)) - 1);
                //printf("diagonal %d = log %d\t", w*lc - c, diag + 1);
                x = x>>(k+2);
            }
            //printf("\n");
        }
        memcpy(D, Dnew, (J+2)*4);
        if(!(D[J] & 1<<(k + (k + 2)*(lc*J -m + k ))))
        {
            //printf("Last nfa:%x, match upon anding with %x \n", D[J], 1<<(k + (k + 2)*(lc*J -m + k )));
            matched[i] = 1;
            //printf("prepared mask of %x: \n", (1<<(k + 1 + (k + 2)*(lc*J -m + k ))) - 1);
            D[J] |= ((1<<(k + 1 + (k + 2)*(lc*J -m + k ))) - 1);
            //printf("After match: D[%d] set to %x\n\n", J, D[J]);
        }
    }

    for(i =0; i < 4; i++)free(B[i]);
    for(i =0; i < 4; i++)free(BB[i]);
    free(D);
    free(Dnew);
}

int main()
{
    const char pattern[] = "annualununannualununannualununan";
    const char text[] = "annualununannualununannualununanllllllannnualununannualununannualununanlllllannualununaaaannualununannualununanllllannualannualununannualununanllllannualunluannuaalunuaannualununan";

    //FILE* fp1 = fopen("dna.txt", "r");
    //FILE* fp2 = fopen("patterns.txt", "r");
    //char text[1000];
    //char pattern[105];
    int pos = 0, k, i;
/*
    while(i != EOF) {
        i = fgetc(fp1);
        text[pos] = (i == EOF)?'\0':(char)i;
        pos++;
    }
*/
    //printf("Text %s", text);
    int* matched = (int*)malloc(sizeof(int)* strlen(text));
    int* matched1 = (int*)malloc(sizeof(int)* strlen(text));
    memset(matched, 0, 4*strlen(text));
    memset(matched1, 0, 4* strlen(text));
/*
    while(fgets(pattern, 104, fp2)!= NULL) {
*/
        for(k = 0;k < 6;k++) {
            unsigned int lc;
            match(text,pattern,k,matched, 1);
            for(lc = 2; lc <= 31/(k+2);lc++) {
                match(text, pattern, k, matched1, lc);
                for(i=0; i<strlen(text); i++) {
                    if(matched[i] != matched1[i]) printf("Error for pattern %s, k=%d, lc = %d, positon %d\n", pattern, k,lc, i);
                }
            }
            printf("For k = %d, matches at:\n", k);
            for(i=0; i<strlen(text); i++) {
                if(matched[i] == 1)printf("%d, ", i);
                //printf("i %d m %d m1 %d ", i, matched[i], matched1[i]);
            }
            printf("\n");
        }
 /*
    }
*/
    free(matched);
    free(matched1);
    //fclose(fp1);
    //fclose(fp2);
    return 0;
}
