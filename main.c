#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "pimglib.h"

int main(int argc, char* argv[]) {
    int filegiven = 0;
    int typegiven = 0;
    int channels = 1;
    int height = 0;
    int width = 0;
    char *image_name = malloc(128);
    char *type = malloc(32);

    int optchar;
    while((optchar = getopt(argc,argv,"f:h:w:c:")) != -1){
        if(optchar == 'f'){
            filegiven = 1;
            strcpy(image_name,optarg);
            strcat(image_name,"\0");
            char *temptype = strchr(image_name,'.');
            while(temptype != NULL){
                temptype = strchr(temptype+1,'.');
                if(temptype != NULL)
                    strcpy(type,temptype);
            }
            strcat(type,"\0");
        }
        if(optchar == 'c'){
            if(strcmp(optarg,"rgb") == 0)
                channels = 3;
        }
        if(optchar == 'h'){
            height = atoi(optarg);
        }
        if(optchar == 'w'){
            width = atoi(optarg);
        }

    }
    parallel_image_filtering_engine(image_name,height,width,channels);
    free(image_name);
    free(type);
}
