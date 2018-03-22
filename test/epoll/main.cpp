
#include <iostream>
#include "Epoll.h"

int main()
{

	printf(".... Test Epoll ....\n");

	CEpoll	 Epoll;
	Epoll.EpStart(6, 10000, 10000);
	


	return 0;
}




