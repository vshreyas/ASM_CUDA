#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

uint32_t lookup(int* table, char c) {
    if(c == 'a')return table[0];
    if(c == 'n')return table[1];
    if(c == 'u')return table[2];
    if(c == 'l')return table[3];
    else return 0x7777;
}

void match(const char* text, const char* pattern, int k, int* matched) {

    int m = strlen(pattern);
    int n = strlen(text);
    uint32_t B[4];
    uint32_t BB[4];
        int i;
    for(i=0;i<4;i++)  {
        B[i] = (1<<m) - 1;
    }

    for(i = 0;i < m; i++) {
        uint32_t mask = ~(1 << i);
        if(pattern[i] == 'a') B[0] &= mask;
        if(pattern[i] == 'n') B[1] &= mask;
        if(pattern[i] == 'u') B[2] &= mask;
        if(pattern[i] == 'l') B[3] &= mask;
        //printf("B: %x %x %x %x\n", B[0], B[1], B[2], B[3]);
    }
    //printf("B: %x %x %x %x\n", B[0], B[1], B[2], B[3]);
    uint32_t mask = (1<<(k+1)) - 1;
    for(i=0;i<4;i++)  {
        BB[i] = 0;
        int j;
        int shift;
        for(j = 0;j < m - k;j++) {
            shift = (m-k-j-1)*(k+2);
            BB[i] |= (((B[i]>>j)& mask) << shift);
            //printf("chunk: %x, shifted by %d\n", (B[i]>>j)& mask, shift);
            //printf("iteration %d:  OR'ed with %x,\n BB is now %x\n", j,((B[i]>>j)& mask)<<(m-k-j-1)*(k+2), BB[i]);
        }
        //printf("BB[%d]: %x\n",i, BB[i]);
    }

    uint32_t D = (1<<(k+1)) -1;
    for(i = 0;i < m - k - 1;i++) {
        D = (D << k+2) + (1<<(k+1)) -1;
    }
    uint32_t initial = D;
    //printf("D intitialized to %x\n", D);
    //Searching
    uint32_t x;
    mask = 1;
    for(i = 0;i < m- k - 1;i++)mask = (mask<<(k+2)) + 1;
    //printf("mask: %x\n", mask);
    for(i = 0;i < n;i++) {
        x = (D >> k+2) | lookup(BB, text[i]);
        //printf("Char read: %c, lookup %x, D>>k+2 %x,x:%x\n", text[i], lookup(BB, text[i]), D>>k+2, x);
        //printf("Substitution: %x, insertion: %x, matches: %x, initial: %x ",  (D << 1) | mask, (D <<(k+3) | (mask<<(k+2) | ((1 << (k+1)) - 1))), (((x + mask)^x) >> 1), initial);
        D = ((D << 1) | mask)
            & (D <<(k+3) | (mask<<(k+2) | ((1 << (k+1)) - 1)))
            & (((x + mask)^x) >> 1)
            & initial;
        //printf("D: %x\n", D);
        if(!(D & 1<<k)) {
            matched[i] = 1;
            D |= (1<<(k+2) - 1);
        }
    }

}

int main() {
    const char pattern[] = "aaaaaaaan";
    const char text[] = "aaannlulaaanluuaalaunnaalnualllaallnllaaaalaaannlulaaanluuaalaunnaalnualllaallnllaaaalnnlaannlulaaanluuaalaunnaalnualllaallnllaaaalaaannlulaaanluuaalaunnaalnualllaallnllaaaal";
    int k = 2;
    int* matched = (int*)malloc(sizeof(int)* strlen(text));
    time_t t1 = time(NULL);
    int i;
    for(i=0;i<100000;i++) match(text, pattern, k, matched);
    time_t t2 = time(NULL);
    for(i=0;i<strlen(text);i++) {
        if(matched[i] == 1)printf("match at :%d\n", i);
    }
    free(matched);
    printf("Time taken: %ld", t2-t1);
    return 0;
}
