#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <math.h>   
#include <algorithm>
#include <vector>
using namespace std;

/** Read the JPEG image at `filename` as an array of bytes.
  Data is returned through the out pointers, while the return
  value indicates success or failure.
  NOTE: 1) if image is RGB, then the bytes are concatenated in R-G-B order
        2) `image` should be freed by the user
 */
static inline int
read_JPEG_file(char *filename,
               int *width, int *height, int *channels, unsigned char *(image[]))
{
  FILE *infile;
  if ((infile = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    return 0;
  }

  struct jpeg_error_mgr jerr;
  struct jpeg_decompress_struct cinfo;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, infile);
  (void) jpeg_read_header(&cinfo, TRUE);
  (void) jpeg_start_decompress(&cinfo);

  *width = cinfo.output_width, *height = cinfo.output_height;
  *channels = cinfo.num_components;
  // printf("width=%d height=%d c=%d\n", *width, *height, *channels);
  *image = (unsigned char*)malloc(*width * *height * *channels * sizeof(*image));
  JSAMPROW rowptr[1];
  int row_stride = *width * *channels;

  while (cinfo.output_scanline < cinfo.output_height) {
    rowptr[0] = *image + row_stride * cinfo.output_scanline;
    jpeg_read_scanlines(&cinfo, rowptr, 1);
  }
  jpeg_finish_decompress(&cinfo);

  jpeg_destroy_decompress(&cinfo);
  fclose(infile);
  return 1;
}


/** Writes the image in the specified file.
  NOTE: works with Grayscale or RGB modes only (based on number of channels)
 */
static inline void
write_JPEG_file(char *filename, int width, int height, int channels,
                unsigned char image[], int quality)
{
  FILE *outfile;
  if ((outfile = fopen(filename, "wb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    exit(1);
  }

  struct jpeg_error_mgr jerr;
  struct jpeg_compress_struct cinfo;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  jpeg_stdio_dest(&cinfo,outfile);

  cinfo.image_width = width;
  cinfo.image_height = height;
  cinfo.input_components = channels;
  cinfo.in_color_space = channels == 1 ? JCS_GRAYSCALE : JCS_RGB;
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE);

  jpeg_start_compress(&cinfo, TRUE);
  JSAMPROW rowptr[1];
  int row_stride = width * channels;
  while (cinfo.next_scanline < cinfo.image_height) {
    rowptr[0] = & image[cinfo.next_scanline * row_stride];
    jpeg_write_scanlines(&cinfo, rowptr, 1);
  }
  jpeg_finish_compress(&cinfo);

  fclose(outfile);
  jpeg_destroy_compress(&cinfo);
}

bool isTopRow(int i, int width, int height) {
    return i < width;
}

bool isBottomRow(int i, int width, int height) {
    return i >= (width-1) * height;
}

bool isLeftMostColumn(int i, int width, int height) {
    return i % width == 0;
}

bool isRightMostColumn(int i, int width, int height) {
    return (i + 1) % width == 0;
}

int convolve(unsigned char* image, int i, int tmplt[3][3], int width, int height) { 
    int sum = 0;

    sum += image[i];
    if (!isTopRow(i, width,height)) {  // One pixel up
        sum += image[i - width] * tmplt[0][1];
        
        if (!isLeftMostColumn(i, width, height))  // One up, one left 
            sum += image[i - width - 1] * tmplt[0][0];
            
        if (!isRightMostColumn(i, width,height))  // One up one right
            sum += image[i - width + 1] * tmplt[0][2];
            
    }

    if (!isLeftMostColumn(i, width,height))  // One left
        sum += image[i - 1] * tmplt[1][0];
        

    if (!isRightMostColumn(i, width, height))  // One right        
        sum += image[i + 1] * tmplt[1][2];
  
    if (i < (width-1) * height) {  // One pixel down
        sum += image[i + width] * tmplt[2][1];
        
        if (!isLeftMostColumn(i, width, height))  // One down, one left            
            sum += image[i + width - 1] * tmplt[2][0];
            
        if (!isRightMostColumn(i, width, height))  // One down one right
            sum += image[i + width + 1] * tmplt[2][2];                
        
    }

    return sum;
}

unsigned char * edge_detect(unsigned char *image, int width, 
                                                  int height,
                                                  unsigned char *magImage,
                                                  unsigned char *orientationImage) {
    int horiz[3][3] = { {-1, -1, -1},
                        {0 ,  0,  0},
                        {1 ,  1,  1}};
    int verti[3][3] = { {-1, 0, 1},
                        {-1, 0, 1},
                        {-1, 0, 1}};
    
    
    for (int i = 0; i < width * height; i++) {
        int horizontal = convolve(image, i, horiz, width, height);        
        int vertical = convolve(image, i, verti, width, height);

        int mag = (int)sqrt(pow(horizontal, 2) + pow(vertical, 2));
        int ori = 255;

        if (horizontal != 0) {
            ori = (int)(atan(vertical / horizontal) * 255);
            ori = ori > 255 ? 255 : ori;
        } 

        magImage[i] = mag > 255 ? 255 : 0;
        orientationImage[i] = ori;

    }
    return image;
}

vector<int>* getMedianVal(unsigned char* image, int i, int width) {
    vector<int>* vec = new vector<int>;

    int index = i - width *2 - 2;
    for (int j = 0; j < 5; j++) {
       for (int l = 0; l < 5; l++) {
          vec->push_back(image[index]);
          index += 1;
       }

       index += width - 5;
    }

    return vec;
}

unsigned char* median(unsigned char* image, int width, int height) {
    unsigned char* newImage = (unsigned char *)malloc(sizeof(char) * width * height);

    for (int i = width * 2 + 2; i < (width-2) * height - 2; i++) {
        if (i + 2 % width == 0)
           i += 4;

        vector<int> *pixelVals = getMedianVal(image, i, width);
        sort(pixelVals->begin(), pixelVals->end());

        newImage[i] = pixelVals->at(13);

    }

    return newImage;
}

int main(int argc, char *argv[])
{
  if (argc < 3) {
    printf("Usage: ./ex1 image_in.jpg image_out.jpg\n");
    return 1;
  }

  unsigned char *image;
  int width, height, channels, i;
  read_JPEG_file(argv[1], &width, &height, &channels, &image);

  unsigned char* magImage = (unsigned char *)malloc(sizeof(char) * width * height);
  unsigned char* orientationImage = (unsigned char *)malloc(sizeof(char) * width * height);
  edge_detect(image, width, height, magImage, orientationImage);

  write_JPEG_file(argv[2], width, height, channels, magImage, 95);
  write_JPEG_file(argv[3], width, height, channels, orientationImage, 95);
  
  write_JPEG_file(argv[4], width, height, channels, median(image, width, height), 95);

  return 0;
}
