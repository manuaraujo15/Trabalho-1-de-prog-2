#ifndef LBP_PROCESSING_H
#define LBP_PROCESSING_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <dirent.h>
#include <getopt.h>
#include <unistd.h>

// Definições de constantes
#define MAX_IMAGENS 100
#define LARGURA_MAX 256
#define MAX_TAM 256
#define MAX_CAMINHO 257
#define BORDA 2
#define ZERO 0
#define DIRETORIO_PADRAO "./base/"

// Declaração de funções
void ignorar_comentarios(FILE *arquivo);

unsigned char** ler_imagem_pgm(char *caminho, int *linhas, int *colunas, int *max_valor, char *tipo);

void calcular_lbp(unsigned char **imagem, unsigned char **imagem_lbp, int linhas, int colunas, float *histograma_normal);

void gerar_histograma(unsigned char **imagem_lbp, int linhas, int colunas, float *histograma_normal);

void salvar_lbp(char *diretorio, char *nome_arquivo, float *lbp_vetor, int tamanho);

unsigned char** carregar_lbp(char *nome_arquivo, int linhas, int colunas,unsigned char **imagem);

void salvar_imagem_pgm(char *diretorio, char *nome_arquivo, unsigned char **imagem, int linha, int coluna, int max_valor, char *tipo);

void desalocar_imagem(unsigned char **imagem, int linha);

void processar_imagens(char *diretorio, char *imagem_referencia);

float distancia_euclidiana(float *hist1, float *hist2, int tamanho);

void carregar_histograma(char *nome_arquivo, char *diretorio,float *histograma);

void encontrar_arquivo_proximo(char *hist_referencia, char *diretorio);

#endif // LBP_PROCESSING_H