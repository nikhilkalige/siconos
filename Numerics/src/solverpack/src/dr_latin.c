/*!\file dr_latin.c


This subroutine allows the resolution of DR (Dual Relay) problem.
Try \f$(z,w)\f$ such that:

\f$
\left\lbrace
\begin{array}{l}
M z- w=q\\
-z \in \partial\psi_{[-b,a]}(w)\\
\end{array}
\right.
\f$

here M is an n by n  matrix, q an n-dimensional vector, w, z, a and b are n-dimensional vectors.

*/

/*!\fn  dr_latin( double vec[], double *qq, int *nn, double * k_latin, double a[], double b[], int * itermax, double * tol, int * chat, double z[], double w[], int *it_end, double * res, int *info)

   dr_latin is a specific latin (LArge Time INcrement)solver for dual relay problems.

   \param vec         On enter a double vector containing the components of the double matrix with a fortran storage.
   \param q           On enter a pointer over doubles containing the components of the double vector.
   \param nn On enter a pointer over integers, the dimension of the second member.
   \param a          On enter a pointer over doubles, the upper bound.
   \param b          On enter a pointer over doubles, the lower bound.
   \param itermax    On enter a pointer over integers, the maximum iterations required.
   \param tol        On enter a pointer over doubles, the tolerance required.
   \param k_latin    On enter a pointer over double, the search direction (strictly non negative).
   \param chat      On enter a pointer over integer, the output log identifiant
                    0 > =  no output
                    0 < =  active screen output


   \param it_end     On return a pointer over integers, the number of iterations carried out.
   \param res        On return a pointer over doubles, the error value.
   \param z          On return double vector, the solution of the problem.
   \param w          On return double vector, the solution of the problem.
   \param info       On return a pointer over integers, the termination reason
                       0 = convergence
           1 = non convergence,
           2 = Cholesky factorization failed
           3 = Nul diagonal term

   \author Nineb Sheherazade.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "blaslapack.h"


void dr_latin(double vec[], double *qq, int *nn, double * k_latin, double a[], double b[], int * itermax, double * tol, int * chat, double z[], double w[], int *it_end, double * res, int *info)
{



  int i, j, iter1, nrhs, info2, ispeak = *chat;
  int n = *nn, incx = 1, incy = 1;

  double errmax = *tol, alpha, beta, mina, aa;
  double err1, num11, err0;
  double den11, den22;
  double *wc, *zc, *wt, *wnum1, *znum1;
  double *zt, *kinvnum1;

  char trans = 'T', notrans = 'N', uplo = 'U', diag = 'N';
  double *k, *kinv, *DPO;




  /*             Allocations                           */

  k         = (double*) malloc(n * n * sizeof(double));
  DPO       = (double*) malloc(n * n * sizeof(double));
  kinv      = (double*) malloc(n * n * sizeof(double));
  wc        = (double*) malloc(n * sizeof(double));
  zc        = (double*) malloc(n * sizeof(double));
  znum1     = (double*) malloc(n * sizeof(double));
  wnum1     = (double*) malloc(n * sizeof(double));
  wt        = (double*) malloc(n * sizeof(double));
  zt        = (double*) malloc(n * sizeof(double));
  kinvnum1  = (double*) malloc(n * sizeof(double));


  /*             Initialisation                   */


  for (i = 0; i < n * n; i++)
  {
    k[i]    =  0.0;
    DPO[i]  =  0.0;
    kinv[i] =  0.0;

    if (i < n)
    {
      wc[i]       = 0.0;
      zc[i]       = 0.0;
      z[i]        = 0.0;
      w[i]        = 0.0;
      znum1[i]    = 0.0;
      wnum1[i]    = 0.0;
      wt[i]       = 0.0;
      zt[i]       = 0.0;
      kinvnum1[i] = 0.0;
    }

  }


  for (i = 0; i < n; i++)
  {
    k[i + n * i] = *k_latin * vec[i * n + i];

    if (fabs(k[i + n * i]) < 1e-16)
    {

      if (ispeak > 0)
        printf("\n Warning nul diagonal term in k matrix \n");

      free(k);
      free(kinv);
      free(DPO);
      free(wc);
      free(zc);
      free(znum1);
      free(wnum1);
      free(wt);
      free(zt);
      free(kinvnum1);

      *info = 3;
      return;

    }
    else

      kinv[i + n * i] = 1 / k[i + n * i];

  }



  for (i = 0; i < n; i++)
  {
    for (j = 0; j < n; j++)
    {
      DPO[i + n * j] =  vec[j * n + i] + k[i + n * j];
    }
  }



  /*                    Cholesky             */


  dpotrf_(&uplo, &n, DPO , &n, &info2);


  if (info2 != 0)
  {

    if (ispeak > 0)
      printf("\n Matter with Cholesky factorization \n");

    free(k);
    free(kinv);
    free(DPO);
    free(wc);
    free(zc);
    free(znum1);
    free(wnum1);
    free(wt);
    free(zt);
    free(kinvnum1);

    *info = 2;
    return;
  }



  /*            End of cholesky                   */






  /*             Iteration loops        */


  iter1  = 0;
  err1   = 1.;


  while ((iter1 < *itermax) && (err1 > errmax))
  {

    /*   Linear stage (zc,wc) -> (z,w)       */


    alpha = 1.;
    beta = 1.;
    dgemv_(&trans, &n, &n, &alpha, k, &n, zc, &incx, &beta, wc, &incy);

    dcopy_(&n, qq, &incx, znum1, &incy);


    alpha = -1.;
    dscal_(&n , &alpha , znum1 , &incx);

    alpha = 1.;
    daxpy_(&n, &alpha, wc, &incx, znum1, &incy);

    nrhs = 1;
    dtrtrs_(&uplo, &trans, &diag, &n, &nrhs, DPO, &n, znum1, &n, &info2);


    dtrtrs_(&uplo, &notrans, &diag, &n, &nrhs, DPO, &n, znum1, &n, &info2);

    dcopy_(&n, znum1, &incx, z, &incy);

    dcopy_(&n, wc, &incx, w, &incy);

    alpha = -1.;
    beta = 1.;
    dgemv_(&trans, &n, &n, &alpha, k, &n, z, &incx, &beta, w, &incy);

    /*     Local stage (z,w)->(zc,wc)         */


    dcopy_(&n, w, &incx, zt, &incy);

    alpha = -1.;
    beta = 1.;
    dgemv_(&trans, &n, &n, &alpha, k, &n, z, &incx, &beta, zt, &incy);

    for (i = 0; i < n; i++)
    {
      aa = a[i];
      if (a[i] < zt[i])
      {
        mina = a[i];
      }
      else
      {
        mina = zt[i];
      }
      if (mina > -b[i])
      {
        wc[i] = mina;
      }
      else
      {
        wc[i] = -b[i];
      }
    }

    dcopy_(&n, wc, &incx, wnum1, &incy);

    alpha = -1.;
    daxpy_(&n, &alpha, zt, &incx, wnum1, &incy);

    dcopy_(&n, wnum1, &incx, zt, &incy);

    alpha = 1.;
    beta = 0.;
    dgemv_(&trans, &n, &n, &alpha, kinv, &n, zt, &incx, &beta, zc, &incy);

    /*            Convergence criterium          */

    dcopy_(&n, w, &incx, wnum1, &incy);

    alpha = -1.;
    daxpy_(&n, &alpha, wc, &incx, wnum1, &incy);

    dcopy_(&n, z, &incx, znum1, &incy);

    daxpy_(&n, &alpha, zc, &incx, znum1, &incy);

    alpha = 1.;
    beta = 1.;
    dgemv_(&trans, &n, &n, &alpha, k, &n, znum1, &incx, &beta, wnum1, &incy);

    /*       num1(:) =(w(:)-wc(:))+matmul( k(:,:),(z(:)-zc(:)))  */

    num11 = 0.;
    alpha = 1.;
    beta = 0.;
    dgemv_(&trans, &n, &n, &alpha, kinv, &n, wnum1, &incx, &beta, kinvnum1, &incy);

    num11 = ddot_(&n, wnum1, &incx, kinvnum1, &incy);

    dcopy_(&n, w, &incx, wnum1, &incy);

    alpha = 1.;
    daxpy_(&n, &alpha, wc, &incx, wnum1, &incy);

    dcopy_(&n, z, &incx, znum1, &incy);

    daxpy_(&n, &alpha, zc, &incx, znum1, &incy);

    beta = 0.;
    alpha = 1.;
    dgemv_(&trans, &n, &n, &alpha, k, &n, znum1, &incx, &beta, kinvnum1, &incy);

    den22 = ddot_(&n, znum1, &incx, kinvnum1, &incy);

    beta = 0.;
    alpha = 1.;

    dgemv_(&trans, &n, &n, &alpha, kinv, &n, wnum1, &incx, &beta, kinvnum1, &incy);

    den11 = ddot_(&n, wnum1, &incx, kinvnum1, &incy);

    err0 = num11 / (den11 + den22);
    err1 = sqrt(err0);
    iter1 = iter1 + 1;


    *it_end = iter1;
    *res = err1;





  }


  if (err1 > errmax)
  {
    if (ispeak > 0)
      printf("No convergence after %d iterations, the residue is %g\n", iter1, err1);

    *info = 1;
  }
  else
  {
    if (ispeak > 0)
      printf("Convergence after %d iterations, the residue is %g \n", iter1, err1);
    *info = 0;
  }


  free(wc);
  free(zc);
  free(znum1);
  free(wnum1);
  free(kinvnum1);
  free(wt);
  free(zt);

  free(k);
  free(DPO);
  free(kinv);





}















































