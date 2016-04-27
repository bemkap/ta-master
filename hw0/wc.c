#include<stdio.h>

int main(int argc, char *argv[]) {
  if(argc<1) return 1;
  FILE*fp=fopen(argv[1],"r");
  if(fp==NULL) return 1;
  int c,wc=0,cc=0,lc=0,i=1;
  while((c=fgetc(fp))!=EOF){
    if(c==' '||c=='\n') i=1;
    else if(c!=' '&&c!='\n'&&i){i=0;wc++;}
    if(c=='\n') lc++;
    cc++;
  }
  printf("%d %d %d %s\n",lc,wc,cc,argv[0]);
  fclose(fp);
  return 0;
}
