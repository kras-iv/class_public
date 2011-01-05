/** @file input.c Documented input module.
 *
 * Julien Lesgourgues, 27.08.2010    
 */

#include "input.h" 

/**
 * Use this routine to extract initial parameters from files 'xxx.ini'
 * and/or 'xxx.pre'. They can be the arguments of the main() routine.
 *
 * If class is embedded into another code, you will probably prefer to
 * call directly input_init() in order to pass input parameters
 * through a 'file_content' structure.
 */

int input_init_from_arguments(
			      int argc, 
			      char **argv,
			      struct precision * ppr,
			      struct background *pba,
			      struct thermo *pth,
			      struct perturbs *ppt,
			      struct bessels * pbs,
			      struct transfers *ptr,
			      struct primordial *ppm,
			      struct spectra *psp,
			      struct nonlinear * pnl,
			      struct lensing *ple,
			      struct output *pop,
			      ErrorMsg errmsg
			      ) {

  /** Summary: */

  /** - define local variables */

  struct file_content fc;
  struct file_content fc_input;
  struct file_content fc_precision;

  char input_file[_ARGUMENT_LENGTH_MAX_];
  char precision_file[_ARGUMENT_LENGTH_MAX_];

  int i;
  char extension[5];

  /** - Initialize the two file_content structures (for input
      parameters and precision parameters) to some null content. If no
      arguments are passed, they will remain null and inform
      init_params() that all parameters take default values. */

  fc.size = 0;
  fc_input.size = 0;
  fc_precision.size = 0;
  input_file[0]='\0';
  precision_file[0]='\0';

  /** If some arguments are passed, identify eventually some 'xxx.ini'
      and 'xxx.pre' files, and store their name. */

  if (argc > 1) {
    for (i=1; i<argc; i++) {
      strncpy(extension,(argv[i]+strlen(argv[i])-4),4);
      extension[4]='\0';
      if (strcmp(extension,".ini") == 0) {
	class_test(input_file[0] != '\0',
		   errmsg,
		   "You have passed more than one input file with extension '.ini', choose one.");
	strcpy(input_file,argv[i]);
      }
      if (strcmp(extension,".pre") == 0) {
	class_test(precision_file[0] != '\0',
		   errmsg,
		   "You have passed more than one precision with extension '.pre', choose one.");
	strcpy(precision_file,argv[i]);
      }
    }
  }
  
  /** - if there is an 'xxx.ini' file, read it and store its content. */

  if (input_file[0] != '\0')
    
    class_call(parser_read_file(input_file,&fc_input,errmsg),
	       errmsg,
	       errmsg);

  /** - if there is an 'xxx.pre' file, read it and store its content. */

  if (precision_file[0] != '\0')
    
    class_call(parser_read_file(precision_file,&fc_precision,errmsg),
	       errmsg,
	       errmsg);

  /** - if one or two files were read, merge their contents in a
      single 'file_content' structure. */

  if ((input_file[0]!='\0') || (precision_file[0]!='\0'))

    class_call(parser_cat(&fc_input,&fc_precision,&fc,errmsg),
	       errmsg,
	       errmsg);

  class_call(parser_free(&fc_input),errmsg,errmsg);
  class_call(parser_free(&fc_precision),errmsg,errmsg);
  
  /** - now, initialize all parameters given the input 'file_content'
      structure.  If its size is null, all parameters take their
      default values. */

  class_call(input_init(&fc,
			ppr,
			pba,
			pth,
			ppt,
			pbs,
			ptr,
			ppm,
			psp,
			pnl,
			ple,
			pop,
			errmsg),
	     errmsg,
	     errmsg);
  
  class_call(parser_free(&fc),errmsg,errmsg);

  return _SUCCESS_;
}

/**
 * Initialize each parameters, first to its default values, and then
 * from what can be interpreted from the values passed in the input
 * 'file_content' structure. If its size is null, all parameters keep
 * their default values.
 */

int input_init(
	       struct file_content * pfc,
	       struct precision * ppr,
	       struct background *pba,
	       struct thermo *pth,
	       struct perturbs *ppt,
	       struct bessels * pbs,
	       struct transfers *ptr,
	       struct primordial *ppm,
	       struct spectra *psp,
	       struct nonlinear * pnl,
	       struct lensing *ple,
	       struct output *pop,
	       ErrorMsg errmsg
	       ) {

  /** Summary: */

  /** - define local variables */

  int flag1,flag2,flag3;
  double param1,param2,param3;
  int int1;
  double * pointer1;
  char string1[_ARGUMENT_LENGTH_MAX_];

  double Omega_tot;

  int i;

  FILE * param_output;
  char param_output_name[_LINE_LENGTH_MAX_];

  double sigma_B; /**< Stefan-Boltzmann constant in W/m^2/K^4 = Kg/K^4/s^3 */

  double n_ncdm,rho_ncdm,p_ncdm,drho_dM_ncdm;

  sigma_B = 2. * pow(_PI_,5) * pow(_k_B_,4) / 15. / pow(_h_P_,3) / pow(_c_,2);

  /** - set all parameters (input and precision) to default values */

  class_call(input_default_params(pba,
				  pth,
				  ppt,
				  pbs,
				  ptr,
				  ppm,
				  psp,
				  pnl,
				  ple,
				  pop),
	     errmsg,
	     errmsg);

  class_call(input_default_precision(ppr),
	     errmsg,
	     errmsg);

  /** - if entries passed in file_content structure, carefully read
      and interpret each of them, and tune accordingly the relevant
      input parameters */

  if (pfc->size == 0) 
    return _SUCCESS_;

  /** (a) background parameters */

  /* h (dimensionless) and [H0/c] in Mpc^{-1} = h / 2999.7 */
  class_call(parser_read_double(pfc,"H0",&param1,&flag1,errmsg),
	     errmsg,
	     errmsg);
  class_call(parser_read_double(pfc,"h",&param2,&flag2,errmsg),
	     errmsg,
	     errmsg);
  class_test((flag1 == _TRUE_) && (flag2 == _TRUE_),
	     errmsg,
	     "In input file, you cannot enter both h and H0, choose one");
  if (flag1 == _TRUE_) {
    pba->H0 = param1 * 1.e3 / _c_;
    pba->h = param1 / 100.;
  }
  if (flag2 == _TRUE_) {
    pba->H0 = param2 *  1.e5 / _c_;
    pba->h = param2;
  }

  /* Omega_0_g (photons) and Tcmb */
  class_call(parser_read_double(pfc,"T_cmb",&param1,&flag1,errmsg),
	     errmsg,
	     errmsg);
  class_call(parser_read_double(pfc,"Omega_g",&param2,&flag2,errmsg),
	     errmsg,
	     errmsg);
  class_call(parser_read_double(pfc,"omega_g",&param3,&flag3,errmsg),
	     errmsg,
	     errmsg);
  class_test(class_at_least_two_of_three(flag1,flag2,flag3),
	     errmsg,
	     "In input file, you can only enter one of Tcmb, Omega_g or omega_g, choose one");

  if (class_none_of_three(flag1,flag2,flag3)) {
    pba->Omega0_g = (4.*sigma_B/_c_*pow(pba->Tcmb,4.)) / (3.*_c_*_c_*1.e10*pba->h*pba->h/_Mpc_over_m_/_Mpc_over_m_/8./_PI_/_G_);
  }
  else {

    if (flag1 == _TRUE_) {
      /* Omega0_g = rho_g / rho_c0, each of them expressed in Kg/m/s^2 */
      /* rho_g = (4 sigma_B / c) T^4 */
      /* rho_c0 = 3 c^2 H0^2 / (8 pi G) */ 
      pba->Omega0_g = (4.*sigma_B/_c_*pow(param1,4.)) / (3.*_c_*_c_*1.e10*pba->h*pba->h/_Mpc_over_m_/_Mpc_over_m_/8./_PI_/_G_);
      pba->Tcmb=param1;
    }

    if (flag2 == _TRUE_) {
      pba->Omega0_g = param2;
      pba->Tcmb=pow(pba->Omega0_g * (3.*_c_*_c_*1.e10*pba->h*pba->h/_Mpc_over_m_/_Mpc_over_m_/8./_PI_/_G_) / (4.*sigma_B/_c_),0.25);
    }

    if (flag3 == _TRUE_) {
      pba->Omega0_g = param3/pba->h/pba->h;
      pba->Tcmb = pow(pba->Omega0_g * (3.*_c_*_c_*1.e10*pba->h*pba->h/_Mpc_over_m_/_Mpc_over_m_/8./_PI_/_G_) / (4.*sigma_B/_c_),0.25);
    }
  }

  Omega_tot = pba->Omega0_g;

  /* Omega_0_b (baryons) */
  class_call(parser_read_double(pfc,"Omega_b",&param1,&flag1,errmsg),
	     errmsg,
	     errmsg);
  class_call(parser_read_double(pfc,"omega_b",&param2,&flag2,errmsg),
	     errmsg,
	     errmsg);
  class_test(((flag1 == _TRUE_) && (flag2 == _TRUE_)),
	     errmsg,
	     "In input file, you can only enter one of Omega_b or omega_b, choose one");
  if (flag1 == _TRUE_)
    pba->Omega0_b = param1;
  if (flag2 == _TRUE_)
    pba->Omega0_b = param2/pba->h/pba->h;

  Omega_tot += pba->Omega0_b;

  /* Omega_0_nur (ultra-relativistic species / massless neutrino) */
  class_call(parser_read_double(pfc,"N_eff",&param1,&flag1,errmsg),
	     errmsg,
	     errmsg);
  class_call(parser_read_double(pfc,"Omega_nur",&param2,&flag2,errmsg),
	     errmsg,
	     errmsg);
  class_call(parser_read_double(pfc,"omega_nur",&param3,&flag3,errmsg),
	     errmsg,
	     errmsg);
  class_test(class_at_least_two_of_three(flag1,flag2,flag3),
	     errmsg,
	     "In input file, you can only enter one of N_eff, Omega_nur or omega_nur, choose one");

  if (class_none_of_three(flag1,flag2,flag3)) {
    pba->Omega0_nur = 3.04*7./8.*pow(4./11.,4./3.)*pba->Omega0_g;
  }
  else {

    if (flag1 == _TRUE_) {
      pba->Omega0_nur = param1*7./8.*pow(4./11.,4./3.)*pba->Omega0_g;
    }
    if (flag2 == _TRUE_) {
      pba->Omega0_nur = param2;
    }
    if (flag3 == _TRUE_) {
      pba->Omega0_nur = param3/pba->h/pba->h;
    }
  }

  Omega_tot += pba->Omega0_nur;

  /* Omega_0_cdm (CDM) */
  class_call(parser_read_double(pfc,"Omega_cdm",&param1,&flag1,errmsg),
	     errmsg,
	     errmsg);
  class_call(parser_read_double(pfc,"omega_cdm",&param2,&flag2,errmsg),
	     errmsg,
	     errmsg);
  class_test(((flag1 == _TRUE_) && (flag2 == _TRUE_)),
	     errmsg,
	     "In input file, you can only enter one of Omega_cdm or omega_cdm, choose one");
  if (flag1 == _TRUE_)
    pba->Omega0_cdm = param1;
  if (flag2 == _TRUE_)
    pba->Omega0_cdm = param2/pba->h/pba->h;

  Omega_tot += pba->Omega0_cdm;

  /* non-cold relics (ncdm) */
  class_call(parser_read_double(pfc,"M_ncdm1",&param1,&flag1,errmsg),
	     errmsg,
	     errmsg);
  class_call(parser_read_double(pfc,"Omega_ncdm1",&param2,&flag2,errmsg),
	     errmsg,
	     errmsg);
  class_call(parser_read_double(pfc,"omega_ncdm1",&param3,&flag3,errmsg),
	     errmsg,
	     errmsg);

  class_test(class_at_least_two_of_three(flag1,flag2,flag3),
	     errmsg,
	     "In input file, you can only enter one of m_ncdm1, Omega_ncdm1 or omega__ncdm1, choose one");

  if (((flag1 == _TRUE_) && (param1 > 0.)) ||
      ((flag2 == _TRUE_) && (param2 > 0.)) ||
      ((flag3 == _TRUE_) && (param3 > 0.))) {

    if (flag1 == _TRUE_) {

      fprintf(stderr,"initialize ncdm variables\n");

      pba->M_ncdm1 = param1;

      class_call(parser_read_double(pfc,"T_ncdm1",&param2,&flag2,errmsg),
		 errmsg,
		 errmsg);

      pba->T_ncdm1 = param2;

      class_call(parser_read_double(pfc,"ksi_ncdm1",&param3,&flag3,errmsg),
		 errmsg,
		 errmsg);      

      pba->ksi_ncdm1 = param3;
      
      class_call(background_ncdm1_init(pba),
		 pba->error_message,
		 errmsg);
      
      class_call(background_ncdm1_momenta(pba,0.,&n_ncdm,&rho_ncdm,&p_ncdm,&drho_dM_ncdm),
		 pba->error_message,
		 errmsg);

      pba->Omega0_ncdm1 = rho_ncdm/pba->H0/pba->H0;

      fprintf(stderr,"Omega_nu=%g\n",pba->Omega0_ncdm1);

    }

    else if (flag2 == _TRUE_) {

      fprintf(stderr,"should NOT go here\n");

      pba->Omega0_ncdm1 = param2;

      class_call(parser_read_double(pfc,"T_ncdm1",&param1,&flag1,errmsg),
		 errmsg,
		 errmsg);

      pba->T_ncdm1 = param1;

      class_call(parser_read_double(pfc,"ksi_ncdm1",&param3,&flag3,errmsg),
		 errmsg,
		 errmsg);      

      pba->ksi_ncdm1 = param3;

      class_call(background_ncdm1_init(pba),
		 pba->error_message,
		 errmsg);

      class_call(background_ncdm1_M_from_Omega(pba),
		 pba->error_message,
		 errmsg);
    }

    else if (flag3 == _TRUE_) {

      fprintf(stderr,"should NOT go here\n");

      pba->Omega0_ncdm1 = param3/pba->h/pba->h;

      class_call(parser_read_double(pfc,"T_ncdm1",&param1,&flag1,errmsg),
		 errmsg,
		 errmsg);

      pba->T_ncdm1 = param1;

      class_call(parser_read_double(pfc,"ksi_ncdm1",&param2,&flag2,errmsg),
		 errmsg,
		 errmsg);      

      pba->ksi_ncdm1 = param2;

      class_call(background_ncdm1_init(pba),
		 pba->error_message,
		 errmsg);

      class_call(background_ncdm1_M_from_Omega(pba),
		 pba->error_message,
		 errmsg);
    }

    pba->m_ncdm1_in_eV = _k_B_/_eV_*pba->T_ncdm1*pba->M_ncdm1*pba->Tcmb;

    fprintf(stderr,"%e %e\n",
	    pba->m_ncdm1_in_eV,
	    pba->m_ncdm1_in_eV/pba->Omega0_ncdm1/pba->h/pba->h);

    Omega_tot += pba->Omega0_ncdm1;
  }

  /* Omega_0_k (curvature) */
  class_read_double("Omega_k",pba->Omega0_k);

  /* Omega_0_lambda (cosmological constant), Omega0_de (dark energy fluid) */
  class_call(parser_read_double(pfc,"Omega_Lambda",&param1,&flag1,errmsg),
	     errmsg,
	     errmsg);
  class_call(parser_read_double(pfc,"Omega_de",&param2,&flag2,errmsg),
	     errmsg,
	     errmsg);
  class_test((flag1 == _TRUE_) && (flag2 == _TRUE_),
	     errmsg,
	     "In input file, you can enter only two out of Omega_Lambda, Omega_de, Omega_k, the third one is inferred");

  if ((flag1 == _FALSE_) && (flag2 == _FALSE_)) {	
    pba->Omega0_lambda = 1.+pba->Omega0_k-pba->Omega0_g-pba->Omega0_nur-pba->Omega0_b-pba->Omega0_cdm-pba->Omega0_ncdm1;
  }
  else {
    if (flag1 == _TRUE_) {
      pba->Omega0_lambda= param1;
      pba->Omega0_de = 1. + pba->Omega0_k - param1 - Omega_tot;
    }
    if (flag2 == _TRUE_) {
      pba->Omega0_lambda= 1. + pba->Omega0_k - param2 - Omega_tot;
      pba->Omega0_de = param2;
    }
  }

  class_test(pba->Omega0_de != 0.,
	     errmsg,
	     "Dark energy fluid not tested yet");
  
  class_test(pba->Omega0_k != 0.,
	     errmsg,
	     "Open/close case not written yet");

  /* scale factor today (arbitrary) */
  class_read_double("a_today",pba->a_today);

  /** (b) assign values to thermodynamics cosmological parameters */

  /* scale factor today (arbitrary) */
  class_read_double("YHe",pth->YHe);

  /* reionization parametrization */
  class_call(parser_read_string(pfc,"reio_parametrization",&string1,&flag1,errmsg),
	     errmsg,
	     errmsg);

  if (flag1 == _TRUE_) {
    flag2=_FALSE_;
    if (strcmp(string1,"reio_none") == 0) {
      pth->reio_parametrization=reio_none;
      flag2=_TRUE_;
      printf("Warning: you are computing a model without reionization (why not...)\n");
    }
    if (strcmp(string1,"reio_camb") == 0) {
      pth->reio_parametrization=reio_camb;
      flag2=_TRUE_;
    }
    class_test(flag2==_FALSE_,
	       errmsg,
	       "could not identify reionization_parametrization value, check that it is one of 'reio_none', 'reio_camb', ...");
  }

  /* reionization parameters if reio_parametrization=reio_camb */
  if (pth->reio_parametrization == reio_camb) {
    class_call(parser_read_double(pfc,"z_reio",&param1,&flag1,errmsg),
	       errmsg,
	       errmsg);
    class_call(parser_read_double(pfc,"tau_reio",&param2,&flag2,errmsg),
	       errmsg,
	       errmsg);
    class_test(((flag1 == _TRUE_) && (flag2 == _TRUE_)),
	       errmsg,
	       "In input file, you can only enter one of z_reio or tau_reio, choose one");
    if (flag1 == _TRUE_) {
      pth->z_reio=param1;
      pth->reio_z_or_tau=reio_z;
    }
    if (flag2 == _TRUE_) {
      pth->tau_reio=param2;
      pth->reio_z_or_tau=reio_tau;
    }
  }

  /** (c) define which perturbations and sources should be computed, and down to which scale */

  ppt->has_perturbations = _FALSE_;
  ppt->has_cls = _FALSE_;

  class_call(parser_read_string(pfc,"output",&string1,&flag1,errmsg),
	     errmsg,
	     errmsg);

  if (flag1 == _TRUE_) {

    if ((strstr(string1,"tCl") != NULL) || (strstr(string1,"TCl") != NULL) || (strstr(string1,"TCL") != NULL)) {
      ppt->has_cl_cmb_temperature = _TRUE_;  
      ppt->has_perturbations = _TRUE_;  
      ppt->has_cls = _TRUE_;
    }

    if ((strstr(string1,"pCl") != NULL) || (strstr(string1,"PCl") != NULL) || (strstr(string1,"PCL") != NULL)) {
      ppt->has_cl_cmb_polarization = _TRUE_;  
      ppt->has_perturbations = _TRUE_;  
      ppt->has_cls = _TRUE_;
    }
    
    if ((strstr(string1,"lCl") != NULL) || (strstr(string1,"LCl") != NULL) || (strstr(string1,"LCL") != NULL)) {
      ppt->has_cl_cmb_lensing_potential = _TRUE_;
      ppt->has_perturbations = _TRUE_; 
      ppt->has_cls = _TRUE_;
    }

    if ((strstr(string1,"mPk") != NULL) || (strstr(string1,"MPk") != NULL) || (strstr(string1,"MPK") != NULL)) {
      ppt->has_pk_matter=_TRUE_; 
      ppt->has_perturbations = _TRUE_;  
    }

    if ((strstr(string1,"mTk") != NULL) || (strstr(string1,"MTk") != NULL) || (strstr(string1,"MTK") != NULL)) {
      ppt->has_matter_transfers=_TRUE_; 
      ppt->has_perturbations = _TRUE_;  
    }

  }

  if (ppt->has_perturbations == _TRUE_) { 

    class_call(parser_read_string(pfc,"modes",&string1,&flag1,errmsg),
	       errmsg,
	       errmsg);

    if (flag1 == _TRUE_) {

      /* if no modes are specified, the default is has_scalars=_TRUE_; 
	 but if they are specified we should reset has_scalars to _FALSE_ before reading */
      ppt->has_scalars=_FALSE_;

      if ((strstr(string1,"s") != NULL) || (strstr(string1,"S") != NULL))
	ppt->has_scalars=_TRUE_; 

      if ((strstr(string1,"v") != NULL) || (strstr(string1,"V") != NULL))
	ppt->has_vectors=_TRUE_;  

      if ((strstr(string1,"t") != NULL) || (strstr(string1,"T") != NULL))
	ppt->has_tensors=_TRUE_;

      class_test(class_none_of_three(ppt->has_scalars,ppt->has_vectors,ppt->has_tensors),
		 errmsg,	       
		 "You wrote: modes=%s. Could not identify any of the modes ('s', 'v', 't') in such input",string1);
    }

    if (ppt->has_scalars == _TRUE_) {

      class_call(parser_read_string(pfc,"ic",&string1,&flag1,errmsg),
		 errmsg,
		 errmsg);

      if (flag1 == _TRUE_) {

	/* if no initial conditions are specified, the default is has_ad=_TRUE_; 
	   but if they are specified we should reset has_ad to _FALSE_ before reading */
	ppt->has_ad=_FALSE_;

	if ((strstr(string1,"ad") != NULL) || (strstr(string1,"AD") != NULL))
	  ppt->has_ad=_TRUE_; 
	
	if ((strstr(string1,"bi") != NULL) || (strstr(string1,"BI") != NULL))
	  ppt->has_bi=_TRUE_; 
	
	if ((strstr(string1,"cdi") != NULL) || (strstr(string1,"CDI") != NULL))
	  ppt->has_cdi=_TRUE_; 
	
	if ((strstr(string1,"nid") != NULL) || (strstr(string1,"NID") != NULL))
	  ppt->has_nid=_TRUE_; 
	
	if ((strstr(string1,"niv") != NULL) || (strstr(string1,"NIV") != NULL))
	  ppt->has_niv=_TRUE_; 
      
	class_test(ppt->has_ad==_FALSE_ && ppt->has_bi ==_FALSE_ && ppt->has_cdi ==_FALSE_ && ppt->has_nid ==_FALSE_ && ppt->has_niv ==_FALSE_,
		   errmsg,	       
		   "You wrote: ic=%s. Could not identify any of the initial conditions ('ad', 'bi', 'cdi', 'nid', 'niv') in such input",string1);
	
      }
    }

    else {

      class_test(ppt->has_cl_cmb_lensing_potential == _TRUE_,
		 errmsg,
		 "Inconsistency: you want C_l's for cmb lensing potential, but no scalar modes\n");

      class_test(ppt->has_pk_matter == _TRUE_,
		 errmsg,
		 "Inconsistency: you want P(k) of matter, but no scalar modes\n");

    }

  }

  /** (d) define the primordial spectrum */

  class_call(parser_read_string(pfc,"P_k_ini type",&string1,&flag1,errmsg),
	     errmsg,
	     errmsg);

  if (flag1 == _TRUE_) {
    flag2=_FALSE_;
    if (strcmp(string1,"analytic_Pk") == 0) {
      ppm->primordial_spec_type = analytic_Pk;
      flag2=_TRUE_;
    }
    class_test(flag2==_FALSE_,
	       errmsg,
	       "could not identify primordial spectrum type, check that it is one of 'analytic_pk', ...");
  }

  if (ppm->primordial_spec_type == analytic_Pk) {

    class_read_double("k_pivot",ppm->k_pivot);

    if (ppt->has_scalars == _TRUE_) {
      
      if (ppt->has_ad == _TRUE_) {

	class_read_double("A_s",ppm->A_s);
	class_read_double("n_s",ppm->n_s);
	class_read_double("alpha_s",ppm->alpha_s);

      }

      if (ppt->has_bi == _TRUE_) {

	class_read_double("f_bi",ppm->f_bi);
	class_read_double("n_bi",ppm->n_bi);
	class_read_double("alpha_bi",ppm->alpha_bi);

      }

      if (ppt->has_cdi == _TRUE_) {

	class_read_double("f_cdi",ppm->f_cdi);
	class_read_double("n_cdi",ppm->n_cdi);
	class_read_double("alpha_cdi",ppm->alpha_cdi);

      }

      if (ppt->has_nid == _TRUE_) {

	class_read_double("f_nid",ppm->f_nid);
	class_read_double("n_nid",ppm->n_nid);
	class_read_double("alpha_nid",ppm->alpha_nid);

      }

      if (ppt->has_niv == _TRUE_) {

	class_read_double("f_niv",ppm->f_niv);
	class_read_double("n_niv",ppm->n_niv);
	class_read_double("alpha_niv",ppm->alpha_niv);

      }

      if ((ppt->has_ad == _TRUE_) && (ppt->has_bi == _TRUE_)) {
	class_read_double_one_of_two("c_ad_bi","c_bi_ad",ppm->c_ad_bi);
	class_read_double_one_of_two("n_ad_bi","n_bi_ad",ppm->n_ad_bi);
	class_read_double_one_of_two("alpha_ad_bi","alpha_bi_ad",ppm->alpha_ad_bi);
      }

      if ((ppt->has_ad == _TRUE_) && (ppt->has_cdi == _TRUE_)) {
	class_read_double_one_of_two("c_ad_cdi","c_cdi_ad",ppm->c_ad_cdi);
	class_read_double_one_of_two("n_ad_cdi","n_cdi_ad",ppm->n_ad_cdi);
	class_read_double_one_of_two("alpha_ad_cdi","alpha_cdi_ad",ppm->alpha_ad_cdi);
      }

      if ((ppt->has_ad == _TRUE_) && (ppt->has_nid == _TRUE_)) {
	class_read_double_one_of_two("c_ad_nid","c_nid_ad",ppm->c_ad_nid);
	class_read_double_one_of_two("n_ad_nid","n_nid_ad",ppm->n_ad_nid);
	class_read_double_one_of_two("alpha_ad_nid","alpha_nid_ad",ppm->alpha_ad_nid);
      }

      if ((ppt->has_ad == _TRUE_) && (ppt->has_niv == _TRUE_)) {
	class_read_double_one_of_two("c_ad_niv","c_niv_ad",ppm->c_ad_niv);
	class_read_double_one_of_two("n_ad_niv","n_niv_ad",ppm->n_ad_niv);
	class_read_double_one_of_two("alpha_ad_niv","alpha_niv_ad",ppm->alpha_ad_niv);
      }

      if ((ppt->has_bi == _TRUE_) && (ppt->has_cdi == _TRUE_)) {
	class_read_double_one_of_two("c_bi_cdi","c_cdi_bi",ppm->c_bi_cdi);
	class_read_double_one_of_two("n_bi_cdi","n_cdi_bi",ppm->n_bi_cdi);
	class_read_double_one_of_two("alpha_bi_cdi","alpha_cdi_bi",ppm->alpha_bi_cdi);
      }

      if ((ppt->has_bi == _TRUE_) && (ppt->has_nid == _TRUE_)) {
	class_read_double_one_of_two("c_bi_nid","c_nid_bi",ppm->c_bi_nid);
	class_read_double_one_of_two("n_bi_nid","n_nid_bi",ppm->n_bi_nid);
	class_read_double_one_of_two("alpha_bi_nid","alpha_nid_bi",ppm->alpha_bi_nid);
      }

      if ((ppt->has_bi == _TRUE_) && (ppt->has_niv == _TRUE_)) {
	class_read_double_one_of_two("c_bi_niv","c_niv_bi",ppm->c_bi_niv);
	class_read_double_one_of_two("n_bi_niv","n_niv_bi",ppm->n_bi_niv);
	class_read_double_one_of_two("alpha_bi_niv","alpha_niv_bi",ppm->alpha_bi_niv);
      }

      if ((ppt->has_cdi == _TRUE_) && (ppt->has_nid == _TRUE_)) {
	class_read_double_one_of_two("c_cdi_nid","c_nid_cdi",ppm->c_cdi_nid);
	class_read_double_one_of_two("n_cdi_nid","n_nid_cdi",ppm->n_cdi_nid);
	class_read_double_one_of_two("alpha_cdi_nid","alpha_nid_cdi",ppm->alpha_cdi_nid);
      }

      if ((ppt->has_cdi == _TRUE_) && (ppt->has_niv == _TRUE_)) {
	class_read_double_one_of_two("c_cdi_niv","c_niv_cdi",ppm->c_cdi_niv);
	class_read_double_one_of_two("n_cdi_niv","n_niv_cdi",ppm->n_cdi_niv);
	class_read_double_one_of_two("alpha_cdi_niv","alpha_niv_cdi",ppm->alpha_cdi_niv);
      }

      if ((ppt->has_nid == _TRUE_) && (ppt->has_niv == _TRUE_)) {
	class_read_double_one_of_two("c_nid_niv","c_niv_nid",ppm->c_nid_niv);
	class_read_double_one_of_two("n_nid_niv","n_niv_nid",ppm->n_nid_niv);
	class_read_double_one_of_two("alpha_nid_niv","alpha_niv_nid",ppm->alpha_nid_niv);
      }  

    }

    if (ppt->has_tensors == _TRUE_) {
    
      class_read_double("r",ppm->r);
      class_read_double("n_t",ppm->n_t);
      class_read_double("alpha_t",ppm->alpha_t);

    }

  }

  /** (e) parameters for final spectra */

  if (ppt->has_cls == _TRUE_) {

    if (ppt->has_scalars == _TRUE_) {
      class_read_double("l_max_scalars",ppt->l_scalar_max);
    }

    if (ppt->has_tensors == _TRUE_) {   
      class_read_double("l_max_tensors",ppt->l_tensor_max);
    }
  }

  class_call(parser_read_string(pfc,
				"lensing",
				&(string1),
				&(flag1),
				errmsg),
	     errmsg,
	     errmsg);
  
  if ((flag1 == _TRUE_) && ((strstr(string1,"y") != NULL) || (strstr(string1,"Y") != NULL))) {
    
    class_test((ppt->has_scalars == _FALSE_) || 
	       (ppt->has_cls == _FALSE_) || 
	       (ppt->has_cl_cmb_lensing_potential == _FALSE_),
	       errmsg,
	       "Lensed Cls only possible if you ask for scalars, temperature and/or polarization Cls, and lensing potential Cls.");
    
    ple->has_lensed_cls = _TRUE_;
    
  }

  if (ppt->has_pk_matter == _TRUE_) {

    class_read_double("P_k_max",ppt->k_scalar_kmax_for_pk);

    class_call(parser_read_list_of_doubles(pfc,
					   "z_pk",
					   &(int1),
					   &(pointer1),
					   &flag1,
					   errmsg),
	       errmsg,
	       errmsg);
    
    if (flag1 == _TRUE_) {
      class_test(int1 > _Z_PK_NUM_MAX_,
		 errmsg,
		 "you want to write some output for %d different values of z, hence you should increase _Z_PK_NUM_MAX_ in include/output.h to at least this number",
		 int1);
      pop->z_pk_num = int1;
      for (i=0; i<int1; i++) {
	pop->z_pk[i] = pointer1[i];
      }
    }
    
    class_call(parser_read_double(pfc,"z_max_pk",&param1,&flag1,errmsg),
	       errmsg,
	       errmsg);
  
    if (flag1==_TRUE_) {
      psp->z_max_pk = param1;
    }
    else {
      psp->z_max_pk = 0.;
      for (i=0; i<pop->z_pk_num; i++)
	psp->z_max_pk = max(psp->z_max_pk,pop->z_pk[i]);
    }
  }

  class_read_string("root",pop->root);

  class_call(parser_read_string(pfc,
				"bessel file",
				&(string1),
				&(flag1),
				errmsg),
	     errmsg,
	     errmsg);
	     
  if ((flag1 == _TRUE_) && ((strstr(string1,"y") != NULL) || (strstr(string1,"Y") != NULL))) {
    pbs->bessel_always_recompute = _FALSE_;
  }

  /** (f) parameter related to the non-linear spectra computation */

  class_call(parser_read_string(pfc,
				"non linear",
				&(string1),
				&(flag1),
				errmsg),
	     errmsg,
	     errmsg);

  if (flag1 == _TRUE_) {

    if ((strstr(string1,"trg") != NULL) || (strstr(string1,"TRG") != NULL)) {
      pnl->method=nl_trg;
    }    
    if ((strstr(string1,"one-loop") != NULL) || (strstr(string1,"oneloop") != NULL) || (strstr(string1,"one loop") != NULL)) {
      pnl->method=nl_trg_one_loop;
    }
    if ((strstr(string1,"test linear") != NULL) || (strstr(string1,"test-linear") != NULL)) {
      pnl->method=nl_trg_linear;
    }
  }

  /** (g) amount of information sent to standard output (none if all set to zero) */

  class_read_int("background_verbose",
		 pba->background_verbose);

  class_read_int("thermodynamics_verbose",
		 pth->thermodynamics_verbose);

  class_read_int("perturbations_verbose",
		 ppt->perturbations_verbose);

  class_read_int("bessels_verbose",
		 pbs->bessels_verbose);

  class_read_int("transfer_verbose",
		 ptr->transfer_verbose);

  class_read_int("primordial_verbose",
		 ppm->primordial_verbose);

  class_read_int("spectra_verbose",
		 psp->spectra_verbose);

  class_read_int("nonlinear_verbose",
		 pnl->nonlinear_verbose);

  class_read_int("lensing_verbose",
		 ple->lensing_verbose);

  class_read_int("output_verbose",
		 pop->output_verbose);

  /** (h) all precision parameters */

  /** h.1. parameters related to the background */

  class_read_double("a_ini_over_a_today_default",ppr->a_ini_over_a_today_default);
  class_read_double("back_integration_stepsize",ppr->back_integration_stepsize);
  class_read_double("tol_background_integration",ppr->tol_background_integration);

  /** h.2. parameters related to the thermodynamics */

  class_read_double("recfast_z_initial",ppr->recfast_z_initial);

  class_read_int("recfast_Nz0",ppr->recfast_Nz0);
  class_read_double("tol_thermo_integration",ppr->tol_thermo_integration);

  class_read_int("recfast_Heswitch",ppr->recfast_Heswitch);
  class_read_double("recfast_fudge_He",ppr->recfast_fudge_He);

  class_read_int("recfast_Hswitch",ppr->recfast_Hswitch);
  class_read_double("recfast_fudge_H",ppr->recfast_fudge_H);
  if (ppr->recfast_Hswitch == _TRUE_) {
    class_read_double("recfast_delta_fudge_H",ppr->recfast_delta_fudge_H);
    class_read_double("recfast_AGauss1",ppr->recfast_AGauss1);
    class_read_double("recfast_AGauss2",ppr->recfast_AGauss2);
    class_read_double("recfast_zGauss1",ppr->recfast_zGauss1);
    class_read_double("recfast_zGauss2",ppr->recfast_zGauss2);
    class_read_double("recfast_wGauss1",ppr->recfast_wGauss1);
    class_read_double("recfast_wGauss2",ppr->recfast_wGauss2);
  }

  class_read_double("recfast_z_He_1",ppr->recfast_z_He_1);
  class_read_double("recfast_delta_z_He_1",ppr->recfast_delta_z_He_1);
  class_read_double("recfast_z_He_2",ppr->recfast_z_He_2);
  class_read_double("recfast_delta_z_He_2",ppr->recfast_delta_z_He_2);
  class_read_double("recfast_z_He_3",ppr->recfast_z_He_3);
  class_read_double("recfast_delta_z_He_3",ppr->recfast_delta_z_He_3);
  class_read_double("recfast_x_He0_trigger",ppr->recfast_x_He0_trigger);
  class_read_double("recfast_x_He0_trigger2",ppr->recfast_x_He0_trigger2);
  class_read_double("recfast_x_He0_trigger_delta",ppr->recfast_x_He0_trigger_delta);
  class_read_double("recfast_x_H0_trigger",ppr->recfast_x_H0_trigger);
  class_read_double("recfast_x_H0_trigger2",ppr->recfast_x_H0_trigger2);
  class_read_double("recfast_x_H0_trigger_delta",ppr->recfast_x_H0_trigger_delta);
  class_read_double("recfast_H_frac",ppr->recfast_H_frac);

  class_read_double("reionization_z_start_max",ppr->reionization_z_start_max);
  class_read_double("reionization_sampling",ppr->reionization_sampling);
  class_read_double("reionization_optical_depth_tol",ppr->reionization_optical_depth_tol);
  class_read_double("reionization_exponent",ppr->reionization_exponent);
  class_read_double("reionization_width",ppr->reionization_width);
  class_read_double("reionization_start_factor",ppr->reionization_start_factor);
  class_read_double("helium_fullreio_redshift",ppr->helium_fullreio_redshift);
  class_read_double("helium_fullreio_width",ppr->helium_fullreio_width);

  class_read_int("thermo_rate_smoothing_radius",ppr->thermo_rate_smoothing_radius);

  /** h.3. parameters related to the perturbations */

  class_read_int("gauge",ppr->gauge);
  class_read_double("k_scalar_min_eta0",ppr->k_scalar_min_eta0);
  class_read_double("k_scalar_max_eta0_over_l_max",ppr->k_scalar_max_eta0_over_l_max);
  class_read_double("k_scalar_step_sub",ppr->k_scalar_step_sub);
  class_read_double("k_scalar_step_super",ppr->k_scalar_step_super);
  class_read_double("k_scalar_step_transition",ppr->k_scalar_step_transition);
  class_read_double("k_scalar_k_per_decade_for_pk",ppr->k_scalar_k_per_decade_for_pk);
  class_read_double("k_tensor_min",ppr->k_tensor_min);
  class_read_double("l_max_over_k_max_tensors",ppr->l_max_over_k_max_tensors);
  class_read_double("k_tensor_step_sub",ppr->k_tensor_step_sub);
  class_read_double("k_tensor_step_super",ppr->k_tensor_step_super);
  class_read_double("k_tensor_step_transition",ppr->k_tensor_step_transition);
  class_read_double("start_small_k_at_eta_g_over_eta_h",ppr->start_small_k_at_eta_g_over_eta_h);
  class_read_double("start_large_k_at_eta_h_over_eta_k",ppr->start_large_k_at_eta_h_over_eta_k);
  class_read_double("tight_coupling_trigger_eta_g_over_eta_h",ppr->tight_coupling_trigger_eta_g_over_eta_h);
  class_read_double("tight_coupling_trigger_eta_g_over_eta_k",ppr->tight_coupling_trigger_eta_g_over_eta_k);
  class_read_double("start_sources_at_eta_g_over_eta_h",ppr->start_sources_at_eta_g_over_eta_h);
  class_read_int("tight_coupling_approximation",ppr->tight_coupling_approximation)
  class_read_int("l_max_g",ppr->l_max_g);
  class_read_int("l_max_pol_g",ppr->l_max_pol_g);
  class_read_int("l_max_nur",ppr->l_max_nur);
  class_read_int("l_max_g_ten",ppr->l_max_g_ten);
  class_read_int("l_max_pol_g_ten",ppr->l_max_pol_g_ten);
  class_read_double("curvature_ini",ppr->curvature_ini);
  class_read_double("entropy_ini",ppr->entropy_ini);
  class_read_double("gw_ini",ppr->gw_ini);
  class_read_double("perturb_integration_stepsize",ppr->perturb_integration_stepsize);
  class_read_double("tol_eta_approx",ppr->tol_eta_approx);
  class_read_double("tol_perturb_integration",ppr->tol_perturb_integration);
  class_read_double("perturb_sampling_stepsize",ppr->perturb_sampling_stepsize);

  class_read_int("free_streaming_approximation",ppr->free_streaming_approximation)
  class_read_double("free_streaming_trigger_eta_h_over_eta_k",ppr->free_streaming_trigger_eta_h_over_eta_k);
  class_read_double("free_streaming_trigger_Omega_r",ppr->free_streaming_trigger_Omega_r);

  /** h.4. parameter related to the Bessel functions */

  class_read_double("l_logstep",ppr->l_logstep);
  class_read_int("l_linstep",ppr->l_linstep);
  class_read_double("bessel_x_step",ppr->bessel_x_step);
  class_read_double("bessel_j_cut",ppr->bessel_j_cut);
  class_read_double("bessel_tol_x_min",ppr->bessel_tol_x_min);
  class_read_double("bessel_x_max_over_l_max",ppr->bessel_x_max_over_l_max);
  class_read_string("bessel_file_name",ppr->bessel_file_name);

  /** h.5. parameter related to the primordial spectra */

  class_read_double("k_per_decade_primordial",ppr->k_per_decade_primordial);

  /** h.6. parameter related to the transfer functions */

  class_read_double("k_step_trans_scalars",ppr->k_step_trans_scalars);
  class_read_double("k_step_trans_tensors",ppr->k_step_trans_tensors);
/*   class_read_int("k_oversampling_scalars",ppr->k_oversampling_scalars); */
/*   class_read_int("k_oversampling_tensors",ppr->k_oversampling_tensors); */

  class_read_int("transfer_cut",ppr->transfer_cut);
  class_read_double("transfer_cut_threshold_osc",ppr->transfer_cut_threshold_osc);
  class_read_double("transfer_cut_threshold_cl",ppr->transfer_cut_threshold_cl);
  class_read_int("transfer_integrate",ppr->transfer_integrate);
  class_read_int("l_switch_limber",ppr->l_switch_limber);

  /** h.7. parameters related to nonlinear calculations */

  class_read_int("b+c spectrum",ppr->has_bc_spectrum);
  class_read_int("double escape",ppr->double_escape);
  class_read_double("z_ini",ppr->z_ini);
  class_read_int("eta_size",ppr->eta_size);
  class_read_double("k_L",ppr->k_L);
  class_read_double("k_min",ppr->k_min);
  class_read_double("logstepx_min",ppr->logstepx_min);

  /** h.8. parameter related to lensing */

  class_read_int("num_mu_minus_lmax",ppr->num_mu_minus_lmax);
  class_read_int("delta_l_max",ppr->delta_l_max);


  /* check various l_max */

  pbs->l_max=0;

  if (ppt->has_scalars == _TRUE_) {
    
    if (ple->has_lensed_cls == _TRUE_)
      ppt->l_scalar_max+=ppr->delta_l_max;

    pbs->l_max=max(ppt->l_scalar_max,pbs->l_max);
  }

  if (ppt->has_tensors == _TRUE_) {   
    pbs->l_max=max(ppt->l_tensor_max,pbs->l_max);
  }

  /** (i) eventually write all the read parameters in a file */

  class_call(parser_read_string(pfc,"write parameters",&string1,&flag1,errmsg),
	     errmsg,
	     errmsg);	

  if ((flag1 == _TRUE_) && ((strstr(string1,"y") != NULL) || (strstr(string1,"Y") != NULL))) {

    sprintf(param_output_name,"%s%s",pop->root,"parameters.ini");

    class_open(param_output,param_output_name,"w",errmsg);

    fprintf(param_output,"# List of input/precision parameters actually read\n");
    fprintf(param_output,"# (all other parameters set to default values)\n");
    fprintf(param_output,"#\n");
    fprintf(param_output,"# This file, written by CLASS, can be used as the input file\n");
    fprintf(param_output,"# of another run provided that it has a suffix xxx.ini or xxx.pre;\n");
    fprintf(param_output,"# in the next run you can change this name with the\n");
    fprintf(param_output,"# parameters = ... entry of your input file.\n");
    fprintf(param_output,"#\n");

    for (i=0; i<pfc->size; i++) {
      if (pfc->read[i] == _TRUE_)
	fprintf(param_output,"%s = %s\n",pfc->name[i],pfc->value[i]);
    }
    fprintf(param_output,"#\n");

    fclose(param_output);
  }

  return _SUCCESS_;

}

/** 
 * All default parameter values (for input parameters)
 *
 * @param pba Input : pointer to background structure 
 * @param pth Input : pointer to thermodynamics structure 
 * @param ppt Input : pointer to perturbation structure
 * @param pbs Input : pointer to bessels structure
 * @param ptr Input : pointer to transfer structure 
 * @param ppm Input : pointer to primordial structure 
 * @param psp Input : pointer to spectra structure
 * @param pop Input : pointer to output structure
 * @return the error status
 */

int input_default_params(
			 struct background *pba,
			 struct thermo *pth,
			 struct perturbs *ppt,
			 struct bessels * pbs,
			 struct transfers *ptr,
			 struct primordial *ppm,
			 struct spectra *psp,
			 struct nonlinear * pnl,
			 struct lensing *ple,
			 struct output *pop
			 ) {

  double sigma_B; /**< Stefan-Boltzmann constant in W/m^2/K^4 = Kg/K^4/s^3 */

  sigma_B = 2. * pow(_PI_,5) * pow(_k_B_,4) / 15. / pow(_h_P_,3) / pow(_c_,2);

  /** - background structure */
      
  pba->h = 0.704;
  pba->H0 = pba->h * 1.e5 / _c_;
  pba->Tcmb = 2.726;
  pba->Omega0_g = (4.*sigma_B/_c_*pow(pba->Tcmb,4.)) / (3.*_c_*_c_*1.e10*pba->h*pba->h/_Mpc_over_m_/_Mpc_over_m_/8./_PI_/_G_);
  pba->Omega0_nur = 3.04*7./8.*pow(4./11.,4./3.)*pba->Omega0_g;
  pba->Omega0_b = 0.02253/0.704/0.704;
  pba->Omega0_cdm = 0.1122/0.704/0.704;
  pba->Omega0_ncdm1 = 0.;
  pba->M_ncdm1 = 0.;
  pba->T_ncdm1 = 1.;
  pba->ksi_ncdm1 = 0.;
  pba->Omega0_k = 0.;
  pba->Omega0_lambda = 1.+pba->Omega0_k-pba->Omega0_g-pba->Omega0_nur-pba->Omega0_b-pba->Omega0_cdm-pba->Omega0_ncdm1;
  pba->Omega0_de = 0.;     
  pba->a_today = 1.;       
  pba->w_de=-1.;
  pba->cs2_de=1.;

  /** - thermodynamics structure */

  pth->YHe=0.25;            
  pth->reio_parametrization=reio_camb;
  pth->reio_z_or_tau=reio_z;
  pth->z_reio=10.3;
  pth->tau_reio=0.085;

  /** - perturbation structure */

  ppt->has_cl_cmb_temperature = _FALSE_;
  ppt->has_cl_cmb_polarization = _FALSE_;
  ppt->has_cl_cmb_lensing_potential = _FALSE_;
  ppt->has_pk_matter = _FALSE_;
  ppt->has_matter_transfers = _FALSE_;

  ppt->has_ad=_TRUE_;  
  ppt->has_bi=_FALSE_;
  ppt->has_cdi=_FALSE_;
  ppt->has_nid=_FALSE_;
  ppt->has_niv=_FALSE_;

  ppt->has_scalars=_TRUE_;  
  ppt->has_vectors=_FALSE_;
  ppt->has_tensors=_FALSE_;  

  ppt->l_scalar_max=2500;
  ppt->l_tensor_max=500;
  ppt->k_scalar_kmax_for_pk=0.1;

  /** - bessels structure */

  pbs->l_max = max(ppt->l_scalar_max,ppt->l_tensor_max);
  pbs->bessel_always_recompute = _TRUE_;

  /** - primordial structure */

  ppm->primordial_spec_type = analytic_Pk;
  ppm->k_pivot = 0.002;
  ppm->A_s = 2.42e-9;
  ppm->n_s = 0.967;
  ppm->alpha_s = 0.;
  ppm->f_bi = 1.;
  ppm->n_bi = 1.;
  ppm->alpha_bi = 0.;
  ppm->f_cdi = 1.;
  ppm->n_cdi = 1.;
  ppm->alpha_cdi = 0.;
  ppm->f_nid = 1.;
  ppm->n_nid = 1.;
  ppm->alpha_nid = 0.;
  ppm->f_niv = 1.;
  ppm->n_niv = 1.;
  ppm->alpha_niv = 0.;
  ppm->c_ad_bi = 0.;
  ppm->n_ad_bi = 0.;
  ppm->alpha_ad_bi = 0.;
  ppm->c_ad_cdi = 0.;
  ppm->n_ad_cdi = 0.;
  ppm->alpha_ad_cdi = 0.;
  ppm->c_ad_nid = 0.;
  ppm->n_ad_nid = 0.;
  ppm->alpha_ad_nid = 0.;
  ppm->c_ad_niv = 0.;
  ppm->n_ad_niv = 0.;
  ppm->alpha_ad_niv = 0.;
  ppm->c_bi_cdi = 0.;
  ppm->n_bi_cdi = 0.;
  ppm->alpha_bi_cdi = 0.;
  ppm->c_bi_nid = 0.;
  ppm->n_bi_nid = 0.;
  ppm->alpha_bi_nid = 0.;
  ppm->c_bi_niv = 0.;
  ppm->n_bi_niv = 0.;
  ppm->alpha_bi_niv = 0.;
  ppm->c_cdi_nid = 0.;
  ppm->n_cdi_nid = 0.;
  ppm->alpha_cdi_nid = 0.;
  ppm->c_cdi_niv = 0.;
  ppm->n_cdi_niv = 0.;
  ppm->alpha_cdi_niv = 0.;
  ppm->c_nid_niv = 0.;
  ppm->n_nid_niv = 0.;
  ppm->alpha_nid_niv = 0.;
  ppm->r = 1.;
  ppm->n_t = 0.;
  ppm->alpha_t = 0.;

  /** - output structure */ 

  pop->z_pk_num = 1;
  pop->z_pk[0] = 0.;  
  sprintf(pop->root,"output/");

  /** - spectra structure */ 

  psp->z_max_pk = pop->z_pk[0];

  /** - nonlinear structure */

  /** - lensing structure */

  ple->has_lensed_cls = _FALSE_;

  /** - nonlinear structure */ 

  pnl->method = nl_none;

  /** - all verbose parameters */ 

  pba->background_verbose = 0;
  pth->thermodynamics_verbose = 0;
  ppt->perturbations_verbose = 0;
  pbs->bessels_verbose = 0;
  ptr->transfer_verbose = 0;
  ppm->primordial_verbose = 0;
  psp->spectra_verbose = 0;
  pnl->nonlinear_verbose = 0;
  ple->lensing_verbose = 0;
  pop->output_verbose = 0;

  return _SUCCESS_;

}

/** 
 * Initialize the precision parameter structure. 
 * 
 * All precision parameters used in the other moduels are listed here
 * and assigned here a default value.
 *
 * @param ppr Input/Ouput: a precision_params structure pointer  
 * @return the error status
 *
 */

int input_default_precision ( struct precision * ppr ) {

  /** Summary: */

  /**
   * - parameters related to the background
   */

  ppr->a_ini_over_a_today_default = 1.e-10;  /* 1.e-7 unless needs large k_max in P(k) */
  ppr->back_integration_stepsize = 0.1;   /* 03.12.10 for chi2plT0.01 */
  ppr->tol_background_integration = 1.e-2;  /* no sizeable impact */

  /**
   * - parameters related to the thermodynamics
   */

  /* for recombination */

  ppr->recfast_z_initial=1.e4;

  ppr->recfast_Nz0=10000;                  /* found to be OK on 3.09.10 */
  ppr->tol_thermo_integration=1.e-5;       /* found to be OK on 3.09.10 */

  ppr->recfast_Heswitch=6;                 /* from recfast 1.4 */
  ppr->recfast_fudge_He=0.86;              /* from recfast 1.4 */

  ppr->recfast_Hswitch = _TRUE_;           /* from recfast 1.5 */
  ppr->recfast_fudge_H = 1.14;             /* from recfast 1.4 */
  ppr->recfast_delta_fudge_H = -0.035;     /* from recfast 1.5 */
  ppr->recfast_AGauss1 = -0.14;            /* from recfast 1.5 */ 
  ppr->recfast_AGauss2 =  0.05;            /* from recfast 1.5 */
  ppr->recfast_zGauss1 =  7.28;            /* from recfast 1.5 */
  ppr->recfast_zGauss2 =  6.75;            /* from recfast 1.5 */
  ppr->recfast_wGauss1 =  0.18;            /* from recfast 1.5 */
  ppr->recfast_wGauss2 =  0.33;            /* from recfast 1.5 */

  ppr->recfast_z_He_1 = 8000.;             /* from recfast 1.4 */
  ppr->recfast_delta_z_He_1 = 50.;         /* found to be OK on 3.09.10 */
  ppr->recfast_z_He_2 = 5000.;             /* from recfast 1.4 */
  ppr->recfast_delta_z_He_2 = 100.;        /* found to be OK on 3.09.10 */
  ppr->recfast_z_He_3 = 3500.;             /* from recfast 1.4 */
  ppr->recfast_delta_z_He_3 = 50.;         /* found to be OK on 3.09.10 */
  ppr->recfast_x_He0_trigger = 0.995;      /* raised from 0.99 to 0.995 for smoother Helium */              
  ppr->recfast_x_He0_trigger2 = 0.995;     /* raised from 0.985 to same as previous one for smoother Helium */
  ppr->recfast_x_He0_trigger_delta = 0.05; /* found to be OK on 3.09.10 */
  ppr->recfast_x_H0_trigger = 0.995;       /* raised from 0.99 to 0.995 for smoother Hydrogen */
  ppr->recfast_x_H0_trigger2 = 0.995;      /* raised from 0.98 to same as previous one for smoother Hydrogen */
  ppr->recfast_x_H0_trigger_delta = 0.05;  /* found to be OK on 3.09.10 */ 

  ppr->recfast_H_frac=1.e-3;               /* from recfast 1.4 */

  /* for reionization */

  ppr->reionization_z_start_max = 50.;
  ppr->reionization_sampling=1.e-2; /*1.e-2*/
  ppr->reionization_optical_depth_tol=1.e-2;
  ppr->reionization_exponent=1.5;

  ppr->reionization_width=0.5;  /* could be converted as 'input parameters' */

  ppr->reionization_start_factor=8.;
  ppr->helium_fullreio_redshift=3.5;
  ppr->helium_fullreio_width=0.5;

  /* general */

  ppr->thermo_rate_smoothing_radius=50;

  /**
   * - parameters related to the perturbations
   */

  ppr->gauge=synchronous;

  ppr->k_scalar_min_eta0=0.032;                /* 14.12.10 for chi2plT0.01  and reasonnable small ls */
  ppr->k_scalar_max_eta0_over_l_max=1.65; /* 14.12.10 for chi2plT0.1 and reasonnable large ls*/
  ppr->k_scalar_step_sub=0.08;           /* 14.12.10 for chi2plT0.1 */
  ppr->k_scalar_step_super=0.005;        /* 14.12.10 for chi2plT0.1 and reasonnable small ls */
  ppr->k_scalar_step_transition=0.2;     /* 14.12.10 for chi2plT0.1 */
  /* when optimizing, start by setting step_super=step_sub (then
     transition is irrelevant). Once this is done, try top increase
     step_sub for various values of transition. Keep the value of
     transition allowing for the largest step_sub. */

  ppr->k_scalar_k_per_decade_for_pk=10.;

  ppr->k_tensor_min=0.1; /* 0.3 -> 0.1 */
  /*  ppr->k_tensor_oscillations=3.5;   */
  ppr->l_max_over_k_max_tensors = 6300.;
  ppr->k_tensor_step_sub=0.01;  /* 0.02 -> 0.005 */
  ppr->k_tensor_step_super=0.0002;  /* 0.01 -> 0.005 */
  ppr->k_tensor_step_transition=0.2;

  ppr->start_small_k_at_eta_g_over_eta_h = 0.006; /* 04.12.10 for chi2plT0.1 */ /* decrease to start earlier in time */
  ppr->start_large_k_at_eta_h_over_eta_k = 1.e-1;  /* decrease to start earlier in time */
  ppr->tight_coupling_trigger_eta_g_over_eta_h=0.008; /* 14.12.10 for chi2plT0.1 */ /* decrease to swith off earlier in time */
  ppr->tight_coupling_trigger_eta_g_over_eta_k=0.05; /* 14.12.10 for chi2plT0.1 */ /* decrease to swith off earlier in time */
  ppr->start_sources_at_eta_g_over_eta_h = 0.01; /* 14.12.10 for chi2plT0.1 */ /* decrease to start earlier in time */
  ppr->tight_coupling_approximation=(int)second_order_CRS;

  ppr->l_max_g=10; /* 03.12.10 for chi2plT0.01 */
  ppr->l_max_pol_g=10; /* 03.12.10 for chi2plT0.01 */
  ppr->l_max_nur=23; /* 14.12.10 for chi2plT0.1 */
  ppr->l_max_g_ten=5;
  ppr->l_max_pol_g_ten=5;

  ppr->curvature_ini=1.; /* initial curvature; used to fix adiabatic initial conditions; must remain fixed to one as long as the primordial adiabatic spectrum stands for the curvature power spectrum */
  ppr->entropy_ini=1.;   /* initial entropy; used to fix isocurvature initial conditions; must remain fixed to one as long as the primordial isocurvature spectrum stands for an entropy power spectrum */
  ppr->gw_ini=0.25; /* to match normalization convention for GW in most of literature and ensure standard definition of r */

  ppr->perturb_integration_stepsize=0.5; /* 0.5 */ 
  ppr->tol_eta_approx=1.e-5; /* 03.12.10 for chi2plT0.01 */
  ppr->tol_perturb_integration=1.e-6; /* 07.12.10 for chi2plT0.01: 1.e-4 for ndf15 */
  ppr->perturb_sampling_stepsize=0.08; /* 14.12.10 for chi2plT0.1 */

  ppr->free_streaming_approximation = fsa_MD_with_reio;
  ppr->free_streaming_trigger_eta_h_over_eta_k = 30.; /* 14.12.10 for chi2plT0.1 */
  ppr->free_streaming_trigger_Omega_r = 0.15; /* 14.12.10 for chi2plT0.1 */

  /**
   * - parameter related to the Bessel functions
   */

  ppr->l_logstep=1.035; /* 14.12.10 for chi2plT0.1 */
  ppr->l_linstep=35; /* 14.12.10 for chi2plT0.1 and reasonnable large ls */
  /* when optimizing, always compare to case linstep=1 (all l's) */

  ppr->bessel_x_step=0.045; /* 14.12.10 for chi2plT0.1 */
  ppr->bessel_j_cut=1.5e-4; /* 14.12.10 for chi2plT0.1 */
  ppr->bessel_tol_x_min =1.e-4; /* 03.12.10 for chi2plT0.01 */
  ppr->bessel_x_max_over_l_max=1.8; /* 03.12.10 for chi2plT0.01 */
  sprintf(ppr->bessel_file_name,"bessels.dat");

  /**
   * - parameter related to the primordial spectra
   */

  ppr->k_per_decade_primordial = 10.; 

  /**
   * - parameter related to the transfer functions
   */
  
  ppr->k_step_trans_scalars=0.2; /* 03.12.10 for chi2plT0.01: difficult to optimize, numerical instability below 0.1, need to study this better */
  ppr->k_step_trans_tensors=0.15;

/*   ppr->k_oversampling_scalars=3; */
/*   ppr->k_oversampling_tensors=1; */

  ppr->transfer_cut=tc_cl; /* 03.12.10 for chi2plT0.01: tc_cl slightly faster (by 20%) for equal precision, but also slightly less robust (better to switch to tc_osc if primordial tilt can depart significantly from one) */
  ppr->transfer_cut_threshold_osc=0.015; /* 03.12.10 for chi2plT0.01 */
  ppr->transfer_cut_threshold_cl=2.e-6; /* 14.12.10 for chi2plT0.01 */

  ppr->transfer_integrate = trapezoidal;

  ppr->l_switch_limber=10;

  /**
   * - parameters related to trg module
   */

  ppr->double_escape=2;
  ppr->has_bc_spectrum=_TRUE_;
  ppr->z_ini = 35.;
  ppr->eta_size = 100.;
  ppr->k_L = 1.e-3;
  ppr->k_min = 1.e-4;
  ppr->logstepx_min = 1.04;

  /**
   * - parameter related to lensing
   */

  ppr->num_mu_minus_lmax=400;
  ppr->delta_l_max=100;

  /**
   * - automatic estimate of machine precision
   */

  get_machine_precision(&(ppr->smallest_allowed_variation));

  class_test(ppr->smallest_allowed_variation < 0,
	     ppr->error_message,
	     "smallest_allowed_variation = %e < 0",ppr->smallest_allowed_variation);

  return _SUCCESS_;

}

int class_version(
		  char * version
		  ) {
  
  sprintf(version,"%s",_VERSION_);
  return _SUCCESS_;
}

/** 
 * Computes automatically the machine precision. 
 *
 * @param smallest_allowed_variation a pointer to the smallest allowed variation
 *
 * Returns the smallest
 * allowed variation (minimum epsilon * _TOLVAR_)
 */

int get_machine_precision(double * smallest_allowed_variation) {
  double one, meps, sum;
  
  one = 1.0;
  meps = 1.0;
  do {
    meps /= 2.0;
    sum = one + meps;
  } while (sum != one);
  meps *= 2.0;
  
  *smallest_allowed_variation = meps * _TOLVAR_;

  return _SUCCESS_;

}
