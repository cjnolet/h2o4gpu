#include <algorithm>
#include <cstring>

#include "gsl/gsl_blas.h"
#include "gsl/gsl_matrix.h"
#include "gsl/gsl_vector.h"
#include "equil_helper.h"
#include "matrix/matrix.h"
#include "matrix/matrix_dense.h"
#include "util.h"

namespace pogs {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// Helper Functions ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
namespace {

// File scoped constants.
const NormTypes kNormEquilibrate = kNorm2; 
const NormTypes kNormNormalize   = kNormFro;

template<typename T>
struct CpuData {
  const T *orig_data;
  CpuData(const T *orig_data) : orig_data(orig_data) { }
};

CBLAS_TRANSPOSE_t OpToCblasOp(char trans) {
  ASSERT(trans == 'n' || trans == 'N' || trans == 't' || trans == 'T');
  return trans == 'n' || trans == 'N' ? CblasNoTrans : CblasTrans;
}

template <typename T>
T NormEst(NormTypes norm_type, const MatrixDense<T>& A);

template <typename T>
void MultDiag(const T *d, const T *e, size_t m, size_t n,
              typename MatrixDense<T>::Ord ord, T *data);

}  // namespace

////////////////////////////////////////////////////////////////////////////////
/////////////////////// MatrixDense Implementation /////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template <typename T>
MatrixDense<T>::MatrixDense(int wDev, char ord, size_t m, size_t n, const T *data)
  : Matrix<T>(m, n, 0), _wDev(wDev), _datatype(0), _data(0) {
  _datay=NULL;
  _vdata=NULL;
  _vdatay=NULL;

  ASSERT(ord == 'r' || ord == 'R' || ord == 'c' || ord == 'C');
  _ord = (ord == 'r' || ord == 'R') ? ROW : COL;

  // Set GPU specific _info.
  CpuData<T> *info = new CpuData<T>(data);
  this->_info = reinterpret_cast<void*>(info);

  // Copy Matrix to CPU
  if(1==0){ // can't because _data contents get modified
    _data = const_cast<T*>(data);
  }
  else{
    _data = new T[this->_m * this->_n];
    ASSERT(_data != 0);
    memcpy(_data, info->orig_data, this->_m * this->_n * sizeof(T));
  }

}
template <typename T>
MatrixDense<T>::MatrixDense(char ord, size_t m, size_t n, const T *data)
  : MatrixDense<T>(0,ord,m,n,data){}



  // no use of datatype when on CPU
template <typename T>
MatrixDense<T>::MatrixDense(int wDev, int datatype, char ord, size_t m, size_t n, T *data)
  : Matrix<T>(m, n, 0), _wDev(wDev), _datatype(datatype),_data(0) {
  _datay=NULL;
  _vdata=NULL;
  _vdatay=NULL;

  ASSERT(ord == 'r' || ord == 'R' || ord == 'c' || ord == 'C');
  _ord = (ord == 'r' || ord == 'R') ? ROW : COL;

  // Set GPU specific _info.
  CpuData<T> *info = new CpuData<T>(data);
  this->_info = reinterpret_cast<void*>(info);
  if(1==1){
    _data = data;
  }
  else{
    _data = new T[this->_m * this->_n];
    ASSERT(_data != 0);
    memcpy(_data, info->orig_data, this->_m * this->_n * sizeof(T));
  }


}
template <typename T>
MatrixDense<T>::MatrixDense(int wDev, char ord, size_t m, size_t n, size_t mValid, const T *data, const T *datay, const T *vdata, const T *vdatay)
  : Matrix<T>(m, n, mValid), _wDev(wDev), _datatype(0),_data(0), _datay(0), _vdata(0), _vdatay(0) {

  ASSERT(ord == 'r' || ord == 'R' || ord == 'c' || ord == 'C');
  _ord = (ord == 'r' || ord == 'R') ? ROW : COL;
  fprintf(stderr,"ord=%c m=%d n=%d mValid=%d\n",ord,(int)m,(int)n,int(mValid));

  CpuData<T> *info = new CpuData<T>(data); // new structure (holds pointer to data and GPU handle)
  CpuData<T> *infoy = new CpuData<T>(datay); // new structure (holds pointer to data and GPU handle)
  CpuData<T> *vinfo = new CpuData<T>(vdata); // new structure (holds pointer to data and GPU handle)
  CpuData<T> *vinfoy = new CpuData<T>(vdatay); // new structure (holds pointer to data and GPU handle)
  this->_info = reinterpret_cast<void*>(info);
  this->_infoy = reinterpret_cast<void*>(infoy);
  this->_vinfo = reinterpret_cast<void*>(vinfo);
  this->_vinfoy = reinterpret_cast<void*>(vinfoy);

  if(1==0){ // can't because _data contents get modified
    _data = const_cast<T*>(data);
    _datay = const_cast<T*>(datay);
    _vdata = const_cast<T*>(vdata);
    _vdatay = const_cast<T*>(vdatay);
  }
  else{
    _data = new T[this->_m * this->_n];
    ASSERT(_data != 0);
    memcpy(_data, info->orig_data, this->_m * this->_n * sizeof(T)); 

    _datay = new T[this->_m];
    ASSERT(_datay != 0);
    memcpy(_datay, infoy->orig_data, this->_m * sizeof(T)); 
    
    _vdata = new T[this->_mvalid * this->_n];
    ASSERT(_vdata != 0);
    memcpy(_vdata, vinfo->orig_data, this->_mvalid * this->_n * sizeof(T)); 

    _vdatay = new T[this->_mvalid];
    ASSERT(_vdatay != 0);
    memcpy(_vdatay, vinfoy->orig_data, this->_mvalid * sizeof(T)); 
  }
  
}

  // no use of datatype
template <typename T>
MatrixDense<T>::MatrixDense(int wDev, int datatype, char ord, size_t m, size_t n, size_t mValid, T *data, T *datay, T *vdata, T *vdatay)
  : Matrix<T>(m, n, mValid), _wDev(wDev), _datatype(datatype),_data(0), _datay(0), _vdata(0), _vdatay(0) {

  _ord = (ord == 'r' || ord == 'R') ? ROW : COL;

  CpuData<T> *info = new CpuData<T>(data);
  CpuData<T> *infoy = new CpuData<T>(datay);
  CpuData<T> *vinfo = new CpuData<T>(vdata);
  CpuData<T> *vinfoy = new CpuData<T>(vdatay);
  this->_info = reinterpret_cast<void*>(info);
  this->_infoy = reinterpret_cast<void*>(infoy);
  this->_vinfo = reinterpret_cast<void*>(vinfo);
  this->_vinfoy = reinterpret_cast<void*>(vinfoy);

  if(1==0){
    _data = const_cast<T*>(data);
    _datay = const_cast<T*>(datay);
    _vdata = const_cast<T*>(vdata);
    _vdatay = const_cast<T*>(vdatay);
  }
  else{
    _data = new T[this->_m * this->_n];
    ASSERT(_data != 0);
    memcpy(_data, info->orig_data, this->_m * this->_n * sizeof(T)); 

    _datay = new T[this->_m];
    ASSERT(_datay != 0);
    memcpy(_datay, infoy->orig_data, this->_m * sizeof(T)); 
    
    _vdata = new T[this->_mvalid * this->_n];
    ASSERT(_vdata != 0);
    memcpy(_vdata, vinfo->orig_data, this->_mvalid * this->_n * sizeof(T)); 

    _vdatay = new T[this->_mvalid];
    ASSERT(_vdatay != 0);
    memcpy(_vdatay, vinfoy->orig_data, this->_mvalid * sizeof(T)); 
  }
#if 0
    if (ord=='r') {
      std::cout << m << std::endl;
      std::cout << n << std::endl;
      for (int i=0; i<m; ++i) {
        std::cout << std::endl;
        for (int j = 0; j < n; ++j) {
          std::cout<< *(_data + i*n+j) << " ";
        }
        std::cout << " -> " << *(_datay + i);
      }
    } else {
      std::cout << m << std::endl;
      std::cout << n << std::endl;
      for (int i=0; i<m; ++i) {
        std::cout << std::endl;
        for (int j = 0; j < n; ++j) {
          std::cout<< *(_data + j*m+i) << " ";
        }
        std::cout << " -> " << *(_datay + i);
      }
    }
#endif

}
  
template <typename T>
MatrixDense<T>::MatrixDense(int wDev, const MatrixDense<T>& A)
  : Matrix<T>(A._m, A._n, A._mvalid), _wDev(wDev), _data(0), _datay(0), _vdata(0), _vdatay(0), _ord(A._ord) {

  CpuData<T> *info_A   = reinterpret_cast<CpuData<T>*>(A._info); // cast from void to CpuData
  CpuData<T> *infoy_A  = reinterpret_cast<CpuData<T>*>(A._infoy); // cast from void to CpuData
  CpuData<T> *vinfo_A  = reinterpret_cast<CpuData<T>*>(A._vinfo); // cast from void to CpuData
  CpuData<T> *vinfoy_A = reinterpret_cast<CpuData<T>*>(A._vinfoy); // cast from void to CpuData

  CpuData<T> *info;
  CpuData<T> *infoy;
  CpuData<T> *vinfo;
  CpuData<T> *vinfoy;
  if(A._data) info = new CpuData<T>(info_A->orig_data); // create new CpuData structure with point to CPU data
  if(A._datay) infoy  = new CpuData<T>(infoy_A->orig_data); // create new CpuData structure with point to CPU data
  if(A._vdata) vinfo  = new CpuData<T>(vinfo_A->orig_data); // create new CpuData structure with point to CPU data
  if(A._vdatay) vinfoy = new CpuData<T>(vinfoy_A->orig_data); // create new CpuData structure with point to CPU data

  if(A._data) this->_info = reinterpret_cast<void*>(info); // back to cast as void
  if(A._datay) this->_infoy = reinterpret_cast<void*>(infoy); // back to cast as void
  if(A._vdata)  this->_vinfo = reinterpret_cast<void*>(vinfo); // back to cast as void
  if(A._vdatay) this->_vinfoy = reinterpret_cast<void*>(vinfoy); // back to cast as void          

  if(1==1){
    _data   = A._data;
    _datay  = A._datay;
    _vdata  = A._vdata;
    _vdatay = A._vdatay;
  }
  else{
    _data = new T[A._m * A._n];
    ASSERT(_data != 0);
    memcpy(_data, info_A->orig_data, A._m * A._n * sizeof(T)); 

    _datay = new T[A._m];
    ASSERT(_datay != 0);
    memcpy(_datay, infoy_A->orig_data, A._m * sizeof(T)); 
    
    _vdata = new T[A._mvalid * A._n];
    ASSERT(_vdata != 0);
    memcpy(_vdata, vinfo_A->orig_data, A._mvalid * A._n * sizeof(T)); 

    _vdatay = new T[A._mvalid];
    ASSERT(_vdatay != 0);
    memcpy(_vdatay, vinfoy_A->orig_data, A._mvalid * sizeof(T)); 
  }

  
}

template <typename T>
MatrixDense<T>::MatrixDense(const MatrixDense<T>& A)
  : MatrixDense<T>(A._wDev, A){}

  template <typename T>
MatrixDense<T>::~MatrixDense() {
  CpuData<T> *info = reinterpret_cast<CpuData<T>*>(this->_info);
  CpuData<T> *infoy = reinterpret_cast<CpuData<T>*>(this->_infoy);
  CpuData<T> *vinfo = reinterpret_cast<CpuData<T>*>(this->_vinfo);
  CpuData<T> *vinfoy = reinterpret_cast<CpuData<T>*>(this->_vinfoy);
  if(info) delete info;
  if(infoy) delete infoy;
  if(vinfo) delete vinfo;
  if(vinfoy) delete vinfoy;
  this->_info = 0;
  this->_infoy = 0;
  this->_vinfo = 0;
  this->_vinfoy = 0;
}

template <typename T>
int MatrixDense<T>::Init() {
  DEBUG_EXPECT(!this->_done_init);
  if (this->_done_init)
    return 1;
  this->_done_init = true;

  return 0;
}

template <typename T>
void MatrixDense<T>::GetTrainX(int datatype, size_t size, T**data) const {
  std::memcpy(*data, _data, size * sizeof(T));
}
template <typename T>
void MatrixDense<T>::GetTrainY(int datatype, size_t size, T**data) const {
  std::memcpy(*data, _datay, size * sizeof(T));
}

template <typename T>
void MatrixDense<T>::GetValidX(int datatype, size_t size, T**data) const {
  std::memcpy(*data, _vdata, size * sizeof(T));
}
template <typename T>
void MatrixDense<T>::GetValidY(int datatype, size_t size, T**data) const {
  std::memcpy(*data, _vdatay, size * sizeof(T));
}


  template <typename T>
int MatrixDense<T>::Mul(char trans, T alpha, const T *x, T beta, T *y) const {
DEBUG_EXPECT(this->_done_init);
if (!this->_done_init)
  return 1;

const gsl::vector<T> x_vec = gsl::vector_view_array<T>(x, this->_n);
gsl::vector<T> y_vec = gsl::vector_view_array<T>(y, this->_m);

if (_ord == ROW) {
  gsl::matrix<T, CblasRowMajor> A =
      gsl::matrix_view_array<T, CblasRowMajor>(_data, this->_m, this->_n);
  gsl::blas_gemv(OpToCblasOp(trans), alpha, &A, &x_vec, beta,
      &y_vec);
  } else {
    gsl::matrix<T, CblasColMajor> A =
        gsl::matrix_view_array<T, CblasColMajor>(_data, this->_m, this->_n);
    gsl::blas_gemv(OpToCblasOp(trans), alpha, &A, &x_vec, beta, &y_vec);
  }

  return 0;
}

  template <typename T>
int MatrixDense<T>::Mulvalid(char trans, T alpha, const T *x, T beta, T *y) const {
DEBUG_EXPECT(this->_done_init);
if (!this->_done_init)
  return 1;

const gsl::vector<T> x_vec = gsl::vector_view_array<T>(x, this->_n);
gsl::vector<T> y_vec = gsl::vector_view_array<T>(y, this->_mvalid);

if (_ord == ROW) {
  gsl::matrix<T, CblasRowMajor> A =
      gsl::matrix_view_array<T, CblasRowMajor>(_vdata, this->_mvalid, this->_n);
  gsl::blas_gemv(OpToCblasOp(trans), alpha, &A, &x_vec, beta,
      &y_vec);
  } else {
    gsl::matrix<T, CblasColMajor> A =
        gsl::matrix_view_array<T, CblasColMajor>(_vdata, this->_mvalid, this->_n);
    gsl::blas_gemv(OpToCblasOp(trans), alpha, &A, &x_vec, beta, &y_vec);
  }

  return 0;
}

template <typename T>
int MatrixDense<T>::Equil(T *d, T *e, bool equillocal) {
  DEBUG_ASSERT(this->_done_init);
  if (!this->_done_init)
    return 1;

  // Number of elements in matrix.
  size_t num_el = this->_m * this->_n;

  // Create bit-vector with signs of entries in A and then let A = f(A),
  // where f = |A| or f = |A|.^2.
  unsigned char *sign = 0;
  size_t num_sign_bytes = (num_el + 7) / 8;
  sign = new unsigned char[num_sign_bytes];
  ASSERT(sign != 0);

  // Fill sign bits, assigning each thread a multiple of 8 elements.
  size_t num_chars = num_el / 8;
  if (kNormEquilibrate == kNorm2 || kNormEquilibrate == kNormFro) {
    SetSign(_data, sign, num_chars, SquareF<T>());
  } else {
    SetSign(_data, sign, num_chars, AbsF<T>());
  }

  // If numel(A) is not a multiple of 8, then we need to set the last couple
  // of sign bits too. 
  if (num_el > num_chars * 8) {
    if (kNormEquilibrate == kNorm2 || kNormEquilibrate == kNormFro) {
      SetSignSingle(_data + num_chars * 8, sign + num_chars,
          num_el - num_chars * 8, SquareF<T>());
    } else {
      SetSignSingle(_data + num_chars * 8, sign + num_chars, 
          num_el - num_chars * 8, AbsF<T>());
    }
  }

  // Perform Sinkhorn-Knopp equilibration.
  SinkhornKnopp(this, d, e, equillocal);

  // Transform A = sign(A) .* sqrt(A) if 2-norm equilibration was performed,
  // or A = sign(A) .* A if the 1-norm was equilibrated.
  if (kNormEquilibrate == kNorm2 || kNormEquilibrate == kNormFro) {
    UnSetSign(_data, sign, num_chars, SqrtF<T>());
  } else {
    UnSetSign(_data, sign, num_chars, IdentityF<T>());
  }

  // Deal with last few entries if num_el is not a multiple of 8.
  if (num_el > num_chars * 8) {
    if (kNormEquilibrate == kNorm2 || kNormEquilibrate == kNormFro) {
     UnSetSignSingle(_data + num_chars * 8, sign + num_chars, 
          num_el - num_chars * 8, SqrtF<T>());
    } else {
      UnSetSignSingle(_data + num_chars * 8, sign + num_chars, 
          num_el - num_chars * 8, IdentityF<T>());
    }
  }

  // Compute D := sqrt(D), E := sqrt(E), if 2-norm was equilibrated.
  if (kNormEquilibrate == kNorm2 || kNormEquilibrate == kNormFro) {
    std::transform(d, d + this->_m, d, SqrtF<T>());
    std::transform(e, e + this->_n, e, SqrtF<T>());
  }

  // Compute A := D * A * E.
  MultDiag(d, e, this->_m, this->_n, _ord, _data);

  // Scale A to have norm of 1 (in the kNormNormalize norm).
  T normA = NormEst(kNormNormalize, *this);
  gsl::vector<T> a_vec = gsl::vector_view_array(_data, num_el);
  gsl::vector_scale(&a_vec, 1 / normA);

  // Scale d and e to account for normalization of A.
  gsl::vector<T> d_vec = gsl::vector_view_array<T>(d, this->_m);
  gsl::vector<T> e_vec = gsl::vector_view_array<T>(e, this->_n);
  gsl::vector_scale(&d_vec, 1 / std::sqrt(normA));
  gsl::vector_scale(&e_vec, 1 / std::sqrt(normA));

  DEBUG_PRINTF("norm A = %e, normd = %e, norme = %e\n", normA,
      gsl::blas_nrm2(&d_vec), gsl::blas_nrm2(&e_vec));

  delete [] sign;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////// Equilibration Helpers //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
namespace {

// Estimates norm of A. norm_type should either be kNorm2 or kNormFro.
template <typename T>
T NormEst(NormTypes norm_type, const MatrixDense<T>& A) {
  switch (norm_type) {
    case kNorm2: {
      return Norm2Est(&A);
    }
    case kNormFro: {
      const gsl::vector<T> a = gsl::vector_view_array(A.Data(),
          A.Rows() * A.Cols());
      return gsl::blas_nrm2(&a) /
          std::sqrt(static_cast<T>(std::min(A.Rows(), A.Cols())));
    }
    case kNorm1:
      // 1-norm normalization doens't make make sense since it treats rows and
      // columns differently.
    default:
      ASSERT(false);
      return static_cast<T>(0.);
  }
}

// Performs A := D * A * E for A in row major
template <typename T>
void MultRow(size_t m, size_t n, const T *d, const T *e, T *data) {
#ifdef _OPENMP
#pragma omp parallel for
#endif
  for (size_t t = 0; t < m * n; ++t)
    data[t] *= d[t / n] * e[t % n];
}

// Performs A := D * A * E for A in col major
template <typename T>
void MultCol(size_t m, size_t n, const T *d, const T *e, T *data) {
#ifdef _OPENMP
#pragma omp parallel for
#endif
  for (size_t t = 0; t < m * n; ++t)
    data[t] *= d[t % m] * e[t / m];
}

template <typename T>
void MultDiag(const T *d, const T *e, size_t m, size_t n,
              typename MatrixDense<T>::Ord ord, T *data) {
  if (ord == MatrixDense<T>::ROW) {
    MultRow(m, n, d, e, data);
  } else {
    MultCol(m, n, d, e, data);
  }
}


}  // namespace

// Explicit template instantiation.
#if !defined(POGS_DOUBLE) || POGS_DOUBLE==1
template class MatrixDense<double>;
#endif

#if !defined(POGS_SINGLE) || POGS_SINGLE==1
template class MatrixDense<float>;
#endif

}  // namespace pogs

