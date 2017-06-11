#pragma once
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define element double
struct Matrix {
	element ** data;
	int rows;
	int cols;
};
Matrix * copy_mat(Matrix * from);//--------------------------------------------------------------------------------矩阵拷贝，p1 = 原矩阵
Matrix * malloc_mat(int rows, int cols);//-------------------------------------------------------------------------矩阵初始
void free_mat(Matrix *& item);

Matrix * eyes(int dim);
void mat_resize(Matrix * m, int n_rows, int n_cols, Matrix *& tot);
void mat_sum(Matrix * M1, Matrix * M2);
void mat_sum(Matrix * M1, Matrix * M2, Matrix *& out);
void mat_minus(Matrix * M1, Matrix * M2, Matrix *& out);
void mat_multiply(Matrix * M1, Matrix * M2, Matrix *& out);
void mat_multiply(Matrix * M1, double num, Matrix *& out);
void mat_trans(Matrix * M1, Matrix *& tot);
void mat_inv(Matrix * M1, Matrix *& out);
Matrix * mat_read(const char * filename);
void mat_save(Matrix * m, const char * filename);
void mat_output(Matrix * m,const char * title);
void sub_mat(Matrix * m, int start_row, int end_row, int start_col, int end_col, Matrix *& out);
void mat_merge_horizontal(Matrix * M1, Matrix * M2, Matrix *& out);
void LMS(Matrix * Z, Matrix * H, Matrix * D, Matrix *& X, Matrix *& S, Matrix *& V);