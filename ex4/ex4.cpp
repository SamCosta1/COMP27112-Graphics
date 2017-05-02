#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <vector>
#include <algorithm>
#include <set>
#include <stack>
#include <iostream>

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


void invertBlack(unsigned char *image, int size) {
    for (int i = 0; i < size; i++)
        if (image[i] == 0)
            image[i] = 255;
}
int * generateHistogram(unsigned char *image, int noPixels) {
    int *histogram = (int *)malloc(256 * sizeof(int));
    for (int i = 0; i < 256; i++) 
        histogram[i] = 0;
    for (int i = 0; i < noPixels; i++) 
        histogram[image[i]]++;

    return histogram;
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



void printNumLabels(unsigned char *labels, int size) {
    int numLabels = 0;

    int* histogram = generateHistogram(labels, size);

    for (int i = 1; i < 256; i++)
        if (histogram[i] != 0)
            numLabels++;
    printf("No Labels: %d\n", numLabels);
}

unsigned char* invert(unsigned char *image, int size) {
    unsigned char *newImage = (unsigned char *)malloc(sizeof(char) * size);

    for (int i = 0; i < size; i++) {
        newImage[i] = 255 - image[i];
    }

    return newImage;
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

    if (newOne->size() > 0)
        eqTable.insert(newOne);
}

void adjustContrast(unsigned char *image, int size, int multiplier) {
    
    for (int i = 0; i < size; i++)
        image[i] = image[i] * multiplier > 255 ? 255 : image[i] * multiplier;
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
                break;
            }
            
            index++;
        }
    }
int index = -1;
    for (int i = 0; i < size; i++) {
        if (labelled[i] <= 0)
            continue;
        int thisLabel = labelled[i];
        
        for (int j = i; j < size; j++)
            if (labelled[j] == thisLabel)
                labelled[j] = index;
        index--;        
    }

    for (int i = 0; i < size; i++)
        labelled[i] *= -1;
    
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
    }/*
 for (set<set<int>* >::iterator it = eqTable.begin(); it != eqTable.end(); ++it) {
     set<int>* s = *it;
     printSet(s);
     break;
 }*/

}

unsigned char* reduceNoise(unsigned char *labels, unsigned char *image, int size) {
    int* histogram = generateHistogram(labels, size);

    for (int i = 0; i < size; i++) {
        if (histogram[labels[i]] < 60) {
            image[i] = 255;
            labels[i] = 0;
        }
    }

    return labels;
}

unsigned char* subtract(unsigned char *image1, unsigned char *image2, int size) {
    unsigned char *newImage = (unsigned char *)malloc(sizeof(char) * size);

    for (int i = 0; i < size; i++) {
        int diff = image1[i] - image2[i];
        newImage[i] = diff < 0 ? 0 : diff;
    }

    return newImage;
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

unsigned char* CCA(unsigned char* image, int width, int height) {
    eqTable.clear();
    int* labelled = (int *)malloc(sizeof(int) * width * height);

    for (int i = 0; i < width * height; i++) 
        if (image[i] == 255)
            labelled[i] = 0;
        else
            labelled[i] = -1;
    
    
    int nextFreeLabel = 1;
 
   
    int *neighbourLabels = (int *)malloc(sizeof(int) * 3);
    for (int index = 0; index < width * height; index++) {
        if (image[index] == 255)             
            continue;
        
        int neighbourLabels[4];

        getNeighbourLabels(neighbourLabels, labelled, width, height, index);
        
        int highest = max(neighbourLabels);
        
        labelled[index] = highest <= 0 ? nextFreeLabel++ : highest;
        
        if (highest > 0)
            addToEqTable(neighbourLabels);        
    }
    
    refactor();

    relabel(labelled, width * height);
    unsigned char *charLabels = toCharArray(labelled, height * width);
    reduceNoise(charLabels, image, width * height);
    printNumLabels(charLabels, width*height);

    return charLabels;
}

unsigned char* threshold(unsigned char* image, int width, int height) {
    int* histogram = generateHistogram(image, width * height);

    int threshold = calculateThreshold(histogram, width, height);

    unsigned char *newImage = (unsigned char *)malloc(sizeof(char) * width * height);
    for (int i = 0; i < width * height; i++) {
        newImage[i] = image[i] > threshold ? 255 : 0;
    }

    printf("threshold: %d, width: %3d, height: %3d\n", threshold, width, height);

    return newImage;
}


bool isAHit(unsigned char* image, int structElement[3][3], int index, int width) {
    index = index - width - 1;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
        
            if (image[index] == 255 && structElement[i][j] == 1)
                return false;
            index++;
        }
        index += width - 3;
    }
    return true;
}
unsigned char * erode(unsigned char* image, int width, int height, int noTimes) {
    unsigned char *newImage = (unsigned char *)malloc(sizeof(char) * width * height);

    int structElement[3][3] = { {0, 1, 0},
                                {1, 1, 1},
                                {0, 1, 0} };
    
    for (int i = width + 1; i < width * height - width - 1; i++) {

        if (isRightMostColumn(i, width, height) || isLeftMostColumn(i, width, height)) 
            continue;

        newImage[i] = isAHit(image, structElement, i, width) ? 0 : 255;  
    }

    if (noTimes == 1)
        return newImage;
    else
        return erode(newImage, width, height, noTimes - 1);
}

void fillHole(unsigned char *in, unsigned char* out, int width, int height, int index) {
    int count = 0;
    stack<int> pixels;
    pixels.push(index);

    while (pixels.size() != 0) {

        int thisIndex = pixels.top();
        pixels.pop();

        if (out[thisIndex] == 255)
            continue;

        out[thisIndex] = 255;

        if (!isTopRow(thisIndex, width, height) && in[thisIndex - width] == 255 
                                            && out[thisIndex - width] != 255) 
            pixels.push(thisIndex - width);

        if (!isLeftMostColumn(thisIndex, width, height) && in[thisIndex - 1] == 255 
                                                        && out[thisIndex - 1] != 255)
            pixels.push(thisIndex - 1);

        if (!isRightMostColumn(thisIndex, width, height) && in[thisIndex + 1] == 255
                                                         && out[thisIndex + 1] != 255)
            pixels.push(thisIndex + 1);

        if (!isBottomRow(thisIndex, width, height) && in[thisIndex + width] == 255
                                                   && out[thisIndex + width] != 255)
            pixels.push(thisIndex + width);
            

       
    }
}

unsigned char *fillHoles(unsigned char *image, int width, int height) {
    unsigned char *newImage = (unsigned char *)malloc(sizeof(char) * width * height);
    
    for (int i = 0; i < width * height; i++)
        newImage[i] = 0;

    fillHole(image, newImage, width, height, 0);
    return newImage;

}

unsigned char* highlightRoundOnes(unsigned char *image, unsigned char *labels, unsigned char* subtractedLabels, int size) {
    double fourPi = 12.5663706144;

    int* labHist  = generateHistogram(labels, size);
    int* subHist = generateHistogram(subtractedLabels, size);

    unsigned char *newImage = (unsigned char *)malloc(sizeof(char) * size);

/*
    for (int i = 0; i < 256; i++) 
        printf("%d   %d\n", labHist[i], subHist[i]);*/

    for (int i = 0; i < size; i++) {
        if (image[i] == 0)
            continue; 

        double p2A = pow(subHist[labels[i]], 2) / labHist[labels[i]];
        
        double roundness = abs(p2A - fourPi);
printf("roundness: %f %f\n", roundness, p2A);
        newImage[i] = roundness < 24 ? 255 : 50; 
    }

    return newImage;

}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Usage: ./ex1 image_in.jpg image_out.jpg\n");
    return 1;
  }

  unsigned char *image;
  unsigned char *labels;
  unsigned char *thresholded;
  unsigned char *eroded;
  unsigned char *subtracted;
  unsigned char *highlighted;
  unsigned char *inverted;
  unsigned char *subtractedLabels;

  int width, height, channels;
  read_JPEG_file(argv[1], &width, &height, &channels, &image);
  image = medianFilter(image, width, height); 
  
  thresholded = fillHoles(threshold(image, width, height), width, height);

  labels = CCA(thresholded, width, height);

  eroded = erode(thresholded, width, height, 2);

  subtracted = subtract(eroded, thresholded, width * height);
  inverted = invert(subtracted, width * height);  

  
  subtractedLabels = CCA(inverted, width, height);

  highlighted = highlightRoundOnes(subtractedLabels, labels, subtractedLabels, width * height);


  write_JPEG_file(argv[2], width, height, channels, thresholded, 95);
  write_JPEG_file(argv[3], width, height, channels, labels, 95);
  write_JPEG_file(argv[4], width, height, channels, eroded, 95);
  write_JPEG_file(argv[5], width, height, channels, subtracted, 95);
  write_JPEG_file(argv[6], width, height, channels, inverted, 95);
  write_JPEG_file(argv[7], width, height, channels, subtractedLabels, 95);
  write_JPEG_file(argv[8], width, height, channels, highlighted, 95);
  
  return 0;

}
