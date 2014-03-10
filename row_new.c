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

void match(const char* text, const char* pattern, int k, int* matched)
{

    int m = strlen(pattern);
    int n = strlen(text);
    uint32_t B[4];
    uint32_t* BB[4];
    int i,w;
    for(i=0; i<4; i++)
    {
        B[i] = (1<<m) - 1;
    }

    for(i = 0; i < m; i++)
    {
        uint32_t mask = ~(1 << i);
        if(pattern[i] == 'a') B[0] &= mask;
        if(pattern[i] == 'n') B[1] &= mask;
        if(pattern[i] == 'u') B[2] &= mask;
        if(pattern[i] == 'l') B[3] &= mask;
        //printf("B: %x %x %x %x\n", B[0], B[1], B[2], B[3]);
    }
    printf("B: %x %x %x %x\n", B[0], B[1], B[2], B[3]);

    unsigned int lc = ((((m-k)*(k+2)) > (31))?(31/(k+2)):(m-k));
    lc = 2;
    unsigned int J =((m-k)/lc) + (((m-k)%lc)  == 0?0:1);
    printf("# of diagonals packed into a word: %d, number of automata/words: %d\n , m %d , k %d \n", lc,J,m,k);

    uint32_t mask = (1<<(k+1)) - 1;
    for(i = 0; i < 4; i++)
    {
        int j;
        int shift;
        BB[i] = (uint32_t* )malloc(J * 32);
        memset(BB[i], 0, J*32);
        for(w =0; w<J; w++)
        {
            for(j = 0; j < lc; j++)
            {
                shift = (lc-j-1)*(k+2);
                BB[i][w] |= (((B[i]>>(w*lc + j))& mask) << shift);
                //printf("chunk: %x, shifted by %d\n", (B[i]>>j)& mask, shift);
                //printf("iteration %d:  OR'ed with %x,\n BB is now %x\n", j,((B[i]>>j)& mask)<<(m-k-j-1)*(k+2), BB[i]);

            }
            printf("BB[%d][%d]: %x \t",i, w, BB[i][w]);
        }
        printf("\n");
    }
    uint32_t* D = malloc((J+2)*32);
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
    for(w=1; w < J + 2; w++) printf("D[%d] intitialized to %x\n", w, D[w]);
    uint32_t* Dnew = malloc((J+2)*32);
    memcpy(Dnew, D, J*32);
    uint32_t initial = D[1];
    uint32_t x;
    mask = 1;
    for(i = 0; i < lc - 1; i++)mask = (mask<<(k+2)) + 1;
    printf("mask: %x\n", mask);
    for(i = 0; i < n; i++)
    {
        printf("Char read: %c, text pos: %d\n\n",  text[i], i);
        for(w=1;w<=J;w++)
        {
            x =  ((D[w] >> (k+2)) | (D[w - 1] << ((k + 2)* (lc - 1))) | ((lookup(BB, text[i]))[w-1])) & ((1 << (k + 2)* lc) - 1);
            printf("lookup %x, D[%d]>>%d=%x, D[w - 1] << %d=%x, x:%x\n", (lookup(BB, text[i]))[w-1], w, k+2, D[w]>>(k+2), ((k + 2)* (lc - 1)), (D[w - 1] << ((k + 2)* (lc - 1))), x);
            printf("Substitution: %x, insertion: %x, matches: %x, initial: %x\n", ((D[w]<<1) | mask), (((D[w] << k+3) | mask| D[w +1] >>((k+2)*(lc - 1) - 1))),
                   (((x + mask) ^ x) >> 1), initial);
            Dnew[w] = ((D[w]<<1) | mask)
                    & (((D[w] << k+3) | mask|((D[w +1] >>((k+2)*(lc - 1)))<<1)))
                    & (((x + mask) ^ x) >> 1)
                    & initial;
            printf("Dnew[%d] : %x\t", w, Dnew[w]);
            int c;
            x = Dnew[w];
            for(c = 0;c < lc;c++) {
                uint32_t diag = x&((1<<(k+2)) - 1);
                printf("diagonal %d = log %d\t", w*lc - c, diag + 1);
                x = x>>(k+2);
            }
            printf("\n");
        }
        memcpy(D, Dnew, J*32);
        if(!(D[J] & 1<<(k + (k + 2)*(lc*J -m + k ))))
        {
            printf("Last nfa:%x, match upon anding with %x \n", D[J], 1<<(k + (k + 2)*(lc*J -m + k )));
            matched[i] = 1;
            printf("prepared mask of %x: \n", (1<<(k + 1 + (k + 2)*(lc*J -m + k ))) - 1);
            D[J] |= ((1<<(k + 1 + (k + 2)*(lc*J -m + k ))) - 1);
            printf("After match: D[%d] set to %x\n\n", J, D[J]);
        }
    }

    for(i =0; i < 4; i++) free(BB[i]);
    free(D);
    free(Dnew);
}

int main()
{
//    const char pattern[] = "AACAATACGATAAC";
//    const char text[] = "CAATACGACAATACGACAATACGACAATACGACAATACGACAATACGATCAATACGACAATACGACAATACGACAATACGACAATACGACAATACGACAATACGACAATACGACAATACGA";
    const char pattern[] = "annualunun";
    const char text[] = "annlulaununu";
    int k = 0;
    int* matched = (int*)malloc(sizeof(int)* strlen(text));
    time_t t1 = time(NULL);
    int i;
    match(text, pattern, k, matched);
    time_t t2 = time(NULL);
    for(i=0; i<strlen(text); i++)
    {
        if(matched[i] == 1)printf("match at :text[%d]\n", i);
    }
    free(matched);
    printf("Time taken: %ld", t2-t1);
    return 0;
}
