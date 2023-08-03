# smallgp.c

Symbolic regression is a process of finding a mathematical formula that best fits a given set of data points without assuming a specific form or structure for the equation

# Introduction

SmallGP is a implementation of Symbolic Regression which is compact by 
means of lines of code and memory usage. 

- Population sizes of hundreds of thousands or millions of inidivuals are possible.
- Functions are restricted to +,-,*,/
- Consists of just 1 C-file without non-standard dependencies
- outputs formulas in Excel-format: just copy&paste (check out video)
- should compile on Linux, MacOS & Windows 

[![Video](https://img.youtube.com/vi/krNsDzarpoY/maxresdefault.jpg)](https://www.youtube.com/watch?v=krNsDzarpoY)

# Usage

SmallGP is a commandline-tool. All important parameters can be changed easily.

Display the help with

```
> smallgp -h
```
to learn which parameters are avaiable.

Example parameters for a test run:

```
dataset: 		problem.data
population size: 	100000
tournament size:	500
maximum generations:	80
maximum prog. length: 	50
maximum depth:		7
#constants: 		200
Pcross:			0.7
Pmut/Node:		0.1
```

To start smallgp with these parameters just run
```
> smallgp -i test.data -p 100000 -t 500 -g 80 -l 50 -d 7 -r 200 -c 0.7 -m 0.1
```
. 
Warning: Parameters are not checked for validity (ex. negative 
probabilities etc.).  	

# Implementation Details

## Representation of individuals

SmallGP uses the concept of genotype-phenotype-duality.

Consider the following tree:

```
   (+)
   / \
  x1 (+) 
     / \
    x2 c2
```	
This can be written in polish notation:

```
 + x1 + x2 c2
```


Due limited amount of different symbols it is possible to store this
expression in a vector of integer values.
```
   (+)
   / \
  x1 (+)   => + x1 + x2 c2 => (2,5,2,6,16)
     / \
    x2 c2
```
The integer vector represents the genotype, while the phenotype 
is a standard  binary-tree, that consists of nodes containing 
information which are holding pointers to their children and 
their predecessor.
```
struct node {
    struct node *right;
    struct node *left;
    struct node **bp;
    unsigned char no;
};
```
node->no specifies the primitive function/terminal represented by this node.
It can easily be deducted that the maximum number of different primitves and
terminals is limited to 255 with using an 8-bit  datatype.
This phenotypic representation enables easy implementation of crossover and 
evaluation. 

With 104 bits memory needed per node it isnt really memory efficient, 
as every node contains just 8 bit of information, so we use the genotype
for storage.

The phenotype is easily converted to the genotype: a vector of 8-bit values.
The genotype is far more memory efficient than the phenotypic representation, 
but the implementation of subtree crossover and evaluation is less trivial.

SmallGP uses the phenotype-genotype-duality to save memory and to allow 
easy manipulation. While for evaluation and crossover individuals are 
temporarily expanded to their phenotype, point mutation can be 
performed on the genotype.

## Crossover

SmallGP implements a size-safe kozastyle subtree-crossover[4]. Subtrees of
appropriate size are selected from two individuals, and then exchanged. It is
guaranteed that the size constraint is met by both individuals after the
operation.

Suppose that size(tree1) < size(tree2).

tree1 can be extended by (MAXLEN - size(tree1)) nodes. The maximum size
of a subtree of tree2 is size(tree2) so we fix a maximum size for the 
subtree selected from tree2 by a positive random integer value 
smaller than
```
((MAX_LEN - size(tree1)) modulo size(tree2)) = maxsize2
```
After the selection of a subtree subtree_2 with a maximal size 
maxsize2 from tree2, we can determine maxsize1. tree2 can be extended 
by (MAXLEN - size(tree2) + size(subtree2)).
As the maximum size of a subtree from tree1 is size(tree1) we choose
a positive random integer value not bigger than
```
((MAX_LEN - size(tree2) + size(subtree2)) modulo size(tree1)) = maxsize1
```


## Mutation

SmallGP implements point-mutation, that simply substitutes a terminal with
another terminal, and a primitive with another primitive [5].

## Strategy

SmallGP uses a generational strategy, with tournament selection.

## Initialization

The tree in the initial population are initialized with a size-safe
"Grow"-Method. The probabilty of choosing a terminal at a certain depth
is obtained as follows:
```
		 |primitives| + |variables| + 1
       P_term =	------------------------------
			|variables| + 1
```
Constants are considered as one virtual terminal symbol.

Due to the "Grow"-Method most of the individuals are very small in
the initial population. 
This, in combination with the previously defined point mutation, can 
cause lack of structural  innovation in some cases. 
So an additional parameter P_term_really is introduced that allows 
to scale between "Grow" and "Full" initialization.
A terminal is set with the probability:
```
	 P_term' =  P_term * P_term_really
```
. With P_term_really = 1 the method equals "Grow", with P_term_really = 0
"Full". The defaultvalue of Pterm is 1.


# References

[1] Genetic Programming, Symbolic Regression 
	http://www.genetic-programming.org/

[2] Tiny GP specification:
	http://cswww.essex.ac.uk/staff/sml/gecco/TinyGP.html

[3] Polish Notation
	http://www.cenius.net/refer/display.php?ArticleID=polishnotation

[4] Subtree Crossover
	http://www.geneticprogramming.com/Tutorial/#anchor179890

[5] Genetic Programming with One-Point Crossover and Point-Mutation 
   (Poli, Langdon)  
	http://cswww.essex.ac.uk/staff/poli/papers/Poli-WSC2-1997.pdf

# License

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


