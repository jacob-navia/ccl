#include "../containers.h"
#define TAILLE 100000000
int main(void)
{
	List *ma_liste;
	int i,y,f;

	ma_liste = iList.Create(sizeof(int));
	for (f=0; f<TAILLE; f++) {
		for (i=0; i<TAILLE; i++) {
			iList.Add(ma_liste,&i);
		}
		for (y=TAILLE-1; y>=0; y--) {
			size_t test = y;
			iList.EraseAt(ma_liste,test);
		}
	}
}
