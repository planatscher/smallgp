#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h> 

int MAX_LEN = 100;
int POPSIZE = 10000;
int DEPTH = 7;		
int GENERATIONS = 10;	
float CROSSOVER_PROB = 0.9;
float PMUT_PER_NODE = 0.01;
int TSIZE = 100;
int SEED = -1;
char* FNAME = "problem.data";
float LOWER_FITNESS_BOUND = 0.00001;
int NCONSTANTS = 200;			
float PTERMREALLY = 1;
float CONSTANTRANGE = 1;
int FITNESSCASES = 0;
int DIMENSION = 0;
float *input;
float *constants;
float ratio;

void readFile(char *filename) {
    FILE *inputfile;
    inputfile = fopen(filename, "r");
    if (inputfile == NULL) {
	printf("Error opening file...\n");
	exit(0);
    } else {
	int i = 0;
	fscanf(inputfile, "%i", &DIMENSION);
	fscanf(inputfile, "%i", &FITNESSCASES);
	input = (float *) malloc((DIMENSION + 1) * FITNESSCASES * sizeof(float));
	while (!feof(inputfile)) {
	    fscanf(inputfile, "%f", &input[i]);
	    i++;
	}
    }
    fclose(inputfile);
}

typedef float (*primitive) (float x, float y);

static float padd  (float x, float y) { return (x + y); }
static float pmult (float x, float y) { return (x * y); }
static float psub  (float x, float y) { return (x - y); }
static float pdiv  (float x, float y) { return ((y == 0.0) ? 0.0 : (x / y)); }

primitive prim[4] = { padd, pmult, psub, pdiv };
char* symbols = "+*-/";
const int PRIMITIVES = 4;

struct node {
    struct node *right;
    struct node *left;
    struct node **bp;
    unsigned char no;
};

struct node *newnode(struct node **bp, struct node *left, struct node *right, unsigned char no) {
    struct node *new = (struct node *) malloc(sizeof(struct node));
    new->right = right;
    new->left = left;
    new->bp = bp;
    new->no = no;
    return new;
}

static int notLeaf(struct node *n)  {
	return (n->left != NULL);
} 
static void destroytree(struct node *n) {
    if (n != NULL) {
		destroytree(n->right);
		destroytree(n->left);
		free(n);
    }
}

int size(struct node *n) {
    return (notLeaf(n))  ? 1 + size(n->left) + size(n->right) : 1;
}


int encodetreerec(struct node *tree, unsigned char *mem, int index) { 
	index++;	
	mem[index] = tree->no + ((tree->right == NULL) ? PRIMITIVES : 0);
	if (notLeaf(tree)) {
		index = encodetreerec(tree->left, mem, index);
		index = encodetreerec(tree->right, mem, index);	
	} 	
	return index;	
}

void encodetree(struct node *tree, unsigned char *mem) {
        int lastindex = encodetreerec(tree, mem, -1);
        mem[lastindex+1] = 255;
}

int pos = 0;
struct node *decodetree(struct node **bp, unsigned char *mem, int resetpos) { 
	struct node *tree;
	if (resetpos) pos = 0;
	tree = newnode(bp, NULL, NULL, mem[pos]);
	pos++;	
	if (tree->no < PRIMITIVES) {
		tree->left = decodetree(&tree->left, mem, 0);
		tree->right = decodetree(&tree->right, mem, 0);	
	} else {
		tree->no = tree->no - PRIMITIVES;
	}
	return tree;
}

static void printtreeindent(struct node *n, int indent) {
    int i = 0;
    if (n->right != NULL) {
		printf("\n");
		for (i = 0; i < indent; i++) printf("   ");
		printf("(%c", symbols[n->no]);
		printtreeindent(n->right, indent + 1);
		printtreeindent(n->left, indent + 1);
		printf(")");
    } else {
		if (n->no >= DIMENSION) {
			printf(" %f", constants[n->no - DIMENSION]);
		} else {
			printf(" X%i", n->no);
		}
    }
}

static void printtreexl(struct node *n, int indent) {
    int i = 0;
    if (n->right != NULL) {
                //printf("\n");
                //for (i = 0; i < indent; i++) printf("   ");
                
               	printf("(");
		printtreexl(n->right, indent + 1);
                printf("%c", symbols[n->no]);
		printtreexl(n->left, indent + 1);
               	printf(")");
    } else {
                if (n->no >= DIMENSION) {
                        printf("%f", constants[n->no - DIMENSION]);
                } else {
                        printf("A%i", n->no + DIMENSION); 
                }  
    }
}

float randfloat() {
    return (((float) rand()) / RAND_MAX);
}

unsigned char getrandomterminal() {
	return rand() % ((rand() % 2) ? DIMENSION:(DIMENSION + NCONSTANTS));
}

unsigned char getrandomprimitive() {
	return rand() % PRIMITIVES;
}

struct node *grow(struct node **bp, int maxdepth, int maxnodes){
    struct node *new = newnode(bp, NULL, NULL, 0);
	if ((maxnodes <= 2) || (maxdepth == 1)|| (randfloat() < (ratio * PTERMREALLY))) {	
		new->no =  getrandomterminal();
    } else {
		new->left = grow(&(new->left), maxdepth - 1, maxnodes- 2);
		new->right = grow(&(new->right), maxdepth - 1,maxnodes - 2 - size(new->left));
		new->no = getrandomprimitive();
    }
    return new;
}

int dataset = 0;
float eval(struct node *n) {
    return (notLeaf(n)) ? 
		((primitive) prim[n->no]) (eval(n->right), eval(n->left)) : 
		(n->no >= DIMENSION)? 
			constants[n->no - DIMENSION] : 
			input[(DIMENSION + 1) * dataset + n->no];
}

struct node *randomnodesc (struct node *tree, int wantedsize)  {
	return (size(tree) > wantedsize) ? randomnodesc(((rand() % 2) ? tree->left : tree->right), wantedsize) : tree;	
}

void crossoversp(struct node *tree1, struct node *tree2) {
    int maxsize1, maxsize2;
    int s_tree1 = size(tree1);
    int s_tree2 = size(tree2);	        
    struct node **tempbp;
    if (s_tree1 > s_tree2) { 
		struct node *temp;
		temp = tree1;
		tree1 = tree2;
		tree2 = temp;
		s_tree1 = size(tree1);
		s_tree2 = size(tree2);
    }	
    maxsize2 = ((rand() % (MAX_LEN - s_tree1)) % s_tree2) + 1; 
    tree2 = randomnodesc(tree2,maxsize2);	
    maxsize1 = 	((rand() % (MAX_LEN + size(tree2) - s_tree2)) % s_tree1) +  1;
    tree1 = randomnodesc(tree1,maxsize1);
    tempbp = tree1->bp;    
    *(tree1->bp) = tree2;
    tree1->bp = tree2->bp;	
    *(tree2->bp) = tree1;
    tree2->bp = tempbp;		
}

void pointmutategen(unsigned char *mem, int index) {	
	if (randfloat() < PMUT_PER_NODE) mem[index] = (mem[index] < PRIMITIVES) ?  getrandomprimitive() : PRIMITIVES + getrandomterminal();
	if (mem[index+1] != 255) pointmutategen(mem, index+1);
}

int sizegen(unsigned char *mem, int index) {	
	return (mem[index] != 255)? sizegen(mem, index+1) : index;
}

int main(int argc, char *argv[]) {
    int c, maxconst;
    extern char *optarg;
    int generation = 0;
    int individual = 0;
    unsigned long long int evaluatednodes = 0;
    unsigned char **popact, **popnew;    
    unsigned char **tempp;    
    float *fitness;
    struct node *actphenotype = NULL;
    struct node *prevphenotype = NULL;
    char reached = 0;  
    printf("SmallGP V 1.1 by Hannes Planatscher\nFree Software under GNU General Public License \n\n");
    while ((c = getopt (argc, argv, "l:p:d:g:c:m:t:s:i:b:f:r:n:e:h")) != -1) 
      switch (c) {
		case 'l': MAX_LEN = atoi(optarg); break;
		case 'p': POPSIZE = atoi(optarg); break; 
		case 'd': DEPTH = atoi(optarg); break; 
		case 'g': GENERATIONS = atoi(optarg); break;            
		case 'c': CROSSOVER_PROB = atof(optarg); break;            	
		case 'm': PMUT_PER_NODE = atof(optarg); break;
		case 'i': FNAME = optarg;	break;            	      
		case 't': TSIZE = atoi(optarg); break;
		case 's': SEED = atoi(optarg); break;  
		case 'f': LOWER_FITNESS_BOUND = atof(optarg);	break;  		
		case 'r': NCONSTANTS = atoi(optarg);  break;
		case 'e': PTERMREALLY = atof(optarg); break;
		case 'n': CONSTANTRANGE = atof(optarg); break;
		case 'h': printf("usage: smallgp [options]\n\noptions:\n\n\t-l\tmaximum program size\n\t-p\tpopulationsize\n\t-d\tmaximal initialization depth\n\t-g\tmax.generations\n\t-c\tprobability of crossover\n\t-m\tprobability of mutation per node \n\t-t\ttournament size\n\t-s\trandom seed\n\t-i\tinputfile (default:problem.data)\n\t-f\tlower bound for fitness\n\t-r\tnumber of random constants\n\t-n\tconstant range (+/-)\n\t-e\tgrow/full scaling (1.0 = grow)\n\t-h\tprints this help-message\n\nReport bugs to hannes@planatscher.net\n"); return 1;       	
		case '?': fprintf (stderr,"\nType 'smallgp -h' (without quotes) to get help on smallgp.\n\n"); exit(0);
		default: abort ();
      }        
    maxconst = 253 - PRIMITIVES - DIMENSION;
    NCONSTANTS = (NCONSTANTS > maxconst)? maxconst : NCONSTANTS;
    popact = (unsigned char **)malloc(POPSIZE * sizeof(unsigned char *));
    popnew = (unsigned char **)malloc(POPSIZE * sizeof(unsigned char *));
    for(individual = 0; individual < POPSIZE; individual++) {
    	popact[individual] = (unsigned char *)malloc(MAX_LEN * sizeof(unsigned char));    
    	popnew[individual] = (unsigned char *)malloc(MAX_LEN * sizeof(unsigned char));		
    }
    constants = (float*) malloc(NCONSTANTS * sizeof(float));
    for (c=0; c <NCONSTANTS; c++) constants[c] = (1 - 2 * randfloat()) * CONSTANTRANGE;
    fitness = malloc(POPSIZE * sizeof(float));            
    readFile(FNAME);
    printf("Settings:\n\nmaximum program size: \t\t\t%i\npopulationsize: \t\t\t%i\nmaximal initialization depth: \t\t%i\nmax.generations: \t\t\t%i\nprobability of crossover: \t\t%f\nprobability of mutation pernode: \t%f \ninputfile: \t\t\t\t%s \ntournament size: \t\t\t%i\nrandom seed: \t\t\t\t%i\nlower bound for fitness: \t\t%f\nnumber of random constants: \t\t%i\nconstant range (+/-): \t\t\t%f\nproblem dimension: \t\t\t%i\nfitness cases: \t\t\t\t%i\ngrow/full scaling:\t\t\t%f\n\n", MAX_LEN,POPSIZE,DEPTH,GENERATIONS,CROSSOVER_PROB,PMUT_PER_NODE,FNAME,TSIZE,SEED,LOWER_FITNESS_BOUND,NCONSTANTS, CONSTANTRANGE, DIMENSION, FITNESSCASES,PTERMREALLY);        
    ratio = ((float) DIMENSION+1) / ((float) (PRIMITIVES + DIMENSION+1)); 
    srand((SEED == -1)? time(NULL):SEED);
    dataset = 0;
    for(individual = 0; individual < POPSIZE; individual++) {
    	struct node *newtree = grow(&newtree, DEPTH,MAX_LEN);
		encodetree(newtree, popact[individual]);
		destroytree(newtree);
    }    
    for (generation = 0; generation < GENERATIONS; generation++) {
		int best = 0;
		long sumsize = 0;
		float sumfitness = 0;
		struct node *bestphen;
		printf("= Generation %5i ===========================================================\n", generation+1);
		for (individual = 0; individual < POPSIZE; individual++)  {	 			
			//  multithreading here

			struct node *phenotype = decodetree(&phenotype,popact[individual],1);
			fitness[individual] = 0;
			for (dataset = 0; dataset < FITNESSCASES; dataset++) 
				fitness[individual] += fabs(input[(DIMENSION + 1) * dataset + DIMENSION] - eval(phenotype));
			if  (!isfinite(fitness[individual])) fitness[individual] = 100000; 
			if  (fitness[best] > fitness[individual]) best = individual;							
			destroytree(phenotype); 							        			
			sumsize += sizegen(popact[individual],0);
			sumfitness += fitness[individual];		

	
		}	
		bestphen = decodetree(&bestphen,popact[best],1);		
		evaluatednodes += sumsize * FITNESSCASES;		
		printf("\ntotal nodes evaluated: %llu \naverage program size: %f\naverage fitness: %f\nbest fitness: %f\nbest individual:\n", evaluatednodes,((float) sumsize) / ((float) POPSIZE),sumfitness / ((float) POPSIZE),fitness[best]);	
//		printtreeindent(bestphen, 1);
		printf("\n");
		printtreexl(bestphen, 1);
		destroytree(bestphen);
		printf("\n\n========================================================================%5i\n\n", generation+1);		
		if (fitness[best] < LOWER_FITNESS_BOUND) {
			reached = 1;
			break;
		}	
		for (individual = 0; individual < POPSIZE; individual++) { 	/* Selection, Crossover, Mutation */
		    int selind = rand() % POPSIZE;
		    int t = 0;	
		    int i = 0;    	   
		    for (t = 0; t < TSIZE; t++) {
				int ri = rand() % POPSIZE;
				selind = (fitness[selind] < fitness[ri]) ? selind : ri;
		    }
		    for (i = 0; i < MAX_LEN; i++) popnew[individual][i] = popact[selind][i];	    	    
		    if ((((individual + 1) % 2) == 0) & (randfloat() < CROSSOVER_PROB)){
			    actphenotype = decodetree(&actphenotype,popnew[individual],1);		    
			    prevphenotype = decodetree(&prevphenotype,popnew[individual - 1],1);
			    crossoversp(actphenotype, prevphenotype);
			    encodetree(actphenotype, popnew[individual]);	    
   		    	    encodetree(prevphenotype, popnew[individual - 1]);
			    destroytree(actphenotype);
			    destroytree(prevphenotype);
		    }
		    if (randfloat() < (1 - CROSSOVER_PROB)) pointmutategen(popnew[individual],0);	    
		}
		tempp = popact;    
		popact = popnew;
		popnew = tempp;
    }
    for (individual = 0; individual < POPSIZE; individual++)  {
    	free(popact[individual]);
	free(popnew[individual]);
    }
    free(popact);
    free(popnew);
    free(fitness);
    free(input);
    printf("%s\n",(reached)? "run successful: lower fitnessbound reached":"run not successful: lower fitnessbound not reached");
    exit(!reached);
}
