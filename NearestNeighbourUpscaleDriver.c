#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEPARATOR "\\"
#endif

#include "NearestNeighbourUpscaleDriver.h"

// Calculate the amount of time in seconds between the provided start and finish timespec structs.
double getElapsedTime(struct timespec start, struct timespec finish){
    return (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
}

// Functions to determine whether or not a string starts with or ends with a particular string.
bool endsWith(char *str, char *toCheck){
    int n = strlen(str);
    int cl = strlen(toCheck);
    if(n < cl){
        return false;
    }
    char *subbuff = (char *)malloc(cl * sizeof(char));
    memcpy(subbuff, &str[n - cl], cl);
    bool result = (strcmp(subbuff, toCheck) == 0);
    free(subbuff);
    return result;
}
bool startsWith(char *str, char* toCheck){
    int n = strlen(str);
    int cl = strlen(toCheck);
    if(n < cl){
        return false;
    }
    char *subbuff = (char *)malloc(cl * sizeof(char));
    memcpy(subbuff, str, cl);
    bool result = (strcmp(subbuff, toCheck) == 0);
    free(subbuff);
    return result;
}

// Construct a filepath from two strings. sizeFile is the number of valid characters in the file string.
char *path_join(char* dir, char* file, int sizeFile){
    int size = strlen(dir) + sizeFile + 2;
    char *buf = (char*)malloc(size*sizeof(char));
    if(NULL == buf){
        return NULL;
    }
    
    strcpy(buf, dir);

    // If the path does not end with the path separator then we need to add it.
    if(!endsWith(dir, PATH_SEPARATOR)){
        strcat(buf, PATH_SEPARATOR);
    }
    // If the file starts with the path separator, ignore it.
    if(startsWith(file, PATH_SEPARATOR)){
        char *filecopy = strdup(file);
        if(NULL == filecopy){
            free(buf);
            return NULL;
        }
        strcat(buf, filecopy + 1);
        free(filecopy);
    }
    else{
        strcat(buf, file);
    }
    return buf;
}

#ifdef _WIN32
char* get_current_dir_name() {
    DWORD length = GetCurrentDirectory(0, NULL);
    if (length == 0) {
        fprintf(stderr, "Error getting current directory length: %lu\n", GetLastError());
        return NULL;
    }

    char* buffer = (char*)malloc(length);
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }

    if (GetCurrentDirectory(length, buffer) == 0) {
        fprintf(stderr, "Error getting current directory: %lu\n", GetLastError());
        free(buffer);
        return NULL;
    }

    return buffer;
}
#endif

void create_dir(char* dir) {
    #ifdef _WIN32
        _mkdir(dir);
    #else 
        mkdir(dir, 0777);
    #endif
}

// Save the image to the specified file path.
void saveImg(u_char *img, int dimX, int dimY, char* file){
    // struct timespec start, finish;
    // clock_gettime(CLOCK_MONOTONIC, &start);
    unsigned error;

    if(CHANNELS_PER_PIXEL == 4){
        error = lodepng_encode32_file(file, img, dimX, dimY);
    }
    else if(CHANNELS_PER_PIXEL == 3){
        error = lodepng_encode24_file(file, img, dimX, dimY);
    }
    else{
        fprintf(stderr, "CHANNELS_PER_PIXEL in the header file is not a valid value. Must be 3 or 4.\n");
        exit(EXIT_FAILURE);
    }

    // clock_gettime(CLOCK_MONOTONIC, &finish);

    if(error){
        fprintf(stderr, "\nUnable to save the image, lodepng returned an error.\nError %u: %s\n", error, lodepng_error_text(error));
    }
    else{
        // printf("Saved to %s (%f secs)\n", file, getElapsedTime(start, finish));
    }
}

bool loadImgRGBA(u_char** img, u_int* width, u_int* height, char* filename){
    u_char *png = 0;
    size_t pngsize;
    LodePNGState state;

    lodepng_state_init(&state);

    unsigned error = lodepng_load_file(&png, &pngsize, filename);    // Load the file from disk into a temporary buffer in memory.
    if(!error){
        error = lodepng_decode(img, width, height, &state, png, pngsize);
        if(error){
            printf("Lodepng encountered an error.\nError: %u, %s\n", error, lodepng_error_text(error));
            return false;
        }
        free(png);
        lodepng_state_cleanup(&state);
        return true;
    }
    else{
        printf("Lodepng encountered an error.\nError: %u, %s\n", error, lodepng_error_text(error));
        return false;
    }
}

bool loadImgRGB(u_char** img, u_int* width, u_int* height, char* filename){
    u_char *png = 0;
    size_t pngsize;

    unsigned error = lodepng_load_file(&png, &pngsize, filename);    // Load the file from disk into a temporary buffer in memory.
    if(!error){
        error = lodepng_decode24(img, width, height, png, pngsize);
        if(error){
            printf("Lodepng encountered an error.\nError: %u, %s\n", error, lodepng_error_text(error));
            return false;
        }
        free(png);
        return true;
    }
    else{
        printf("Lodepng encountered an error.\nError: %u, %s\n", error, lodepng_error_text(error));
        return false;
    }
}

void processImage(char* inputFile, char* outputFile, int scale) {
    u_char *img = 0;    // Place to hold the original image we read from disk.
    u_int dimX = 0;    // Width of the original image.
    u_int dimY = 0;    // Height of the original image.

    // Load the image from disk.
    if(CHANNELS_PER_PIXEL == 4){
        if(!loadImgRGBA(&img, &dimX, &dimY, inputFile)){
            return;
        }
    }
    else if(CHANNELS_PER_PIXEL == 3){
        if(!loadImgRGB(&img, &dimX, &dimY, inputFile)){
            return;
        }
    }
    else{
        fprintf(stderr, "CHANNELS_PER_PIXEL in the header file is not a valid value. Must be 3 or 4.\n");
        return;
    }

    // Allocate memory for the upscaled image.
    u_char *upscaledImg = (u_char *)malloc((dimX*scale) * (dimY*scale) * CHANNELS_PER_PIXEL * sizeof(u_char));
    if(NULL == upscaledImg){
        fprintf(stderr, "Unable to allocate upscaledImg array... May have run out of RAM.\n");
        return;
    }

    // printf("Start upscaling the image...\n");

    struct timespec start, finish;
    clock_gettime(CLOCK_MONOTONIC, &start);    // Start the timer.

    if(CHANNELS_PER_PIXEL == 4){
        upscaleNN_RGBA(img, upscaledImg, (int)dimX, (int)dimY, scale);    // Upscale the RGBA image.
    }
    else if(CHANNELS_PER_PIXEL == 3){
        upscaleNN_RGB(img, upscaledImg, (int)dimX, (int)dimY, scale);    // Upscale the RGB image.
    }

    clock_gettime(CLOCK_MONOTONIC, &finish);    // Stop the timer.
    // printf("Finished upscaling the image.\t(%f secs)\n", getElapsedTime(start, finish));

    free(img);    // Free the original image.

    // printf("\nStart saving the image...\n");
    saveImg(upscaledImg, dimX*scale, dimY*scale, outputFile);    // Save the upscaled image to disk.

    free(upscaledImg);    // Free the upscaled image,
}

#ifdef _WIN32
void processDirectory(const char* inputDir, const char* outputDir, int scale) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;

    char searchPath[1024];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", inputDir);

    hFind = FindFirstFile(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Cannot open directory: %s\n", inputDir);
        return;
    }

    create_dir((char*)outputDir);

    do {
        const char* name = findFileData.cFileName;

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) {
                char path[1024];
                snprintf(path, sizeof(path), "%s\\%s", inputDir, name);
                char outPath[1024];
                snprintf(outPath, sizeof(outPath), "%s\\%s", outputDir, name);
                processDirectory(path, outPath, scale);
            }
        } else {
            char* ext = strrchr(name, '.');
            if (ext && (strcmp(ext, ".png") == 0 || strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)) {
                char inPath[1024];
                snprintf(inPath, sizeof(inPath), "%s\\%s", inputDir, name);
                char outPath[1024];
                snprintf(outPath, sizeof(outPath), "%s\\%s", outputDir, name);
                processImage(inPath, outPath, scale);
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}
#endif

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "No input directory, and/or no scale provided.\nUsage: ./upscale <INPUT_DIR> <SCALE>\n");
        return EXIT_FAILURE;
    }
    char *inputDir = argv[1];
    int scale = 0;

    char *temp;
    long value = strtol(argv[2], &temp, 10);
    if (temp != argv[2] && *temp == '\0') {
        scale = (int)value;
        if (scale < 1) {
            fprintf(stderr, "Invalid data in scale argument. Must be a non-zero integer.\nUsage: ./upscale <INPUT_DIR> <SCALE>\n");
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "Invalid data in scale argument. Must be a non-zero integer.\nUsage: ./upscale <INPUT_DIR> <SCALE>\n");
        return EXIT_FAILURE;
    }

    char outputDir[1024];
    snprintf(outputDir, sizeof(outputDir), "%s_upscaled", inputDir);
    processDirectory(inputDir, outputDir, scale);

    return EXIT_SUCCESS;
}
