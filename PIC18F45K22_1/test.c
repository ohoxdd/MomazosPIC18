
#include "GLCD.h"
#include "test.h"

int sum(int a, int b) {
	return a + b;
}

void writeMatrix(int start_page, int start_col, int fil, int col, char* matrix) {
	int array_ind = 0;
	for (int i = 0; i < fil; ++i) {
		for (int j = 0; j < col; ++j) {
			writeByte(start_page + i, start_col + j, matrix[array_ind]);
			array_ind++;
		}
	}
}
