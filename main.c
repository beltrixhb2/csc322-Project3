#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

//Type definition
typedef struct{
    int replacement;
    bool valid;
    int tag;
}line_type;

typedef struct{
    line_type* line;
}set_type;

typedef struct{
    set_type* set;
    int S;
    int E;
    int B;
    int m;
    char policy[4];
    int h;
    int p;
    int s;
    int b;
    int t;
    int H;
    int M;
    int add_len;
}cache_type;

//Get integer address from string
int hexStringToInt(char hexString[16]) {
    int result = (int)strtol(hexString, NULL, 16);
    return result;
}

//Read address in direct mapped cache
int readDirect(cache_type *cache, int address, FILE *output_file){
    if (address==-1){
        return 0;
    }
    int tag = address >> (cache->m - cache->t);
    int setIndex = (address & (((1 << cache->s) - 1)<<cache->b))>>cache->b;
    if (cache->set[setIndex].line[0].valid&&(cache->set[setIndex].line[0].tag==tag)){
        cache->H++;
        fprintf(output_file, "%0*x H\n", cache->add_len, address); 
    }else{
        cache->M++;
        cache->set[setIndex].line[0].valid = true;
        cache->set[setIndex].line[0].tag=tag;
        fprintf(output_file, "%0*x M\n", cache->add_len, address);   
    }
    return 1;
}

//Read address in set-associative cache with LFU replacement
int readLFU(cache_type *cache, int address, FILE *output_file){
    if (address==-1){
        return 0;
    }
    int tag = address >> (cache->m - cache->t);
    int setIndex = (address & (((1 << cache->s) - 1)<<cache->b))>>cache->b;
    bool hit = false;
    int lru_line = 0;
    int lru_value = INT_MAX;
    for (int i=0; i<cache->E; i++){
        if (lru_value>cache->set[setIndex].line[i].replacement){
            lru_value = cache->set[setIndex].line[i].replacement;
            lru_line = i;
        }
        if (cache->set[setIndex].line[i].valid&&(cache->set[setIndex].line[i].tag==tag)){
            hit = true;
            cache->H++;
            cache->set[setIndex].line[i].replacement++;
            fprintf(output_file, "%0*x H\n", cache->add_len, address);   
        }
    }
    if (!hit){
        cache->M++;
        cache->set[setIndex].line[lru_line].valid = true;
        cache->set[setIndex].line[lru_line].tag=tag;
        cache->set[setIndex].line[lru_line].replacement = 1;
        fprintf(output_file, "%0*x M\n", cache->add_len, address);   
    }
    return 1;
}

//Read address in set-associative cache with LRU replacement
int readLRU(cache_type *cache, int address, int cycle, FILE *output_file){ 
    if (address==-1){
        return 0;
    }
    int tag = address >> (cache->m - cache->t);
    int setIndex = (address & (((1 << cache->s) - 1)<<cache->b))>>cache->b;
    bool hit = false;
    int lru_line = 0;
    int lru_value = INT_MAX;
    for (int i=0; i<cache->E; i++){
        if (lru_value>=cache->set[setIndex].line[i].replacement){
            lru_value = cache->set[setIndex].line[i].replacement;
            lru_line = i;
        }
        if (cache->set[setIndex].line[i].valid&&(cache->set[setIndex].line[i].tag==tag)){
            hit = true;
            cache->H++;
            cache->set[setIndex].line[i].replacement = cycle;
            fprintf(output_file, "%0*x H\n", cache->add_len, address);   
        }
    }
   if (!hit){
        cache->M++;
        cache->set[setIndex].line[lru_line].valid = true;
        cache->set[setIndex].line[lru_line].tag=tag;
        cache->set[setIndex].line[lru_line].replacement = cycle;
        fprintf(output_file, "%0*x M\n", cache->add_len, address);   
    }
    return 1;
}




int main(int argc, char *argv[]){
    //Open files
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file1> <input_file2>\n", argv[0]);
        return 1;
    }
    FILE *input_file, *output_file;

    input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        perror("Error opening input file");
        return 1;
    }
   output_file = fopen(argv[2], "w");
    if (output_file == NULL) {
        perror("Error opening output file");
        return 1;
    }

    //Create cache and alocate memory 
    cache_type *cache = malloc(sizeof(cache_type));
    cache->H = 0;
    cache->M = 0;

    //Input cache parameters
    fscanf(input_file, "%d", &cache->S);
    fscanf(input_file, "%d", &cache->E);
    fscanf(input_file, "%d", &cache->B);
    fscanf(input_file, "%d", &cache->m);
    fscanf(input_file, "%3s", cache->policy);
    fscanf(input_file, "%d", &cache->h);
    fscanf(input_file, "%d", &cache->p);

    //Calculate necasary parameters
    cache->s = 0;
    int temp = cache->S;
    while (temp > 1) {
        temp /= 2;
        cache->s++;
    }

    cache->b = 0;
    temp = cache->B;
    while (temp > 1) {
        temp /= 2;
        cache->b++;
    }
   
    cache->add_len = -1;
    temp = cache->m;
    while (temp > 1) {
        temp /= 2;
        cache->add_len++;
    }

    cache->t = cache->m - (cache->s + cache->b);


    //Allocate necesary memory
    cache->set = malloc(cache->S*sizeof(set_type));
    for (int i=0; i<cache->S; i++){
        cache->set[i].line = malloc(cache->E*sizeof(line_type));
        for (int j=0; j<cache->E; j++){
            cache->set[i].line[j].replacement = 0;
            cache->set[i].line[j].valid = false;
        }
    }

    //Read first address
    char hexaddress[17];
    int address;
    fscanf(input_file, "%16s", hexaddress);
    address = hexStringToInt(hexaddress); 

    //In each case read addresses until the function returns 0 (address -1 read)
    if (cache->E==1){
        while(readDirect(cache, address, output_file)){
            fscanf(input_file, "%16s", hexaddress);
            address = hexStringToInt(hexaddress); 

        }    
    }else if (strcmp(cache->policy, "LFU") == 0){
        while(readLFU(cache, address, output_file)){
            fscanf(input_file, "%16s", hexaddress);
            address = hexStringToInt(hexaddress); 
        }
    }else{
        int cycle = 1;
        while(readLRU(cache, address, cycle, output_file)){
            fscanf(input_file, "%16s", hexaddress);
            address = hexStringToInt(hexaddress); 
            cycle++;
        }
    }
    
    //Calculate and print miss rate and total cycles
    int missRate = round((cache->M * 100.0) / (cache->M + cache->H));
    int cycles = (cache->H*cache->h)+(cache->M*(cache->p+cache->h));  
    fprintf(output_file, "%d %d\n", missRate, cycles);
   
    //Close files 
    fclose(input_file);
    fclose(output_file);

    //Free memory
    for (int i=0; i<cache->S; i++){
        free(cache->set[i].line);
    }
    free(cache->set);
    free(cache);
}




















