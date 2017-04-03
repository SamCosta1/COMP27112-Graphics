#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>


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
  *image = (unsigned char *)malloc(*width * *height * *channels * sizeof(*image));
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

void generateHistogram(int *histogram, unsigned char *image, int noPixels) {
    for (int i = 0; i < 256; i++) 
        histogram[i] = 0;
    for (int i = 0; i < noPixels; i++) 
        histogram[image[i]]++;
    for (int i = 0; i < 256; i++) {
     //   printf("%3d: %d\n", i, histogram[i]);
    }
}

int getLowestValue(int *histogram) {
    int i = 0;
    while (histogram[i] <= 0)
        i++;
    return i;
}

int average(int *histogram, int bottom, int top) {
    int noPixels = 0;
    int total = 0;

    for (int i = bottom; i < top; i++) {
        total += i * histogram[i];
        noPixels += histogram[i];
    }

    if (noPixels == 0)
        return 0;

    return total / noPixels;
}


int calculateThreshold(int *histogram, int width, int height) {
    int threshold = getLowestValue(histogram);
    int newVal = -1;
    while (threshold != newVal) {
        int lower = average(histogram, 0, threshold);
        int heigher = average(histogram, threshold, 256);
        newVal = (lower + heigher) / 2;       
        threshold++;
   }

    return threshold - 1;
}

Boolean isTopRow(int index, unsigned char *image, int width, int height) {
    return i < width;
}

Boolean isBottomRow(int index, unsigned char *image, int width, int height) {
    return i >= (width-1) * height
}

Boolean isLeftMostColumn(int index, unsigned char *image, int width, int height) {
    return i % width == 0;
}

Boolean isRightMostColumn(int index, unsigned char *image, int width, int height) {
    return (i + 1) % width == 0;
}

unsigned char* medianFilter(unsigned char *image, int width, int height) {
    unsigned char* newImg = (unsigned char *)malloc(sizeof(char) * width * height);
    for (int i = 0; i < width * height; i++) {
        int numAdded = 1;
        int sum = 0;

        sum += image[i];
        if (!isTopRow(i, image, width,height)) {  // One pixel up

            sum += image[i - width];
            numAdded++;
            if (!isLeftMostColumn(i, image, width,height)) { // One up, one left 
                sum += image[i - width - 1]; 
                numAdded++;
            }
            if (!isRightMostColumn(i, image, width,height)) { // One up one right
                sum += image[i - width + 1];
                numAdded++;
            }
        }

        if (!isLeftMostColumn(i, image, width,height)) { // One left
            sum += image[i - 1]; 
            numAdded++;
        }

        if (!isRightMostColumn(i, image, width,height)) { // One right        
            sum += image[i + 1]; 
            numAdded++;
        }

        if (i < (width-1) * height) {  // One pixel down
            sum += image[i + width]; 
            numAdded++;
            if (!isLeftMostColumn(i, image, width,height)) {  // One down, one left            
                sum += image[i + width - 1]; 
                numAdded++;
            }
            if (!isRightMostColumn(i, image, width,height)) { // One dow one right
                sum += image[i + width + 1]; 
                numAdded++;
            }
        }

        newImg[i] = sum / numAdded;    
    }

    return newImg;
}

unsigned char* CCA(unsigned char* image, int width, int height) {
    unsigned char* labelled = (unsigned char *)malloc(sizeof(char) * width * height);
    
    return labelled;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Usage: ./ex1 image_in.jpg image_out.jpg\n");
    return 1;
  }

  unsigned char *image;
  int width, height, channels;
  read_JPEG_file(argv[1], &width, &height, &channels, &image);

  int histogram[256];
  generateHistogram(histogram, image, width * height);

  int threshold = calculateThreshold(histogram, width, height);

  for (int i = 0; i < width * height; i++) {
    image[i] = image[i] > threshold ? 255 : 0;
  }
  
  //image = medianFilter(image, width, height);
  image = CCA(image);
  printf("threshold: %d, width: %3d, height: %3d\n", threshold, width, height);
  write_JPEG_file(argv[2], width, height, channels, image, 95);

  return 0;
}
