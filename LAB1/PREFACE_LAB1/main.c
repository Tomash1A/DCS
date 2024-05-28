// #include  <msp430xG46x.h> /* include peripheral declarations */
#include <msp430.h>

#define M 10 
#define max(x,y) x>y ? x : y

int ComputeTrace(int Mat[M][M]);
int DiagonalsMax(int Mat[M][M]);
void Transpose(int Mat1[M][M],int Mat2[M][M]); 
void ReplaceDiagonals(int Mat1[M][M],int Mat2[M][M]);
void FillMatrix(int Mat[M][M]);

//-------------- Global variables ------------
  int maxTrace,maxDiag;
//--------------------------------------------
void main(){
  WDTCTL = WDTPW | WDTHOLD;     // Stop WDT
        
  int Mat1[M][M],Mat2[M][M];
  int mat1Trace,mat2Trace,max1Diag,max2Diag;
  int Selector=0;
  
  FillMatrix(Mat1);
  
  while(1) {  	   
	   
  switch(Selector){
    case 0: 
      break;     
    case 1: 
      mat1Trace = ComputeTrace(Mat1);
      mat2Trace = ComputeTrace(Mat2); 
      maxTrace = mat1Trace > mat2Trace ? mat1Trace : mat2Trace;
      Selector = 0;
      break;
    case 2: 
      Transpose(Mat1,Mat2);
      Selector = 0;
      break;
    case 3: 
      ReplaceDiagonals(Mat1,Mat2);
      Selector = 0;
      break;
    case 4: 
      max1Diag = DiagonalsMax(Mat1);
      max2Diag = DiagonalsMax(Mat2);
      maxDiag = max1Diag > max2Diag ? max1Diag : max2Diag;
      Selector = 0;
      break;
  }
 }	
}
//---------------------------------------------------------------
//                  Trace Computation
//---------------------------------------------------------------
int ComputeTrace(int Mat[M][M]){
	int Trace=0,i;
	for(i=0 ; i<M ; i++) Trace += Mat[i][i]; 
	return Trace;
}
//---------------------------------------------------------------
//                  DiagonalsMax Computation
//---------------------------------------------------------------
int DiagonalsMax(int Mat[M][M]){
	int Max=0,i;
		for(i=0 ; i<M ; i++){
			Max = max(Mat[i][i],Max);
			Max = max(Mat[i][N-1-i],Max);
		} 
		return Max;
}
//---------------------------------------------------------------
//                  Diagonals Replacement 
//---------------------------------------------------------------
void ReplaceDiagonals(int Mat1[M][M],int Mat2[M][M]){
	int i;
	    FillMatrix(Mat2);
		for(i=0 ; i<M ; i++){
			Mat2[i][i] = Mat1[i][M-1-i];
			Mat2[i][M-1-i] = Mat1[i][i];
		} 
}
//---------------------------------------------------------------
//                  Matrix Transpose 
//---------------------------------------------------------------
void Transpose(int Mat1[M][M],int Mat2[M][M]){
	int i,j;
		for(i=0 ; i<M ; i++){
			for(j=0 ; j<M ; j++){
				Mat2[i][j] = Mat1[j][i];
				Mat2[j][i] = Mat1[i][j];
			}
		} 
}
//---------------------------------------------------------------
//                  Fill Matrix
//---------------------------------------------------------------
void FillMatrix(int Mat[M][M]){
	int i,j;
	for(i=0 ; i<M ; i++){
		for(j=0 ; j<M ; j++){
			Mat[i][j] = i*M+j;		
		}
	} 
}
