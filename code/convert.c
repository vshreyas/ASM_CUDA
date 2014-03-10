#include <stdio.h>
#include <stdlib.h>
int main()
{
    FILE* fin = fopen("fasta.txt", "r");
    if(fin == NULL)printf("ERROR");
    FILE* fout = fopen("twobit.out", "wb");
    int c = 0;
    char x;
    int i;
    while(i < 36)
    {
        x = 0;
        c = fgetc(fin);
        if(c != EOF)
        {
            if(c=='A')x=0;
            if(c=='C')x=1;
            if(c=='T')x=2;
            if(c=='G')x=3;
            x<<2;
            c = fgetc(fin);
            if(c!= EOF )
            {
                if(c=='A')x+=0;
                if(c=='C')x+=1;
                if(c=='T')x+=2;
                if(c=='G')x+=3;
                x<<2;
                c = fgetc(fin);
                if(c != EOF)
                {
                    if(c=='A')x+=0;
                    if(c=='C')x+=1;
                    if(c=='T')x+=2;
                    if(c=='G')x+=3;
                    x<<2;
                    c = fgetc(fin);
                    if(c != EOF)
                    {
                        if(c=='A')x+=0;
                        if(c=='C')x+=1;
                        if(c=='T')x+=2;
                        if(c=='G')x+=3;
                    }
                }
            }
            printf("%d", x);
            fwrite(&x, 1, 1, fout);
        }
        else break;
        i++;
    }
    fclose(fin);
    fclose(fout);
    return 0;
}
