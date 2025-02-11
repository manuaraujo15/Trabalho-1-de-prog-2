#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <dirent.h>
#include <getopt.h>
#include <unistd.h>
#include "lbp.h"

#define MAX_IMAGENS 100
#define LARGURA_MAX 256
#define MAX_TAM 256
#define MAX_CAMINHO 257
#define BORDA 2
#define ZERO 0
#define DIRETORIO_PADRAO "./base/" 

void ignorar_comentarios(FILE *arquivo) {
    int c;
    while ((c = fgetc(arquivo)) == '#') {
        while (fgetc(arquivo) != '\n');
    }
    fseek(arquivo, -1, SEEK_CUR);
}



// Função para liberar memória de um histograma
void desalocar_histograma(float *histograma) {
    if (histograma != NULL) {
        free(histograma);
    }
}
void desalocar_imagem(unsigned char **imagem, int linha) {
    // Primeiro, libera cada linha alocada
    if(imagem != NULL){
        for (int i = ZERO; i < linha; i++) {
            free(imagem[i]);
        }
        // Depois, libera o ponteiro principal
        free(imagem);
    }
}


unsigned char** ler_imagem_pgm(char *caminho, int *linhas, int *colunas, int *max_valor, char *tipo) {
    FILE *arquivo = fopen(caminho, "r"); // "rb" para leitura em modo binário
    if (arquivo == NULL) {
        printf("Erro na leirura ao abrir o arquivo %s\n", caminho);
        fclose(arquivo);
        return NULL;
    }

    // Ler o cabeçalho
    fscanf(arquivo, "%s", tipo); // Ler "P5"
    // printf("Tipo PGM: %s\n", tipo);
    if (strcmp(tipo, "P2") != ZERO && strcmp(tipo, "P5") != ZERO) {
       // printf("Formato PGM não suportado: %s ", tipo);
        //printf("Tipo PGM: %s\n", tipo);
        fclose(arquivo);
        return NULL;
    }
    
    // Ignorar comentários (linhas começando com '#')
    char c = fgetc(arquivo);
    while (c == '#') {
        while (fgetc(arquivo) != '\n'); // Pula a linha do comentário
        c = fgetc(arquivo);
    }
    ungetc(c, arquivo); // Devolve o último caractere lido

    // Ler as dimensões e o valor máximo de cinza
    fscanf(arquivo, "%d %d", colunas, linhas);
    fscanf(arquivo, "%d", max_valor);
    
    // Ler o único caractere de nova linha após o max_valor
    fgetc(arquivo);

    // Alocar memória para a imagem
    unsigned char **imagem = malloc(*linhas * sizeof(unsigned char *));
    for (int i = ZERO; i < *linhas; i++) {
        imagem[i] = malloc(*colunas * sizeof(unsigned char));
    }

    if (strcmp(tipo, "P2") == ZERO) {
    // Ler os valores dos pixels
        for (int i = 0; i < *linhas; i++) {
            for (int j = 0; j < *colunas; j++) {
                fscanf(arquivo, "%hhu", &imagem[i][j]);
            }
        }
    }
    if (strcmp(tipo, "P5") == ZERO) {
        for (int i = ZERO; i < *linhas; i++) {
            if (fread(imagem[i], sizeof(unsigned char), *colunas, arquivo) != *colunas) {
                printf("Erro ao ler os dados da imagem ");
                fclose(arquivo);
                return NULL;
            }
        }
    }

    fclose(arquivo);
    return imagem;
}

void calcular_lbp(unsigned char **imagem, unsigned char **imagem_lbp, int linhas, int colunas,float *histograma_normal) {

    int mascara[] = {128, 64,  32, 16,8 , 4,2, 1};
    // Processa a imagem (exceto bordas)
     // Preenche as bordas com valor ZERO (opcional)
    for (int i = 0; i < MAX_TAM; i++) {
        histograma_normal[i] = ZERO;
    }

    for (int i = 1; i < colunas-1; i++) {
        imagem_lbp[ZERO][i] = ZERO; // Primeira linha
        imagem_lbp[linhas-1][i] = ZERO; // Última linha
    }
    for (int i = 1; i < linhas-1; i++) {
        imagem_lbp[i][ZERO] = ZERO; // Primeira coluna
        imagem_lbp[i][colunas-1] = ZERO; // Última coluna
    }
    for (int i = 1; i < linhas - 1; i++) {
        for (int j = 1; j < colunas - 1; j++) {

            unsigned char centro = imagem[i][j];
            int valor_lbp = ZERO;

            // Comparar o pixel central com seus 8 vizinhos
            if (imagem[i-1][j-1] >= centro) valor_lbp += mascara[ZERO];
            if (imagem[i-1][j]   >= centro) valor_lbp += mascara[1];
            if (imagem[i-1][j+1] >= centro) valor_lbp += mascara[2];
            if (imagem[i][j+1]   >= centro) valor_lbp += mascara[3];
            if (imagem[i+1][j+1] >= centro) valor_lbp += mascara[4];
            if (imagem[i+1][j]   >= centro) valor_lbp += mascara[5];
            if (imagem[i+1][j-1] >= centro) valor_lbp += mascara[6];
            if (imagem[i][j-1]   >= centro) valor_lbp += mascara[7];

            imagem_lbp[i][j] = valor_lbp;
            histograma_normal[valor_lbp]++;     
        }
    }

}
void gerar_histograma(unsigned char **imagem_lbp, int linhas, int colunas, float *histograma_normal) {
    // Normalizar o histograma
    int total_pixels = (linhas - 1) * (colunas - 1);
    for (int i = ZERO; i < MAX_TAM; i++) {
        if (histograma_normal[i] < ZERO) {
            printf("Erro: valor negativo no histograma para LBP %d: %f\n", i, histograma_normal[i]);
        }
    }
    for (int i = ZERO; i < MAX_TAM; i++) {
        histograma_normal[i] =  (float)histograma_normal[i] / total_pixels;
    }

    /*for (int i = ZERO; i < MAX_TAM; i++) 
    printf("antes v[%d] = %f\n",i,  histograma_normal[i]);*/
}


void salvar_lbp( char *diretorio, char *nome_arquivo, float *lbp_vetor, int tamanho) {
    char caminho_completo[MAX_CAMINHO];
    snprintf(caminho_completo, sizeof(caminho_completo), "%s/%s", diretorio, nome_arquivo);

    FILE *arquivo = fopen(caminho_completo, "wb+");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo para salvar o vetor LBP: %s\n", nome_arquivo);
        exit(1);
    }

    int result;
    result = fwrite(lbp_vetor, sizeof(float), tamanho, arquivo);
    if(result == 0){
        printf("ERRO AO GRAVA LBP");
    }

    fclose(arquivo);
}


unsigned char** carregar_lbp( char *nome_arquivo, int linhas, int colunas,unsigned char **imagem) {
    FILE *arquivo = fopen(nome_arquivo, "rb");
    if (!arquivo) {
        return NULL; // Arquivo não existe, será necessário recalcular o LBP
    }
    // Aloca memória para armazenar o vetor LBP
     unsigned char **imagem_lbp = (unsigned char **)malloc(linhas * sizeof(unsigned char *));
    for (int i = ZERO; i < linhas; i++) {
        imagem_lbp[i] = (unsigned char *)malloc(colunas * sizeof(unsigned char));
    }
    // Lê o arquivo binário .lbp e carrega o vetor LBP
    for (int i = ZERO; i < linhas; i++) {
        fread(imagem_lbp[i], sizeof(unsigned char), colunas, arquivo);
    }
    imagem = imagem_lbp;
    fclose(arquivo);
    return imagem_lbp;
}

void salvar_imagem_pgm(char *diretorio, char *nome_arquivo, unsigned char **imagem, int linha, int coluna, int max_valor,  char *tipo) {
 
    char caminho_completo[MAX_TAM];
    snprintf(caminho_completo, sizeof(caminho_completo), "%s/%s", diretorio, nome_arquivo);
    
    FILE *arquivo = fopen(caminho_completo, "wb");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo para salvar: %s\n", caminho_completo);
        return;
    }
    if (strcmp(tipo, "P5") == ZERO){
        fprintf(arquivo, "%s\n%d %d\n%d\n", "P5", coluna,linha, max_valor);
        for (int i = 1; i < linha-1; i++) {
            fwrite(imagem[i], sizeof(unsigned char), coluna, arquivo);
        }
    }
    if(strcmp(tipo, "P2") == ZERO){
        fprintf(arquivo, "%s\n%d %d\n%d\n", "P2", coluna,linha, max_valor);
        // Escrever os pixels da imagem
        for (int i = 1; i < linha-1; i++) {
            for (int j = 1; j < coluna-1; j++) {
                fprintf(arquivo, "%d ", imagem[i][j]);  // Escreve cada pixel
            }
            fprintf(arquivo, "\n");  // Nova linha após cada linha de pixels
        }
    }
    // Salvar em formato binário (P5)
    fclose(arquivo);
}

void processar_imagens( char *diretorio,  char *imagem_referencia ) {
    DIR *dir;
    struct dirent *entry;
    // Carregar o histograma da imagem de referência
    // Abrir o diretório
    dir = opendir(diretorio);
    if (dir == NULL) {
        printf("Erro ao abrir o diretório.\n");
        exit(1);
    }
 // Ler todos os arquivos do diretório
    while ((entry = readdir(dir)) != NULL) {
        // Ignorar "." e ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char nome_pgm[MAX_TAM];
        strcpy(nome_pgm, entry->d_name);
        nome_pgm[strlen(nome_pgm) - 4] = '\0';
        strcat(nome_pgm, "_out.pgm");
        // Gerar o nome completo do arquivo com o diretório
        char caminho_completo[MAX_CAMINHO];
        snprintf(caminho_completo, sizeof(caminho_completo), "%s/%s", diretorio, entry->d_name);
        char tipo[5];
        int linha, coluna, max_valor;

        // Gerar o nome do arquivo LBP com a extensão .lbp
        char nome_lbp[MAX_TAM];
        strcpy(nome_lbp, entry->d_name);
        nome_lbp[strlen(nome_lbp) - 4] = '\0';
        strcat(nome_lbp, ".lbp");


        // Carregar a imagem PGM
        unsigned char **imagem = ler_imagem_pgm(caminho_completo, &linha, &coluna,&max_valor,tipo);
        if (imagem == NULL) {
            desalocar_imagem(imagem,linha);
            return ;
        }

        float *hist_base = (float *)malloc(MAX_TAM * sizeof(float));
        if (hist_base == NULL) {
            desalocar_histograma(hist_base);
                return ;
            }
        // Verifica se o arquivo .lbp já existe
            unsigned char **imagem_lbp = NULL;
        imagem_lbp= carregar_lbp(nome_lbp,linha , coluna,imagem_lbp);
        if (imagem != NULL){
            if (imagem_lbp == NULL ){
                //printf("Arquivo LBP não encontrado. Calculando LBP para a imagem %s\n", entry->d_name);

                // Aloca memória para a imagem LBP
                unsigned char **imagem_lbpp = (unsigned char **)malloc(linha * sizeof(unsigned char *));
                for (int i = ZERO; i < linha; i++) 
                    imagem_lbpp[i] = (unsigned char *)malloc(coluna * sizeof(unsigned char));
                
                for (int i = ZERO; i < linha; i++) 
                    for (int j = ZERO; j < coluna; j++)  
                        imagem_lbpp[i][j] = ZERO;;

                // Calcula o LBP da imagem

                calcular_lbp(imagem, imagem_lbpp,linha , coluna,hist_base);
                if (imagem_lbpp == NULL ){
                        desalocar_imagem(imagem_lbpp,linha);
                        printf("NULL");
                }

                gerar_histograma(imagem_lbpp,linha ,coluna, hist_base);
                //for (int i = ZERO; i < MAX_TAM; i++) printf("v[%d] = %f\n",i,  hist_base[i]);
                //for (int i = ZERO; i < linha; i++)  for (int j = ZERO; j < coluna; j++) if(imagem_lbpp>ZERO )printf("v[%d] = %d\n",j,  imagem_lbpp[i][j]);
                // Salva o vetor LBP no arquivo binário
                salvar_lbp(diretorio,nome_lbp, hist_base, MAX_TAM);
                if (imagem_lbpp != NULL )
                desalocar_imagem(imagem_lbpp,linha);

            } 

        } 
            desalocar_imagem(imagem_lbp, linha);
    
            desalocar_imagem(imagem, linha);

            desalocar_histograma(hist_base);


    }

  // Fechar o diretório e o arquivo binário
    closedir(dir);
    //printf("Processamento concluído. Os histogramas foram gravados em ");

}
float distancia_euclidiana(float *hist1, float *hist2, int tamanho) {
    double soma = 0;
    for (int i = ZERO; i < tamanho; i++) {
        double diferenca = hist1[i] - hist2[i];
        soma += diferenca * diferenca;
    }
    return sqrt(soma);
}

// Função para carregar um histograma salvo
void carregar_histograma( char *nome_arquivo,  char *diretorio,float *histograma) {
    char caminho_completo[MAX_TAM];
    snprintf(caminho_completo, sizeof(caminho_completo), "%s/%s", diretorio, nome_arquivo);

    FILE *arquivo = fopen(caminho_completo, "r");
    if (arquivo == NULL) {
        //printf("Erro ao aaaaabrir o arquivo %s\n", nome_arquivo);
    }
    char *extensao = strrchr(nome_arquivo, '.');  // Encontra a última ocorrência de '.'
    if (extensao == NULL || strcmp(extensao, ".lbp") != 0) {
        fclose(arquivo);
        return ;
    }
    // Ignorar comentários (linhas começando com '#')
    // Aloca memória para o histograma
  
    if (histograma == NULL) {
        printf("Erro ao alocar memória para o histograma.\n");
        fclose(arquivo);
        return ;
    }
    memset(histograma, 0, MAX_TAM * sizeof(float));

    int result;
    // Lê os dados do arquivo binário
    result = fread(histograma, sizeof(float), MAX_TAM, arquivo);
    
    /* printf("Nro de elementos lidos: %d\n", result);
    for (int i=0; i<result; i++)
        printf("%lf\n", histograma[i]);*/
    // Verifica se a leitura foi bem-sucedida
    if (result != MAX_TAM) {
        fclose(arquivo);
        return;
    }
    fclose(arquivo);
    
}


// Função para calcular a distância entre histogramas e encontrar o arquivo mais próximo
void encontrar_arquivo_proximo( char *hist_referencia, char *diretorio) {
    DIR *dir;
    struct dirent *entry;
    float menor_distancia = -1;
    char arquivo_mais_proximo[MAX_TAM];
    char caminho[MAX_CAMINHO];

    char nome_arquivo[MAX_TAM];
    strcpy(nome_arquivo,hist_referencia);
    char *extensao = strrchr(hist_referencia, '.');  // Encontra a última ocorrência de '.'
    if (extensao == NULL || strcmp(extensao, ".lbp") != 0) {
        nome_arquivo[strlen(nome_arquivo) - 4] = '\0';
        strcat(nome_arquivo, ".lbp");
        hist_referencia = nome_arquivo;
        //printf(" epa %s", hist_referencia);
    }
    float *ref = (float *)malloc(MAX_TAM * sizeof(float));
            for (int i = ZERO; i < MAX_TAM; i++)  ref[i] = 0;

    carregar_histograma(hist_referencia,diretorio, ref);
    
    if (ref == NULL) {
        //printf(" epa %s", hist_referencia);
        nome_arquivo[strlen(hist_referencia) - 4] = '\0';
        strcat(nome_arquivo, ".pgm");
        //printf(" epa %s", nome_arquivo);
        char tipo[5];
        int linha, coluna, max_valor;
        snprintf(caminho, sizeof(caminho), "%s/%s", diretorio, nome_arquivo);

        unsigned char **imagem2 = ler_imagem_pgm(caminho, &linha, &coluna,&max_valor,tipo);
        if (imagem2 == NULL) {
            desalocar_imagem(imagem2,linha);
            printf("Erro ao carregar a imagem. %s \n",caminho);
            return ;
        }
              //  printf("Erro ao carregar a imagem.\n");
        unsigned char **imagem_lbpp = (unsigned char **)malloc(linha * sizeof(unsigned char *));
        for (int i = ZERO; i < linha; i++) 
            imagem_lbpp[i] = (unsigned char *)malloc(coluna * sizeof(unsigned char));
        for (int i = ZERO; i < linha; i++) 
            for (int j = ZERO; j < coluna; j++)  
                imagem_lbpp[i][j] = ZERO;;

                // Calcula o LBP da imagem
        float *hist_base = (float *)malloc(MAX_TAM * sizeof(float));

        calcular_lbp(imagem2, imagem_lbpp,linha , coluna,hist_base);
        
        if (imagem_lbpp == NULL ) printf("NULL");
        
        nome_arquivo[strlen(hist_referencia) - 4] = '\0';
        strcat(nome_arquivo, ".lbp");
        //printf(" epa %s", nome_arquivo);
        gerar_histograma(imagem_lbpp,linha ,coluna, hist_base);
        

        salvar_lbp(diretorio,nome_arquivo, hist_base, MAX_TAM);
                desalocar_histograma(hist_base);
        for (int i = ZERO; i < MAX_TAM; i++)  ref[i] = 0;

        carregar_histograma(hist_referencia,diretorio,ref);
        desalocar_imagem(imagem_lbpp, linha);
        desalocar_imagem(imagem2, linha);


    }
           // printf("Erro aaaaao carregar o histograma: %s\n", caminho);
    if (ref == NULL)    desalocar_histograma(ref);

    dir = opendir(diretorio);
    if (dir == NULL) {
        printf("Erro ao abrir o diretório.\n");
        exit(1);
    }

    // Ler todos os arquivos do diretório
    while ((entry = readdir(dir)) != NULL) {
        // Ignorar "." e ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, hist_referencia) == 0 ||
            strcmp(entry->d_name, "entry->d_name.pgm") == 0 )
            continue;
        float *hist_atual= (float *)malloc(MAX_TAM * sizeof(float)); ;

        for (int i = ZERO; i < MAX_TAM; i++)  hist_atual[i] = 0;
        // Carregar o histograma do arquivo atual
        carregar_histograma(entry->d_name,diretorio,hist_atual);
        if (hist_atual == NULL) {
            continue;
        }
        //for (int i = ZERO; i < MAX_TAM; i++) printf("v[%d] = %f\n",i,  hist_atual[i]);

        // Calcular a distância entre os histogramas
        float distancia = distancia_euclidiana(ref, hist_atual, MAX_TAM);
        desalocar_histograma(hist_atual);

        // Verificar se essa distância é a menor até agora
        if (menor_distancia == -1 || distancia < menor_distancia) {
            menor_distancia = distancia;
            strcpy(arquivo_mais_proximo, entry->d_name);
        }
        // Liberar o histograma atual
    }
    // Resultado
    if (menor_distancia != -1) {
        printf("%s %lf\n", arquivo_mais_proximo, menor_distancia);
    }   

       
    desalocar_histograma(ref);
    closedir(dir);


}
void salva_imagens( char *diretorio,  char *nome, char *imagem_referencia ) {
    //printf("antes %s", diretorio);

    int linha, coluna, max_valor;
    // Gerar o nome do arquivo LBP com a extensão .lbp
    char tipo[3];
    char nome_lbp[MAX_TAM];
    strcpy(nome_lbp,nome);
    float *hist_base = (float *)malloc(MAX_TAM * sizeof(float));
    nome_lbp[strlen(nome) - 8] = '\0';
    strcat(nome_lbp, ".pgm");
    //printf(" nome %s\n", nome_lbp);
    // Carregar a imagem PGM
    if (strchr(imagem_referencia, '/') == NULL) {
        char caminho_completo[MAX_TAM];
        snprintf(caminho_completo, sizeof(caminho_completo), "%s/%s", diretorio, imagem_referencia);
        imagem_referencia = caminho_completo;
    }else{
        diretorio = DIRETORIO_PADRAO;
    }
    unsigned char **imagem = ler_imagem_pgm(imagem_referencia, &linha, &coluna,&max_valor,tipo);
    if(imagem == NULL)   
        printf("deu rui");  
    // Ler todos os arquivos do diretório
    unsigned char **imagem_lbpp = (unsigned char **)malloc(linha * sizeof(unsigned char *));
    for (int i = ZERO; i < linha; i++)
        imagem_lbpp[i] = (unsigned char *)malloc(coluna * sizeof(unsigned char));
    
    for (int i = ZERO; i < linha; i++) 
        for (int j = ZERO; j < coluna; j++)  
            imagem_lbpp[i][j] = ZERO;
            // Calcula o LBP da imagem
    calcular_lbp(imagem, imagem_lbpp,linha , coluna,hist_base);
        desalocar_histograma(hist_base);

    if (imagem_lbpp == NULL ) printf("NULL");
    //printf("%s", diretorio);
    salvar_imagem_pgm(diretorio,nome, imagem_lbpp, linha, coluna, max_valor, tipo);
    //printf("oiiiiiiiiiiiiii");
    desalocar_imagem(imagem,linha);
    desalocar_imagem(imagem_lbpp,linha);
}

int main(int argc, char *argv[]) {   
     char *opcao = argv[1];
    char *diretorio_base = argv[2];
    char *nome_imagem = argv[4];
    char caminho_completo[1024] = DIRETORIO_PADRAO;
    char *entrada = NULL;
    char option;
    //printf("%s", opcao);
    // Usar getopt para processar os argumentos da linha de comando
    while ((option = getopt(argc, argv, "i:d:o")) != -1) {
        switch (option) {
            case 'd':
                // Prioriza a opção -d, então o arquivo de entrada -i será ignorado
                diretorio_base = optarg;
                if (strcmp(opcao, "-d") == 0 ){
                    processar_imagens(diretorio_base,nome_imagem);
                    encontrar_arquivo_proximo(nome_imagem,diretorio_base); 
                } 
                break;
            case 'i':
                // Somente processa -i se -d não foi fornecido
                entrada = optarg;
                if (strchr(entrada, '/') == NULL) {
                    // Se o nome do arquivo não contém '/', assume-se que ele está no diretório padrão
                    snprintf(caminho_completo, sizeof(caminho_completo), "%s", DIRETORIO_PADRAO);
                } else {
                    // Se já contém um caminho, usa-se diretamente o nome fornecido
                    strncpy(caminho_completo, entrada, sizeof(caminho_completo));
                }
                if (strcmp(opcao, "-i") == 0 )  salva_imagens(caminho_completo,nome_imagem,diretorio_base);

                break;
        }
    }
    return ZERO;
}