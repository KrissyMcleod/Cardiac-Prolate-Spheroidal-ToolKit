/*=========================================================================

Program:   ImagingSciences
Module:    $Id: itkProlateSpheroidalTransform.txx 1 2010-05-21 14:00:33Z nt08 $
Language:  C++
Author:    $Author: nt08 $
Date:      $Date: 2010-05-21 14:00:33 +0000 (Fri, 21 May 2010) $
Version:   $Revision: 1 $

Copyright (c) 2010 King's College London - Division of Imaging Sciences. All rights reserved.
See Copyright.txt for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef _itk_ProlateSpheroidalTransform_txx_
#define _itk_ProlateSpheroidalTransform_txx_

#include "itkProlateSpheroidalTransform.h"
#include <itksys/SystemTools.hxx>

#include <fstream>
#include <cstdlib>
#include <sstream>
#include <cmath>
#include <limits>
#include <cassert>

namespace itk
{
  /*
   * Constructor. 
   */
  template <class TPixelType>
  ProlateSpheroidalTransform<TPixelType>::ProlateSpheroidalTransform() :
    Superclass (OutputSpaceDimension, ParametersDimension)
  {
    this->m_Parameters.SetSize (ParametersDimension);
    this->m_Parameters.Fill (0.0);

    this->m_FixedParameters.SetSize (0);
    
    this->m_Jacobian.SetSize (OutputSpaceDimension,InputSpaceDimension);
    this->m_Jacobian.Fill (0.0);
    
    m_Forward = 1;
    
    m_Center.Fill (0);
    m_LongAxisPoint.Fill (0);
    m_ShortAxisPoint.Fill (0);
    
    m_Focus1.Fill (0);
    m_Focus2.Fill (0);
    m_InternalTransform.SetIdentity();
    m_InternalTransformInverse.SetIdentity ();

    m_LongAxisPoint[0] = std::cosh (1.0);
    m_ShortAxisPoint[1] = std::sinh (1.0);

    this->ComputeTransformation();
  }


  /*
   * Destructor. 
   */
  template <class TPixelType>
  ProlateSpheroidalTransform<TPixelType>::~ProlateSpheroidalTransform()
  {
    
  }


  template <class TPixelType>
  void ProlateSpheroidalTransform<TPixelType>::ComputeTransformation (void)
  {
    unsigned int count = 0;
    for (unsigned int i=0; i<3; i++) this->m_Parameters[count++] = m_Center[i];
    for (unsigned int i=0; i<3; i++) this->m_Parameters[count++] = m_LongAxisPoint[i];
    for (unsigned int i=0; i<3; i++) this->m_Parameters[count++] = m_ShortAxisPoint[i];

    m_Axis1 = m_LongAxisPoint  - m_Center;
    m_Axis2 = m_ShortAxisPoint - m_Center;

    m_Lambda1 = m_Axis1.GetNorm();
    m_Lambda2 = m_Axis2.GetNorm();
    m_Lambda3 = m_Lambda2;

    m_Axis1.Normalize ();
    m_Axis2.Normalize ();
    m_Axis3 = CrossProduct (m_Axis1, m_Axis2);
    // We absolutely need the axes of the spheroid to be orthogonal !
    // By trusting Axis1, we can re-compute axis2 by cross-product
    m_Axis2 = CrossProduct (m_Axis3, m_Axis1);

    // A is the rigid transformation matrix from R1 to R0
    for (unsigned int i=0; i<3; i++)
    {
      m_InternalTransform[i][0] = m_Axis1[i];
      m_InternalTransform[i][1] = m_Axis2[i];
      m_InternalTransform[i][2] = m_Axis3[i];
      m_InternalTransform[i][3] = m_Center[i];
    }
    
    // A-1 is then the transformation matrix from R0 to R1
    m_InternalTransformInverse = m_InternalTransform.GetInverse();

    // The coefficients a b and c correspond to the coefficients of the
    // ellipsoid equation : x2/a2 + y2/b2 + z2/c2 = 1, defined in R1
    m_Coefficients[0] =  m_Lambda1;
    m_Coefficients[1] =  m_Lambda2;
    m_Coefficients[2] =  m_Lambda3;
    // so we have a > b = c ==> prolate spheroid, centered in R1 origin, aligned at x axis, in R1;
    double Eccentricity = std::sqrt (m_Lambda1 * m_Lambda1 - m_Lambda2 * m_Lambda2) / m_Lambda1;
    m_Eccentricity = Eccentricity;

    m_Focus1 = m_Center + Eccentricity * m_Lambda1 * m_Axis1;
    m_Focus2 = m_Center - Eccentricity * m_Lambda1 * m_Axis1;
    
    // The semi foci distance (d in Costa paper 1996)
    m_SemiFociDistance = m_Focus1.EuclideanDistanceTo (m_Focus2) / 2.0;
    
  }
  
  template <class TPixelType>
  typename ProlateSpheroidalTransform<TPixelType>::OutputPointType ProlateSpheroidalTransform<TPixelType>::TransformPoint(const InputPointType  &x ) const
  {
    OutputPointType ret;
    if (m_Forward)
      ret = this->ToProlate (x);
    else
      ret = this->ToCartesian (x);
    return ret;
  }

  template <class TPixelType>
  typename ProlateSpheroidalTransform<TPixelType>::OutputVectorType ProlateSpheroidalTransform<TPixelType>::TransformVector(const InputVectorType &v, const InputPointType &p) const
  {
    OutputVectorType ret;
    if (m_Forward)
      ret = this->ToProlate (v, p);
    else
      ret = this->ToCartesian (v, p);
    return ret;
  }

  template <class TPixelType>
  typename ProlateSpheroidalTransform<TPixelType>::OutputVnlVectorType ProlateSpheroidalTransform<TPixelType>::TransformVector(const InputVnlVectorType &v, const InputPointType &p) const
  {
    VectorType mv, mret;
    OutputVnlVectorType ret;
    for (unsigned int i=0; i<3; i++) mv[i] = v[i];
    mret = this->TransformVector (mv, p);
    for (unsigned int i=0; i<3; i++) ret[i] = mret[i];
    return ret;
  }

  template <class TPixelType>
  typename ProlateSpheroidalTransform<TPixelType>::OutputCovariantVectorType ProlateSpheroidalTransform<TPixelType>::TransformCovariantVector(const InputCovariantVectorType &v, const InputPointType &p) const
  {
    VectorType mv, mret;
    OutputCovariantVectorType ret;
    for (unsigned int i=0; i<3; i++) mv[i] = v[i];
    mret = this->TransformVector (mv, p);
    for (unsigned int i=0; i<3; i++) ret[i] = mret[i];
    return ret;
  }

  template <class TPixelType>
  void ProlateSpheroidalTransform<TPixelType>::SetParameters (const ParametersType &p)
  {
    this->m_Parameters = p;
    unsigned int count = 0;
    for (unsigned int i=0; i<3; i++) m_Center[i] = this->m_Parameters[count++];
    for (unsigned int i=0; i<3; i++) m_LongAxisPoint[i] = this->m_Parameters[count++];
    for (unsigned int i=0; i<3; i++) m_ShortAxisPoint[i] = this->m_Parameters[count++];

    this->ComputeTransformation();
  }

  template <class TPixelType>
  void ProlateSpheroidalTransform<TPixelType>::SetParametersByValue (const ParametersType &p)
  {
    this->m_Parameters.SetSize (p.GetSize());
    for (unsigned int i=0; i<p.GetSize(); i++)
      this->m_Parameters[i] = p[i];
    unsigned int count = 0;
    for (unsigned int i=0; i<3; i++) m_Center[i] = this->m_Parameters[count++];
    for (unsigned int i=0; i<3; i++) m_LongAxisPoint[i] = this->m_Parameters[count++];
    for (unsigned int i=0; i<3; i++) m_ShortAxisPoint[i] = this->m_Parameters[count++];

    this->ComputeTransformation();
  }

  template <class TPixelType>
  typename ProlateSpheroidalTransform<TPixelType>::JacobianType & ProlateSpheroidalTransform<TPixelType>::GetJacobian(const InputPointType  &x) const
  {
    // jacobian according to the "parameters" is definitely NOT defined...
    itkWarningMacro (<<"jacobian according to the \"parameters\" is definitely NOT defined..."<<"\n");
    
    return this->m_Jacobian;
  }

  template <class TPixelType>
  vnl_matrix_fixed<TPixelType,3,3> ProlateSpheroidalTransform<TPixelType>::GetJacobianWithRespectToCoordinates(const InputPointType  &x) const
  {

    vnl_matrix_fixed<TPixelType,3,3> jacobian(0.0);
    jacobian[0][0] = jacobian[1][1] = jacobian[2][2] = 1.0;
    
    // error overwhich we consider the system to be inconsistent (non- orthogonal)
    const double epsilon_orthogonality = 0.001;

    // recover the Prolate coordinates point
    PointType xsi = m_Forward ? this->TransformPoint (x) : x;

    // error underwhich we display a warning because we reached the singularity
    const double epsilon_singularity = 0.001;
        
    if ( (xsi[0] <= epsilon_singularity) || (xsi[1] <= epsilon_singularity) )
    {
      itkWarningMacro (<<"singularity point : "<<xsi<<"\n"
		       <<"jacobian matrix undefined --> set to identity");
      return jacobian;
    }
    
    UniformVectorType ns, as, ds;
    VectorType n, a, d;
    ns[0] = sinh(xsi[0]) * std::cos(xsi[1]);
    ns[1] = cosh(xsi[0]) * std::sin(xsi[1]) * std::cos(xsi[2]);
    ns[2] = cosh(xsi[0]) * std::sin(xsi[1]) * std::sin(xsi[2]);
    ns[3] = 0;
    
    as[0] = - cosh(xsi[0]) * std::sin(xsi[1]);
    as[1] =   sinh(xsi[0]) * std::cos(xsi[1]) * std::cos(xsi[2]);
    as[2] =   sinh(xsi[0]) * std::cos(xsi[1]) * std::sin(xsi[2]);
    as[3] = 0;
    
    ds[0] = 0;
    ds[1] = - sinh(xsi[0]) * std::sin(xsi[1]) * std::sin(xsi[2]);
    ds[2] = sinh(xsi[0]) * std::sin(xsi[1]) * std::cos(xsi[2]);
    ds[3] = 0;
    
    // the contra-variant basis vectors are in R1 referential
    // they have to be transformed to R0 with m_InternalTransform
    // Also, they have to be scaled by the
    // constant \f$ f_{1/2} \f$ (m_SemiFociDistance).
    ns = m_InternalTransform * (m_SemiFociDistance * ns);
    as = m_InternalTransform * (m_SemiFociDistance * as);
    ds = m_InternalTransform * (m_SemiFociDistance * ds);
    
    // As we are looking for only re-orientation component of
    // the Jacobian Matrix, we simply multiply the columns by
    // the scale factors:
    double h[3] = {1.0, 1.0, 1.0};
    this->EvaluateScaleFactors (xsi.GetDataPointer(), h);
    
    for (unsigned int i=0; i<3; i++)
    {
      n[i] = ns[i] * h[0];
      a[i] = as[i] * h[1];
      d[i] = ds[i] * h[2];
    }

    // The resulting basis vectors are normals and direct now,
    // with a numerical error < 0.0005 in practice
    
    // fill the output jacobian matrix with column vectors
    for (unsigned int i=0; i<3; i++)
    {
      jacobian[i][0] = n[i];
      jacobian[i][1] = a[i];
      jacobian[i][2] = d[i];
    }

    // a warning is shown is the basis is inconsistent (non-orthogonality) 
    // [jacobian] is a local orthonormal prolate spheroid basis at x;
    // and should be orthogonal and normal basis, let's check that:
    if ( std::abs ( vnl_determinant (jacobian) - 1.0 ) > epsilon_orthogonality ) 
    {
      itkWarningMacro (<<"CAUTION !!! prolate local basis vectors are not orthogonal..."<<std::endl
      		       <<"determinant : "<<vnl_determinant (jacobian)<<std::endl);
    }
    
    // transpose the matrix if we are looking for the inverse transformation
    if (!m_Forward)
      jacobian.inplace_transpose();

    // NB : the jacobian matrix is orthonormal but is NOT a rotation matrix.
    // Its determinant is always negative egual to -1 (roto-inversion).
    // This is due to the fact that by construction the basis (g1,g2,g3)
    // is orthogonal but NOT direct. however it does not influence any
    // further process in prolate sph.
    // NB2 : THIS HAS CHANGED. the basis is now DIRECT
    // There was no reason why the basis was actually indirect.
    // A sign error was introduced to $ g_3 $ making the basis indirect,
    // this has been corrected, Note that it did not affect any further results.
    return jacobian;
    
  }
  
  template <class TPixelType>
  void ProlateSpheroidalTransform<TPixelType>::EvaluateLocalBasis(InputPointType x,
						      VectorType &n,
						      VectorType &a,
						      VectorType &d) const
  {
    vnl_matrix_fixed<double,3,3> jacobian = this->GetJacobianWithRespectToCoordinates (x);
    // The basis vectors correspond to each column of the jacobian matrix
    for (unsigned int i=0; i<3; i++)
    {
      n[i] = jacobian[i][0];
      a[i] = jacobian[i][1];
      d[i] = jacobian[i][2];
    }
  }
  
  template <class TPixelType>
  bool ProlateSpheroidalTransform<TPixelType>::GetInverse( Self* inverse) const
  {
    inverse->SetParameters (this->m_Parameters);
    inverse->SetForward (!this->GetForward());
    return true;
  }
  
  template <class TPixelType>
  void ProlateSpheroidalTransform<TPixelType>::SetIdentity(void)
  {
    m_Center.Fill (0.0);
    m_LongAxisPoint.Fill (0.0);
    m_ShortAxisPoint.Fill (0.0);
    m_LongAxisPoint[0] = std::cosh (1.0);
    m_ShortAxisPoint[1] = std::sinh (1.0);

    this->ComputeTransformation();
  }

  template <class TPixelType>
  typename ProlateSpheroidalTransform<TPixelType>::PointType ProlateSpheroidalTransform<TPixelType>::ToCartesian(PointType xsi) const
  {

    
    // xsi has constrains and bounds, lets check them;
    if ( ( (xsi[0] < 0.0) ) ||
	 ( (xsi[1] < 0.0) || (xsi[1] >     vnl_math::pi) ) ||
	 ( (xsi[2] < 0.0) || (xsi[2] > 2 * vnl_math::pi) ) )
    {
      itkDebugMacro (<<"inconsistent Prolate Coordinate : "<<xsi);
    }
    
    // put the point back into the x-aligned aligned spheroid
    UniformVectorType xs;

    xs[0] = m_SemiFociDistance * std::cosh(xsi[0]) * std::cos ( xsi [1]);
    xs[1] = m_SemiFociDistance * std::sinh(xsi[0]) * std::sin ( xsi [1]) * std::cos (xsi[2]);
    xs[2] = m_SemiFociDistance * std::sinh(xsi[0]) * std::sin ( xsi [1]) * std::sin (xsi[2]);    
    xs[3] = 1;
    
    xs = m_InternalTransform* xs;

    PointType x;
    for (unsigned int i=0; i<3; i++)
      x[i] = xs[i];
    return x;
  }


  
  template <class TPixelType>
  typename ProlateSpheroidalTransform<TPixelType>::PointType ProlateSpheroidalTransform<TPixelType>::ToProlate(PointType x) const
  {
    PointType xsi;

    // numerical precision for null tests
    const double epsilon = vcl_numeric_limits<double>::epsilon();

    // put the point back into the x-aligned aligned spheroid,
    // i.e. in referential R1
    UniformVectorType xs;
    for (unsigned int i=0; i<3; i++)
    {
      xs[i]=x[i];
    }
    
    xs[3] = 1;
    xs = m_InternalTransformInverse* xs;

    // define a few variables for the polynomial in alpha = sin^2(\xsi_2)
    double A = m_SemiFociDistance*m_SemiFociDistance;
    double B = xs[1]*xs[1] + xs[2]*xs[2];
    double C = xs[0]*xs[0];
    double gamma1 = (A - B - C + std::sqrt((A-B-C)*(A-B-C) + 4*A*B) ) / (2.0*A);
    // by definition gamma1 is positive and should be lower than 1
    // as we'll compute asin(sqrt(gamma1)) below.
    // make sure of this. there could be numerical errors.
    if (gamma1 > (1.0 - epsilon))
      gamma1 = 1.0 - epsilon;
    
    VectorType axis;
    
    // axis2 is the y axis, origin of xsi[2] angle.
    VectorType axis2(0.0);
    axis2[1] = 1;
    
    for (unsigned int i=0; i<3; i++)
      axis[i] = xs[i];
    
    // we project our point to the yz plane by nulling the x component.
    axis[0] = 0;
    // then we normalize it --> we are in the xy unit circle
    // if axis.GetNorm() is null then it means we are directly along the
    // axis of revolution, where xsi[2] is undefined.
    // then we impose xsi[2] = 0 by using axis = axis2.
    if (axis.GetNorm() > epsilon)
      axis.Normalize();
    else
      axis = axis2;
    // then xsi[2] is well defined, apart from cos ambiguity.
    xsi[2] = std::acos (axis[1]);
    // cos ambiguity removed by checking the z sign
    if (axis[2] < 0)
      xsi[2] = 2*vnl_math::pi - xsi[2];
    
    // the real xsi[1] is here,
    xsi[1] = std::asin (std::sqrt (gamma1));
    // sin ambiguity removed by checking the x sign
    if (xs[0] < 0)
      xsi[1] = vnl_math::pi - xsi[1];
    // the real xsi[0] is here,
    double j1;
    if (gamma1 > epsilon)
    {
      j1 = std::sqrt (B / (A * gamma1));
    }
    else
    {
      // if gamma1 is null, it means we are the axis of revolution,
      // then we have :
      // j1 = std::sqrt (C/A - 1);
      // this equation holds everywhere along the axis BUT between focii,
      // where this train terminates. i.e. prolates coord. are not defined...
      if (std::abs (xs[0]) > m_SemiFociDistance)
      {
	j1 = std::sqrt (C/A - 1);
      }
      else
      {
	// we reached the singularity axis between focii, impose xsi[0] = 0
	// by using j1 = 0:
	j1 = 0;
	itkWarningMacro (<<"reach singularity axis between foci\n");
      }
    }

    // the function arcsinh is taken as sqrt and logs
    xsi[0] = std::log (j1 + std::sqrt (j1 * j1 + 1.0));

    // xsi has constrains and bounds, let's check them;
    if ( ( (xsi[0] < 0.0) ) ||
	 ( (xsi[1] < 0.0) || (xsi[1] >     vnl_math::pi) ) ||
	 ( (xsi[2] < 0.0) || (xsi[2] > 2 * vnl_math::pi) ) )
    {
      itkWarningMacro (<<"inconsistent Prolate Coordinate : "<<xsi);
    }

    // all singularity situations have been handled, even between focii,
    // no need to test for null xsi[0] now.

    return xsi;
    
  }

  
  template <class TPixelType>
  typename ProlateSpheroidalTransform<TPixelType>::VectorType ProlateSpheroidalTransform<TPixelType>::ToCartesian(VectorType v, PointType p) const
  {
    
    // xsi has constrains and bounds, lets check them;
    if ( ( (p[0] < 0.0) ) ||
	 ( (p[1] < 0.0) || (p[1] >     vnl_math::pi) ) ||
	 ( (p[2] < 0.0) || (p[2] > 2 * vnl_math::pi) ) )
    {
      itkWarningMacro (<<"inconsistent Prolate Coordinate : "<<p);
    }    
    
    MatrixType matrix; matrix = this->GetJacobianWithRespectToCoordinates(p);
    VectorType ret = matrix * v;

    return ret;
  }

  template <class TPixelType>
  typename ProlateSpheroidalTransform<TPixelType>::VectorType ProlateSpheroidalTransform<TPixelType>::ToProlate(VectorType v, PointType p) const
  {
    MatrixType matrix; matrix = this->GetJacobianWithRespectToCoordinates(p);
    MatrixType transpose; transpose = matrix.GetTranspose ();
    VectorType ret = transpose * v;
    
    return ret;
  }

  template <class TPixelType>
  void ProlateSpheroidalTransform<TPixelType>::EvaluateScaleFactors (double xsi[3], double h[3]) const
  {
    // error underwhich we display a warning because we reached the singularity
    const double epsilon_singularity = 0.001;
    if ( (xsi[0] <= epsilon_singularity) || (xsi[1] <= epsilon_singularity) )
    {
      itkWarningMacro (<<"singularity point : "<<xsi<<"\n"
		       <<"scale factors undefined --> set to null");
      h[0] = h[1] = h[2] = 0.0;
      return;
    }
    
    double d = m_SemiFociDistance;
    
    h[0] = 1.0 / ( d * std::sqrt( std::sinh (xsi[0]) * std::sinh (xsi[0]) + std::sin (xsi[1]) * std::sin (xsi[1]) ) );
    h[1] = 1.0 / ( d * std::sqrt( std::sinh (xsi[0]) * std::sinh (xsi[0]) + std::sin (xsi[1]) * std::sin (xsi[1]) ) );
    h[2] = 1.0 / ( d * std::sinh (xsi[0]) * std::sin (xsi[1]) );
  }

  template <class TPixelType>
  void ProlateSpheroidalTransform<TPixelType>::PrintSelf(std::ostream& os, Indent indent) const
  {
    this->Superclass::PrintSelf(os,indent);

    os << indent << "Excentricity : "<<m_Eccentricity << "\n";
    os << indent << "Semi Foci Distance : "<<m_SemiFociDistance << "\n";
    
    os << indent << "Coefficients: " 
       << "\n\ta: " << m_Coefficients[0]
       << "\n\tb: " << m_Coefficients[1]
       << "\n\tc: " << m_Coefficients[2] << "\n";
    os << indent << "Lambdas: " 
       << "\n\tl1: " << m_Lambda1
       << "\n\tl2: " << m_Lambda2
       << "\n\tl3: " << m_Lambda3 << "\n";
    os << indent << "Center: " 
       << "\n\tO1: " << m_Center[0]
       << "\n\tO2: " << m_Center[1]
       << "\n\tO3: " << m_Center[2] << "\n";
    os << indent << "v1: " 
       << "\n\tv11: " << m_Axis1[0]
       << "\n\tv12: " << m_Axis1[1]
       << "\n\tv13: " << m_Axis1[2] << "\n";
    os << indent << "v2: " 
       << "\n\tv21: " << m_Axis2[0]
       << "\n\tv22: " << m_Axis2[1]
       << "\n\tv23: " << m_Axis2[2] << "\n";
    os << indent << "v3: " 
       << "\n\tv31: " << m_Axis3[0]
       << "\n\tv32: " << m_Axis3[1]
       << "\n\tv33: " << m_Axis3[2] << "\n";
  }

  /*
   * Evaluate ellipsoid equation.
   */
  template <class TPixelType>
  double ProlateSpheroidalTransform<TPixelType>::EvaluateFunction(PointType x) const
  {
    // Transform input point from R0 to the R1 scheme,
    // where the ellipsoid equation is defined
    UniformVectorType xs;
    for (unsigned int i=0; i<3; i++)
      xs[i]=x[i];
    xs[3] = 1;

    xs = m_InternalTransformInverse* xs;

    const double *a = m_Coefficients;
  
    // ellipsoid formula : x2/a2 + y2/b2 + z2/c2 = 1
    return ( xs[0]*xs[0] / (a[0]*a[0]) + xs[1]*xs[1] / (a[1]*a[1]) + xs[2]*xs[2] / (a[2]*a[2]) - 1 );
  }


  /*
   * Evaluate ellipsoid equation.
   */
  template <class TPixelType>
  double ProlateSpheroidalTransform<TPixelType>::EvaluateFunction(double x[3]) const
  {
    PointType xi;
    for (unsigned int i=0; i<3; i++)
      xi[i] = x[i];
    return this->EvaluateFunction (xi);
  }
  
  /*
   * Evaluate ellipsoid equation gradient.
   */
  template <class TPixelType>
  void ProlateSpheroidalTransform<TPixelType>::EvaluateGradient(PointType x, VectorType &n) const
  {

    // Transform input point from R0 to the R1 frame,
    // where the ellipsoid equation is defined
    UniformVectorType xs;
    for (unsigned int i=0; i<3; i++)
      xs[i]=x[i];
    xs[3] = 1;

    xs = m_InternalTransformInverse * xs;

    const double *a=m_Coefficients;
    // define the gradient at xs (in R1)
    UniformVectorType ns;

    for (unsigned int i=0; i<3; i++)
      ns[i] = 2.0 * ( 1.0 / (a[i]*a[i]) ) * xs[i];

    // ns is a vector
    ns[3] = 0;
    ns = m_InternalTransform * ns;
    
    n[0] = ns[0];
    n[1] = ns[1];
    n[2] = ns[2];
  }


  template <class TPixelType>
  double ProlateSpheroidalTransform<TPixelType>::EstimateGeodesicLength1(PointType xi, VectorType dxi, unsigned int divisions) const
  {
    VectorType step = dxi / (double)(divisions);
    double length = 0.0;
    PointType point1 = xi, point2 = xi;
    PointType cart1, cart2;
    VectorType dL;
    
    for (unsigned int i=0; i<divisions; i++)
    {
      point2 = point1 + step;
      cart1 = this->ToCartesian (point1);
      cart2 = this->ToCartesian (point2);
      dL = cart2 - cart1;
      length += dL.GetNorm();
      point1 = point2;
    }

    return length;
  }

  template <class TPixelType>
  double ProlateSpheroidalTransform<TPixelType>::EstimateGeodesicLength2(PointType xi, VectorType dxi, unsigned int divisions) const
  {
    VectorType step = dxi / (double)(divisions);
    PointType pt = xi;
    double h[3];
    
    double length = 0.0;
    for (unsigned int i=0; i<divisions; i++)
    {
      this->EvaluateScaleFactors (pt.GetDataPointer(), h);
      double ds = 0.0;
      for (unsigned int j=0; j<3; j++) ds += ( step[j] * step[j] ) / ( h[j] * h[j] );
      ds = std::sqrt ( ds );
      length += ds;
      pt += step;
      if (pt[2] > (2 * vnl_math::pi))
        pt[2] -= 2 * vnl_math::pi;
      if (pt[2] < 0)
        pt[2] += 2 * vnl_math::pi;

    }

    return length;
  }


} // end namespace itk


#endif
