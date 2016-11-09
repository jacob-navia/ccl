#include "../intdlist.h"
#include <list>
#include <iostream>
#include <stdio.h>

#define TABSIZE 1000
int main()
{
	std::list<int> tab[TABSIZE];
#if 1
    std::cout << sizeof(std::list<int>) << " "
              << sizeof(std::list<int>::iterator) << "\n"
              << sizeof(INTERFACE_STRUCT_INTERNAL_NAME(int)) << " "
              << sizeof(ITERATOR(int)) << "\n";
    
#endif
	for (int i=0; i< TABSIZE; i++) {
		std::list<int> ll;
		tab[i] = ll;
	}
	printf("Tab is: %zu\n",sizeof(tab));
}
