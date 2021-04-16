//THIS C PROGRAM WAS TAKEAN FROM: http://www.comp.dit.ie/rlawlor/Alg_DS/sorting/quickSort.c
//WE DID NOT WRITE IT

void quicksort(int number[],int first,int last);
void cmplx_mult_scalar( int dest[], int src0[], int src1[], int size );
asm("li $sp, 0x23FFFFFC");
int entry()
{
  
  int size = 100;
  int dest[size*2];

  for ( int i = 0; i < size*2; i++ )
    dest[i] = 0;

  int src0[size*3];
  int src1[size*3];

  for (int i = 0;i <size*3 ; i++)
  {
    src0[i] = 2*i;
    src1[i] = i;
  }

  cmplx_mult_scalar( dest, src0, src1, size );

  return 0;

   //quicksort(number,0,i-1);
   //return number[0];
}

void cmplx_mult_scalar( int dest[], int src0[],
                        int src1[], int size )
{
  int i;
  for ( i = 0; i < size; i++ ) {
    dest[i*2]   = (src0[i*2] * src1[i*2]) - (src0[i*2+1] * src1[i*2+1]);
    dest[i*2+1] = (src0[i*2] * src1[i*2+1]) + (src0[i*2+1] * src1[i*2]);
  }
}


void quicksort(int number[],int first,int last){
   int i, j, pivot, temp;

   if(first<last){
      pivot=first;
      i=first;
      j=last;

      while(i<j){
         while(number[i]<=number[pivot]&&i<last)
            i++;
         while(number[j]>number[pivot])
            j--;
         if(i<j){
            temp=number[i];
            number[i]=number[j];
            number[j]=temp;
         }
      }

      temp=number[pivot];
      number[pivot]=number[j];
      number[j]=temp;
      quicksort(number,first,j-1);
      quicksort(number,j+1,last);

   }
}

