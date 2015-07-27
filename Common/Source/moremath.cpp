// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

// Quagmire - i was really surprised, but i couldnt find the equivilent standard library function
signed char sign(signed int tmp) {
	if (tmp > 0)
		return 1;
	else if (tmp < 0)
		return -1;
	else
		return 0;
}

signed char sign(double tmp) {
	if (tmp > 0)
		return 1;
	else if (tmp < 0)
		return -1;
	else
		return 0;
}
