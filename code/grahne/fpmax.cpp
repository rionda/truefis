/*
   Author:  Jianfei Zhu  
            Concordia University
   Date:    Sep. 26, 2003

Copyright (c) 2003, Concordia University, Montreal, Canada
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
   - Neither the name of Concordia University nor the names of its
       contributors may be used to endorse or promote products derived from
       this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.

*/

/* This is an implementation of FP-growth* / FPmax* /FPclose algorithm.
 *
 * last updated Sep. 26, 2003
 *
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <time.h>
#include "common.h"
#include "buffer.h"

using namespace std;    // two of two, yay!

#define LINT sizeof(int)

int *ITlen;
int* bran;
int* prefix;

int* order_item;		// given order i, order_item[i] gives itemname
int* item_order;		// given item i, item_order[i] gives its new order 
						//	order_item[item_order[i]]=i; item_order[order_item[i]]=i;
bool* current_fi;
int* compact;
int* supp;

MFI_tree** mfitrees;
CFI_tree** cfitrees;

stack* list;
int TRANSACTION_NO=0;
int ITEM_NO=100;
int THRESHOLD;

memory* fp_buf;

void printLen()
{
	int i, j;
	for(i=ITEM_NO-1; i>=0&&ITlen[i]==0; i--);
	for(j=0; j<=i; j++) 
		printf("%d\n", ITlen[j]);
}


int main(int argc, char **argv)
{
	if (argc < 3)
	{
	  cout << "usage: fmi <infile> <MINSUP> [<outfile>]\n";
	  exit(1);
	}
	THRESHOLD = atoi(argv[2]);

	int i;
	FI_tree* fptree;

	Data* fdat=new Data(argv[1]);

	if(!fdat->isOpen()) {
		cerr << argv[1] << " could not be opened!" << endl;
		exit(2);
	}

	fp_buf=new memory(60, 4194304L, 8388608L, 2);
	fptree = (FI_tree*)fp_buf->newbuf(1, sizeof(FI_tree));
	fptree->init(-1, 0);
	fptree -> scan1_DB(fdat);
	ITlen = new int[fptree->itemno];
	bran = new int[fptree->itemno];
	compact = new int[fptree->itemno];
	prefix = new int[fptree->itemno];

#ifdef CFI
		list=new stack(fptree->itemno, true); 
#else
		list=new stack(fptree->itemno); 
#endif

	assert(list!=NULL && bran!=NULL && compact!=NULL && ITlen!=NULL && prefix!=NULL);

	for(i =0; i < fptree->itemno; i++)
	{
		ITlen[i] = 0L;
		bran[i] = 0;
	}

	fptree->scan2_DB(fdat);
    fdat->close();


	FSout* fout;
	if(argc==4)
	{
		fout = new FSout(argv[3]);

		//print the count of emptyset
#ifdef FI
		fout->printSet(0, NULL, TRANSACTION_NO);
#endif

#ifdef CFI
		if(TRANSACTION_NO != fptree->count[0])
			fout->printSet(0, NULL, TRANSACTION_NO);
#endif			
	}else
		fout = NULL;


	if(fptree->Single_path())
	{
		Fnode* node;
		int i=0;
		for(node=fptree->Root->leftchild; node!=NULL; node=node->leftchild)
		{
			list->FS[i++]=node->itemname;
#ifdef CFI
				list->counts[i-1] = node->count;
#endif
		}

#ifdef FI
			fptree->generate_all(fptree->itemno, fout);
#endif

#ifdef CFI
			int Count;
			i=0;
			while(i<fptree->itemno)
			{
				Count = list->counts[i];
				for(; i<fptree->itemno && list->counts[i]==Count; i++);
				ITlen[i-1]++;
				fout->printSet(i, list->FS, Count);
			}
#endif
		
#ifdef MFI
			fout->printSet(fptree->itemno, list->FS, fptree->head[fptree->itemno-1]->count);
			ITlen[i-1]=1;
#endif
		printLen();
		return 0;
	}

	current_fi = new bool[fptree->itemno];
	supp=new int[fptree->itemno];		//for keeping support of items
	assert(supp!=NULL&&current_fi!=NULL);

	for(i = 0; i<fptree->itemno; i++)
	{
		current_fi[i] = false;
		supp[i]=0;
	}

#ifdef MFI
	MFI_tree* LMFI;
		mfitrees = (MFI_tree**)new MFI_tree*[fptree->itemno];
		memory* Max_buf=new memory(40, 1048576L, 5242880, 2);
		LMFI = (MFI_tree*)Max_buf->newbuf(1, sizeof(MFI_tree));
		LMFI->init(Max_buf, fptree, NULL, NULL, -1);
		fptree->set_max_tree(LMFI);
		mfitrees[0] = LMFI;
		fptree->FPmax(fout);
#endif

#ifdef CFI
	CFI_tree* LClose;
		cfitrees = (CFI_tree**)new CFI_tree*[fptree->itemno];
		memory* Close_buf=new memory(40, 1048576L, 5242880, 2);
		LClose = (CFI_tree*)Close_buf->newbuf(1, sizeof(CFI_tree));
		LClose->init(Close_buf, fptree, NULL, NULL, -1);
		fptree->set_close_tree(LClose);
		cfitrees[0] = LClose;
		fptree->FPclose(fout);
#endif

#ifdef FI
		fptree->FP_growth(fout);
#endif

	printLen();
	if(fout)
		fout->close();

//	delete fp_buf;
	delete list;
	delete []current_fi;
	delete []supp;

	return 0;
}
