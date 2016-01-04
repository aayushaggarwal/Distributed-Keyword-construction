#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>


int ReadWords(const char *filename, char *words1[])
{
    FILE *f = fopen(filename, "rt"); // checking for NULL is boring; i omit it
    int i=0,j;
    char temp1[100]; // assuming the words cannot be too long
    
    if (f == NULL)
    {
    printf("Error file missing\n");
    exit(-1);
    }

while(!feof(f))
{
       j=fscanf(f, "%s", temp1);
       words1[i] = strdup(temp1);
        i=i+1;
}
    fclose(f);

    // The result of this function is the number of words in the file
    return i;
}

int main(int argc, char **argv)
{
    char *words[4][100000];//assuming max words to be 1000000 in a file
    int i,j,k,n,q,df,p=0;
    int tid;
    long int total_words=0;   
    
    
FILE *fp = fopen("results.txt","w");
omp_set_num_threads(4);
#pragma omp parallel for reduction(+ : total_words)
for(j=0;j<argc-1;j++)
{ 
   tid = omp_get_thread_num();
   printf("Hello World from thread = %d\n", tid);
   printf("file %d",j);
    // Read the file; store all words in the list
    n = ReadWords(argv[j+1],words[j]);
    total_words += n;
    printf("\n n=%d ",n);
    
}


printf("total words%li %d",total_words,n);
for(j=0;j<argc-1;j++)
    {
       for(i=0;i<1000;i++)
       {
          fprintf(fp,"%s \n",words[j][i]);
       }
     
    }    

return 0;
fclose(fp);
}
