#include <containers.h>
int main(int argc,char *argv[])
{
         FILE *f;
         int i=1,r;
         Dictionary *dict;
         char buf[8192];
         if (argc < 2) {
                 fprintf(stderr,"%s <file name>\n",argv[0]);
                 return -1;
         }
         f = fopen(argv[1],"r");
         if (f == NULL) {
				printf("Could not open %s\n",argv[1]);
                 return -1;
		}
         dict = iDictionary.Create(0,500);
         if (dict == NULL) {
				printf("Could not create dictionary\n");
                 return -1;
		}
         while (fgets(buf,sizeof(buf),f)) {
                 r= iDictionary.Add(dict,buf,NULL);
                 if (r > 0)
                         printf("[%3d] %s",i,buf);
                 else if (r < 0) break;
                 i++;
         }
         iDictionary.Finalize(dict);
         fclose(f);
} 
