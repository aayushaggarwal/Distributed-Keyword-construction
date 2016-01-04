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
char *words[1000000];
char *stop_words[600];
int WMAX;
int FMAX;
DIR* FD;
struct dirent* in_file;
FILE    *common_file;
FILE    *entry_file;
char *file_name;
double startTime, endTime;


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
    while ((in_file = readdir(FD))) 
    {
      if (!strcmp (in_file->d_name, "."))
            continue;
      if (!strcmp (in_file->d_name, ".."))    
            continue;        
     file_name = malloc(strlen("bible/")+strlen(in_file->d_name));
     strcpy(file_name,"bible/");
     strcat(file_name,in_file->d_name);
     entry_file = fopen(file_name, "r");
     if (entry_file == NULL)
        {   fprintf(stderr, "Error : Failed to open entry file - %s\n", strerror(errno));
            fclose(common_file);
            return 1;
        }
     char oneword[100];
     char c;
     do {
        c = fscanf(entry_file,"%s",oneword); 
        fprintf(common_file,"%s  \n", oneword);
        } while (c != EOF);             
        fclose(entry_file);       
    }
    fclose(common_file);
  
    //REMOVE DUPLICATES FROM COMMONFILE.TXT
    number_of_words = ReadWords("commonfile.txt", words, 100000);
    int i1,i2,k;
    for(i1=0;i1<number_of_words;i1++){
    for(i2=i1+1;i2<number_of_words;)
        {if(strcmp(words[i2],words[i1]) ==0)
            {
                for(k=i2;k<number_of_words;k++)
                    words[k] = words[k+1];
                number_of_words--;
            }
            else i2++;
        }
   }
    //WRITING THESE WORDS IN WORDS.TXT FILE
    /*int j1;
    FILE *fpp = fopen("words.txt","w");
    for(j1=0;j1<number_of_words;j1++)
        {
         fprintf(fpp,"%s\n", words[j1]);
        }
    fclose(fpp);*/
    //READ STOP WORDS AND ELIMINATE THEM FROM WORDS[]    
    s=ReadStopWords("stopwords.txt",stop_words);
    for(i1=0;i1<s;i1++){
    for(i2=0;i2<number_of_words;)
        {if(strcasecmp(words[i2],stop_words[i1]) ==0)
            {
                for(k=i2;k<number_of_words;k++)
                    words[k] = words[k+1];
                number_of_words--;
            }
            else i2++;
        }
   }

  
   //WRITING THE FINAL WORDS TO BE SEARCHED IN SEARCH_WORDS.TXT
   /*FILE *fppp = fopen("search_words.txt","w");
   for(j1=0;j1<number_of_words;j1++)
        { fprintf(fppp,"%s\n", words[j1]);
        }
    fclose(fppp);*/

if(rank ==0)
   {
   numberOfFiles = filesInCorpus();
   WMAX = atoi(argv[1]);
   FMAX = atoi(argv[2]);
   printf("%d\n",WMAX );

   }
int term_freq[number_of_words][numberOfFiles+1];
double idf[number_of_words];
double tfidf[number_of_words][numberOfFiles+1];
int counter;
FILE *dff = fopen("idf.txt","w");
//parallel for
for(counter =0;counter < number_of_words; counter++)
  { 
   if(rank ==0)
   {
   strcpy(szSearchWord,words[counter]);
   hasSearchWord = rank;
   searchWordLength = strlen(szSearchWord);
   }
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
    //fprintf(dff, "<%s , %d>\n",szSearchWord ,df );
    idf[counter] = numberOfFiles/df;
    fprintf(dff, "<%s , %f>\n",szSearchWord ,idf[counter]);
    }
 }//for loop of counter 
fclose(dff); //idf.txt

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
 /* for (i = 0; i < number_of_words; i++)
    {
       for(j=1;j<=numberOfFiles;j++)
       { printf("term_freq[%d][%d] = %d\n", i,j, term_freq[i][j] );
       }
    }  */ 
}

FILE *tfidf_f = fopen("tf-idf.txt","w");

if(rank ==0)
{
int k1,k2;
//#pragma omp parallel for ordered
for(k1=0;k1<number_of_words;k1++)
{  
//#pragma omp parallel for ordered
  for(k2 =1;k2<=numberOfFiles;k2++)
   { 
    // #pragma omp atomic
     //#pragma ordered
    tfidf[k1][k2] = term_freq[k1][k2] * log10(idf[k1]);
    //fprintf(tfidf_f,"tfidf[%d][%d] = %f\n",k1,k2,tfidf[k1][k2] );
    fprintf(tfidf_f, "%f   ",tfidf[k1][k2] );
  }
  fprintf(tfidf_f, "\n" );
}
}
fclose(tfidf_f);
/*if(rank ==0){
double a[number_of_words];
int itr;int k4;
/*for(itr =1;itr<=numberOfFiles;itr++)
{*/
/*for(k4=0;k4<number_of_words;k4++)
  {a[k4] = tfidf[k4][3];}
int s1 = number_of_words;

    for(i1=0;i1<s1;i1++){
    for(i2=i1+1;i2<s1;)
        {if(a[i2]==a[i1])
            {
                for(k=i2;k<s1;k++)
                    a[k] = a[k+1];
                s1--;
            }
            else i2++;
        }
   }
quicksort(a, 0, s1);
for(k4 = 0;k4 < s1; k4++){
        printf("%f\t", a[k4]);
    }
    printf("\n");
//printf("%f--\n", a[number_of_words-WMAX]);
//}
}*/

/*


double tempo[number_of_words];
int num_threads;
#pragma omp parallel
  {
  #pragma omp master
    {
    num_threads = omp_get_num_threads();
    }
  }
int itr;int k4;
//for(itr =1;itr<=numberOfFiles;itr++)
//{
 for(k4=0;k4<number_of_words;k4++)
 {a[k4] = tfidf[k4][1];}
 mergesort_parallel_omp(a, number_of_words, tempo, num_threads);
*/


endTime = omp_get_wtime();
if(rank ==0)
  printf("TOTAL TIME TAKEN IS: %f\n",endTime - startTime );
MPI_Finalize();

}

