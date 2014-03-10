#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char charset[4] = {'A', 'C', 'T', 'G'};

char* GetRandomSubstring(char* s, int len, int sublen) {
    char* str = (char*)malloc(sublen);
    int start = rand()%(len-sublen);
    strncpy(str, s+start, sublen - 1);
    str[sublen - 1] = '\0';
    return str;
}

void RandSubs(char* s, int len) {
    int pos = rand()%(len-2);
    int x = rand()%4;
    s[pos] = (s[pos]== charset[x])?charset[(x + 1)%4]:charset[x];
}

char* RandIns(char* s, int len) {
    char* str = (char*) malloc(len);
    int pos = rand()%(len - 2);
    strncpy(str, s, pos);
    str[pos] = charset[rand()%4];
    strncpy(str+pos+1, s+pos, len - pos - 1);
    str[len - 1] = '\0';
    return str;
}

void RandDel(char* s, int len) {
    int pos = rand()%(len -1);
    while(pos < len - 1) {
        s[pos] = s[pos + 1];
        pos++;
    }
}

int main(int argc, char* argv[]) {
    if(argc < 1) return 0;
    FILE* fp = fopen(argv[1], "r");
    char* s = (char*)malloc(100000);
    srand((unsigned int)s);
    int i = 0,j=0,pos =0, e =0;
    while(i != EOF) {
        i = fgetc(fp);
        s[pos] = (i == EOF)?'\0':(char)i;
        pos++;
    }
    //printf("Original: %s\n, length %d", s, strlen(s));
    for(i=7;i<100;i++) {
        char* sub = GetRandomSubstring(s, strlen(s), i);
        //printf("%d th substr: %s\n", i, sub);
        for(e = 0;e<4*i/5;e++) {
            //for hamming this is enough
            for(j =0;j < e/3;j++) {RandSubs(sub, i); printf("%s\n", sub);}
            //comment out insertion & deletion if using hamming
            for(j =0;j < (e - e/3) - e/3;j++)  {
                char* t = RandIns(sub, i);
                printf("%s\n", t);
                free(t);
            }
            for(j =0;j < e/3;j++)  {RandDel(sub, i);printf("%s\n", sub);}
            //end of insertion and deletion
       }
       free(sub);
    }
    free(s);
    fclose(fp);
    return 0;
}
