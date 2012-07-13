//
//  main.cpp
//  yandextest
//
//  Created by Andrey Yunoshev on 01.07.12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//
#define __GXX_EXPERIMENTAL_CXX0X__

#include <iostream>
#include <sys/time.h>
#include <stdint.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <errno.h>

using namespace std;

#define BUCKET_FILES_TEMPLATE "radix.data.%d.%.3zu"

template<typename T>
struct FileRadixBuffer
{
    union {
        unsigned char valueBytes[sizeof(T)];
        T value;
    };
    static const size_t buffersCount = 256;
    size_t maxElementsInBuffer;
    
    vector<T> buffers[buffersCount];
    
    FileRadixBuffer(size_t sizeInBytes)
    {
        maxElementsInBuffer = sizeInBytes/(sizeof(T) * buffersCount);
        printf("\tcreate %zu buffers, each size %2.2fmb/%zu elements\n", buffersCount, (double)sizeInBytes / (buffersCount * 1024 * 1024), maxElementsInBuffer);

        for (int i =0; i<buffersCount; i++)
            buffers[i].reserve(maxElementsInBuffer);
    }

    void flushBuffer(size_t bufferIndex, int pos)
    {
        char buffer[64];
        sprintf(buffer, BUCKET_FILES_TEMPLATE, pos, bufferIndex);
        //printf("flush radix buffer to file %s (size %zu elements)\n", buffer, counters[bufferIndex]);
        std::ofstream out(buffer, std::ios::out | std::ios::binary | std::ios::app);
        // C-style save
        out.write((char*)&buffers[bufferIndex][0], buffers[bufferIndex].size() * sizeof(T));
        out.close();
        buffers[bufferIndex].resize(0);
    }
    
    void addValue(T _value, int pos)
    {
        value = _value;
        int bufferIndex = valueBytes[pos];
//        if (pos == 1) printf("value %X at pos %i byte %X counter %zu (%2.2f%%)\n", value, pos, bufferIndex, counters[bufferIndex], counters[bufferIndex] * 100.0f / maxElementsInBuffer);
        buffers[bufferIndex].push_back(value);
        if (buffers[bufferIndex].size() >= buffers[bufferIndex].capacity())
            flushBuffer(bufferIndex, pos);
    }
    
    void flushAll(int pos)
    {
        for (int i =0; i<buffersCount; i++)
            flushBuffer(i, pos);
    }
    
};

template<typename T>
void bufferToRadixBuckets(T* inBuffer,  size_t count, FileRadixBuffer<T> &fileRadixBuffer, int pos)
{
    for (size_t counter = 0; counter < count; counter++)
        fileRadixBuffer.addValue(inBuffer[counter], pos);
}


template<typename T>
void fileToRadixBuckets(std::string fileName, FileRadixBuffer<T> &fileRadixBuffer, T* inBuffer, size_t bufferSizeInElements, int pos)
{
    std::ifstream in(fileName.c_str(), std::ios::in | std::ios::binary);
    if(in.is_open())
    {
        printf("\tread file %s (using buffer by %ld elements) and fill radix buffers by pos %i..",fileName.c_str(), bufferSizeInElements, pos);
        //in.seekg(0, std::ios_base::end);
        //size_t inLen = in.tellg();
        //in.seekg(0, std::ios_base::beg);        
        //printf("\tfile size is %zu elements\n", inLen/sizeof(T));
        size_t readElementsCount = 0;
        size_t readTotalInBytes = 0;
        //if (inLen > 0)
        do
        {
            // C-style load
            in.read((char*)inBuffer, bufferSizeInElements * sizeof(T) );
            readTotalInBytes += in.gcount();
            readElementsCount = in.gcount() / sizeof(T);
            if (readElementsCount > 0)
            {
                //printf("\t\tread buffer, size %ld elements (%2.2f%%)\r", readElementsCount, readTotalInBytes * 100.0f / inLen);
                bufferToRadixBuckets<T>(inBuffer, readElementsCount, fileRadixBuffer, pos);
            }
        } while (!in.eof());
        printf("\r");
        
    } else {
        printf("Can`t open file %s\n", fileName.c_str());
    }
    
        
    
}

template<typename T>
void fileRadixFileSort(std::vector<std::string> fileNames, FileRadixBuffer<T> &fileRadixBuffer, T* inBuffer, size_t bufferSizeInElements, int pos = 0)
{
    printf("Iteration %i/%zu\n", pos, sizeof(T));
    printf("begin read %lu files (first %s), and sort at %i pos....\n", 
           fileNames.size(), fileNames[0].c_str(), pos);
    int bucketCount = fileRadixBuffer.buffersCount;
    std::vector<std::string> bucketNames;
    bucketNames.reserve(bucketCount);
    char buffer[64];
    for (size_t i =0; i<bucketCount; i++)
    {
        sprintf(buffer, BUCKET_FILES_TEMPLATE, pos, i);
        bucketNames.push_back(buffer);
    }
    
    for(int i =0; i<fileNames.size(); i++)
      fileToRadixBuckets<T>(fileNames[i], fileRadixBuffer, inBuffer, bufferSizeInElements, pos);
    printf("\n");
    
    fileRadixBuffer.flushAll(pos);
    
    pos++;
    if (pos < sizeof(T))
        fileRadixFileSort<uint32_t>(bucketNames, fileRadixBuffer, inBuffer, bufferSizeInElements, pos);
}

int main(int argc, char* argv[])
{
    if(argc < 1)
    {
        printf("Usage: %s file_for_sort\n", argv[0]);
        printf("Warning: this version need 5x disk space for sorting file, create alot tempory files\n");
        return -1;
    }
    
    std::vector<std::string> fileNames;
    fileNames.push_back(argv[1]);
    
    typedef uint32_t DATA_TYPE;

    size_t memoryForBuffersInBytes      =  (size_t)(255 * 1024 * 1024);
    size_t memoryForReadBufferInBytes   =  (size_t)(10  * 1024 * 1024);
    size_t memoryForRadixBufferInBytes  =  memoryForBuffersInBytes - memoryForReadBufferInBytes;
    
    size_t         inBufferSizeInElements = memoryForReadBufferInBytes/sizeof(DATA_TYPE);
    printf("Allocate read buffer, size %zu elements, in memory %zumb\n",inBufferSizeInElements, memoryForReadBufferInBytes/(1024 * 1024));
    // anyway we use C-style read from file, lets use C-style buffer, not vector
    DATA_TYPE*      inBuffer = new DATA_TYPE[inBufferSizeInElements];
    printf("Create radix buffers, in memory %zumb\n", memoryForRadixBufferInBytes/ (1024 * 1024));
    FileRadixBuffer<DATA_TYPE>  fileRadixBuffer(memoryForRadixBufferInBytes);
    fileRadixFileSort<DATA_TYPE>(fileNames, fileRadixBuffer, inBuffer, inBufferSizeInElements);
    
    printf("Deallocate read buffer...\n");
    delete[] inBuffer;
    
    return 0;
}

