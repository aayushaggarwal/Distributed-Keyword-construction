#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpi.h"
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <omp.h>
#include <math.h>
#include <time.h>



int numberOfFiles;
int number_of_words; 
char *words[50000];
char *stop_words[600];
int WMAX;
int FMAX;
DIR* FD;
struct dirent* in_file;
FILE    *common_file;
FILE    *entry_file;
char *file_name;
double startTime, endTime;
char *words1[25][50000];
int n,p1[25];
long int total_words;
char *search_words[50000];
char *file_name2[25];//implicitly defining max number of files=25
    int f=0;

int filesInCorpus(){
    int len;
    struct dirent *pDirent;
    DIR *pDir;
    numberOfFiles =0;
    pDir = opendir("bible");
    if (pDir != NULL) {
      while ((pDirent = readdir(pDir)) != NULL) {
        len = strlen (pDirent->d_name);
        if (strcmp (".txt", &(pDirent->d_name[len - 4])) == 0)
              numberOfFiles++;       
        }
    closedir (pDir);
    }
    return numberOfFiles;
}

int Search_in_File(char *fname, char *str, int *count) {
  FILE *fp;
  int line_num = 1;
  int find_result = 0;
  char temp[512];
  *count = 0;
 
  if((fp = fopen(fname, "r")) == NULL) { return(-1);}
  while(fgets(temp, 512, fp) != NULL) {
    if((strstr(temp, str)) != NULL) {
      *count = *count +1;
      find_result=1;    
      }line_num++;
  }
   if(fp) {
    fclose(fp);
  }
  return(find_result);
}


int ReadFileWords(const char *filename, char *words1[])
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
/*rewind(f);
while(!feof(f))
{ 
       
       j=fscanf(f, " %[^\n]s", temp1);
       words1[i] = strdup(temp1);
        i=i+1;
}*/
    fclose(f);

    // The result of this function is the number of words in the file
    return i;
}

int ReadWords(const char *filename, char *words[], int max_number_of_words)
{
    FILE *f = fopen(filename, "rt"); 
    int i,j;
    char temp[100]; 
    for (i = 0; i < max_number_of_words; ++i)
    { if (fscanf(f, "%s", temp) != 1)
        break;
        words[i] = strdup(temp);
    }
    fclose(f);
    return i;
}

int ReadStopWords(const char *filename, char *words[])
{
    FILE *f = fopen(filename, "rt"); 
    int i=0,j;
    char temp[100]; 
    if (f == NULL)
    {
    printf("Error file missing\n");
    exit(-1);
    }
    while(!feof(f))
    {
       j=fscanf(f, "%s", temp);
       words[i] = strdup(temp);
       i=i+1;
    }
    fclose(f);
    return i;
}

int partition(double * a, int p, int r)
{
    double lt[r-p];
    double gt[r-p];
    int i;
    int j;
    double key = a[r];
    int lt_n = 0;
    int gt_n = 0;

#pragma omp parallel for
    for(i = p; i < r; i++){
        if(a[i] < a[r]){
            lt[lt_n++] = a[i];
        }else{
            gt[gt_n++] = a[i];
        }   
    }   

    for(i = 0; i < lt_n; i++){
        a[p + i] = lt[i];
    }   

    a[p + lt_n] = key;

    for(j = 0; j < gt_n; j++){
        a[p + lt_n + j + 1] = gt[j];
    }   

    return p + lt_n;
}

void quicksort(double * a, int p, int r)
{
    int div;

    if(p < r){ 
        div = partition(a, p, r); 
#pragma omp parallel sections
        {   
#pragma omp section
            quicksort(a, p, div - 1); 
#pragma omp section
            quicksort(a, div + 1, r); 

        }
    }

}
int main(int argc, char *argv[]) {
  if(argc !=3)
    printf("Error :usage ./a.out WMAX FMAX\n");
   int  nTasks, rank, rc; 
   char szSearchWord[100];
   int hasSearchWord = 0;   
   int searchWordLength;
   omp_set_nested(1);
   omp_set_num_threads(4);
   int s;
   
   rc = MPI_Init(&argc,&argv);
   if (rc != MPI_SUCCESS) 
   {
     printf ("Error starting MPI program. Terminating.\n");
     MPI_Abort(MPI_COMM_WORLD, rc);
    }
   MPI_Comm_size(MPI_COMM_WORLD,&nTasks);
   MPI_Comm_rank(MPI_COMM_WORLD,&rank);
   startTime = omp_get_wtime();
   
   char    buffer[BUFSIZ];
   int i=0,j=0;
   char temp[1000];
   common_file = fopen("commonfile.txt", "w");
   if (common_file == NULL)
     {   fprintf(stderr, "Error : Failed to open common_file - %s\n", strerror(errno));
        return 1;
     } 
   if (NULL == (FD = opendir ("bible/"))) 
    {   fprintf(stderr, "Error : Failed to open input directory - %s\n", strerror(errno));
        fclose(common_file);
        return 1;
    }
 
    //WRITING ALL POSSIBLE WORDS IN COMMONFILE.TXT
    
  if(rank ==0)
  {
    
    while ((in_file = readdir(FD))) 
    {
      if (!strcmp (in_file->d_name, "."))
            continue;
      if (!strcmp (in_file->d_name, ".."))    
            continue;        
     file_name = (char *)malloc((strlen("bible/")+strlen(in_file->d_name)+1)*sizeof(char));
     strcpy(file_name,"bible/");
     strcat(file_name,in_file->d_name);
     
      
     file_name2[f]=file_name;
      printf("%s %d\n",file_name2[f],f );
      f++; 
    }    
    printf("no of files%d\n",f);
    
    int tid;
    #pragma omp parallel for reduction(+ : total_words)
    for(j=0;j<f;j++)
    { 
       //tid=omp_get_thread_num();
       //printf("hello FROM%d\n",tid );

     printf("file %d",j); 
     n = ReadFileWords(file_name2[j],words1[j]);
     total_words += n;
     p1[j]=n;
     printf("\n n=%d  ",p1[j]);
    }
  
     for(i=0;i<f;i++)
       {
          for(j=0;j<p1[i];j++)
          {
          fprintf(common_file,"%s \n",words1[i][j]);
          }
       }
     fclose(common_file);
     printf("total words %li and totoal files %d\n",total_words,f);
  }
         
     //REMOVE DUPLICATES FROM COMMONFILE.TXT
     number_of_words = ReadWords("commonfile.txt", search_words, 1000000);
     int i1,i2,k;
     for(i1=0;i1<number_of_words;i1++){
     for(i2=i1+1;i2<number_of_words;)
        {if(strcmp(search_words[i2],search_words[i1]) ==0)
            {
                for(k=i2;k<number_of_words;k++)
                    search_words[k] = search_words[k+1];
                number_of_words--;
            }
            else i2++;
        }
     }
    
    //WRITING THESE WORDS IN WORDS.TXT FILE
    int j1;
    FILE *fpp = fopen("words.txt","w");
    for(j1=0;j1<number_of_words;j1++)
        {
         fprintf(fpp,"%s\n", search_words[j1]);
        }
    fclose(fpp);
    //READ STOP WORDS AND ELIMINATE THEM FROM WORDS[]    
        
    s=ReadStopWords("stopwords.txt",stop_words);
    
    int p;
    k=0; 
    
  #pragma omp  simd for reduction(+ : k)
  for(i=0;i<number_of_words;i++) 
   {
     for(j=0;j<s;j++)
     {
      if((strcasecmp(search_words[i],stop_words[j]) == 0))
       { 
        p=1;
        //printf("%d\n",i);
        //printf("stop word");
        break;
       }
       else{ p=2;}
     }

     if(p != 1)
      {
      //printf("not a stop word\n");
      words[k]=search_words[i];
      k = k+1; 
      }
   
  }
  
   
  number_of_words=k;
   
   //WRITING THE FINAL WORDS TO BE SEARCHED IN SEARCH_WORDS.TXT
   FILE *fppp = fopen("search_words.txt","w");
   for(j1=0;j1<number_of_words;j1++)
        { fprintf(fppp,"%s\n", words[j1]);
        }
    fclose(fppp);
/*--------------------------------------------------------------------------------------------------------------------*/

if(rank ==0)
   {
   numberOfFiles = f;
   WMAX = atoi(argv[1]);
   FMAX = atoi(argv[2]);
   printf("%d %d\n",WMAX,f );

   }

int term_freq[number_of_words][numberOfFiles+1];
double idf[number_of_words];
double tfidf[number_of_words][numberOfFiles+1];
double maxarray[numberOfFiles]; 
int yesno[number_of_words][numberOfFiles+1];
int counter;
FILE *dff = fopen("idf.txt","w");

for(counter =0;counter < number_of_words; counter++)
  { 
   if(rank ==0)
   {
   strcpy(szSearchWord,words[counter]);
   hasSearchWord = rank;
   searchWordLength = strlen(szSearchWord);
   }
   printf("pass 1");
   MPI_Bcast(&searchWordLength, 1, MPI_INT, hasSearchWord, MPI_COMM_WORLD);
   MPI_Bcast(szSearchWord, searchWordLength+1, MPI_CHAR, hasSearchWord, MPI_COMM_WORLD);
   int portion = 0;
   int startNum = 0;
   int endNum = 0;
   if(rank ==0 )
   {
      portion = numberOfFiles / nTasks;
      startNum = 1;
      endNum = portion;
      int i;
      for (i=1; i < nTasks; i++)
        {
          int curStartNum = (i * portion) +1;
          int curEndNum = (i+1) * portion;
          if (i == nTasks-1) { curEndNum = numberOfFiles;}  
          if (curStartNum < 0) { curStartNum = 0; }
          MPI_Send(&curStartNum, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
          MPI_Send(&curEndNum, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
        }
    }
    else
    {
      MPI_Status status;
      MPI_Recv(&startNum, 1, MPI_INT, 0,1, MPI_COMM_WORLD, &status);
      MPI_Recv(&endNum, 1, MPI_INT, 0,1, MPI_COMM_WORLD, &status); 
    } 
    printf("pass 2");
    int df =0;
    int curIndex;
    for(curIndex = startNum;curIndex<=endNum;curIndex++)
    { int *tf;
      tf = malloc(sizeof(int));
      char path[10] = "bible/";
      char file_index[5];
      char ext[5] = ".txt";
      sprintf(file_index, "%d", curIndex);
      char *name_with_extension;
      name_with_extension = malloc(strlen(path)+strlen(file_index)+strlen(ext));
      strcpy(name_with_extension,path);
      strcat(name_with_extension, file_index);
      strcat(name_with_extension, ext);
      int result = Search_in_File(name_with_extension,szSearchWord, tf);
      if(result == 1)
       df++;   
      FILE *tempf = fopen("tf.txt","a");
      fprintf(tempf,"%d %d %d\n",counter,curIndex,*tf); 
      fclose(tempf);
    }
    if (rank == 0)
    {
     MPI_Status status;
     int i;
     for (i=1; i < nTasks; i++)
     {
      int temp;
      MPI_Recv(&temp, 1, MPI_INT, i,3, MPI_COMM_WORLD, &status);
      df += temp;
     }
    }
    else
      MPI_Send(&df, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
    if (rank == 0)
    { 
    idf[counter] = numberOfFiles/df;
    fprintf(dff, "<%s , %f>\n",szSearchWord ,idf[counter]);
    }
 }//for loop of counter 
fclose(dff); //idf.txt
/*MPI_Barrier(MPI_COMM_WORLD);
if(rank == 0){

  FILE *pt;
  pt = fopen("tf.txt", "r");
  int i,j; int x,y;
  if (pt == NULL)
    {
        printf("Error Reading File\n");
        return 0;
    }
    for (i = 0; i < number_of_words; i++)
    {
       for(j=1;j<=numberOfFiles;j++)
       { fscanf(pt, "%d,", &x );
         fscanf(pt, "%d,", &y );
         fscanf(pt, "%d,", &term_freq[x][y] );
       }
    }     
  fclose(pt);
}
MPI_Barrier(MPI_COMM_WORLD);
if(rank ==0)
{
int k1,k2;
//#pragma omp parallel for schedule(runtime)
for(k1=0;k1<number_of_words;k1++)
{  
  //#pragma opm parallel for schedule(dynamic)
  for(k2 =1;k2<=numberOfFiles;k2++)
   { 
    tfidf[k1][k2] = term_freq[k1][k2] * log10(idf[k1]);
  }
}
}

MPI_Barrier(MPI_COMM_WORLD);
FILE *tfidf_f = fopen("tf-idf.txt","w");

if(rank == 0){
  printf("here\n");
  int k1,k2;
for(k1=0;k1<number_of_words;k1++)
{  
  for(k2 =1;k2<=numberOfFiles;k2++)
   { 
   fprintf(tfidf_f, "%f   ",tfidf[k1][k2] );
   }
 fprintf(tfidf_f, "\n" );
}
}
fclose(tfidf_f);
MPI_Barrier(MPI_COMM_WORLD);
if(rank ==0){
int index;
for(index = 1;index<=numberOfFiles;index++)
{
  double a[number_of_words];
  int k4;

  for(k4=0;k4<number_of_words;k4++)
  {
    a[k4] = tfidf[k4][index];
  }
  
  int i11,i22;
  int s1 = number_of_words;
 
    for(i11=0;i11<s1;i11++)
    {
     for(i22=i11+1;i22<s1;)
        {if(a[i22]==a[i11])
            {
             for(k=i22;k<s1;k++)
                a[k] = a[k+1];
             s1--;
            }
         else i22++;
         }
     }
     quicksort(a, 0, s1);
     maxarray[index] = a[s1-WMAX]; 
     printf("%f\n",maxarray[index] );  
}

}
MPI_Barrier(MPI_COMM_WORLD);
if(rank ==0)
{
int k1,k2;
//#pragma omp parallel for schedule(runtime)
for(k1=0;k1<number_of_words;k1++)
{  
  //#pragma opm parallel for schedule(dynamic)
  for(k2 =1;k2<=numberOfFiles;k2++)
   { 
    //tfidf[k1][k2] = term_freq[k1][k2] * log10(idf[k1]);
    if(tfidf[k1][k2] >= maxarray[k2])
      yesno[k1][k2] = 1;
    else
      yesno[k1][k2] = 0;
  }
}
}



FILE *file_index = fopen("file_index","w");
if(rank ==0)
{
int k1,k2;

for(k1=0;k1<number_of_words;k1++)
{  
    fprintf(file_index, "%s --->\t",words[k1] );
  for(k2 =1;k2<=numberOfFiles;k2++)
   { 
    if(yesno[k1][k2]==1)
    {
      fprintf(file_index, "%d.txt\t",k2 );
    }

   }
   fprintf(file_index, "\n" );
 }
}
fclose(file_index);

*/
endTime = omp_get_wtime();
if(rank ==0)
  printf("TOTAL TIME TAKEN IS: %f\n",endTime - startTime );
MPI_Finalize();

}

