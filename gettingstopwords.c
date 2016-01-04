#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int GettingStopWords(char *stopwords[])
{
    FILE *f = fopen("stopwords.txt", "rt"); // checking for NULL is boring; i omit it
    int i=0,j;
    char temp[100]; // assuming the words cannot be too long
    
    if (f == NULL)
    {
    printf("Error file missing\n");
    exit(-1);
    }

while(!feof(f))
{
       j=fscanf(f, "%s", temp);
       stopwords[i] = strdup(temp);
        i=i+1;
}
    fclose(f);

    // The result of this function is the number of words in the file
    return i;
}

void main()
{

char *stopwords[5000];
int p,i;
FILE *fp = fopen("stopwords2.txt","w");
p=GettingStopWords(stopwords);
printf("p%d",p);
char * new_str ;
for (i=0;i<p;i++)
{/*
 if((new_str = malloc(strlen(str1)+strlen(str2)+1)) != NULL)
   {
    new_str[0] = '\0';   
    strcat(new_str,"Stopwords(";
    strcat(new_str,stopwords[i]);
    strcat(new_str,"\")");
   } 
   else
   {
    fprintf(STDERR,"malloc failed!\n");
    
   }*/
 fprintf(fp,"Stopwords.add($%s$) ",stopwords[i]);
} 
fclose(fp);
}
