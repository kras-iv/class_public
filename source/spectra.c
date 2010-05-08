/** @file cl.c Documented \f$ C_l^{X}, P(k), ... \f$ module
 * Julien Lesgourgues, 18.04.2010    
 *
 * This module computes the power spectra \f$ C_l^{X}, P(k), ... \f$'s given the transfer functions and
 * primordial spectra.
 */

#include "spectra.h"

int spectra_cl_at_l(
		    struct spectra * psp,
		    int index_mode,
		    double l,
		    double * cl
		    ) {

  int last_index;

  class_test((index_mode < 0) || (index_mode >= psp->md_size),
	     psp->error_message,
	     "index_mode=%d out of range[%d:%d]",index_mode,0,psp->md_size);
  
  class_test((l < psp->l[index_mode][0]) || (l > psp->l[index_mode][psp->l_size[index_mode]-1]),
	     psp->error_message,
	     "l=%f out of range [%f:%f]",psp->l[index_mode][0],psp->l[index_mode][psp->l_size[index_mode]-1]);

  class_call(array_interpolate_spline(psp->l[index_mode],
				      psp->l_size[index_mode],
				      psp->cl[index_mode],
				      psp->ddcl[index_mode],
				      psp->ic_size[index_mode]*psp->ct_size,
				      l,
				      &last_index,
				      cl,
				      psp->ic_size[index_mode]*psp->ct_size,
				      psp->error_message),
	     psp->error_message,
	     psp->error_message);

  return _SUCCESS_;

}

/**
 * Interpolate the spectrum at an arbitrary redhsifts, for all tabulated values of k between kmin and kmax.
 *
 * The output vector pk[] is in the format:
 *
 *     pk[index_ic * psp->k_size + index_k]
 *
 * with index_ic=0, ..., ppt->ic_size[index_mode]
 *      index_k =0, ..., psp->k_size
 *
 * it corresponds physically to the values of the power spectrum P(psp->k[index_k]) 
 * for the initial condtions (adiabatic, cdi, etc...) of index_ic
 *
 * k[] is in units of 1/Mpc, pk[] in Mpc^3
 *
 */
int spectra_pk_at_z(
		    struct background * pba,
		    struct spectra * psp,
		    int index_mode,
		    double z,
		    double * pk
		    ) {

  int last_index;
  double eta_requested;
  int index_ic;

  class_call(background_eta_of_z(z,&eta_requested),
	     pba->error_message,
	     psp->error_message);

  /* interpolation makes sense only if there are at least two values of eta. deal with case of one value. */
  if (psp->eta_size == 1) {

    class_test(z!=0.,
	       psp->error_message,
	       "asked z=%e but only P(k,z=0) has been tabulated",z);

    /* case z=0 */
    for (index_ic=0; index_ic < psp->ic_size[index_mode]*psp->k_size; index_ic++) {
      pk[index_ic] = psp->pk[index_ic];
    }
    return _SUCCESS_;
    
  }

  class_test((eta_requested < psp->eta[0]) || (eta_requested > psp->eta[psp->eta_size-1]),
	     psp->error_message,
	     "eta(z)=%e out of bounds [%e:%e]",
	     eta_requested,psp->eta[0],psp->eta[psp->eta_size-1]);


  class_call(array_interpolate_spline(psp->eta,
				      psp->eta_size,
				      psp->pk,
				      psp->ddpk,
				      psp->ic_size[index_mode]*psp->k_size,
				      eta_requested,
				      &last_index,
				      pk,
				      psp->ic_size[index_mode]*psp->k_size,
				      psp->error_message),
	     psp->error_message,
	     psp->error_message);

  return _SUCCESS_;

}

/**
 * Interpolate the spectrum at an arbitrary redhsifts and wavenumber between kmin and kmax
 * (in the case of a smooth primordial spectrum, k may be between 0 and kmax: extrapolation 
 *  is performed for suoper-hubble scales down to k=0, given the values of the tilt, running, etc.)
 *
 *  @param k Input : wavenumber k in units of 1/Mpc 
 *  @param z Input : redhsift z
 *  @param index_ic : index of initial condition (ppt->index_ic_ad for adiabatic, etc...)
 *  @param pk Ouput : "P(k,z) in units of Mpc^3  
 */
int spectra_pk_at_k_and_z(
			  struct background * pba,
			  struct primordial * ppm,
			  struct spectra * psp,
			  int index_mode,
			  int index_ic,
			  double k,
			  double z,
			  double * pk
			  ) {

  double * temporary_pk;
  double * spline_pk;
  double * spline_ddpk;
  int index_k;
  int last_index;
  double * pkini_k;
  double * pkini_kmin;

  /* check input parameters (z will be checked in spectra_pk_at_z) */

  class_test((index_ic < 0) || (index_ic >= psp->ic_size[index_mode]),
	     psp->error_message,
	     "index_ic=%d out of bounds [%d:%d]",
	     index_ic,0,psp->ic_size[index_mode]);

  class_test((k < 0) || (k > psp->k[psp->k_size-1]),
	     psp->error_message,
	     "k=%e out of bounds [%e:%e]",k,0,psp->k[psp->k_size-1]);

  /* get P(k) at the right value of z */

  class_alloc(temporary_pk,
	      sizeof(double)*psp->ic_size[index_mode]*psp->k_size,
	      psp->error_message);

/*   temporary_pk = malloc(sizeof(double)*psp->ic_size[index_mode]*psp->k_size); */
/*   if (temporary_pk == NULL) { */
/*     sprintf(psp->error_message,"%s(L:%d) : Could not allocate temporary_pk",__func__,__LINE__); */
/*     return _FAILURE_; */
/*   } */

  if(spectra_pk_at_z(pba,psp,index_mode,z,temporary_pk) == _FAILURE_) {
    sprintf(psp->transmit_message,"%s(L:%d) : error in spectra_pk_at_z()\n=>%s",__func__,__LINE__,psp->error_message);
    sprintf(psp->error_message,psp->transmit_message);
    return _FAILURE_;
  } 


  /* deal with case k < kmin */

  if (k < psp->k[0]) {

    /* extrapolate down to zero if analytical primordial spectrum: in this case we know that on super-Hubble scales:        P(k) = A k P_ini(k) 
       so     
       P(k) = P(kmin) * (k P_ini(k)) / (kmin P_ini(kmin)) 
    */
    if (ppm->primordial_spec_type == smooth_Pk) {
      
      if (k == 0.) {
	*pk=0.;
	return _SUCCESS_;
      }
	
      pkini_k = malloc (sizeof(double)*psp->ic_size[index_mode]);
      if (pkini_k == NULL) {
	sprintf(psp->error_message,"%s(L:%d) : Could not allocate pkini_k",__func__,__LINE__);
	return _FAILURE_;
      }
      pkini_kmin = malloc (sizeof(double)*psp->ic_size[index_mode]);
      if (pkini_kmin == NULL) {
	sprintf(psp->error_message,"%s(L:%d) : Could not allocate pkini_kmin",__func__,__LINE__);
	return _FAILURE_;
      }
      
      if(primordial_spectrum_at_k(ppm,index_mode,k,pkini_k)==_FAILURE_) {
	sprintf(psp->transmit_message,"%s(L:%d) : error in primordial_spectrum_at_k()\n=>%s",__func__,__LINE__,ppm->error_message);
	sprintf(psp->error_message,psp->transmit_message);
	return _FAILURE_;
      }
      if(primordial_spectrum_at_k(ppm,index_mode,psp->k[0],pkini_kmin)==_FAILURE_) {
	sprintf(psp->transmit_message,"%s(L:%d) : error in primordial_spectrum_at_k()\n=>%s",__func__,__LINE__,ppm->error_message);
	sprintf(psp->error_message,psp->transmit_message);
	return _FAILURE_;
      }

      *pk=temporary_pk[index_ic*psp->k_size]*k*pkini_k[index_ic]/psp->k[0]/pkini_kmin[index_ic];

      free(temporary_pk);
      free(pkini_k);
      free(pkini_kmin);

      return _SUCCESS_;

    }

    /* unable to extrapolate if numerical primordial spectrum */
    else {
      sprintf(psp->error_message,"%s(L:%d) : k=%e < k_min=%e, and cannot extrapolate in this case",__func__,__LINE__,k,psp->k[0]);
      return _FAILURE_;
    }

  }

  /* now, spline and interpolate */

  spline_pk = malloc(sizeof(double)*psp->k_size);
  if (spline_pk == NULL) {
    sprintf(psp->error_message,"%s(L:%d) : Could not allocate spline_pk",__func__,__LINE__);
    return _FAILURE_;
  }

  for (index_k=0; index_k < psp->k_size; index_k++) {
    spline_pk[index_k] = temporary_pk[index_ic*psp->k_size+index_k];
  }

  free(temporary_pk);
  
  spline_ddpk = malloc(sizeof(double)*psp->k_size);
  if (spline_ddpk == NULL) {
    sprintf(psp->error_message,"%s(L:%d) : Could not allocate spline_ddpk",__func__,__LINE__);
    return _FAILURE_;
  }

  if (array_spline_table_lines(psp->k,
			       psp->k_size,
			       spline_pk,
			       1,
			       spline_ddpk,
			       _SPLINE_EST_DERIV_,
			       psp->transmit_message) == _FAILURE_) {
    sprintf(psp->error_message,"%s(L:%d) : error in array_spline_table_lines()\n=>%s",__func__,__LINE__,psp->transmit_message);
    return _FAILURE_;
  }

  if (array_interpolate_spline(psp->k,
			       psp->k_size,
			       spline_pk,
			       spline_ddpk,
			       1,
			       k,
			       &last_index,
			       pk,
			       1,
			       psp->transmit_message) == _FAILURE_) {
    sprintf(psp->error_message,"%s(L:%d) : error in array_interpolate_spline()\n=>%s",__func__,__LINE__,psp->transmit_message);
    return _FAILURE_;
  }

  free(spline_pk);
  free(spline_ddpk);

  return _SUCCESS_;

}


/**
 * Computes the \f$ C_l^{X}, P(k), ... \f$'s
 *
 * @param ppt Input : Initialized perturbation structure
 * @param ptr Input : Initialized transfers structure
 * @param ppm Input : Initialized primordial structure
 * @param pcl Output : Initialized cls structure
 * @return the error status
 */
int spectra_init(
		 struct background * pba,
		 struct perturbs * ppt,
		 struct transfers * ptr,
		 struct primordial * ppm,
		 struct spectra * psp
		 ) {

  /** - define local variables */
  int index_mode; /* index running over modes (scalar, tensor, ...) */

  if (psp->spectra_verbose > 0)
    printf("Computing output spectra\n");

  psp->md_size = ppt->md_size;

  class_alloc(psp->ic_size,
	      sizeof(int)*psp->md_size,
	      psp->error_message);

  for (index_mode=0; index_mode < psp->md_size; index_mode++) 
    psp->ic_size[index_mode] = ppt->ic_size[index_mode];

  if (spectra_indices(ppt,ptr,psp) == _FAILURE_) {
    sprintf(psp->transmit_message,"%s(L:%d) : error in spectra_indices()\n=>%s",__func__,__LINE__,psp->error_message);
    sprintf(psp->error_message,"%s",psp->transmit_message);
    return _FAILURE_;
  }

  /** - deal with cl's, if any */

  if (ptr->tt_size > 0) {

/*     if (spectra_cl(ppt,ptr,ppm,psp) == _FAILURE_) { */
/*       sprintf(psp->transmit_message,"%s(L:%d) : error in spectra_cl()\n=>%s",__func__,__LINE__,psp->error_message); */
/*       sprintf(psp->error_message,"%s",psp->transmit_message); */
/*       return _FAILURE_; */
/*     } */


    class_call(spectra_cl(ppt,ptr,ppm,psp),
	       psp->error_message,
	       psp->error_message);


  }
  else {
    psp->ct_size=0;
  }

  /** - deal with pk's, if any */

  if (ppt->has_pk_matter == _TRUE_) {

    if (spectra_pk(pba,ppt,ppm,psp) == _FAILURE_) {
      sprintf(psp->transmit_message,"%s(L:%d) : error in spectra_pk()\n=>%s",__func__,__LINE__,psp->error_message);
      sprintf(psp->error_message,"%s",psp->transmit_message);
      return _FAILURE_;
    }

  }
  else {
    psp->k_size=0;
  }

  return _SUCCESS_;
}

int spectra_free(
		 struct spectra * psp
		 ) {

  int index_mode;

  if (psp->ct_size > 0) {

    for (index_mode = 0; index_mode < psp->md_size; index_mode++) {
      free(psp->l[index_mode]);
      free(psp->cl[index_mode]);
      free(psp->ddcl[index_mode]);
    }
    free(psp->l);
    free(psp->l_size);
    free(psp->cl);
    free(psp->ddcl);
  }

  if (psp->k_size > 0) {

    free(psp->eta);
    free(psp->k);
    free(psp->pk);
    if (psp->eta_size > 0) {
      free(psp->ddpk);
    }
  }    

  return _SUCCESS_;
 
}

int spectra_indices(
		    struct perturbs * ppt,
		    struct transfers * ptr,
		    struct spectra * psp
		    ){

  int index_ct;

  if (ptr->tt_size > 0) {

    index_ct=0;
    if (ppt->has_cl_cmb_temperature == _TRUE_) {
      psp->index_ct_tt=index_ct;
      index_ct++;
    }
    if (ppt->has_cl_cmb_polarization == _TRUE_) {
      psp->index_ct_ee=index_ct;
      index_ct++;
    }
    if ((ppt->has_cl_cmb_temperature == _TRUE_) && 
	(ppt->has_cl_cmb_polarization == _TRUE_)) {
      psp->index_ct_te=index_ct;
      index_ct++;
    }
    if ((ppt->has_cl_cmb_polarization == _TRUE_) && 
	(ppt->has_tensors == _TRUE_)) {
      psp->index_ct_bb=index_ct;
      index_ct++;
    }
    if (ppt->has_cl_cmb_lensing_potential == _TRUE_) {
      psp->index_ct_pp=index_ct;
      index_ct++;
    }
    if ((ppt->has_cl_cmb_temperature == _TRUE_) && 
	(ppt->has_cl_cmb_lensing_potential == _TRUE_)) {
      psp->index_ct_tp=index_ct;
      index_ct++;
    }
    psp->ct_size = index_ct;

  }

  return _SUCCESS_;

}

int spectra_cl(
	       struct perturbs * ppt,
	       struct transfers * ptr,
	       struct primordial * ppm,
	       struct spectra * psp
	       ) {

  /** - define local variables */
  int index_mode; /* index running over modes (scalar, tensor, ...) */
  int index_ic; /* index running over initial conditions */
  int index_tt; /* index running over transfer type (temperature, polarisation, ...) */
  int index_k; /* index running over wavenumber */
  int index_l;  /* multipoles */
  int index_ct;
  double k; /* wavenumber */
  double clvalue;
  int cl_integrand_num_columns;
  double * cl_integrand;
  double * transfer;
  double * primordial_pk;  /*pk[index_ic]*/

  psp->l_size = malloc (sizeof(int)*psp->md_size);
  if (psp->l_size == NULL) {
    sprintf(psp->error_message,"%s(L:%d) : Could not allocate l_size",__func__,__LINE__);
    return _FAILURE_;
  }

  psp->l = malloc (sizeof(double *)*psp->md_size);
  if (psp->l == NULL) {
    sprintf(psp->error_message,"%s(L:%d) : Could not allocate l",__func__,__LINE__);
    return _FAILURE_;
  }

  psp->cl = malloc (sizeof(double *)*psp->md_size);
  if (psp->cl == NULL) {
    sprintf(psp->error_message,"%s(L:%d) : Could not allocate cl",__func__,__LINE__);
    return _FAILURE_;
  }
  
  psp->ddcl = malloc (sizeof(double *)*psp->md_size);
  if (psp->ddcl == NULL) {
    sprintf(psp->error_message,"%s(L:%d) : Could not allocate ddcl",__func__,__LINE__);
    return _FAILURE_;
  }

  /** - loop over modes (scalar, tensors, etc) */
  for (index_mode = 0; index_mode < psp->md_size; index_mode++) {

    psp->l_size[index_mode] = ptr->l_size[index_mode];

    psp->l[index_mode] = malloc(sizeof(double)*psp->l_size[index_mode]);
    if (psp->l[index_mode] == NULL) {
      sprintf(psp->error_message,"%s(L:%d) : Could not allocate l[index_mode]",__func__,__LINE__);
      return _FAILURE_;
    }


    for (index_l=0; index_l < psp->l_size[index_mode]; index_l++) {
      psp->l[index_mode][index_l] = (double)ptr->l[index_mode][index_l];
    }

    psp->cl[index_mode] = malloc(sizeof(double)*psp->ct_size*psp->ic_size[index_mode]*ptr->l_size[index_mode]);
    if (psp->cl[index_mode] == NULL) {
      sprintf(psp->error_message,"%s(L:%d) : Could not allocate cl[index_mode]",__func__,__LINE__);
      return _FAILURE_;
    }

    psp->ddcl[index_mode] = malloc(sizeof(double)*psp->ct_size*psp->ic_size[index_mode]*ptr->l_size[index_mode]);
    if (psp->ddcl[index_mode] == NULL) {
      sprintf(psp->error_message,"%s(L:%d) : Could not allocate ddcl[index_mode]",__func__,__LINE__);
      return _FAILURE_;
    }

    cl_integrand_num_columns = 1+psp->ct_size*2; /* one for k, ct_size for each type, ct_size for each second derivative of each type */

    cl_integrand = malloc(ptr->k_size[index_mode]*cl_integrand_num_columns*sizeof(double));
    if (cl_integrand == NULL) {
      sprintf(psp->error_message,"%s(L:%d) : Could not allocate cl_integrand",__func__,__LINE__);
      return _FAILURE_;
    }

    primordial_pk=malloc(psp->ic_size[index_mode]*sizeof(double));
    if (primordial_pk == NULL) {
      sprintf(psp->error_message,"%s(L:%d) : Could not allocate primordial_pk",__func__,__LINE__);
      return _FAILURE_;
    }

    transfer=malloc(ptr->tt_size*sizeof(double));
    if (transfer == NULL) {
      sprintf(psp->error_message,"%s(L:%d) : Could not allocate transfer",__func__,__LINE__);
      return _FAILURE_;
    }

    /** - loop over initial conditions */
    for (index_ic = 0; index_ic < psp->ic_size[index_mode]; index_ic++) {

      /** - loop over l values defined in the transfer module. For each l: */
      for (index_l=0; index_l < ptr->l_size[index_mode]; index_l++) {

	/** - compute integrand \f$ \Delta_l(k)^2 / k \f$ for each k in the transfer function's table (assumes flat primordial spectrum) */

	for (index_k=0; index_k < ptr->k_size[index_mode]; index_k++) {

	  k = ptr->k[index_mode][index_k];

	  cl_integrand[index_k*cl_integrand_num_columns+0] = k;
	    
	  if (primordial_spectrum_at_k(ppm,index_mode,k,primordial_pk) == _FAILURE_) {
	    sprintf(psp->error_message,"%s(L:%d) : error in primordial_spectrum_at_k()\n=>%s",__func__,__LINE__,ppm->error_message);
	    return _FAILURE_;
	  }

	  /* above routine checks that k>0: no possible division by zero below */

	  for (index_tt=0; index_tt < ptr->tt_size; index_tt++) {

	    transfer[index_tt] = 
	      ptr->transfer[index_mode]
	      [((index_ic * ptr->tt_size + index_tt)
		* ptr->l_size[index_mode] + index_l)
	       * ptr->k_size[index_mode] + index_k];

	  }

	  if (ppt->has_cl_cmb_temperature == _TRUE_) {
	    cl_integrand[index_k*cl_integrand_num_columns+1+psp->index_ct_tt]=primordial_pk[index_ic]
	      * transfer[ptr->index_tt_t]
	      * transfer[ptr->index_tt_t]
	      * 4. * _PI_ / k;
	  }
	      
	  if (ppt->has_cl_cmb_polarization == _TRUE_) {
	    cl_integrand[index_k*cl_integrand_num_columns+1+psp->index_ct_ee]=primordial_pk[index_ic]
	      * transfer[ptr->index_tt_p]
	      * transfer[ptr->index_tt_p]
	      * 4. * _PI_ / k;
	  }

	  if ((ppt->has_cl_cmb_temperature == _TRUE_) && (ppt->has_cl_cmb_polarization == _TRUE_)) {
	    cl_integrand[index_k*cl_integrand_num_columns+1+psp->index_ct_te]=primordial_pk[index_ic]
	      * transfer[ptr->index_tt_t]
	      * transfer[ptr->index_tt_p]
	      * 4. * _PI_ / k;
	  }

	  if ((ppt->has_cl_cmb_polarization == _TRUE_) && (ppt->has_tensors == _TRUE_)) {

	    if (index_mode == ppt->index_md_scalars) {
	      cl_integrand[index_k*cl_integrand_num_columns+1+psp->index_ct_bb]=0.;
	    }
	    if (index_mode == ppt->index_md_tensors) {
	      sprintf(psp->error_message,"%s(L:%d) : tensors not coded yet",__func__,__LINE__);
	      return _FAILURE_;
	    }
	      
	  }
	      
	  if (ppt->has_cl_cmb_lensing_potential == _TRUE_) {
	    cl_integrand[index_k*cl_integrand_num_columns+1+psp->index_ct_pp]=primordial_pk[index_ic]
	      * transfer[ptr->index_tt_lcmb]
	      * transfer[ptr->index_tt_lcmb]
	      * 4. * _PI_ / k;
	  }

	  if ((ppt->has_cl_cmb_temperature == _TRUE_) && (ppt->has_cl_cmb_lensing_potential == _TRUE_)) {
	    cl_integrand[index_k*cl_integrand_num_columns+1+psp->index_ct_tp]=primordial_pk[index_ic]
	      * transfer[ptr->index_tt_t]
	      * transfer[ptr->index_tt_lcmb]
	      * 4. * _PI_ / k;
	  }

	}

	for (index_ct=0; index_ct<psp->ct_size; index_ct++) {

	  if (array_spline(cl_integrand,
			   cl_integrand_num_columns,
			   ptr->k_size[index_mode],
			   0,
			   1+index_ct,
			   1+psp->ct_size+index_ct,
			   _SPLINE_EST_DERIV_,
			   psp->transmit_message) == _FAILURE_) {
	    sprintf(psp->error_message,"%s(L:%d) : error in array_spline_table_lines \n=>%s",__func__,__LINE__,psp->transmit_message);
	    return _FAILURE_;
	  }
	    
	  if (array_integrate_all_spline(cl_integrand,
					 cl_integrand_num_columns,
					 ptr->k_size[index_mode],
					 0,
					 1+index_ct,
					 1+psp->ct_size+index_ct,
					 &clvalue,
					 psp->transmit_message) == _FAILURE_) {
	    sprintf(psp->error_message,"%s(L:%d) : error in array_spline_table_lines \n=>%s",__func__,__LINE__,psp->transmit_message);
	    return _FAILURE_;
	  }

	  psp->cl[index_mode]
	    [(index_l * psp->ic_size[index_mode] + index_ic) * psp->ct_size + index_ct]
	    = clvalue;

	}

      }

    }

    free(cl_integrand);

    free(primordial_pk);

    free(transfer);

    if (array_spline_table_lines(psp->l[index_mode],
				 psp->l_size[index_mode],
				 psp->cl[index_mode],
				 psp->ic_size[index_mode]*psp->ct_size,
				 psp->ddcl[index_mode],
				 _SPLINE_EST_DERIV_,
				 psp->transmit_message) == _FAILURE_) {
      sprintf(psp->error_message,"%s(L:%d) : error in array_spline_table_lines()\n=>%s",__func__,__LINE__,psp->transmit_message);
      return _FAILURE_;
    }

  }

  return _SUCCESS_;
}

int spectra_pk(
	       struct background * pba,
	       struct perturbs * ppt,
	       struct primordial * ppm,
	       struct spectra * psp
	       ) {

  int index_mode; /* index running over modes (scalar, tensor, ...) */
  int index_ic; /* index running over initial conditions */
  int index_k; /* index running over wavenumber */
  int index_eta; /* index running over conformal time */
  double * primordial_pk;  /*pk[index_ic]*/
  int last_index_back;
  double * pvecback_sp_long;
  double Omega_m;
  double eta_min;

  if (ppt->has_scalars == _FALSE_) {
    sprintf(psp->error_message,"%s(L:%d) : you cannot ask for matter power spectrum since you turned off scalar modes",__func__,__LINE__);
    return _FAILURE_;
  }

  index_mode = ppt->index_md_scalars;

  /* if z_max_pk<0, return error */
  if (psp->z_max_pk < 0) {
    sprintf(psp->error_message,"%s(L:%d) : aksed for z=%e, cannot compute P(k) in the future",__func__,__LINE__,psp->z_max_pk);
    return _FAILURE_;
  }

  /* if z_max_pk=0, there is just one value to store */
  if (psp->z_max_pk == 0.) {
    psp->eta_size=1;
  }

  /* if z_max_pk>0, store several values (with a confortable margin above z_max_pk) in view of interpolation */
  else{

    /* find the first relevant value of eta (last value in the table eta_ampling before eta(z_max)) and infer the number of vlaues of eta at which P(k) must be stored */

    if (background_eta_of_z(psp->z_max_pk,&eta_min) == _FAILURE_) {
      sprintf(psp->error_message,"%s(L:%d) : error in background_at_eta()\n=>%s",__func__,__LINE__,pba->error_message);
      return _FAILURE_;
    }   

    index_eta=0;
    if (eta_min < ppt->eta_sampling[index_eta]) {
      sprintf(psp->error_message,"%s(L:%d) : you asked for zmax=%e, i.e. etamin=%e, smaller than first possible value =%e",__func__,__LINE__,psp->z_max_pk,eta_min,ppt->eta_sampling[0]);
      return _FAILURE_;
    }
    while (ppt->eta_sampling[index_eta] < eta_min){
      index_eta++;
    }
    index_eta --;
    /* whenever possible, take a few more values in to avoid boundary effects in the interpolation */
    if (index_eta>0) index_eta--; 
    if (index_eta>0) index_eta--; 
    if (index_eta>0) index_eta--; 
    if (index_eta>0) index_eta--; 
    psp->eta_size=ppt->eta_size-index_eta;

  }

  /** - allocate and fill table of eta values at which P(k,eta) is stored */
  psp->eta=malloc(sizeof(double)*psp->eta_size);
  if (psp->eta == NULL) {
    sprintf(psp->error_message,"%s(L:%d) : Could not allocate eta",__func__,__LINE__);
    return _FAILURE_;
  }
  for (index_eta=0; index_eta<psp->eta_size; index_eta++) {
    psp->eta[index_eta]=ppt->eta_sampling[index_eta-psp->eta_size+ppt->eta_size];
  }

  /** - allocate and fill table of k values at which P(k,eta) is stored */
  psp->k_size = ppt->k_size[index_mode];
  psp->k = malloc(sizeof(double)*psp->k_size);
  if (psp->k == NULL) {
    sprintf(psp->error_message,"%s(L:%d) : Could not allocate k",__func__,__LINE__);
    return _FAILURE_;
  }
  for (index_k=0; index_k<psp->k_size; index_k++) {
    psp->k[index_k]=ppt->k[index_mode][index_k];
  }

  /** - allocate temporary vectors where the primordial spectrum and the background quantitites will be stored */
  primordial_pk=malloc(psp->ic_size[index_mode]*sizeof(double));
  if (primordial_pk == NULL) {
    sprintf(psp->error_message,"%s(L:%d) : Could not allocate primordial_pk",__func__,__LINE__);
    return _FAILURE_;
  }
  pvecback_sp_long = malloc(pba->bg_size*sizeof(double));
  if (pvecback_sp_long==NULL) {
    sprintf(psp->error_message,"%s(L:%d): Cannot allocate pvecback_sp_long \n",__func__,__LINE__);
    return _FAILURE_;
  }

  /** - allocate and fill array of P(k,eta) values */
  psp->pk = malloc(sizeof(double)*psp->eta_size*psp->k_size*psp->ic_size[index_mode]);
  if (psp->pk == NULL) {
    sprintf(psp->error_message,"%s(L:%d) : Could not allocate pk",__func__,__LINE__);
    return _FAILURE_;
  }

  for (index_eta=0 ; index_eta < psp->eta_size; index_eta++) {

    if (background_at_eta(ppt->eta_sampling[index_eta-psp->eta_size+ppt->eta_size], long_info, normal, &last_index_back, pvecback_sp_long) == _FAILURE_) {
      sprintf(psp->error_message,"%s(L:%d) : error in background_at_eta()\n=>%s",__func__,__LINE__,pba->error_message);
      return _FAILURE_;
    }  

    Omega_m = pvecback_sp_long[pba->index_bg_Omega_b];
    if (pba->has_cdm == _TRUE_) {
      Omega_m += pvecback_sp_long[pba->index_bg_Omega_cdm];
    }

    for (index_k=0; index_k<psp->k_size; index_k++) {

      if (primordial_spectrum_at_k(ppm,index_mode,psp->k[index_k],primordial_pk) == _FAILURE_) {
	sprintf(psp->error_message,"%s(L:%d) : error in primordial_spectrum_at_k()\n=>%s",__func__,__LINE__,ppm->error_message);
	return _FAILURE_;
      }

      /* loop over initial conditions */
      for (index_ic = 0; index_ic < psp->ic_size[index_mode]; index_ic++) {
	
	/* primordial spectrum: 
	   P_R(k) = 1/(2pi^2) k^3 <R R>
	   so, primordial curvature correlator: 
	   <R R> = (2pi^2) k^-3 P_R(k) 
	   so, gravitational potential correlator:
	   <phi phi> = (2pi^2) k^-3 (source_phi)^2 P_R(k) 
	   so, matter power spectrum (using Poisson):
	   P(k) = <delta_m delta_m>
	        = 4/9 H^-4 Omega_m^-2 (k/a)^4 <phi phi>
	        = 4/9 H^-4 Omega_m^-2 (k/a)^4 (source_phi)^2 <R R> 
		= 8pi^2/9 H^-4 Omega_m^-2 k/a^4 (source_phi)^2 <R R> */

	psp->pk[(index_eta * psp->ic_size[index_mode] + index_ic) * psp->k_size + index_k] =
	  8.*_PI_*_PI_/9./pow(pvecback_sp_long[pba->index_bg_H],4)/pow(Omega_m,2)*psp->k[index_k]/pow(pvecback_sp_long[pba->index_bg_a],4)
	  *primordial_pk[index_ic]
	  *pow(ppt->sources[index_mode]
	       [index_ic * ppt->tp_size + ppt->index_tp_g]
	       [(index_eta-psp->eta_size+ppt->eta_size) * ppt->k_size[index_mode] + index_k],2);
	
      }
      
    }

  }

  /* if interpolation of P(k,eta) needed (as a function of eta), spline
     the table */  
  if (psp->eta_size > 1) {

    psp->ddpk = malloc(sizeof(double)*psp->eta_size*psp->k_size*psp->ic_size[index_mode]);
    if (psp->ddpk == NULL) {
      sprintf(psp->error_message,"%s(L:%d) : Could not allocate pk",__func__,__LINE__);
      return _FAILURE_;
    }

    if (array_spline_table_lines(psp->eta,
				 psp->eta_size,
				 psp->pk,
				 psp->ic_size[index_mode]*psp->k_size,
				 psp->ddpk,
				 _SPLINE_EST_DERIV_,
				 psp->transmit_message) == _FAILURE_) {
      sprintf(psp->error_message,"%s(L:%d) : error in array_spline_table_lines()\n=>%s",__func__,__LINE__,psp->transmit_message);
      return _FAILURE_;
    }

  }

  free (primordial_pk);
  free (pvecback_sp_long);

  return _SUCCESS_;
}