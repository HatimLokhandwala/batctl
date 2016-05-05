#include <stdio.h>
#include <string.h>
#include "neighbourRead.h"

FILE *fp;
char *NEIGHBORPATH= "/sys/kernel/debug/batman_adv/bat0/neighbors";

int openFile(){
	fp = fopen(NEIGHBORPATH,"rw");
	//char *line = NULL;
	
	if(fp==NULL) return 1;
	else return 0;
}


int readFile(){
	char line[1000];
	fgets(line,sizeof(line),fp);
	//printf("%s",line);
		fgets(line,sizeof(line),fp);
	//printf("%s",line);
		fgets(line,sizeof(line),fp);
	//if(strcmp(line,"No batman nodes in range ...\n")==0)	return 1;
	//else
	 return 0;
}

int getNeighbourTable(){
	rewind(fp);
	char line[1000];
	fgets(line,sizeof(line),fp);
	fgets(line,sizeof(line),fp);
	fgets(line,sizeof(line),fp);
	
	//printf("%s",line);
	return 0;
}


void  closeFile(){
		fclose(fp);
		
	}









