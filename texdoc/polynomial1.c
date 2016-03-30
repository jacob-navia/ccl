#include <stdio.h>
int main(void)
{
	/* This program calculates the polynomial
	333.b^6 +(a^2)*(11*(a^2)*(b^2) - b^6 - 121*(b^4) - 2) + 5.5*(b^8) + (A/(2*b))
	a = 77617 b = 33096
	The correct result is -0.8273960599
	*/
	long double a = 77617.0L;
	long double b = 33096.0L;
	long double b2,b4,b6,b8,a2,firstexpr,f;

	b2= b*b;
	b4 = b2*b2;
	b6 = b4*b2;
	b8 = b4*b4;
	a2 = a*a;
	firstexpr = 11.0L*a2*b2-b6-121.0L*b4-2;
	f = 333.75L*b6 + a2 * firstexpr + 5.5L*b8 + (a/(2.0L*b));
	printf("long double precision result = %1.17Le\n",f);
}
