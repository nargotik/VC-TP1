/**
 * Este ficheiro contem as funções que foram criadas a fim de reconhecimento das matriculas propostas no TP1
 * @brief Ficheiro que contem funções criadas para o processamento de uma imagem a fim de extrair a matricula e seus caracteres
 * @file plate-recognizer.c
 * @date 04 Maio de 2020
 * @author Rafael Pereira <a13871@alunos.ipca.pt>
 * @author Óscar Silva <a14383@alunos.ipca.pt>
 * @author Daniel Torres <a17442@alunos.ipca.pt>
 */

#include <limits.h> // PATH_MAX
#include <string.h> // strcpy()
#include <stdlib.h> // EXIT_FAILURE, EXIT_SUCCESS
#include <dirent.h>
#include <sys/stat.h> // para o stat() // S_ISDIR Macro
#include <sys/types.h>
#include <stdio.h> // puts() printf
#include <math.h>
#include "plate-recognizer.h"

char output_dir[PATH_MAX] = "";

void debugSave(char *filen,int id, IVC *src) {
    char fileimagename[PATH_MAX];
    sprintf(fileimagename,"%s%s_%d.ppm",output_dir,filen,id);
    vc_write_image(fileimagename, src);
}


/**
 * Verifica se o directorio existe e se é um directorio
 * @param path
 * @return 1 se for directorio e existir
 */
int directory_exists(const char *path) {
    struct stat filestats;
    stat(path, &filestats);
    return S_ISDIR(filestats.st_mode);
}

/**
 * Returns if file exist
 * @param path
 * @return
 */
int file_exists(const char *path) {
    struct stat filestats;
    stat(path, &filestats);
    return S_ISREG(filestats.st_mode);
}


/**
 * Convert reg to grayscale from r g b
 * @param r
 * @param g
 * @param b
 * @return
 */
int rgb_to_gray(int r, int g, int b) {
    int grey = (r * 0.299) + (g * 0.587) + (b * 0.114);
    return (unsigned char)(grey>255 ? 255 : grey);
}

/**
 *
 * @param src
 */
void invertImageBinary(IVC *src) {
    int size = src->height * src->width;
    for (int x = 0; x < size;x++) {
        src->data[x] = (src->data[x]== 0 ? 255:0);
    }
}

/**
 * Fill Image with a value
 * @param src
 * @param value
 */
void fillImage(IVC *src, unsigned char value) {
    long int maxpixels = src->width * src->height;
    for (int xy = 0; xy < maxpixels; xy++) {
        src->data[xy] = value;
        src->data[xy+1] = value;
        src->data[xy+2] = value;
    }
}

/**
 * Extract blog from picture and return white ratio with threshold
 * @param src
 * @param dst
 * @param blob
 * @return
 */
float extractBlob(IVC *src, IVC *dst, OVC blob) {

    // To count pixels from this threshold as potential plate
    int threshold = 200;

    int total_white = 0;
    int bytesperline_src = src->width * src->channels;

    // Mete a imagem a branco
    fillImage(dst,255);

    // Percorre a altura do blob para extrair o blob
    for (int yy = blob.y; yy <= blob.y + blob.height;yy++) {
        // Percorre a largura do blog
        for (int xx = blob.x; xx <= blob.x + blob.width;xx++) {
            int pos = yy * bytesperline_src + xx * src->channels;
            dst->data[pos] = (unsigned char)src->data[pos] ;
            dst->data[pos+1] = (unsigned char)src->data[pos+1];
            dst->data[pos+2] = (unsigned char)src->data[pos+2];

            // Faz um clareamente de 50 tambem
            int grey = rgb_to_gray(dst->data[pos]+50,dst->data[pos+1]+50,dst->data[pos+1]+50);
            // Soma 1 se o pixel for maior que threshold
            // Converte para binário onfly para contar pixeis brancos
            total_white = total_white + (grey > threshold ? 1 : 0);
        }
    }
    return ((float)total_white / blob.area);
}

/**
 * Extract blog from picture and return white ratio with threshold
 * @param src
 * @param dst
 * @param blob
 * @return
 */
float extractBlobBinary(IVC *src, IVC *dst, OVC blob) {
    int o=1;

    if (src->channels != 1) return 0;

    int bytesperline_src = src->width * src->channels;
    int bytesperline_dst = dst->width * dst->channels;

    // Percorre a altura do blob para extrair o blob
    for (int yy = blob.y; yy <= blob.y + blob.height-1;yy++) {
        // Percorre a largura do blog
        for (int xx = blob.x; xx <= blob.x + blob.width-1;xx++) {
            int inside_condition =
                    (
                            (xx >= blob.x) &&  // x tem de ser maior que x do blob da matricula
                            ((xx) < (blob.x + blob.width)) &&
                            (yy >= blob.y) &&
                            ((yy) < (blob.y + blob.height))
                    );

            int pos = yy * bytesperline_src + xx * src->channels;

            if (inside_condition) {
                int pos_dst = (yy-blob.y) * bytesperline_dst + (xx-blob.x) * dst->channels;
                dst->data[pos_dst] = (unsigned char)src->data[pos];
                o++;
            }

        }
    }
    return 1;
}

/**
 * Processes a probable plate to find if it has 6 numbers or digits
 * devolve os blobs encontrados na matricula
 * @param src
 * @return
 */
int processPlate(IVC *src, OVC* blobs_caracteres, int *numero_blobs, OVC blob, OVC found_plate[0], OVC blobs_matricula[6]) {
    char fileimagename[PATH_MAX];

    OVC *blobs_plate;
    IVC *image = vc_image_new(src->width, src->height, 1, src->levels);
    IVC *image2 = vc_image_new(src->width, src->height, 1, src->levels);
    IVC *image3 = vc_image_new(src->width, src->height, 1, src->levels);
    int numero2=0;

    debugSave("plate_original",0,src);

    // Remove cores
    vc_color_remove(src,12,250);
    debugSave("plate_colorremove",1,src);

    // Transforma em grayscale
    vc_rgb_to_gray(src, image2);
    debugSave("plate_gray",2,image2);

    // Clareia a imagem
    vc_brigten(image2,100);
    debugSave("plate_brigten",3,image2);

    // Coloca a imagem em binário e faz um erode
    vc_gray_to_binary(image2, image3, 180);
    vc_binary_erode(image3, image2, 3);
    debugSave("plate_binary_erode",4,image2);

    // Inverte a imagem
    invertImageBinary(image2);
    debugSave("plate_binary_invert",5,image2);

    *numero_blobs = 0;

    // Cria imagem de blobs
    blobs_caracteres =  vc_binary_blob_labelling(image2, image, numero_blobs);

    // Vai buscar a info dos blobs
    vc_binary_blob_info(image,blobs_caracteres, *numero_blobs);


    // Apenas blobs com mais de metade da altura que a matricula
    int min_height = blob.height * (float)0.4;
    float max_racio = 0.8;

    int encontrados=0;

    for (int e = 0; e < *numero_blobs; e++) {
        int height_condition = 0;
        int racio_condition = 0;
        int inside_condition = 0;

        float wh_racio = (float)(blobs_caracteres[e].width) / blobs_caracteres[e].height;

        // O tamanho nao pode ser maior que o blob da matricula
        height_condition = blobs_caracteres[e].height > min_height && blobs_caracteres[e].height < blob.height;
        racio_condition = wh_racio < max_racio;
        inside_condition =
                (
                        (blobs_caracteres[e].x >= blob.x) &&  // x tem de ser maior que x do blob da matricula
                        ((blobs_caracteres[e].x + blobs_caracteres[e].width) <= (blob.x + blob.width)) &&
                        (blobs_caracteres[e].y >= blob.y) &&
                        ((blobs_caracteres[e].y + blobs_caracteres[e].height) <= (blob.y + blob.height))
                );

        if (height_condition && racio_condition && inside_condition) {
            // Encontrou um numero ou letra
            /*printf("\n\nE: %d\n",e);
            printf("Area: %d\n", blobs_caracteres[e].area);
            printf("Width: %d\n", blobs_caracteres[e].width);
            printf("Height: %d\n", blobs_caracteres[e].height);
            printf("X: %d\n", blobs_caracteres[e].x);
            printf("Y: %d\n", blobs_caracteres[e].y);
            printf("Racio: %.2f\n", wh_racio);*/
            encontrados++;
            if (encontrados > 6) return 0;
            blobs_matricula[encontrados-1] = blobs_caracteres[e];
            found_plate[0] = blob;
            // Testing
            // Desenha os potenciais blobs
            desenha_bounding_box(src, &blobs_caracteres[e], 1);

            // To save digits
            IVC *temp_save = vc_image_new(blobs_caracteres[e].width, blobs_caracteres[e].height, 1, src->levels);
            extractBlobBinary(image2,temp_save,blobs_caracteres[e]);

            debugSave("caracteres",encontrados,temp_save);
        }

    }

    return encontrados;
}


/**
 *
 * @param src
 * @param blobs
 * @param numeroBlobs
 * @param blob_matricula
 * @param found_blobs_caracteres
 * @return
 */
int potentialBlobs(IVC *src,OVC* blobs,int numeroBlobs, OVC blob_matricula[1], OVC found_blobs_caracteres[6]) {

    // 4% of pixels
    int ideal_area = src->width * src->height * 0.03;

    // width / height racio potential
    // Pode-se mexer
    float wh_inf=3, wh_sup=4.5;

    float area_inf=ideal_area;// - 5000, area_sup=ideal_area + 5000;

    // Percentagem de branco a partir da qual é considerado matricula
    // Pode-se mexer
    float white_ideal = 0.3;

    //printf("Ideal Area: %d",ideal_area);


    // Verificaçao de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->channels != 3)) return 0;

    //percorre os blobs da imagem
    for (int i = 0; i < numeroBlobs; i++) {

        int wh_potential = 0, area_potential = 0;
        float wh_racio = 0;

        wh_racio = (float)blobs[i].width / blobs[i].height;

        // Potencial wh_racio
        wh_potential = (wh_racio > wh_inf) && (wh_racio < wh_sup);
        area_potential = (blobs[i].area > area_inf);// && (blobs[i].area < area_sup);

        if(wh_potential && area_potential) {
            IVC *plate = vc_image_new(src->width, src->height, 3, src->levels);

            // Potential plate extract
            float white_ratio = extractBlob(src,plate, blobs[i]);

            if (white_ratio > white_ideal) {
                // FOUND THE PLATE ?!?!?
                // Verifica se agora há 6 blobs lá dentro todos catitas com um ratio:D
                OVC *blobs_caracteres;
                int numero_caracteres=0, numeros_encontrados = 0;
                // Retira os blobs da imagem da matricula

                numeros_encontrados = processPlate(plate, blobs_caracteres, &numero_caracteres, blobs[i], blob_matricula, found_blobs_caracteres);

                if (numeros_encontrados == 6) {
                    // ENCONTREI UMA MATRICULA têm 6 digitos lá dentro
                    blob_matricula[0] = blobs[i];

                    // Matricula = blobs[i]
                    // Caracteres =
                    return 1;
                }


            }
        }


    }

    return 0;
}

/**
 * Processa uma imagem passada por argumento e faz o output do processamento para a pasta passada
 * @param name nome da imagem a processar
 * @param directorio directorio de output
 */
int processImage(char *name, char *directorio) {
    char ficheiro[PATH_MAX] = "";
    OVC blob_matricula[1];
    OVC blobs_caracteres[6];

    strcat(output_dir,directorio);
    //strcat(ficheiro,directorio);
    strcat(ficheiro,name);

    IVC *image[10];
    OVC *blobs[2];
    OVC *blobs_plate;
    int numero, numero2;
    int i;

    // Original file
    image[0] = vc_read_image(ficheiro);

    if (image[0] == NULL) {
        printf("ERROR -> vc_read_image():\n\tFile not found!\n");
        exit(EXIT_FAILURE);
    }

    image[1] = vc_image_new(image[0]->width, image[0]->height, 1, image[0]->levels);
    image[2] = vc_image_new(image[0]->width, image[0]->height, 1, image[0]->levels);
    image[3] = vc_image_new(image[0]->width, image[0]->height, 1, image[0]->levels);
    image[5] = vc_image_new(image[0]->width, image[0]->height, 1, image[0]->levels);
    image[4] = vc_read_image(ficheiro);

    debugSave("original",1,image[4]);
    // Remove cores
    vc_color_remove(image[4],12,250);
    debugSave("main_color_remove",2,image[4]);

    // Transforma em grayscale
    vc_rgb_to_gray(image[4], image[1]);
    debugSave("main_rgb_to_gray",3,image[1]);

    // Clareia a imagem
    vc_brigten(image[1],100);
    debugSave("main_brigten",4,image[1]);

    // Coloca a imagem em binário
    vc_gray_to_binary(image[1], image[5], 254);
    debugSave("main_binary",5,image[5]);

    // Fecha com kernel 2
    vc_binary_close(image[5], image[3], 2);
    debugSave("main_close",6,image[3]);

    // Dilata a imagem
    vc_binary_dilate(image[3], image[2], 3);
    debugSave("main_dilate",7,image[2]);

    // Cria imagem de blobs
    blobs_plate = vc_binary_blob_labelling(image[2], image[3],&numero2);
    debugSave("main_blobs",8,image[3]);

    // Vai buscar a info dos blobs
    vc_binary_blob_info(image[3],blobs_plate, numero2);


    int found = potentialBlobs(image[0], blobs_plate, numero2, blob_matricula, blobs_caracteres);
    if (found == 1) {
        // Desenha os potenciais blobs
        desenha_bounding_box(image[0], blob_matricula, 1);
        debugSave("main_plate_bounding",9,image[0]);

        desenha_bounding_box(image[0], blobs_caracteres, 6);
        debugSave("main_plate_bounding_chars",9,image[0]);

    } else {
        desenha_bounding_box(image[0], blobs_plate, numero2);
        debugSave("main_plate_notfound",9,image[0]);


    }
    return found;
}


/**
 * Clareamento de imagem pela soma
 * @param src
 * @param value
 * @return
 */
int vc_brigten(IVC *src, int value) {

    unsigned char *datasrc = (unsigned char *)src->data;
    int bytesperline_src = src->width * src->channels;
    int channels_src = src->channels;
    int width = src->width;
    int height = src->height;
    int x, y;
    long int pos_src, pos_dst;

    // Verifica��o de Erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if (!((src->channels == 3) || (src->channels == 1))) return 0;

    // Ciclo que vai percorrer todos os pixeis da imagem e converter a imagem
    for (y = 0; y<height; y++)
    {
        for (x = 0; x<width; x++)
        {
            pos_src = y * bytesperline_src + x * channels_src;
            datasrc[pos_src] = ((datasrc[pos_src] + value) > 255) ? 255 : datasrc[pos_src] + value;
            if (channels_src == 3) {
                datasrc[pos_src+1] = ((datasrc[pos_src+1] + value) > 255) ? 255 : datasrc[pos_src] + value;
                datasrc[pos_src+2] = ((datasrc[pos_src+2] + value) > 255) ? 255 : datasrc[pos_src] + value;
            }
        }
    }
    return 1;
}

/**
 * Calcula o desvio padrão entre rgb
 * @param r
 * @param g
 * @param b
 * @return valor de threshold
 */
int calcula_desvio(int r, int g, int b) {
    int soma = 0;
    float media, SD = 0.0;
    soma = r + g + b;
    media = soma / 3;
    SD = (r - media)*(r - media);
    SD = SD + (g - media)*(g - media);
    SD = SD + (b - media)*(b - media);
    return sqrt(SD / 3);
}

/**
 * Remove cores de uma imagem tendo em conta o desvio padrão
 * @param image
 * @param threshold
 * @param color
 * @return
 */
int vc_color_remove(IVC *image, int threshold, int color) {
    unsigned char *data = (unsigned char *) image->data;
    int width = image->width;
    int height = image->height;
    int bytesperline = image->bytesperline;
    int channels = image->channels;
    int x,y;
    long int pos;

    // Verificação de erros
    if((image->width <= 0) || (image->height <= 0) || (image->data == NULL)) return 0;
    if(channels != 3) return 0;

    for(y = 0; y < height; y++) {
        for(x = 0; x < width; x++) {
            pos = y * bytesperline + x * channels;
            if (calcula_desvio(data[pos],data[pos+1],data[pos+3]) >= threshold) {
                data[pos] = color;
                data[pos + 1] = color;
                data[pos + 2] = color;
            }

        }
    }

    return 1;
}


/**
 * Desenha uma bounding box ao redor de um blog
 * @param src
 * @param blobs
 * @param numeroBlobs
 * @return
 */
int desenha_bounding_box(IVC *src, OVC* blobs, int numeroBlobs) {

    unsigned char *datasrc = (unsigned char*)src->data;
    int bytesperline_src = src->width * src->channels;

    // Apenas para imagens com 3 canais
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->channels != 3)) return 0;

    // Percorre os Blobs os blobs da imagem
    for (int i = 0; i < numeroBlobs; i++) {
        //percorre a altura da box
        for (int yy = blobs[i].y; yy <= blobs[i].y + blobs[i].height;yy++) {
            //percorre a largura da box
            for (int xx = blobs[i].x; xx <= blobs[i].x + blobs[i].width;xx++) {

                //condiçao para colocar a (255,0,0) apenas os pixeis do limite da caixa
                int limite_y = (yy == blobs[i].y || yy == blobs[i].y + blobs[i].height);
                int limit_x = (xx == blobs[i].x || xx == blobs[i].x + blobs[i].width);

                if (limite_y || limit_x) {
                    long int  posk = yy * bytesperline_src + xx * src->channels;
                    datasrc[posk] = 255;
                    datasrc[posk+1] = 0;
                    datasrc[posk+2] = 0;
                }
            }
        }
    }
    return 1;
}
