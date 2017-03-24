/****************************************************************************/
/*								            */
/* This file is part of libDDD, a library for manipulation of DDD and SDD.  */
/*     						                            */
/*     Copyright (C) 2001-2008 Yann Thierry-Mieg, Jean-Michel Couvreur      */
/*                             and Denis Poitrenaud                         */
/*     						                            */
/*     This program is free software; you can redistribute it and/or modify */
/*     it under the terms of the GNU Lesser General Public License as       */
/*     published by the Free Software Foundation; either version 3 of the   */
/*     License, or (at your option) any later version.                      */
/*     This program is distributed in the hope that it will be useful,      */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/*     GNU LEsserGeneral Public License for more details.                   */
/*     						                            */
/* You should have received a copy of the GNU Lesser General Public License */
/*     along with this program; if not, write to the Free Software          */
/*Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */
/*     						                            */
/****************************************************************************/

#include <iostream>
using namespace std;

#include "ddd/DDD.h"
#include "ddd/DED.h"
#include "ddd/MemoryManager.h"
#include "ddd/statistic.hpp"

typedef enum {A, B, C, D,E, F, G} var;
var variables;
const char* vn[]= {"A", "B", "C", "D", "E", "F", "G"};

void initName() {
	for (int i=A; i<=G; i++)
		DDD::varName(i,vn[i]);
}

int main(){
	// Define a name for each variable
	initName();


	DDD d1(A,1);
	DDD d2(B,2);
	d2=d1;


	Statistic s(d1,"before garbage",CSV);

	MemoryManager::garbage();
	Statistic s2(d2,"after garbage",CSV);

	s.print_header(cout);
	s.print_line(cout);
	s2.print_line(cout);

	return 0;
}
