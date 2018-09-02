#ifndef __PIMGLIB__
#define __PIMGLIB__

extern int off;
extern int block_width;
extern int block_height;

extern double h[3][3];

int parallel_image_filtering_engine(char *image_name, int height, int width, int channels);

//CORRECT
static inline unsigned char middle_pixel_filter(unsigned char* block_data,int i, int j){
    return
        block_data[(i-1)*block_width +j-off] * h[0][0] + block_data[(i-1)*block_width +j] * h[0][1] + block_data[(i-1)*block_width +j+off] * h[0][2] + \
        block_data[i*block_width +j-off]     * h[1][0] + block_data[i*block_width +j]     * h[1][1] + block_data[i*block_width +j+off]     * h[1][2] + \
        block_data[(i+1)*block_width +j-off] * h[2][0] + block_data[(i+1)*block_width +j] * h[2][1] + block_data[(i+1)*block_width +j+off] * h[2][2];
}


//CORRECT
static inline unsigned char right_column_wNeighboors_filter(unsigned char* block_data,int i,int k,unsigned char* right_column){
    return
        block_data[(i-1)*block_width + block_width- 2*off + k] * h[0][0] + block_data[(i-1)*block_width + block_width-off+k] * h[0][1] + right_column[(i-1)*off+k] * h[0][2] +\
        block_data[i*block_width + block_width-2*off + k] * h[1][0]      + block_data[i*block_width + block_width-off+k] * h[1][1]     + right_column[i*off+k] * h[1][2] +\
        block_data[(i+1)*block_width + block_width-2*off + k] * h[2][0]  + block_data[(i+1)*block_width + block_width-off+k] * h[2][1] + right_column[(i+1)*off+k] * h[2][2];

}

//CORRECTED (ekana replace to k me j kai sta orismata to idio)
static inline unsigned char right_column_filter(unsigned char* block_data,int i, int j){
    return
        block_data[(i-1)*block_width + j - off] * h[0][0] + block_data[(i-1)*block_width + j] * h[0][1] + block_data[i*block_width + j] * h[0][2] +\
        block_data[i*block_width + j - off] * h[1][0]     + block_data[i*block_width + j] * h[1][1]     + block_data[i*block_width + j] * h[1][2] +\
        block_data[(i+1)*block_width + j - off] * h[2][0] + block_data[(i+1)*block_width + j] * h[2][1] + block_data[i*block_width + j] * h[2][2];
}

//CORRECT
static inline unsigned char left_column_wNeighboors_filter(unsigned char* block_data,int i,int k,unsigned char* left_column){
    return
        left_column[(i-1)*off+k] * h[0][0] + block_data[(i-1)*block_width + k] * h[0][1] + block_data[(i-1)*block_width + k + off] * h[0][2] +\
        left_column[i*off+k] * h[1][0]     + block_data[i*block_width + k] * h[1][1]     + block_data[i*block_width + k + off] * h[1][2] +\
        left_column[(i+1)*off+k] * h[2][0] + block_data[(i+1)*block_width + k] * h[2][1] + block_data[(i+1)*block_width + k+off] * h[2][2];
}

//CORRECT
static inline unsigned char left_column_filter(unsigned char* block_data,int i,int j){
    return
        block_data[i*block_width + j] * h[0][0] + block_data[(i-1) * block_width + j] * h[0][1] + block_data[(i-1) * block_width + j + off] * h[0][2] +\
        block_data[i*block_width + j] * h[1][0] + block_data[i*block_width + j] * h[1][1]       + block_data[i * block_width + j + off] * h[1][2] +\
        block_data[i*block_width + j] * h[2][0] + block_data[(i+1) * block_width + j] * h[2][1] + block_data[(i+1)*block_width + j+off] * h[2][2];
}

//CORRECT
static inline unsigned char up_line_wNeighboors_filter(unsigned char* block_data,int j,unsigned char* up_line){
    return
        up_line[j-off] * h[0][0]                  + up_line[j] * h[0][1]                 + up_line[j+off] * h[0][2] +\
        block_data[j-off] * h[1][0]               + block_data[j] * h[1][1]              + block_data[j+off] * h[1][2] +\
        block_data[block_width + j-off] * h[2][0] + block_data[block_width +j] * h[2][1] + block_data[block_width +j+off] * h[2][2];
}

//CORRECT
static inline unsigned char up_line_filter(unsigned char* block_data,int i,int j){
    return
        block_data[i*block_width + j] * h[0][0]             + block_data[i*block_width + j] * h[0][1]       + block_data[i*block_width + j] * h[0][2] +\
        block_data[i*block_width + j-off]* h[1][0]          + block_data[i*block_width + j] * h[1][1]       + block_data[i * block_width + j + off] * h[1][2] +\
        block_data[(i+1) * block_width + j - off] * h[2][0] + block_data[(i+1) * block_width + j] * h[2][1] + block_data[(i+1)*block_width + j + off]* h[2][2];
}


//CORRECT
static inline unsigned char down_line_wNeighboors_filter(unsigned char* block_data,int j,unsigned char* down_line){
    return
        block_data[(block_height-2)*block_width + j - off] * h[0][0] + block_data[(block_height-2)*block_width + j] * h[0][1] + block_data[(block_height-2)*block_width + j + off] * h[0][2] +\
        block_data[(block_height-1)*block_width + j - off] * h[1][0] + block_data[(block_height-1)*block_width + j] * h[1][1] + block_data[(block_height-1)*block_width + j + off] * h[1][2] +\
        down_line[j-off] * h[2][0]                                   + down_line[j] * h[2][1]                                 + down_line[j+off] * h[2][2];
}

//CORRECT
static inline unsigned char down_line_filter(unsigned char* block_data,int i,int j){
    return
        block_data[(i-1)*block_width + j - off] * h[0][0] + block_data[(i-1)*block_width + j] * h[0][1] + block_data[(i-1)*block_width + j + off] * h[0][2] +\
        block_data[i*block_width + j - off] * h[1][0]     + block_data[i*block_width + j] * h[1][1]     + block_data[i*block_width + j + off] * h[1][2] +\
        block_data[i*block_width + j] * h[2][0]           + block_data[i*block_width + j] * h[2][1]     + block_data[i*block_width + j] * h[2][2];

}

//CORRECT
static inline unsigned char left_up_corner_wNeighboors_filter(unsigned char* block_data,int k,unsigned char* left_column,unsigned char* up_line,unsigned char* left_up_corner){
    return
        left_up_corner[k] * h[0][0]  + up_line[k] * h[0][1]                   + up_line[k+off] * h[0][2] +\
        left_column[k] * h[1][0]     + block_data[k] * h[1][1]                + block_data[k+off] * h[1][2] +\
        left_column[k+off] * h[2][0] + block_data[block_width  + k] * h[2][1] + block_data[block_width + k +off] * h[2][2];
}

//CORRECT
static inline unsigned char left_up_corner_filter(unsigned char* block_data,int j){
    return
        block_data[j] * h[0][0] + block_data[j] * h[0][1]               + block_data[j] * h[0][2] +\
        block_data[j] * h[1][0] + block_data[j] * h[1][1]               + block_data[j + off] * h[1][2] +\
        block_data[j] * h[2][0] + block_data[block_width + j] * h[2][1] + block_data[block_width + j + off] * h[2][2];
}

//CORRECTED
static inline unsigned char left_down_corner_wNeighboors_filter(unsigned char* block_data,int k,unsigned char* left_column,unsigned char* down_line,unsigned char* left_down_corner){
    return
        left_column[block_height*off-2*off+k] * h[0][0] + block_data[(block_height-2)*block_width + k] * h[0][1] + block_data[(block_height-2)*block_width + k+off] * h[0][2] +\
        left_column[block_height*off-off +k] * h[1][0]  + block_data[(block_height-1)*block_width + k] * h[1][1] + block_data[(block_height-1)*block_width + k+off] * h[1][2] +\
        left_down_corner[k] * h[2][0]                   + down_line[k] * h[2][1]                                 + down_line[k + off] * h[2][2];
}

//CORRECT
static inline unsigned char left_down_corner_filter(unsigned char* block_data,int j){
    return
        block_data[(block_height-1)*block_width + j] * h[0][0]  + block_data[(block_height-2)*block_width + j]   * h[0][1] + block_data[(block_height-2)*block_width + j + off] * h[0][2] +\
        block_data[(block_height-1)*block_width + j] * h[1][0]  + block_data[(block_height-1)*block_width + j]   * h[1][1] + block_data[(block_height-1)*block_width +j + off]  * h[1][2] +\
        block_data[(block_height-1)*block_width + j] * h[2][0]  + block_data[(block_height-1)*block_width + j]   * h[2][1] + block_data[(block_height-1)*block_width + j]      * h[2][2];

}

//CORRECT
static inline unsigned char right_up_corner_wNeighboors_filter(unsigned char* block_data,int k,unsigned char* right_column,unsigned char* up_line,unsigned char* right_up_corner){
    return
        up_line[block_width-2*off+k] * h[0][0]                  + up_line[block_width-off +k] * h[0][1]                 + right_up_corner[k] * h[0][2] +\
        block_data[block_width-2*off+k] * h[1][0]               + block_data[block_width-off+k] * h[1][1]               + right_column[k] * h[1][2] +\
        block_data[block_width + block_width-2*off+k] * h[2][0] + block_data[block_width + block_width-off+k] * h[2][1] + right_column[k+off] * h[2][2];

}

//CORRECTED
static inline unsigned char right_up_corner_filter(unsigned char* block_data,int j){
    return
        block_data[j]       * h[0][0]               + block_data[j]   * h[0][1]                      + block_data[j]  * h[0][2] +\
        block_data[j - off] * h[1][0]               + block_data[j]   * h[1][1]                      + block_data[j]  * h[1][2] +\
        block_data[block_width + j - off] * h[2][0] + block_data[block_width  + j] * h[2][1]         + block_data[j] * h[2][2];
}

//CORRECT(apla ekana aplopoihseis sta 1 2 4 5)
static inline unsigned char right_down_corner_wNeighboors_filter(unsigned char* block_data,int k,unsigned char* right_column,unsigned char* down_line,unsigned char* right_down_corner){
    return
        block_data[(block_height-1)*block_width -2*off + k] * h[0][0] + block_data[(block_height-1)*block_width - off  + k] * h[0][1] + right_column[block_height*off-2*off + k] * h[0][2] +\
        block_data[block_height*block_width - 2*off + k] * h[1][0]    + block_data[block_height*block_width -off + k] * h[1][1]       + right_column[block_height*off-off + k] * h[1][2] +\
        down_line[block_width-2*off + k] * h[2][0]                    + down_line[block_width-off + k] * h[2][1]                      + right_down_corner[k] * h[2][2];
}

//CORRECTED
static inline unsigned char right_down_corner_filter(unsigned char* block_data,int j){
    return
        block_data[(block_height-2)*block_width + j - 2*off] * h[0][0]     + block_data[(block_height-2)*block_width + j - off] * h[0][1] + block_data[(block_height-1)*block_width + j - off] * h[0][2] +\
        block_data[(block_height-1)*block_width + j - 2*off] * h[1][0]     + block_data[(block_height-1)*block_width + j - off] * h[1][1] + block_data[(block_height-1)*block_width + j - off] * h[1][2] +\
        block_data[(block_height-1)*block_width + j - off]       * h[2][0] + block_data[(block_height-1)*block_width + j - off] * h[2][1] + block_data[(block_height-1)*block_width + j - off] * h[2][2];
}

static inline unsigned char left_down_corner_wDownNeighbor_filter(unsigned char* block_data,int k,unsigned char* down_line){
    return
        block_data[(block_height-1)*block_width + k] * h[0][0] + block_data[(block_height-2)*block_width + k] * h[0][1] + block_data[(block_height-2)*block_width + k + off] * h[0][2] +\
        block_data[(block_height-1)*block_width + k] * h[1][0] + block_data[(block_height-1)*block_width + k] * h[1][1] + block_data[(block_height-1)*block_width + k + off] * h[1][2] +\
        block_data[(block_height-1)*block_width + k] * h[2][0] + down_line[k] * h[2][1]                                 + down_line[k + off] * h[2][2];
}

static inline unsigned char right_up_corner_wRightNeighbor_filter(unsigned char* block_data,int k,unsigned char* right_column){
    return
        block_data[block_width + k - off] * h[0][0]                     + block_data[block_width + k - off] * h[0][1]               + block_data[block_width + k - off] * h[0][2] +\
        block_data[block_width + k - off - off] * h[1][0]               + block_data[block_width + k - off] * h[1][1]               + right_column[k] * h[1][2] +\
        block_data[block_width + block_width + k - off - off] * h[2][0] + block_data[block_width + block_width + k - off] * h[2][1] + right_column[k+off] * h[2][2];
}

static inline unsigned char right_down_corner_wDownNeighbor_filter(unsigned char* block_data,int k,unsigned char* down_line){
    return
        block_data[(block_height-1)*block_width-2*off + k] * h[0][0] + block_data[(block_height-1)*block_width -off + k] * h[0][1] + block_data[block_height*block_width -off + k] * h[0][2] +\
        block_data[block_height*block_width -2*off + k] * h[1][0]    + block_data[block_height*block_width -off + k] * h[1][1]     + block_data[block_height*block_width -off + k] * h[1][2] +\
        down_line[block_width-2*off + k] * h[2][0]                   + down_line[block_width-off + k] * h[2][1]                    + block_data[block_height*block_width -off + k] * h[2][2];

}

static inline unsigned char left_up_corner_wLeftNeighbor_filter(unsigned char* block_data,int k,unsigned char* left_column){
    return
        block_data[k] * h[0][0]      + block_data[k] * h[0][1]               + block_data[k] * h[0][2] +\
        left_column[k] * h[1][0]     + block_data[k] * h[1][1]               + block_data[k+off] * h[1][2] +\
        left_column[k+off] * h[2][0] + block_data[block_width + k] * h[2][1] + block_data[block_width + k + off] * h[2][2];
}

static inline unsigned char left_up_corner_wUpNeighbor_filter(unsigned char* block_data,int k,unsigned char* up_line){
    return
        block_data[k] * h[0][0] + up_line[k] * h[0][1]                  + up_line[k+off] * h[0][2] +\
        block_data[k] * h[1][0] + block_data[k] * h[1][1]               + block_data[k+off] * h[1][2] +\
        block_data[k] * h[2][0] + block_data[block_width + k] * h[2][1] + block_data[block_width + k + off] * h[2][2];
}

static inline unsigned char right_down_corner_wRightNeighbor_filter(unsigned char* block_data,int k,unsigned char* right_column){
    return
        block_data[(block_height-1)*block_width-2*off + k] * h[0][0] + block_data[(block_height-1)*block_width+k -off] * h[0][1] + right_column[block_height*off-2*off +k] * h[0][2] +\
        block_data[block_height*block_width-2*off + k] * h[1][0]     + block_data[block_height*block_width -off + k] * h[1][1]   + right_column[block_height*off-off +k] * h[1][2] +\
        block_data[block_height*block_width -off + k] * h[2][0]      + block_data[block_height*block_width -off + k] * h[2][1]   + block_data[block_height*block_width -off + k] * h[2][2];

}

static inline unsigned char right_up_corner_wUpNeighbor_filter(unsigned char* block_data,int k,unsigned char* up_line){
    return
        up_line[block_width-2*off +k] * h[0][0]                 + up_line[block_width-off +k] * h[0][1]                 + block_data[block_width-off +k] * h[0][2] +\
        block_data[block_width-2*off+k] * h[1][0]               + block_data[block_width-off +k] * h[1][1]              + block_data[block_width-off +k] * h[1][2] +\
        block_data[block_width + block_width-2*off+k] * h[2][0] + block_data[block_width + block_width-off+k] * h[2][1] + block_data[block_width-off +k] * h[2][2];
}

static inline unsigned char left_down_corner_wLeftNeighbor_filter(unsigned char* block_data,int k,unsigned char* left_column){
    return
        left_column[block_height*off-2*off+k] * h[0][0]            + block_data[(block_height-2)*block_width + k] * h[0][1] + block_data[(block_height-2)*block_width + k +off] * h[0][2] +\
        left_column[block_height*off-off+k] * h[1][0]              + block_data[(block_height-1)*block_width + k] * h[1][1] +  block_data[(block_height-1)*block_width + k +off] * h[1][2] +\
        block_data[(block_height-1)*block_width + k] * h[2][0]     + block_data[(block_height-1)*block_width + k] * h[2][1] + block_data[(block_height-1)*block_width + k] * h[2][2];
}

#endif
