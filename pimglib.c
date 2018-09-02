#include "stdio.h"
#include "stdlib.h"
#include "mpi.h"
#include "time.h"
#include "math.h"
#include "string.h"
#include "pimglib.h"
//#include "inlines.h"

#define DIE printf("Error Occured. Application Exits\n"); exit(1);

int off;
int block_width;
int block_height;
/* Declare Image Filter  */
double h[3][3] = {{ 1.0/9, 1.0/9,  1.0/9},\
                  {1.0/9,  1.0/9, 1.0/9},\
                  { 1.0/9, 1.0/9,  1.0/9}};

int parallel_image_filtering_engine(char *image_name, int img_height, int img_width, int channels){

    /* Initialize the MPI environment */
    MPI_File file;
    MPI_Comm comm;
    MPI_Status status;

    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    if(world_size != 1 && world_size != 4 && world_size != 9 && world_size != 16 && world_size != 25 && world_size != 36){
        DIE
    }

    //get process' world rank
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    /* Read image time calculation */
    MPI_Barrier(MPI_COMM_WORLD);
    double start;
    if(world_rank == 0){
        printf("Image to edit: %s\nDimensions: %d x %d\nColor Channels: %d\n",image_name,img_height,img_width,channels);
        start = MPI_Wtime();
    }

    /* Open image and calculate the block size of each process */
    MPI_File_open( MPI_COMM_WORLD, image_name, MPI_MODE_RDONLY, MPI_INFO_NULL, &file );


    MPI_Offset fsize;
    MPI_File_get_size(file,&fsize);
    int block_size = fsize / world_size;
    int height = img_height;
    int width = img_width;

    off = 1;
    int width_multiplier = 1;
    if(channels == 3){
        width_multiplier = 3;
        off = 3;
    }

    width *= width_multiplier;

    block_height = height / (int) sqrt(world_size);
    block_width = width  / (int) sqrt(world_size);
    if(world_rank == 0){
        printf("Parallel Processing of Image with %d processes\n",world_size);
        printf("File Size: %lld\n",fsize);
        printf("Block Size: %d\n",block_size);
        printf("Block height: %d Block width: %d\n",block_height,block_width);
    }


    /* Allocate memory to read from file and allocate new space to write the modified data */
    unsigned char *block_data = malloc( block_height * block_width * sizeof(unsigned char));

    unsigned char **new_block_data = malloc( block_height * sizeof(unsigned char*));
    for(int i = 0; i < block_height; i++)
        new_block_data[i] = malloc( block_width * sizeof(unsigned char));

    int seek_offset = (int) (world_rank % (int) sqrt(world_size))*block_width + (int)( world_rank / (int)sqrt(world_size) ) * block_height * width;

    for(int i = 0; i < block_height; i++){ //height iterations
        MPI_File_seek(file,seek_offset + i * width, MPI_SEEK_SET);
        for(int j = 0; j < block_width; j++){ //width iterations
            MPI_File_read(file,block_data+i*block_width+j,1,MPI_UNSIGNED_CHAR,&status);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if(world_rank == 0)
        printf("File read in %f sec\n",MPI_Wtime() - start);

    int sqrt_world_size = sqrt(world_size);

    /* Processing Time Calculation */
    MPI_Barrier(MPI_COMM_WORLD);
    if(world_rank == 0)
        start = MPI_Wtime();

    /* Categorize each process by the position of the block that it processes */

    /* Declare derived datatype for column pixels to send/recv */
    MPI_Datatype MPI_Block_Column;
    MPI_Type_vector(block_height, off, block_width, MPI_UNSIGNED_CHAR, &MPI_Block_Column);
    MPI_Type_commit(&MPI_Block_Column);

    double prevAvg = 0.0;
    double curAvg;
    int stop = 0;

    while(1){
        if(world_rank == 0){
            curAvg = 0.0;
            for(int j = 0; j < block_width*block_height; j++)
                curAvg += (double) block_data[j];

            curAvg = curAvg / block_width*block_height;


            if(prevAvg != 0.0 && abs(curAvg - prevAvg)/prevAvg < 0.01)
                stop = 1;

            printf("Percentage Change: %f\n",abs(curAvg - prevAvg)/prevAvg);

            prevAvg = curAvg;
        }

        MPI_Bcast(&stop, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        if (stop == 1)
            break;

        /* Only one Process */
        if(world_size == 1){
            for(int i = 0; i < block_height; i++){
                for(int j = 0; j < block_width; j++){
                    //left up corner of image
                    if(i == 0 && j < off)
                        new_block_data[i][j] = left_up_corner_filter(block_data,j);
                    //right up corner of image
                    else if(i == 0 && j >= block_width - off)
                        new_block_data[i][j] = right_up_corner_filter(block_data,j);
                    //left down corner of image
                    else if(i == block_height - 1 && j < off)
                        new_block_data[i][j] = left_down_corner_filter(block_data,j);
                    //right down corner of image
                    else if(i == block_height -1 && j >= block_width - off)
                        new_block_data[i][j] = right_down_corner_filter(block_data,j);
                    //up line of image
                    else if(i == 0)
                        new_block_data[i][j] = up_line_filter(block_data,i,j);
                    //left column of image
                    else if(j < off)
                        new_block_data[i][j] = left_column_filter(block_data,i,j);
                    //right column of image
                    else if(j >= block_width - off)
                        new_block_data[i][j] = right_column_filter(block_data,i,j);
                    //down line of image
                    else if(i == block_height - 1)
                        new_block_data[i][j] = down_line_filter(block_data,i,j);
                    //middle pixels
                    else
                        new_block_data[i][j] = middle_pixel_filter(block_data,i,j);
                }
            }
        }

        /*Multiple processes*/

        /*Left up Corner Process */
        else if(world_rank == 0){
            unsigned char* down_line = malloc(block_width * sizeof(unsigned char));
            unsigned char* right_column = malloc(block_height * off * sizeof(unsigned char));
            unsigned char* right_down_corner = malloc(off * sizeof(unsigned char));

            MPI_Request request_for_down_line;
            MPI_Request request_for_right_down_corner;
            MPI_Request request_for_right_column;
            MPI_Request ignored;

            //down line
            MPI_Irecv(down_line, block_width, MPI_UNSIGNED_CHAR, sqrt_world_size, 0,MPI_COMM_WORLD,&request_for_down_line);
            MPI_Isend(block_data + (block_height-1)*block_width, block_width, MPI_UNSIGNED_CHAR, sqrt_world_size, 0,MPI_COMM_WORLD,&ignored);

            //right_down_corner
            MPI_Irecv(right_down_corner, off, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size + 1, 0,MPI_COMM_WORLD,&request_for_right_down_corner);
            MPI_Isend(block_data + (block_height-1)*block_width + block_width - off, off, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size + 1, 0,MPI_COMM_WORLD,&ignored);

            //right column
            MPI_Irecv(right_column, block_height * off, MPI_UNSIGNED_CHAR, world_rank + 1, 0, MPI_COMM_WORLD, &request_for_right_column);
            MPI_Isend(block_data + block_width-off, 1, MPI_Block_Column, world_rank + 1, 0,MPI_COMM_WORLD,&ignored);

            //EPIKALUPSH ME PRAKSEIS
            for(int i = 0; i < block_height-1; i++){
                for(int j = 0; j < block_width-off; j++){
                    //left up corner
                    if( i == 0 && j < off)
                        new_block_data[i][j] = left_up_corner_filter(block_data,j);
                    //up line
                    else if(i == 0)
                        new_block_data[i][j] = up_line_filter(block_data,i,j);
                    //left column
                    else if(j < off)
                        new_block_data[i][j] = left_column_filter(block_data,i,j);
                    //middle pixel
                    else
                        new_block_data[i][j] = middle_pixel_filter(block_data,i,j);
                }
            }

            MPI_Wait(&request_for_down_line, &status);
            //left down corner of block
            for(int k = 0; k < off; k++)
                new_block_data[block_height-1][k] = left_down_corner_wDownNeighbor_filter(block_data,k,down_line);

            //down line of block
            for(int j = off; j < block_width -off; j++)
                new_block_data[block_height-1][j] = down_line_wNeighboors_filter(block_data,j,down_line);

            MPI_Wait(&request_for_right_column, &status);
            //right up corner of block
            for(int k = 0; k < off; k++)
                new_block_data[0][block_width + k - off] = right_up_corner_wRightNeighbor_filter(block_data,k,right_column);

            //right column
            for(int i = 1; i < block_height -1; i++)
                for(int k = 0; k < off; k ++)
                    new_block_data[i][block_width-off + k] = right_column_wNeighboors_filter(block_data,i,k,right_column);

            MPI_Wait(&request_for_right_down_corner, &status);
            //right down corner
            for(int k = 0; k < off; k++)
                new_block_data[block_height-1][block_width-off + k] = right_down_corner_wNeighboors_filter(block_data,k,right_column,down_line,right_down_corner);

        }
        /* Up Right Process */
        else if(world_rank == sqrt_world_size -1){ //cat2 - panw deksia gwnia
            unsigned char* down_line = malloc(block_width * sizeof(unsigned char));
            unsigned char* left_column = malloc(block_height * off * sizeof(unsigned char));
            unsigned char* left_down_corner = malloc(off * sizeof(unsigned char));

            MPI_Request request_for_down_line;
            MPI_Request request_for_left_down_corner;
            MPI_Request request_for_left_column;
            MPI_Request ignored;

            //down line
            MPI_Irecv(down_line, block_width, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size, 0,MPI_COMM_WORLD,&request_for_down_line);
            MPI_Isend(block_data + (block_height-1)*block_width, block_width, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size, 0,MPI_COMM_WORLD,&ignored);

            //left column
            MPI_Irecv(left_column, block_height * off, MPI_UNSIGNED_CHAR, world_rank - 1, 0,MPI_COMM_WORLD,&request_for_left_column);
            MPI_Isend(block_data, 1, MPI_Block_Column, world_rank - 1, 0,MPI_COMM_WORLD,&ignored);

            //left down corner
            MPI_Irecv(left_down_corner, off, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size - 1, 0,MPI_COMM_WORLD,&request_for_left_down_corner);
            MPI_Isend(block_data + (block_height-1)*block_width, off, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size - 1, 0,MPI_COMM_WORLD,&ignored);

            //standard
            for(int i = 0; i < block_height -1; i++){
                for(int j = off; j < block_width; j++){
                    //right up corner
                    if( i == 0 && j >= block_width-off)
                        new_block_data[i][j] = right_up_corner_filter(block_data,j);
                    //up line
                    else if(i == 0)
                        new_block_data[i][j] = up_line_filter(block_data,i,j);
                    //right column
                    else if(j >= block_width-off)
                        new_block_data[i][j] = right_column_filter(block_data,i,j);
                    //middle pixels
                    else
                        new_block_data[i][j] =  middle_pixel_filter(block_data,i,j);
                }
            }

            MPI_Wait(&request_for_down_line, &status);
            //right down corner of block
            for(int k = 0; k < off; k++)
                new_block_data[block_height-1][block_width-off + k] = right_down_corner_wDownNeighbor_filter(block_data,k,down_line);

            //down line
            for(int j = off; j < block_width -off; j++)
                new_block_data[block_height-1][j] = down_line_wNeighboors_filter(block_data,j,down_line);

            MPI_Wait(&request_for_left_column, &status);
            //left top corner
            for(int k = 0; k < off; k++)
                new_block_data[0][k] = left_up_corner_wLeftNeighbor_filter(block_data,k,left_column);

            //left column
            for(int i = 1; i < block_height -1; i++)
                for(int k = 0; k < off ; k++)
                    new_block_data[i][k] = left_column_wNeighboors_filter(block_data,i,k,left_column);

            MPI_Wait(&request_for_left_down_corner, &status);
            //left down corner
            for(int k = 0; k < off ; k++)
                new_block_data[block_height-1][k] = left_down_corner_wNeighboors_filter(block_data,k,left_column,down_line,left_down_corner);

        }
        /* Down Left Process */
        else if(world_rank == world_size -1 - sqrt_world_size +1){ //cat3 - katw aristera gwnia
            unsigned char* up_line = malloc(block_width * sizeof(unsigned char));
            unsigned char* right_column = malloc(block_height * off * sizeof(unsigned char));
            unsigned char* right_up_corner = malloc(off * sizeof(unsigned char));

            MPI_Request request_for_up_line;
            MPI_Request request_for_right_up_corner;
            MPI_Request request_for_right_column;
            MPI_Request ignored;

            //up line
            MPI_Irecv(up_line, block_width, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size, 0,MPI_COMM_WORLD,&request_for_up_line);
            MPI_Isend(block_data, block_width, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size, 0,MPI_COMM_WORLD,&ignored);

            //deksia sthlh
            MPI_Irecv(right_column, block_height * off, MPI_UNSIGNED_CHAR, world_rank + 1, 0, MPI_COMM_WORLD, &request_for_right_column);
            MPI_Isend(block_data + block_width - off, 1, MPI_Block_Column, world_rank + 1, 0,MPI_COMM_WORLD,&ignored);

            //right_up_corner
            MPI_Irecv(right_up_corner, off, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size + 1, 0,MPI_COMM_WORLD,&request_for_right_up_corner);
            MPI_Isend(block_data + block_width - off, off, MPI_UNSIGNED_CHAR,  world_rank - sqrt_world_size + 1, 0,MPI_COMM_WORLD,&ignored);

            //standard
            for(int i = 1; i < block_height; i++){
                for(int j = 0; j < block_width -off; j++){
                    //left down corner
                    if( i == block_height-1 && j < off)
                        new_block_data[i][j] = left_down_corner_filter(block_data,j);
                    //down line
                    else if(i == block_height-1)
                        new_block_data[i][j] = down_line_filter(block_data,i,j);
                    //left column
                    else if(j < off)
                        new_block_data[i][j] = left_column_filter(block_data,i,j);
                    //middle
                    else
                        new_block_data[i][j] = middle_pixel_filter(block_data,i,j);
                }
            }

            MPI_Wait(&request_for_up_line, &status);
            //left up corner
            for(int k = 0; k < off; k++)
                new_block_data[0][k] = left_up_corner_wUpNeighbor_filter(block_data,k,up_line);

            //up line
            for(int j = off; j < block_width -off; j++)
                new_block_data[0][j] = up_line_wNeighboors_filter(block_data,j,up_line);

            MPI_Wait(&request_for_right_column, &status);
            //right_column
            for(int i = 1; i < block_height -1; i++)
                for(int k = 0; k < off; k++)
                    new_block_data[i][block_width-off+k] = right_column_wNeighboors_filter(block_data,i,k,right_column);

            //right_down_corner
            for(int k = 0; k < off; k++)
                new_block_data[block_height-1][block_width-off] = right_down_corner_wRightNeighbor_filter(block_data,k,right_column);

            //right up corner
            MPI_Wait(&request_for_right_up_corner, &status);
            for(int k = 0; k < off; k++)
                new_block_data[0][block_width-off + k] = right_up_corner_wNeighboors_filter(block_data,k, right_column, up_line, right_up_corner);

        }
        /* Down Right Process */
        else if(world_rank == world_size -1){ //cat4 - katw deksia gwnia
            unsigned char* up_line = malloc(block_width * sizeof(unsigned char));
            unsigned char* left_column = malloc(block_height * off* sizeof(unsigned char));
            unsigned char* left_up_corner = malloc(off * sizeof(unsigned char));

            MPI_Request request_for_up_line;
            MPI_Request request_for_left_up_corner;
            MPI_Request request_for_left_column;
            MPI_Request ignored;

            //up line
            MPI_Irecv(up_line, block_width, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size, 0,MPI_COMM_WORLD,&request_for_up_line);
            MPI_Isend(block_data, block_width, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size, 0,MPI_COMM_WORLD,&ignored);

            //left column
            MPI_Irecv(left_column, block_height * off, MPI_UNSIGNED_CHAR, world_rank - 1, 0,MPI_COMM_WORLD,&request_for_left_column);
            MPI_Isend(block_data, 1, MPI_Block_Column, world_rank - 1, 0,MPI_COMM_WORLD,&ignored);

            //left up corner
            MPI_Irecv(left_up_corner, off, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size - 1, 0,MPI_COMM_WORLD,&request_for_left_up_corner);
            MPI_Isend(block_data, off, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size - 1, 0,MPI_COMM_WORLD,&ignored);

            //standard
            for(int i = 1; i < block_height; i++){
                for(int j = off; j < block_width; j++){
                    //right down corner
                    if( i == block_height-1 && j >= block_width-off)
                        new_block_data[i][j] = right_down_corner_filter(block_data,j);
                    //down line
                    else if(i == block_height-1)
                        new_block_data[i][j] = down_line_filter(block_data,i,j);
                    //right column
                    else if(j >= block_width-off)
                        new_block_data[i][j] = right_column_filter( block_data, i, j);
                    //middle pixel
                    else
                        new_block_data[i][j] =  middle_pixel_filter(block_data,i,j);
                }
            }

            MPI_Wait(&request_for_up_line, &status);
            //right up corner
            for(int k = 0; k < off; k++)
                new_block_data[0][block_width-off + k] = right_up_corner_wUpNeighbor_filter(block_data,k,up_line);

            //up line
            for(int j = off; j < block_width -off; j++)
                new_block_data[0][j] =  up_line_wNeighboors_filter(block_data,j,up_line);

            MPI_Wait(&request_for_left_column, &status);
            //left column
            for(int i = 1; i < block_height -1; i++)
                for(int k = 0; k < off; k++)
                    new_block_data[i][k] = left_column_wNeighboors_filter(block_data,i,k,left_column);

            //left down corner
            for(int k = 0; k < off; k++)
                new_block_data[block_height-1][k] = left_down_corner_wLeftNeighbor_filter(block_data,k,left_column);

            MPI_Wait(&request_for_left_up_corner, &status);
            //up left corner
            for(int k = 0; k < off; k++)
                new_block_data[0][k] = left_up_corner_wNeighboors_filter(block_data,k,left_column,up_line,left_up_corner);
        }
        /* Up Line of Processes w/out Corners */
        else if(world_rank >= 1 && world_rank <= sqrt_world_size -2){ //cat5 - panw grammh diergasiwn
            unsigned char* down_line = malloc(block_width * sizeof(unsigned char));

            unsigned char* right_column = malloc(block_height * off * sizeof(unsigned char));
            unsigned char* left_column = malloc(block_height *off * sizeof(unsigned char));

            unsigned char* right_down_corner = malloc(off * sizeof(unsigned char));
            unsigned char* left_down_corner = malloc(off * sizeof(unsigned char));

            MPI_Request request_for_down_line;
            MPI_Request request_for_right_column;
            MPI_Request request_for_left_column;
            MPI_Request request_for_left_down_corner;
            MPI_Request request_for_right_down_corner;
            MPI_Request ignored;

            //down line
            MPI_Irecv(down_line, block_width, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size, 0,MPI_COMM_WORLD,&request_for_down_line);
            MPI_Isend(block_data + (block_height-1)*block_width, block_width, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size, 0,MPI_COMM_WORLD,&ignored);

            //left column
            MPI_Irecv(left_column, block_height * off, MPI_UNSIGNED_CHAR, world_rank - 1, 0,MPI_COMM_WORLD,&request_for_left_column);
            MPI_Isend(block_data, 1, MPI_Block_Column, world_rank - 1, 0,MPI_COMM_WORLD,&ignored);

            //right column
            MPI_Irecv(right_column, block_height * off, MPI_UNSIGNED_CHAR, world_rank + 1, 0, MPI_COMM_WORLD, &request_for_right_column);
            MPI_Isend(block_data + block_width-off, 1, MPI_Block_Column, world_rank + 1, 0,MPI_COMM_WORLD,&ignored);

            //right down corner
            MPI_Irecv(right_down_corner, off, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size + 1, 0,MPI_COMM_WORLD,&request_for_right_down_corner);
            MPI_Isend(block_data + (block_height-1)*block_width + block_width - off, off, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size + 1, 0,MPI_COMM_WORLD,&ignored);

            //left down corner
            MPI_Irecv(left_down_corner, off, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size - 1, 0,MPI_COMM_WORLD,&request_for_left_down_corner);
            MPI_Isend(block_data + (block_height-1)*block_width, off, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size - 1, 0,MPI_COMM_WORLD,&ignored);

            //standard
            for(int i = 0; i < block_height -1; i++){
                for(int j = off; j < block_width -off; j++){
                    //up line
                    if(i == 0)
                        new_block_data[i][j] = up_line_filter(block_data,i,j);
                    //middle pixel
                    else
                        new_block_data[i][j] = middle_pixel_filter(block_data,i,j);
                }
            }

            MPI_Wait(&request_for_down_line, &status);
            //down line
            for(int j = off; j < block_width -off; j++)
                new_block_data[block_height-1][j] = down_line_wNeighboors_filter(block_data,j,down_line);

            MPI_Wait(&request_for_left_column, &status);
            //left top corner
            for(int k = 0; k < off; k++)
                new_block_data[0][k] = left_up_corner_wLeftNeighbor_filter(block_data,k,left_column);

            //left column
            for(int i = 1; i < block_height -1; i++)
                for(int k = 0; k < off; k++)
                    new_block_data[i][k] = left_column_wNeighboors_filter(block_data,i,k,left_column);

            MPI_Wait(&request_for_right_column, &status);
            //right up corner of block
            for(int k = 0; k < off; k++)
                new_block_data[0][block_width + k - off] = right_up_corner_wRightNeighbor_filter(block_data,k,right_column);

            //right column
            for(int i = 1; i < block_height -1; i++)
                for(int k = 0; k < off; k ++)
                    new_block_data[i][block_width-off + k] = right_column_wNeighboors_filter(block_data,i,k,right_column);

            MPI_Wait(&request_for_left_down_corner, &status);
            //left down corner
            for(int k = 0; k < off ; k++)
                new_block_data[block_height-1][k] = left_down_corner_wNeighboors_filter(block_data,k,left_column,down_line,left_down_corner);

            MPI_Wait(&request_for_right_down_corner, &status);
            //right down corner
            for(int k = 0; k < off; k++)
                new_block_data[block_height-1][block_width-off + k] = right_down_corner_wNeighboors_filter(block_data,k,right_column,down_line,right_down_corner);
        }
        /* Down Line of Processes w/out Corners */
        else if(world_rank >= world_size - sqrt_world_size +1 && world_rank <= world_size -2){ //cat6 katw grammh diergasiwn
            unsigned char* up_line = malloc(block_width * sizeof(unsigned char));

            unsigned char* right_column = malloc(block_height * off * sizeof(unsigned char));
            unsigned char* left_column = malloc(block_height * off * sizeof(unsigned char));

            unsigned char* left_up_corner = malloc(off * sizeof(unsigned char));
            unsigned char* right_up_corner = malloc(off * sizeof(unsigned char));

            MPI_Request request_for_up_line;
            MPI_Request request_for_left_up_corner;
            MPI_Request request_for_right_up_corner;
            MPI_Request request_for_left_column;
            MPI_Request request_for_right_column;
            MPI_Request ignored;

            //up line
            MPI_Irecv(up_line, block_width, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size, 0,MPI_COMM_WORLD,&request_for_up_line);
            MPI_Isend(block_data, block_width, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size, 0,MPI_COMM_WORLD,&ignored);

            //right column
            MPI_Irecv(right_column, block_height * off, MPI_UNSIGNED_CHAR, world_rank + 1, 0, MPI_COMM_WORLD, &request_for_right_column);
            MPI_Isend(block_data + block_width - off, 1, MPI_Block_Column, world_rank + 1, 0,MPI_COMM_WORLD,&ignored);

            //left column
            MPI_Irecv(left_column, block_height * off, MPI_UNSIGNED_CHAR, world_rank - 1, 0,MPI_COMM_WORLD,&request_for_left_column);
            MPI_Isend(block_data, 1, MPI_Block_Column, world_rank - 1, 0,MPI_COMM_WORLD,&ignored);

            //left up corner
            MPI_Irecv(left_up_corner, off, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size - 1, 0,MPI_COMM_WORLD,&request_for_left_up_corner);
            MPI_Isend(block_data, off, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size - 1, 0,MPI_COMM_WORLD,&ignored);

            //right_up_corner
            MPI_Irecv(right_up_corner, off, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size + 1, 0,MPI_COMM_WORLD,&request_for_right_up_corner);
            MPI_Isend(block_data + block_width - off, off, MPI_UNSIGNED_CHAR,  world_rank - sqrt_world_size + 1, 0,MPI_COMM_WORLD,&ignored);

            //standard
            for(int i = 1; i < block_height; i++){
                for(int j = 1; j < block_width -1; j++){
                    //down line
                    if(i == block_height - 1)
                        new_block_data[i][j] = down_line_filter(block_data,i,j);
                    //middle pixel
                    else
                        new_block_data[i][j] = middle_pixel_filter(block_data,i,j);
                }
            }

            MPI_Wait(&request_for_up_line, &status);
            //up line
            for(int j = off; j < block_width -off; j++)
                new_block_data[0][j] = up_line_wNeighboors_filter(block_data,j,up_line);

            MPI_Wait(&request_for_right_column, &status);
            //right column
            for(int i = 1; i < block_height -1; i++)
                for(int k = 0; k < off; k ++)
                    new_block_data[i][block_width-off + k] = right_column_wNeighboors_filter(block_data,i,k,right_column);

            //right_down_corner
            for(int k = 0; k < off; k++)
                new_block_data[block_height-1][block_width-off] = right_down_corner_wRightNeighbor_filter(block_data,k,right_column);


            MPI_Wait(&request_for_left_column, &status);
            //left column
            for(int i = 1; i < block_height -1; i++)
                for(int k = 0; k < off ; k++)
                    new_block_data[i][k] = left_column_wNeighboors_filter(block_data,i,k,left_column);

            //left down corner
            for(int k = 0; k < off; k++)
                new_block_data[block_height-1][k] = left_down_corner_wLeftNeighbor_filter(block_data,k,left_column);

            MPI_Wait(&request_for_left_up_corner, &status);
            //left up corner
            for(int k = 0; k < off; k++)
                new_block_data[0][k] = left_up_corner_wNeighboors_filter(block_data,k,left_column,up_line,left_up_corner);

            //right up corner
            MPI_Wait(&request_for_right_up_corner, &status);
            for(int k = 0; k < off; k++)
                new_block_data[0][block_width-off + k] = right_up_corner_wNeighboors_filter(block_data,k, right_column, up_line, right_up_corner);
        }
        /* Left Column of Processes w/out Corners */
        else if(world_rank >= sqrt_world_size && world_rank <= world_size - 2*sqrt_world_size && world_rank % sqrt_world_size == 0){
            unsigned char* up_line = malloc(block_width * sizeof(unsigned char));
            unsigned char* down_line = malloc(block_width * sizeof(unsigned char));

            unsigned char* right_column = malloc(block_height * off * sizeof(unsigned char));

            unsigned char* right_up_corner = malloc(off * sizeof(unsigned char));
            unsigned char* right_down_corner = malloc(off * sizeof(unsigned char));

            MPI_Request request_for_up_line;
            MPI_Request request_for_down_line;
            MPI_Request request_for_right_up_corner;
            MPI_Request request_for_right_down_corner;
            MPI_Request request_for_right_column;
            MPI_Request ignored;

            //down line
            MPI_Irecv(down_line, block_width, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size, 0,MPI_COMM_WORLD,&request_for_down_line);
            MPI_Isend(block_data + (block_height-1)*block_width, block_width, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size, 0,MPI_COMM_WORLD,&ignored);

            //up line
            MPI_Irecv(up_line, block_width, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size, 0,MPI_COMM_WORLD,&request_for_up_line);
            MPI_Isend(block_data, block_width, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size, 0,MPI_COMM_WORLD,&ignored);

            //right column
            MPI_Irecv(right_column, block_height * off, MPI_UNSIGNED_CHAR, world_rank + 1, 0, MPI_COMM_WORLD, &request_for_right_column);
            MPI_Isend(block_data + block_width - off, 1, MPI_Block_Column, world_rank + 1, 0,MPI_COMM_WORLD,&ignored);

            //right_up_corner
            MPI_Irecv(right_up_corner, off, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size + 1, 0,MPI_COMM_WORLD,&request_for_right_up_corner);
            MPI_Isend(block_data + block_width - off, off, MPI_UNSIGNED_CHAR,  world_rank - sqrt_world_size + 1, 0,MPI_COMM_WORLD,&ignored);

            //right down corner
            MPI_Irecv(right_down_corner, off, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size + 1, 0,MPI_COMM_WORLD,&request_for_right_down_corner);
            MPI_Isend(block_data + (block_height-1)*block_width + block_width - off, off, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size + 1, 0,MPI_COMM_WORLD,&ignored);

            //standard
            for(int i = 1; i < block_height -1; i++){
                for(int j = 0; j < block_width - off; j++){
                    //left column of image
                    if(j < off)
                        new_block_data[i][j] = left_column_filter(block_data,i,j);
                    //middle pixel
                    else
                        new_block_data[i][j] = middle_pixel_filter(block_data,i,j);
                }
            }

            MPI_Wait(&request_for_up_line, &status);
            //left up corner
            for(int k = 0; k < off; k++)
                new_block_data[0][k] = left_up_corner_wUpNeighbor_filter(block_data,k,up_line);

            //up line
            for(int j = off; j < block_width -off; j++)
                new_block_data[0][j] = up_line_wNeighboors_filter(block_data,j,up_line);

            MPI_Wait(&request_for_down_line, &status);
            //left down corner of block
            for(int k = 0; k < off; k++)
                new_block_data[block_height-1][k] = left_down_corner_wDownNeighbor_filter(block_data,k,down_line);

            //down line of block
            for(int j = off; j < block_width -off; j++)
                new_block_data[block_height-1][j] = down_line_wNeighboors_filter(block_data,j,down_line);

            MPI_Wait(&request_for_right_column, &status);
            //right column
            for(int i = 1; i < block_height -1; i++)
                for(int k = 0; k < off; k ++)
                    new_block_data[i][block_width-off + k] = right_column_wNeighboors_filter(block_data,i,k,right_column);

            //right up corner
            MPI_Wait(&request_for_right_up_corner, &status);
            for(int k = 0; k < off; k++)
                new_block_data[0][block_width-off + k] = right_up_corner_wNeighboors_filter(block_data,k, right_column, up_line, right_up_corner);

            MPI_Wait(&request_for_right_down_corner, &status);
            //right down corner
            for(int k = 0; k < off; k++)
                new_block_data[block_height-1][block_width-off + k] = right_down_corner_wNeighboors_filter(block_data,k,right_column,down_line,right_down_corner);
        }
        /* Right Column of Processes w/out Corners */
        else if(world_rank >= 2*sqrt_world_size -1 && world_rank <= world_size -1 - sqrt_world_size && world_rank % sqrt_world_size == sqrt_world_size - 1){
            unsigned char* up_line = malloc(block_width * sizeof(unsigned char));
            unsigned char* down_line = malloc(block_width * sizeof(unsigned char));

            unsigned char* left_column = malloc(block_height * off * sizeof(unsigned char));

            unsigned char* left_up_corner = malloc(off * sizeof(unsigned char));
            unsigned char* left_down_corner = malloc(off * sizeof(unsigned char));

            MPI_Request request_for_up_line;
            MPI_Request request_for_down_line;
            MPI_Request request_for_left_up_corner;
            MPI_Request request_for_left_down_corner;
            MPI_Request request_for_left_column;
            MPI_Request ignored;

            //up line
            MPI_Irecv(up_line, block_width, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size, 0,MPI_COMM_WORLD,&request_for_up_line);
            MPI_Isend(block_data, block_width, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size, 0,MPI_COMM_WORLD,&ignored);

            //left column
            MPI_Irecv(left_column, block_height * off, MPI_UNSIGNED_CHAR, world_rank - 1, 0,MPI_COMM_WORLD,&request_for_left_column);
            MPI_Isend(block_data, 1, MPI_Block_Column, world_rank - 1, 0,MPI_COMM_WORLD,&ignored);

            //down line
            MPI_Irecv(down_line, block_width, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size, 0,MPI_COMM_WORLD,&request_for_down_line);
            MPI_Isend(block_data + (block_height-1)*block_width, block_width, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size, 0,MPI_COMM_WORLD,&ignored);

            //left up corner
            MPI_Irecv(left_up_corner, off, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size - 1, 0,MPI_COMM_WORLD,&request_for_left_up_corner);
            MPI_Isend(block_data, off, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size - 1, 0,MPI_COMM_WORLD,&ignored);

            //left down corner
            MPI_Irecv(left_down_corner, off, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size - 1, 0,MPI_COMM_WORLD,&request_for_left_down_corner);
            MPI_Isend(block_data + (block_height-1)*block_width, off, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size - 1, 0,MPI_COMM_WORLD,&ignored);

            //standard
            for(int i = 1; i < block_height -1; i++){
                for(int j = off; j < block_width; j++){
                    //right column
                    if(j >= block_width-off)
                        new_block_data[i][j] = right_column_filter(block_data,i,j);
                    //middle pixel
                    else
                        new_block_data[i][j] = middle_pixel_filter(block_data,i,j);
                }
            }

            MPI_Wait(&request_for_up_line, &status);
            //right up corner
            for(int k = 0; k < off; k++)
                new_block_data[0][block_width-off + k] = right_up_corner_wUpNeighbor_filter(block_data,k,up_line);

            //up line
            for(int j = off; j < block_width -off; j++)
                new_block_data[0][j] = up_line_wNeighboors_filter(block_data,j,up_line);

            MPI_Wait(&request_for_down_line, &status);
            //right down corner of block
            for(int k = 0; k < off; k++)
                new_block_data[block_height-1][block_width-off + k] = right_down_corner_wDownNeighbor_filter(block_data,k,down_line);

            //down line of block
            for(int j = off; j < block_width -off; j++)
                new_block_data[block_height-1][j] = down_line_wNeighboors_filter(block_data,j,down_line);

            MPI_Wait(&request_for_left_column, &status);
            //left column
            for(int i = 1; i < block_height -1; i++)
                for(int k = 0; k < off ; k++)
                    new_block_data[i][k] = left_column_wNeighboors_filter(block_data,i,k,left_column);

            MPI_Wait(&request_for_left_up_corner, &status);
            //up left corner
            for(int k = 0; k < off; k++)
                new_block_data[0][k] = left_up_corner_wNeighboors_filter(block_data,k,left_column,up_line,left_up_corner);

            MPI_Wait(&request_for_left_down_corner, &status);
            //left down corner
            for(int k = 0; k < off ; k++)
                new_block_data[block_height-1][k] = left_down_corner_wNeighboors_filter(block_data,k,left_column,down_line,left_down_corner);
        }
        /* The rest (middle) Processes */
        else{
            unsigned char* up_line = malloc(block_width * sizeof(unsigned char));
            unsigned char* down_line = malloc(block_width * sizeof(unsigned char));

            unsigned char* right_column = malloc(block_height * off * sizeof(unsigned char));
            unsigned char* left_column = malloc(block_height * off * sizeof(unsigned char));

            unsigned char* right_up_corner = malloc(off * sizeof(unsigned char));
            unsigned char* right_down_corner = malloc(off * sizeof(unsigned char));
            unsigned char* left_up_corner = malloc(off * sizeof(unsigned char));
            unsigned char* left_down_corner = malloc(off * sizeof(unsigned char));

            MPI_Request request_for_up_line;
            MPI_Request request_for_down_line;
            MPI_Request request_for_left_up_corner;
            MPI_Request request_for_right_up_corner;
            MPI_Request request_for_left_down_corner;
            MPI_Request request_for_right_down_corner;
            MPI_Request request_for_left_column;
            MPI_Request request_for_right_column;
            MPI_Request ignored;

            //up line
            MPI_Irecv(up_line, block_width, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size, 0,MPI_COMM_WORLD,&request_for_up_line);
            MPI_Isend(block_data, block_width, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size, 0,MPI_COMM_WORLD,&ignored);

            //left column
            MPI_Irecv(left_column, block_height * off, MPI_UNSIGNED_CHAR, world_rank - 1, 0,MPI_COMM_WORLD,&request_for_left_column);
            MPI_Isend(block_data, 1, MPI_Block_Column, world_rank - 1, 0,MPI_COMM_WORLD,&ignored);

            //down line
            MPI_Irecv(down_line, block_width, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size, 0,MPI_COMM_WORLD,&request_for_down_line);
            MPI_Isend(block_data + (block_height-1)*block_width, block_width, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size, 0,MPI_COMM_WORLD,&ignored);

            //right column
            MPI_Irecv(right_column, block_height * off, MPI_UNSIGNED_CHAR, world_rank + 1, 0, MPI_COMM_WORLD, &request_for_right_column);
            MPI_Isend(block_data + block_width - off, 1, MPI_Block_Column, world_rank + 1, 0,MPI_COMM_WORLD,&ignored);

            //left up corner
            MPI_Irecv(left_up_corner, off, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size - 1, 0,MPI_COMM_WORLD,&request_for_left_up_corner);
            MPI_Isend(block_data, off, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size - 1, 0,MPI_COMM_WORLD,&ignored);

            //left down corner
            MPI_Irecv(left_down_corner, off, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size - 1, 0,MPI_COMM_WORLD,&request_for_left_down_corner);
            MPI_Isend(block_data + (block_height-1)*block_width, off, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size - 1, 0,MPI_COMM_WORLD,&ignored);

            //right up corner
            MPI_Irecv(right_up_corner, off, MPI_UNSIGNED_CHAR, world_rank - sqrt_world_size + 1, 0,MPI_COMM_WORLD,&request_for_right_up_corner);
            MPI_Isend(block_data + block_width - off, off, MPI_UNSIGNED_CHAR,  world_rank - sqrt_world_size + 1, 0,MPI_COMM_WORLD,&ignored);

            //right down corner
            MPI_Irecv(right_down_corner, off, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size + 1, 0,MPI_COMM_WORLD,&request_for_right_down_corner);
            MPI_Isend(block_data + (block_height-1)*block_width + block_width - off, off, MPI_UNSIGNED_CHAR, world_rank + sqrt_world_size + 1, 0,MPI_COMM_WORLD,&ignored);

            //standard
            for(int i = 1; i < block_height -1; i++){
                for(int j = off; j < block_width -off; j++){
                    new_block_data[i][j] = middle_pixel_filter(block_data,i,j);
                }
            }

            MPI_Wait(&request_for_up_line, &status);
            //up line
            for(int j = off; j < block_width -off; j++)
                new_block_data[0][j] = up_line_wNeighboors_filter(block_data,j,up_line);

            MPI_Wait(&request_for_down_line, &status);
            //down line of block
            for(int j = off; j < block_width -off; j++)
                new_block_data[block_height-1][j] = down_line_wNeighboors_filter(block_data,j,down_line);


            MPI_Wait(&request_for_left_column, &status);
            //left column
            for(int i = 1; i < block_height -1; i++)
                for(int k = 0; k < off ; k++)
                    new_block_data[i][k] = left_column_wNeighboors_filter(block_data,i,k,left_column);

            MPI_Wait(&request_for_right_column, &status);
            //right column
            for(int i = 1; i < block_height -1; i++)
                for(int k = 0; k < off; k ++)
                    new_block_data[i][block_width-off + k] = right_column_wNeighboors_filter(block_data,i,k,right_column);

            MPI_Wait(&request_for_left_up_corner, &status);
            //up left corner
            for(int k = 0; k < off; k++)
                new_block_data[0][k] = left_up_corner_wNeighboors_filter(block_data,k,left_column,up_line,left_up_corner);

            MPI_Wait(&request_for_left_down_corner, &status);
            //left down corner
            for(int k = 0; k < off ; k++)
                new_block_data[block_height-1][k] = left_down_corner_wNeighboors_filter(block_data,k,left_column,down_line,left_down_corner);

            MPI_Wait(&request_for_right_up_corner, &status);
            //right up corner
            for(int k = 0; k < off; k++)
                new_block_data[0][block_width-off + k] = right_up_corner_wNeighboors_filter(block_data,k, right_column, up_line, right_up_corner);

            MPI_Wait(&request_for_right_down_corner, &status);
            //right down corner
            for(int k = 0; k < off; k++)
                new_block_data[block_height-1][block_width-off + k] = right_down_corner_wNeighboors_filter(block_data,k,right_column,down_line,right_down_corner);
        }
        for(int i=0;i<block_height;i++)
            memcpy(block_data + i*block_width, new_block_data[i], block_width);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if(world_rank == 0)
        printf("Image Processed in %f sec\n",MPI_Wtime() - start);

    /* Write modified image to file */
    MPI_File fileout;
    MPI_File_open( MPI_COMM_WORLD, "./waterfall/output_gray_all1.raw", MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fileout );

    MPI_Barrier(MPI_COMM_WORLD);
    if(world_rank == 0)
        start = MPI_Wtime();

    for(int i = 0; i < block_height; i++){ //height iterations
        MPI_File_seek(fileout,seek_offset + i * width, MPI_SEEK_SET);

        for(int j = 0; j < block_width; j++){ //width iterations
            MPI_File_write(fileout,new_block_data[i]+j,1,MPI_UNSIGNED_CHAR,&status);
        }
    }

    MPI_File_close(&fileout);

    MPI_Barrier(MPI_COMM_WORLD);
    if(world_rank == 0)
        printf("File wrote in %f sec\n",MPI_Wtime() - start);

    // Finalize the MPI environment.
    MPI_Finalize();
    return 0;
}

//TODO
//veltioseis~~~~~~~~~~~~~
//1) diavazoume apo to arxeio ana block_width oxi ana byte (ginetai me subarray???)

//MUST~~~~~~~~~~~~~~~~~~~
//1) epitixis epikoinwnia metaksu diergasiwn gia na paroun afta pou theloun
//2) efarmogh filtrou
//3) epikalipsi me prakseis

/*
bbbbb|bbbbb|bbbbb
bb0bb|bb1bb|bb2bb
bbbbb|bbbbb|bbbbb
-------------------
bbbbb|bbbbb|bbbbb
bb3bb|bb4bb|bb5bb
bbbbb|bbbbb|bbbbb
-------------------
bbbbb|bbbbb|bbbbb
bb6bb|bb7bb|bb8bb
bbbbb|bbbbb|bbbbb */
/*
printf("rank %d out of %d processors says\n",world_rank, world_size);
for(int j = 0; j < block_size; j++){
    printf("%u ",block_data[j]);
}
printf("\n\n");
*/

/*
for(int i = 0; i < block_height; i++){
    for(int j = 0; j < block_width; j++){

    }
}

ws = n
0 - sqrt(n) -> prwth grammh

//gonies
cat1 -> proc 0  panw aristera
cat2 -> sqrt(n) -1  panw deksia
cat3 -> n -1 - sqrt(n) +1  katw aristera
cat4 -> n -1    katw deksia

//akriana blocks (oxi gonies)
cat5 -> 1 ews sqrt(n) -1 -1 panw grammh
cat6 -> n - 1 - sqrt(n) + 2 ews n -1 -1 katw grammh
cat7 -> apo sqrt(n) ews n-1-sqrt(n)+1-sqrt(n)   me bhma sqrt(n)  aristera sthlh
cat8 -> apo 2*sqrt(n) -1 ews n -1 - sqrt(n)  me bhma sqrt(n) deksia sthlh

cat9 -> ta ypoloipa (ta endiamesa)
*/


    // Print off a hello world message
    //printf("Hello world from  rank %d out of %d processors\n",world_rank, world_size);
    /*
    srand(time(NULL)+world_rank);

    int *testdata = malloc(100*sizeof(int));
    for(int j = 0; j < 100; j++){
        testdata[j] = rand() % 10;
    }


    printf("rank %d out of %d processors says\n",world_rank, world_size);
    for(int j = 0; j < 100; j++){
        if(j % 10 == 0 && j != 0) putchar('\n');
        printf("%d ",testdata[j]);

    }
    printf("\n\n");
    */


/*FILE* fileout;

char *fname  = malloc(100);
char *temp  = malloc(100);
strcpy(temp,"./waterfall/output_gray_");
sprintf(fname,"%s%d.raw",temp,world_rank);
printf("fname: %s\n",fname);

//MPI_File_open( MPI_COMM_WORLD, fname, MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fileout );
fileout = fopen(fname, "w+");
fwrite(block_data, sizeof(unsigned char), block_size, fileout);

fclose(fileout);
//MPI_File_write(fileout, block_data,block_size, MPI_UNSIGNED_CHAR,&status);
//MPI_File_close(&fileout); */
/*
if(world_rank == 0){
    unsigned char* image = malloc(WIDTH * HEIGHT * sizeof(unsigned char));

    MPI_File_open( MPI_COMM_WORLD, "./waterfall/output_gray.raw", MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &file );
    unsigned char* image_block;
    for(int i = 1; i < world_size; i++){
        image_block = malloc(block_size * sizeof(unsigned char));
        MPI_Recv(image_block,block_size,MPI_UNSIGNED_CHAR,i,0,MPI_COMM_WORLD,&status);

        int k = 0;
        for(int i = 0; i < block_height; i++){ //height iterations
            MPI_File_seek(file,seek_offset + i * WIDTH, MPI_SEEK_SET);

            for(int j = 0; j < block_width; j++){ //width iterations
                MPI_File_read(file,block_data+k,1,MPI_UNSIGNED_CHAR,&status);
                image[i]
                k++;
            }
        }
    }


    MPI_File_write(fileout, const void *buf,
        //int count, MPI_Datatype datatype,MPI_Status *status);
}
else{
    MPI_Send(block_data,block_size,MPI_UNSIGNED_CHAR,0,0,MPI_COMM_WORLD);

} */

/*
const int array_of_sizes[2] = { HEIGHT, WIDTH};
const int array_of_subsizes[2] = { block_height, block_width };

const int array_of_starts[2] = { (int) (world_rank / sqrt(world_size)) * block_height, (int) (world_rank % (int)sqrt(world_size)) * block_width};
printf("starts:= x: %d y: %d\n",array_of_starts[0],array_of_starts[1]);

MPI_Datatype Block;
MPI_Type_create_subarray(2, array_of_sizes,array_of_subsizes, array_of_starts,
MPI_ORDER_C, MPI_UNSIGNED_CHAR, &Block);

MPI_Type_commit(&Block);
*/

/*
printf("Sending : ");
printf("\nL\n");
for(int i = 0; i < 40; i++)
    printf("%d ",block_data[i][block_width-1]);

printf("\nH\n");
for(int i = block_height-1; i > block_height - 40; i--)
    printf("%d ",block_data[i][block_width-1]); */

// *(block_data + (i-1)*block_width + j - off) -> *(block_data + (i-1)*block_width + j - off)
// *(block_data + (i-1)*block_width + j)   -> *(block_data + (i-1)*block_width + j)
// *(block_data + (i-1)*block_width + j + off) -> *(block_data + (i-1)*block_width + j + off)
// *(block_data + i*block_width + j - off)   -> *(block_data + i*block_width + j - off)
// block_data[i][j]     -> *(block_data + i*block_width + j)
// block_data[i][j+1]   -> *(block_data + i*block_width + j + off)
// block_data[i+1][j-1] -> *(block_data + (i+1)*block_width + j - off)
// block_data[i+1][j]   -> *(block_data + (i+1)*block_width + j)
// block_data[i+1][j+1] -> *(block_data + (i-1)*block_width + j - off)

//block_data[block_height-2][block_width-2]
//block_data[block_height-2][block_width-1]
//block_data[block_height-1][block_width-2]
//block_data[block_height-1][block_width-1]
//block_data[block_height-2][0]
//block_data[block_height-2][1]
//block_data[block_height-1][0]
//block_data[block_height-1][1]

//block_data[0][block_width-2]
//block_data[0][block_width-1]
