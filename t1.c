#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <mpi.h>
#include <sys/time.h>

void bs(int n, int vetor[])
{
    int c=0, d, troca, trocou =1;

    while (c < (n-1) & trocou )
        {
        trocou = 0;
        for (d = 0 ; d < n - c - 1; d++)
            if (vetor[d] > vetor[d+1])
                {
                troca      = vetor[d];
                vetor[d]   = vetor[d+1];
                vetor[d+1] = troca;
                trocou = 1;
                }
        c++;
        }
}

/* calcula o parentesco considerando uma arvore binária heap-like */
void calcula_parentesco(int size, int id, int *pai, int *filhoEsq, int *filhoDir, int *level)
{
	*filhoEsq = 2 * id + 1;
	*filhoDir = 2 * id + 2;
	*pai = (int)floor((id - 1) / 2);
	*level = floor(log2(*pai + 1)) + 1;

	if (*filhoEsq >= size) {
		*filhoEsq = -1;
	}

	if (*filhoDir >= size) {
		*filhoDir = -1;
	}
}

/* intercala as duas particoes do vetor */
int *interleaving(int vetor[], int tam)
{
	int *vetor_auxiliar;
	int i1, i2, i_aux;

	vetor_auxiliar = (int *)malloc(sizeof(int) * tam);

	i1 = 0;
	i2 = tam / 2;

	for (i_aux = 0; i_aux < tam; i_aux++) {
		if (((vetor[i1] <= vetor[i2]) && (i1 < (tam / 2)))
		    || (i2 == tam))
			vetor_auxiliar[i_aux] = vetor[i1++];
		else
			vetor_auxiliar[i_aux] = vetor[i2++];
	}

	return vetor_auxiliar;
}

int main(int argc, char *argv[])
{
	MPI_Status status;
	int *vetor;
	int id, size;
	int tag = 666;
	int pai, filhoEsq, filhoDir, level;
	int tam, mid;
	int *vetor_aux;
	struct timeb start_time, end_time;

	MPI_Init(&argc, &argv);

	if (argc != 2) {
		printf("Usage: %s size\n", argv[0]);
		exit(1);
	}

	tam = atoi(argv[1]);

	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	if (id == 0)
		ftime(&start_time);

	vetor = (int *)malloc(tam * sizeof(int));
	if (vetor == NULL) {
		printf("Erro ao alocar vetor\n");
		exit(1);
	}

	/* inicializa o vetor */
	if (id == 0) {
		int i;
		for (i = 0; i < tam; i++) {
			vetor[i] = tam - i;
		}
    }

	calcula_parentesco(size, id, &pai, &filhoEsq, &filhoDir, &level);
	if (id == 0) {

        // com 1 processo somente ordena localmente
        if (filhoEsq == -1 || filhoDir == -1) {
            bs(tam, vetor);
        } else {
            mid = tam / 2;

            // envia particao para o filho da esquerda
            MPI_Send(vetor, mid, MPI_INT, filhoEsq, tag, MPI_COMM_WORLD);

            // envia particao para o filho da direita
            MPI_Send(vetor + mid, mid, MPI_INT, filhoDir, tag, MPI_COMM_WORLD);

            // recebe resultados do filho da esquerda
            MPI_Recv(vetor, mid, MPI_INT, filhoEsq, tag, MPI_COMM_WORLD, &status);

            // recebe resultados do filho da direita
            MPI_Recv(vetor + mid, mid, MPI_INT, filhoDir, tag, MPI_COMM_WORLD, &status);

            // mescla os resultados recebidos dos filhos
            vetor_aux = interleaving(vetor, tam);
            //imprime(vetor_aux, tam);
            free(vetor_aux);
        }   
	} else {
		tam = tam / pow(2, level);
		mid = tam / 2;

		// recebe particao do pai
		MPI_Recv(vetor, tam, MPI_INT, pai, tag, MPI_COMM_WORLD,
			 &status);

		// verifica se possui filhos
		if ((filhoEsq != -1) && (filhoDir != -1)) {
			// se possui filhos, envia particoes para eles

			// envia particao para o filho da esquerda
			MPI_Send(vetor, mid, MPI_INT, filhoEsq, tag,
				 MPI_COMM_WORLD);

			// envia particao para o filho da direita
			MPI_Send(vetor + mid, mid, MPI_INT, filhoDir, tag,
				 MPI_COMM_WORLD);

			// recebe resultados do filho da esquerda
			MPI_Recv(vetor, mid, MPI_INT, filhoEsq, tag,
				 MPI_COMM_WORLD, &status);

			// recebe resultados do filho da direita
			MPI_Recv(vetor + mid, mid, MPI_INT, filhoDir, tag,
				 MPI_COMM_WORLD, &status);

			// mescla os resultados recebidos dos filhos
			vetor_aux = interleaving(vetor, tam);

			// envia resultados para o pai
			MPI_Send(vetor_aux, tam, MPI_INT, pai, tag,
				 MPI_COMM_WORLD);
			free(vetor_aux);
		} else {
			struct timeb proc_start_time, proc_end_time;

			// se nao possui filhos, ordena a particao localmente
			ftime(&proc_start_time);
            bs(tam, vetor);
			ftime(&proc_end_time);
			printf("%d: bubblesort %lf (%d)\n", id,
			       (proc_end_time.time - proc_start_time.time) +
			       (double)((proc_end_time.millitm -
					 proc_start_time.millitm)) / 1000, tam);

			// envia resultados para o pai
			MPI_Send(vetor, tam, MPI_INT, pai, tag, MPI_COMM_WORLD);
		}
	}

	if (id == 0) {
		ftime(&end_time);
		printf("%d %lf (%d)\n", size,
		       (end_time.time - start_time.time) +
		       (double)((end_time.millitm - start_time.millitm)) / 1000,
		       tam);
	}

	MPI_Finalize();
	free(vetor);
	return 1;

}
