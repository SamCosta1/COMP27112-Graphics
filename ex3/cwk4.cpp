#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <vector>
#include <algorithm>
#include <set>

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


unsigned char* blurFilter(unsigned char *image, int width, int height) {
    unsigned char* newImg = (unsigned char *)malloc(sizeof(char) * width * height);
    for (int i = 0; i < width * height; i++) {
        int numAdded = 1;
        int sum = 0;

        sum += image[i];
        if (!isTopRow(i, width,height)) {  // One pixel up
            sum += image[i - width];
            numAdded++;
            if (!isLeftMostColumn(i, width, height)) { // One up, one left 
                sum += image[i - width - 1]; 
                numAdded++;
            }
            if (!isRightMostColumn(i, width,height)) { // One up one right
                sum += image[i - width + 1];
                numAdded++;
            }
        }

        if (!isLeftMostColumn(i, width,height)) { // One left
            sum += image[i - 1]; 
            numAdded++;
        }

        if (!isRightMostColumn(i, width, height)) { // One right        
            sum += image[i + 1]; 
            numAdded++;
        }

        if (i < (width-1) * height) {  // One pixel down
            sum += image[i + width]; 
            numAdded++;
            if (!isLeftMostColumn(i, width, height)) {  // One down, one left            
                sum += image[i + width - 1]; 
                numAdded++;
            }
            if (!isRightMostColumn(i, width, height)) { // One dow one right
                sum += image[i + width + 1]; 
                numAdded++;
            }
        }

        newImg[i] = sum / numAdded;    
    }

    return newImg;
}

unsigned char* medianFilter(unsigned char *image, int width, int height) {
    unsigned char* newImg = (unsigned char *)malloc(sizeof(char) * width * height);
    for (int i = 0; i < width * height; i++) {
        int numAdded = 0;
        int values[9];

        values[numAdded++] = image[i];
        if (!isTopRow(i, width,height)) {  // One pixel up
            values[numAdded++] = image[i - width];
            
            if (!isLeftMostColumn(i, width, height))  // One up, one left 
                values[numAdded++] = image[i - width - 1]; 
              
            if (!isRightMostColumn(i, width,height))  // One up one right
                values[numAdded++] = image[i - width + 1];               
        }

        if (!isLeftMostColumn(i, width,height))  // One left
            values[numAdded++] = image[i - 1]; 
          
        

        if (!isRightMostColumn(i, width, height))  // One right        
            values[numAdded++] = image[i + 1]; 

        if (i < (width-1) * height) {  // One pixel down
            values[numAdded++] = image[i + width]; 
            
            if (!isLeftMostColumn(i, width, height))   // One down, one left            
                values[numAdded++] = image[i + width - 1]; 
            
            
            if (!isRightMostColumn(i, width, height))  // One down one right
                values[numAdded++] = image[i + width + 1]; 
               
        }
        
        std::sort(values, values+numAdded);        
        newImg[i] = numAdded % 2 == 0 ? (values[numAdded / 2] + values[(numAdded / 2) + 1]) / 2
                                      : values[numAdded / 2]; 
        
    }

    return newImg;
}

unsigned char* toCharArray(int* labelled, int size) {
    unsigned char *array = (unsigned char *)malloc(size * sizeof(unsigned char *));
    for (int i = 0; i < size; i++)
        array[i] = labelled[i];
    return array;
}

void getNeighbourLabels(int *neighbourLabels, int *labelled, int width, int height, int i) {
    int numLabels = 1;       
    for (int j = 0; j < 4; j++)
        neighbourLabels[j] = -1;

    if (!isTopRow(i, width,height)) {  // One pixel up            
        
        neighbourLabels[numLabels++] = labelled[i - width];
        if (!isLeftMostColumn(i, width, height))  // One up, one left 
            neighbourLabels[numLabels++] = labelled[i - width - 1];
        
        if (!isRightMostColumn(i, width,height))  // One up one right
            neighbourLabels[numLabels++] = labelled[i - width + 1];
    }

    if (!isLeftMostColumn(i, width,height)) // One left
        neighbourLabels[numLabels++] = labelled[i-1];
}


int max(int *neighbourLabels) {
    int highest = -1;
    for (int i = 0; i < 4; i++) {
        if (highest < neighbourLabels[i])
            highest = neighbourLabels[i];
    }
    return highest;
}

bool contains(set<int>* set, int val) {
    return set->find(val) != set->end();
}

void addAll(set<int>* base, set<int>* toAdd) {
    base->insert(toAdd->begin(), toAdd->end());
}

void printSet(set<int> *set1) {
    for (set<int>::iterator it = set1->begin(); it != set1->end(); ++it) 
        printf(" %d ", *it);
    printf("\n");
}

bool hasEmptyUnion(set<int>* set1, set<int>* set2) {
    for (set<int>::iterator it = set1->begin(); it != set1->end(); ++it) 
        if (contains(set2, *it))
            return false;
    return true;
}

set<set<int>* > eqTable;

void addToEqTable(int * v) {
    set<int>* newOne = new set<int>;
    
    for(int i = 0; i < 4; i++)
        if (v[i] > 0)
            newOne->insert(v[i]);  

    if (newOne->size() > 1)
        eqTable.insert(newOne);
}

void adjustContrast(unsigned char *image, int size) {
    int max = 0;
    for (int i = 0; i < size; i++)
        if (max < image[i])
            max = image[i];

    if (max == 0)
        return;

    int multiplier = 255 / max;

    
    for (int i = 0; i < size; i++)
        image[i] = image[i] * multiplier;
}

void relabel(int *labelled, int size) {

    for (int i = 0; i < size; i++) {
        if (labelled[i] == 0)
            continue;

        int index = 1;
        for (set<set<int>* >::iterator it = eqTable.begin(); it != eqTable.end(); ++it) {
            set<int>* s = *it;

            if (contains(s, labelled[i])) {
                labelled[i] = index;
                continue;
            }
            
            index++;

        }
    }
}

void refactor() {
    int index = 0;
    for (set<set<int>* >::iterator it = eqTable.begin(); it != eqTable.end(); ++it) {
	    set<int>* baseSet = *it;
        index++;

        for (set<set<int>* >::iterator it2 = eqTable.begin(); it2 != eqTable.end(); ++it2) {
            if (it == it2)
                continue;

            set<int>* s = *it2;
            

            if (!hasEmptyUnion(s, baseSet)) {                
                addAll(baseSet, s);
                eqTable.erase(it2);
               
            } 

            
            
        }       
    }


}

unsigned char* CCA(unsigned char* image, int width, int height) {
    int* labelled = (int *)malloc(sizeof(int) * width * height);
    for (int i = 0; i < width * height; i++) {
        labelled[i] = -1;
    }
    
    int nextFreeLabel = 1;
    int index = 0;
   
    int *neighbourLabels = (int *)malloc(sizeof(int) * 3);
    for (int index = 0; index < width * height; index++) {
        int* neighbourLabels = (int *)malloc(sizeof(int) * 4);
        if (image[index] == 255) {
            labelled[index] = 0;
            continue;
        }
        getNeighbourLabels(neighbourLabels, labelled, width, height, index);
        
        int highest = max(neighbourLabels);
        
        labelled[index] = highest <= 0 ? nextFreeLabel++ : highest;
        
        if (highest > 0)
            addToEqTable(neighbourLabels);        
    }
    
    refactor();

    relabel(labelled, width * height);
    return toCharArray(labelled, height * width);
}

void threshold(unsigned char* image, int width, int height) {
    int histogram[256];
    generateHistogram(histogram, image, width * height);

    int threshold = calculateThreshold(histogram, width, height);

    for (int i = 0; i < width * height; i++) {
        image[i] = image[i] > threshold ? 255 : 0;
    }
    printf("threshold: %d, width: %3d, height: %3d\n", threshold, width, height);
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Usage: ./ex1 image_in.jpg image_out.jpg\n");
    return 1;
  }

  unsigned char *image;
  int width, height, channels;
  read_JPEG_file(argv[1], &width, &height, &channels, &image);
   
  threshold(image, width, height);
  image = medianFilter(image, width, height);
  image = CCA(image, width, height);

  adjustContrast(image, width * height);
  
  printf("Num Distinct labels: %d\n", eqTable.size());

  write_JPEG_file(argv[2], width, height, channels, image, 95);
  
  return 0;
}
