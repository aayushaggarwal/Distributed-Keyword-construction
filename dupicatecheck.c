#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>


int addToArray(const char * toadd, char * strarray[], int strcount)
{
    const int toaddlen = strlen(toadd);
    int i;
    // Add new string to end.
    // Remember to add one for the \0 terminator.
    strarray[strcount] = malloc(sizeof(char) * (toaddlen + 1));
    strncpy(strarray[strcount], toadd, toaddlen + 1);
    printf("pass 2");
    // Search for a duplicate.
    // Note that we are cutting the new array short by one.
    for(i = 0; i < strcount; ++i)
    {
        if (strncmp(toadd,strarray[i],toaddlen) == 0)
        {
            // Found duplicate.
            // Remove it and compact.
            // Note use of new array size here.  
            free(strarray[i]);
            for(int k = i + 1; k < strcount + 1; ++k)
                strarray[i] = strarray[k];

            strarray[strcount] = NULL;
            return strcount;
        }
    }

    // No duplicate found.
    return (strcount + 1);
}



/* This is just a sample code, modify it to meet your need */
int main(int argc, char **argv)
{
    DIR* FD;
    struct dirent* in_file;
    FILE    *common_file;
    FILE    *entry_file;
    char    buffer[BUFSIZ];
    int i=0,j=0,k;
    char temp[1000];
    char *words[1000000];

    /* Openiing common file for writing */
    common_file = fopen("/home/aayush/Desktop/paracom/commonfile.txt", "w");
    if (common_file == NULL)
    {
        fprintf(stderr, "Error : Failed to open common_file - %s\n", strerror(errno));

        return 1;
    }

    /* Scanning the in directory */
    if (NULL == (FD = opendir ("/home/aayush/Desktop/paracom/books/"))) 
    {
        fprintf(stderr, "Error : Failed to open input directory - %s\n", strerror(errno));
        fclose(common_file);

        return 1;
    }
    while ((in_file = readdir(FD))) 
    {
        /* On linux/Unix we don't want current and parent directories
         * On windows machine too, thanks Greg Hewgill
         */
        if (!strcmp (in_file->d_name, "."))
            continue;
        if (!strcmp (in_file->d_name, ".."))    
            continue;
        

        entry_file = fopen(in_file->d_name, "r");
        if (entry_file == NULL)
        {
            fprintf(stderr, "Error : Failed to open entry file - %s\n", strerror(errno));
            fclose(common_file);

            return 1;
        }

        
        while(fgets(buffer, BUFSIZ, entry_file) != NULL)
        {
            j=fscanf(entry_file, "%s", temp);
            i=addToArray(strdup(temp),words,i);
            fprintf(common_file,"%s  %d \n", words[i],i);
            i=i+1;
        }
        printf("pass 1\n");    
       
        fclose(entry_file);
    }

    
    


printf("%d",i);



    return 0;
fclose(common_file);
}
