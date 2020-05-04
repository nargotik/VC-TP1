/**
 * Este ficheiro contem o main do projecto proposto no TP1
 * @brief Este ficheiro contem o main do projecto proposto no TP1
 * @file main.c
 * @date 04 Maio de 2020
 * @author Rafael Pereira <a13871@alunos.ipca.pt>
 * @author Óscar Silva <a14383@alunos.ipca.pt>
 * @author Daniel Torres <a17442@alunos.ipca.pt>
 */

#include <string.h> // strcpy()
#include <stdlib.h> // EXIT_FAILURE, EXIT_SUCCESS
#include <dirent.h>
#include <sys/stat.h> // para o stat() // S_ISDIR Macro
#include <sys/types.h>
#include <stdio.h> // puts() printf
#include <limits.h> // PATH_MAX
#include "plate-recognizer.h"


/**
 * Main function
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv) {

    char directorio[PATH_MAX];
    char ficheiro[PATH_MAX];

    if (argc == 3) {
        //
        strcpy(ficheiro,argv[1]);
        strcpy(directorio,argv[2]);


        printf("\nStarting processing %s....\n",ficheiro);

        if (processImage(ficheiro, directorio)) {
            printf("\nValid Plate FOUND! ¯\\\\_(ツ)_/¯\n");
        } else {
            printf("\nPlate not FOUND! :( \n");
        }

        printf("\nProcessing of %s finished.\n",ficheiro);
        return(EXIT_SUCCESS);
    } else {
        printf("Invalid arguments!\n\n");
        printf("USage: \n"
               "\t%s [FILENAME] [OUTPUT DIR]",argv[0]);
        return(EXIT_FAILURE);
    }


    return EXIT_SUCCESS;

}