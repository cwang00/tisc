/*
	LIBRARY  FOR  SURFACE PROCESSES of tisc.c

	1995-2018 Daniel Garcia-Castellanos
	Check copyright and other information in tisc/doc/ 
*/

#include "types_n_defs.h"

#define	NDERS   		8	/*Number of derivatives or sorrounding nodes at every node*/

/*convert between sediment thickness and sediment grain mass*/
#define MASS2SEDTHICK(mass)	((mass) /(denssedim-sed_porosity*denswater)/dx/dy)	/*converts sediment mass into sediment thickness*/
#define THICK2SEDMASS(thick)	((thick)*(denssedim-sed_porosity*denswater)*dx*dy)	/*converts sediment thickness into sediment mass*/

extern struct GRIDNODE	*sortcell;
extern struct DRAINAGE	**drainage;
extern struct LAKE_INFO *Lake;		/*Lake[0] does not exist; Lake[1] is the sea or the first normal lake.*/
extern struct CS2D 	*CrossSection;
extern struct BLOCK 	*Blocks;

extern int
	erosed_model, 
	Nx, Ny, 
	idt_eros,
	DOY_month[12],
	nlakes, 		/*number of lakes >= 0 */
	nbasins, 
	numBlocks,
	hydro_model, 
	verbose_level,
	**lake_former_step;

extern float 	Time,
	densenv, denssedim, denscrust, 
	dx, dy, dxy, 
	evaporation_ct, 		/*[m3/s/m2].*/
	K_river_cap, 		/*Constant of river transport capacity [kg/m3].*/
	K_ice_eros, 
	erodibility, 		/*Default length scale of fluvial erosion */
	erodibility_sed, 	/*Length scale of fluvial erosion of sediment Blocks*/
	tau_c, 
	spl_m, spl_n, 
	l_fluv_sedim, 		/*Length scale of fluvial sedimentation */
	lost_rate, 		/*Percent of lost water per unit length */
	permeability, /*[m2]*/
	sed_porosity, 
	rain, Krain, relative_humidity, /*[m3/s/m2], [m3/s/m2/m], []*/
	windazimut, CXrain, CYrain, 	/*[deg], [m], [m]*/
	sea_level, temp_sea_level, 
	total_rain,
	total_bedrock_eros_mass,
	total_sed_mass,
	viscwater, 
	xmin, xmax, ymin, ymax;

extern float 	
	**Dq, 
	**eros_now, 
	**ice_thickness, 
	**ice_sedm_load, 
	**ice_velx_sl, **ice_vely_sl, 
	**ice_velx_df, **ice_vely_df, 
	**evaporation, 
	**Tavg_mon, // Daily average temperature for 12 months
	**Tmax_mon, // Daily max T for 12 months
	**Tmin_mon, // Daily min T for 12 months
	**RHmean_mon, // Daily mean relative humidity for 12 months
	**Rn_mon, // Solar radiation
	**et_eq, // penman equilibrium evaporation [L/T]
	**et_eq_mon,
	**et_pt, // potential evapotranspiration [L/T]
	**et_pt_mon,
	**et_a, // actual evapotranspiration [L/T]
	**elk, // lake evaporation [L/T]
	**etr, // riparian evapotranspiration [L/T]
	**eth, // hillslope evapotranspiration [L/T]
	**et_tot, // total evapotranspiration from grid cells [L/T]
	**et_tot_ant, // Total evapotranspiration from previous erosion/LSM time step [L/T]
	**relHumidity, // Relative humidity at each erosition time step [-]
	**W_grid,
	**Wmax_grid,
	**T_mean_annual_file,
	**P_mean_annual_file,
	**Pmaz_grid,
	**precipitation, 
	**precipitation_snow, 
	**precipitation_file, 
	**topo, 
	**slope_grid,
	**azimuth_grid,
	**accumul_erosion, 
	**Blocks_base;

extern char
	eros_bound_cond[5];


extern float 
	g,
	denswater, 
	lheat,
	pi,
	densice; 


/*Declaration of functions at libreria.c*/
float 	**alloc_matrix  (int num_fil, int num_col);
int 	free_matrix 	(float **matrix, int num_fil);
int 	diffusion_2D	(float **Matrix, float **d_Matrix, int Nx, int Ny, float Kdiff, float dx, float dy, float dteros);


/*Declaration of functions below*/
int 	Add_Node_To_Lake (int row, int col, int i_lake);
int 	Add_Outlet_To_Lake (int row_sd, int col_sd, int row_tr, int col_tr, int i_lake);
int 	Attempt_Delete_Node_From_Lake (int row, int col);
int 	Deallocate_Lake (int i_lake);
int 	Define_Lake (int i_lake);
int 	Delete_Node_From_Lake (int row, int col);
int 	Divide_Lake (int row, int col);
int 	Erode (double d_mass, int row, int col);
int 	constant_rate_eros (float **topo, float Keroseol, float Ksedim, float sea_level, int water_load, float dt, float Time);
int 	Fluvial_Transport(struct GRIDNODE *sortcell, float dt_st, int erosed_model, float *total_lost_sed_mass, int lake_instant_fill);
int 	Ice_EroSed (float **ice_velx_sl, float **ice_vely_sl, float dt_eros, float *total_ice_eros, float *total_ice_sedim);
int 	Ice_Flow (float **ice_velx_sl, float **ice_vely_sl, float **ice_velx_df, float **ice_vely_df, float dt_st, float *total_ice_melt, float *total_ice_precip, float *total_lost_water, float *total_evap_water);
int 	Lake_Fill (struct LAKE_INFO *Lake, int row, int col, float hl, float dt_st, int lake_instant_fill);
float 	Lake_Input_Discharge (int ilake);
int 	Lake_Node_Number(int row, int col);
int 	Lake_Outlet_Number (int row, int col);
float 	Minimum_Neg_Slope (int i, int j, int *dr_row, int *dr_col);
int 	New_Lake ();
int 	Calculate_Precipitation_Evaporation (); 
float 	Orographic_Precipitation_with_local_slope (int i, int j, float windvel, float windazimut);
int 	Orographic_Precipitation_Evaporation_conservative (float windvel, float windazimut, float);
int 	Precipitation_Evaporation_at_cell (int i, int j, float *Wcol, float windvel, float dtwind);
float 	max_water_in_air_colum (int i, int j);
int 	Damn_River_Node (int ia, int ja, int i,  int j);
int 	Rise_Damn_Node (int iia, int jja, int i, int j);
int 	Sediment (double d_mass, int row, int col);
int 	Unify_Lakes (int i_lake, int i_lake_to_delete);
int 	Diffusive_Eros (float Kerosdif, float dt, float dt_eros);
int 	Landslide_Transport (float critical_slope, float dt, float dt_eros);
int 	read_file_node_defs(float dt_st);

/*Declaration of functions related to upgraded land surface model*/
void 	arcslope(int nr, int nc, float dy, float dx, float **zz, float **slp);
void 	arcdzxy(int nr, int nc, float dy, float dx, float **zz, float dzx[nr][nc], float dzy[nr][nc]);
float 	et_riparian_hillslope(float Qw,float dd,int row,int col);
int 	land_surface_process(void);
void 	evapotranspiration_grid(float Tmaa_zr, float Rmmt, float Pma_zr);
float 	evaporation_penman_equilibrium(float d_Rn,float T_avg_cell,float elev_cell);
float 	evapotranspiration_potential(int DOY,float T_avg_cell,float dLat);
float 	evapotranspiration_actual(float P, float PET);
float 	width_channel(float Qw, float kw, float aw);
float 	width_riparian(float wc, float kr, float ar);
float   e0(float TT);
float   slope_es_fcn(float T_avg);
float   Psych_fcn(float dCellP);
float   Pair_fcn(float elev);
float    netSolarRadiation(int DOY,float T_max_cell,float T_min_cell,
				float RHmean,float windspeed,float Kcln,float K_rs,
				float albedo,float dLat,float elev_cell, float slope,
				float Azimuth);



int Surface_Transport (float **topo, float **topo_ant, float dt, float dt_eros, int erosed_model, int lake_instant_fill) 
{
	/*
	  THIS ROUTINE COMPUTES CHANGES IN TOPOGRAPHY AND ISOSTATIC LOAD DUE
	  TO FLUVIAL AND GLACIAR EROSION/TRANSPORT/SEDIMENTATION.
	*/

	int 	n_iters;	/*Number of substeps to subdivide fluvial processes*/
	float	dt_st, 
		**Dtopo, **topoini, 
		d_mass, 	/*Increment of suspended mass in this cell (positive means erosion).*/
		total_lost_sed_mass=0, 
		total_lost_water, total_evap_water, total_underground_water, 
		total_ice_melt, total_ice_precip, 
		oldicevol=0, oldicesedvol=0, total_ice_eros=0, total_ice_sedim=0;

	if (!hydro_model) return(0);

	PRINT_DEBUG("Calculating surface transport");

	Dtopo  = alloc_matrix(Ny, Nx);
	topoini  = alloc_matrix(Ny, Nx);

	calculate_topo(topo);

	/*Restore the previous topo and keep the topo increment to add it slowly*/
	for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++)  {
		topoini[i][j]=topo[i][j];
		Dtopo[i][j]=topo[i][j]-topo_ant[i][j]; 
		topo[i][j]=topo_ant[i][j];
	}
	if (K_ice_eros) {
		for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
			oldicevol += ice_thickness[i][j];
			oldicesedvol += ice_sedm_load[i][j];
		}
	}

	n_iters = MAX_2(floor(dt/dt_eros+.5), 1);
	dt_st = dt/n_iters;
	PRINT_INFO("n_iters=%3d", n_iters);

	/*Distributes transport in 'n_iters' substeps:*/
	for (int iter=0; iter<n_iters; iter++) {	  	  
		if (verbose_level>=3) fprintf(stdout, "\b\b\b%3d", n_iters-iter); fflush(stdout);

		total_ice_melt = total_ice_precip = 0;
		total_rain = total_lost_water = total_evap_water = total_underground_water = 0;

		/*Adds a proportional part of the last topo increment.*/
		for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) topo[i][j] += Dtopo[i][j]/n_iters;

		for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
			drainage[i][j].masstr = 0;
			drainage[i][j].discharge = 0;
		}

		/*Resorts the matrix of topography.*/
		ReSort_Matrix (topo, sortcell, Nx, Ny);

		Define_Drainage_Net(sortcell);

		Calculate_Precipitation_Evaporation();
		
		Ice_Flow(ice_velx_sl, ice_vely_sl, ice_velx_df, ice_vely_df, dt_st, &total_ice_melt, &total_ice_precip, &total_lost_water, &total_evap_water);

		read_file_node_defs(dt_st);

		Calculate_Discharge(sortcell, &total_lost_water, &total_evap_water, &total_underground_water);

		Fluvial_Transport(sortcell, dt_st, erosed_model, &total_lost_sed_mass, lake_instant_fill);

		Ice_EroSed(ice_velx_sl, ice_vely_sl, dt_st, &total_ice_eros, &total_ice_sedim);
		idt_eros += 1;
	}

	if (verbose_level>=3) fprintf(stdout, "\b\b\b"); fflush(stdout);

	/*Print relevant statistics*/
	if (verbose_level>=1) {
			float 	error;
		PRINT_GRID_INFO (secsperyr*precipitation, "precipit.", "m/yr");
		PRINT_GRID_INFO (secsperyr*evaporation,   "evaporat.", "m/yr");
		PRINT_SUMLINE("rain_now : %+8.2e m3/s  evap_wat: %+8.2e m3/s outp_water: %+8.2e m3/s undergr_water: %+8.2e m3/s", total_rain, total_evap_water, total_lost_water, total_underground_water); 
		if (total_rain) error=-(total_rain-total_evap_water-total_lost_water+total_ice_melt)/total_rain*100; else error = (total_ice_melt-total_evap_water-total_lost_water)/total_ice_melt*100;
			if (fabs(error)>=1)
				PRINT_WARNING("water_balance: %.1f%% (>0 => disch>rain)", error);
	}
	if (K_ice_eros && verbose_level>=1) {
		float 	vel_dfmax=-1e15, vel_df, vel_slmax=-1e15, vel_sl, max_ice_thick=0, newicevol=0, newicesedvol=0;		
		int 	i_vel_slmax, j_vel_slmax, i_vel_dfmax, j_vel_dfmax, imax, jmax;
		for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
			vel_df = sqrt(ice_velx_df[i][j]*ice_velx_df[i][j]+ice_vely_df[i][j]*ice_vely_df[i][j]);
			if (vel_df>vel_dfmax) {vel_dfmax = vel_df; i_vel_dfmax=i; j_vel_dfmax=j;}
			vel_sl = sqrt(ice_velx_sl[i][j]*ice_velx_sl[i][j]+ice_vely_sl[i][j]*ice_vely_sl[i][j]);
			if (vel_sl>vel_slmax) {vel_slmax = vel_sl; i_vel_slmax=i; j_vel_slmax=j;}
		}
		for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
			newicevol += ice_thickness[i][j]; 
			newicesedvol += ice_sedm_load[i][j]; 
			if (max_ice_thick <= ice_thickness[i][j]) {
				max_ice_thick = ice_thickness[i][j];
				imax=i; jmax=j;
			}
		}
		{
				float 	error, incr_rate;
			incr_rate = (newicevol-oldicevol)*dx*dy/dt;
			PRINT_SUMLINE("snow_now : %+8.2e m3/s  melt_ice: %+8.2e m3/s ice_incr:   %+8.2e m3/s", total_ice_precip, total_ice_melt, incr_rate);
			if (total_ice_precip) error=(total_ice_precip-total_ice_melt-incr_rate)/total_ice_precip*100; else error=(total_ice_melt+incr_rate)/total_ice_melt*100;
			if (fabs(error)>=1)
					PRINT_SUMLINE("ice_balance:   %.1f%% (>0 => snow>ice_incr)", error);
		}
		PRINT_SUMLINE("temp.@sea: %.2f C;   @ 1000 m: %.2f C", TEMPERATURE(0)-TEMP_FREEZE_WATER, TEMPERATURE(1000)-TEMP_FREEZE_WATER);
		PRINT_SUMLINE("ice	  :  max= %.0f m @ %.0f,%.0f km   vol= %.2e km3   vel_max(df,sl)= %.2f,%.2f m/yr", 
			ice_thickness[imax][jmax], (jmax*dx+xmin)/1e3, (ymax-imax*dy)/1e3, newicevol*dx*dy/1e9, 
			vel_dfmax*secsperyr,  vel_slmax*secsperyr);
		if (verbose_level>=3) fprintf(stdout, " @ %.0f,%.0f & %.0f,%.0f km", 
			(j_vel_dfmax*dx+xmin)/1e3, (ymax-i_vel_dfmax*dy)/1e3,  
			(j_vel_slmax*dx+xmin)/1e3, (ymax-i_vel_slmax*dy)/1e3);
		{
				float 	error, incr_rate;
			incr_rate = (newicesedvol-oldicesedvol)*dx*dy*denscrust;
			PRINT_SUMLINE("ice_eros : %+8.2e N	 ice_sedim: %+8.2e N   inc_glac_sd: %+8.2e N  ", total_ice_eros*g, total_ice_sedim*g, incr_rate*g);
				if (total_ice_eros) error=-(total_ice_eros-total_ice_sedim-incr_rate)/total_ice_eros*100; else error=(incr_rate-total_ice_sedim)/incr_rate*100;
			if (fabs(error)>=1)
					PRINT_WARNING("icesed_balance: %.1f%% (>0 => sed>eros)", error);
		}
	}
	if (hydro_model && verbose_level>=1) {
			int i_biggest_nosea=0, n_biggest_nosea=0, largest_river=0;
			for (int i=1; i<=nlakes; i++) {
				if (Lake[i].n > n_biggest_nosea) {
					if (Lake[i].n_sd) {
					if (topo[Lake[i].row_sd[0]][Lake[i].col_sd[0]] > sea_level)
						n_biggest_nosea = Lake[i].n; i_biggest_nosea = i;
					}
					else	n_biggest_nosea = Lake[i].n; i_biggest_nosea = i;
				}
			}
			for (int i=1; i<=nlakes; i++) {
				if (i==i_biggest_nosea || (Lake[i].n>ceil((double) Nx*Ny/500) && verbose_level>=2) || (Lake[i].n>ceil((double) Nx*Ny/2000) && verbose_level>=3)) {
				PRINT_SUMLINE("lake %3d/%d : %7.2e km3 %7.2e km2 %4.0f m ", i, nlakes, Lake[i].vol/1e9, Lake[i].n*dx*dy/1e6, Lake[i].alt);
				if (Lake[i].n) fprintf(stdout, "%4.0f,%-4.0f %2d out ", (Lake[i].col[0]*dx+xmin)/1e3, (ymax-Lake[i].row[0]*dy)/1e3, Lake[i].n_sd);
				if (Lake[i].n_sd) {
					if (topo[Lake[i].row_sd[0]][Lake[i].col_sd[0]]>sea_level) {
					 fprintf(stdout, "@	  %3.0f,%-3.0f %5.1f m3/s", (Lake[i].col_sd[0]*dx+xmin)/1e3, (ymax-Lake[i].row_sd[0]*dy)/1e3, Lake_Input_Discharge(i));
					}
					else fprintf(stdout, "Sea	%3.0f,%-3.0f %5.1f m3/s", (Lake[i].col[Lake[i].n-1]*dx+xmin)/1e3, (ymax-Lake[i].row[Lake[i].n-1]*dy)/1e3, Lake_Input_Discharge(i));
				}
				else	 fprintf(stdout, "Endorh %3.0f,%-3.0f %5.1f m3/s", (Lake[i].col[Lake[i].n-1]*dx+xmin)/1e3, (ymax-Lake[i].row[Lake[i].n-1]*dy)/1e3, Lake_Input_Discharge(i));
				}
			}
		{
			float max_river_discharge=0; int imax, jmax;
			for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
					if (max_river_discharge <= drainage[i][j].discharge) {
						max_river_discharge = drainage[i][j].discharge;
						imax=i; jmax=j;
					}
			}
			PRINT_SUMLINE("river_max: %8.2f m3/s %8.2f kg/s @ %6.1f,%.1f km, %.1f m",
					drainage[imax][jmax].discharge, drainage[imax][jmax].masstr, (jmax*dx+xmin)/1e3, (ymax-imax*dy)/1e3, topo[imax][jmax]);
		}
		{
			float max_eros=0, max_sedim=0, diff;
			calculate_topo(topo);
			for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
				diff=topo[i][j]-topoini[i][j]; 
					if (max_eros > diff) {
						max_eros = diff;
					}
					if (max_sedim < diff) {
						max_sedim = diff;
					}
			}
			PRINT_SUMLINE("topo_diff_eros_max= %8.2f mm/yr	sedim_max: %8.2f mm/yr",
					max_eros*1e3/(dt/secsperyr), max_sedim*1e3/(dt/secsperyr) );
		}
	}
	PRINT_SUMLINE("eros_nosd: %+8.2e N	 sedim_inc: %+8.2e N   outp_seds:   %+8.2e N  ",  total_bedrock_eros_mass*g, total_sed_mass*g, total_lost_sed_mass*g);
	{
		float error;
		if (total_bedrock_eros_mass) error = -(total_bedrock_eros_mass-total_sed_mass-total_lost_sed_mass)/total_bedrock_eros_mass*100; else error = (total_sed_mass-total_lost_sed_mass)/total_sed_mass*100;
		if (fabs(error)>=1)
			PRINT_WARNING("seds._balance: %.1f%% (>0 => sed>eros)", error);
	}

	free_matrix(Dtopo, Ny);
	free_matrix(topoini, Ny);
	return (1);
}



int Calculate_Discharge (struct GRIDNODE *sortcell, float *total_lost_water, float *total_evap_water, float *total_underground_water)
{
	/*
	  CALCULATES THE LIQUID WATER DISCHARGE ALONG THE HYDROLOGICAL NETWORK.
	  Modifies the drainage network accounting for LAKE EVAPORATION and its 
	  reduction of lake area in case of ENDORHEISM.
	*/

	int 	il, 
		row, col, drow, dcol;
	float	runoff, 
		dd, dxdivdy=dx/dy;

	PRINT_DEBUG("Calculating discharge");
	/*
	  This loop starts from the highest cell and 
	  descends node by node transferring water and eroded mass
	*/
	for (int isort=0; isort < Nx*Ny; isort++) {
			row  = sortcell[isort].row;
			col  = sortcell[isort].col;
			drow = drainage[row][col].dr_row;
			dcol = drainage[row][col].dr_col;
			il   = drainage[row][col].lake;

			//Check if this is an endorheic lake node and then check if the 
			//lake surface must be lowered by making this a river node
			if (il) {
				float lake_evap=0;
				for (int i=0; i<Lake[il].n;i++) lake_evap += evaporation[Lake[il].row[i]][Lake[il].col[i]];
				lake_evap *= dx*dy;
				if (Lake_Input_Discharge(il)<lake_evap && Lake[il].n>1) {
					PRINT_DEBUGPLUS("Deletion attempt for [%d][%d] from lake %d ; lake_inp:%f ; evap:%f ; lakenodes:%d", row,col, il, Lake_Input_Discharge(il), lake_evap, Lake[il].n);
					Attempt_Delete_Node_From_Lake (row,col);
					drow = drainage[row][col].dr_row;
					dcol = drainage[row][col].dr_col;
					il = drainage[row][col].lake;
				}
			}

			/*Calculate the distance to the output node*/
			/*
			This is used in the calculation of discharge lost along the network
			during water transfer through lost_rate. ChaoWang202007281820
			*/
			switch (drainage[row][col].type) {
			  case 'R': // This runs the code for 'E'
			  case 'E':
				if (IN_DOMAIN(drow, dcol))
					dd = sqrt(dy*(drow-row)*dy*(drow-row) + dx*(dcol-col)*dx*(dcol-col));
				else
					dd = 0;
				break;
			  case 'L':
				dd = 0;
				break;
			  default:
				PRINT_ERROR("[%d][%d] has no defined drainage type in Calculate_Discharge.", row, col);
			}

			/*Adds the rainfall water (m3/s) to the water transported to this cell: */
			runoff = precipitation[row][col] * dx*dy;
			/*Put the rain of open lakes and sea in their outlets. Closed lakes keep it at the recipient node*/
			if (drainage[row][col].type == 'L') {
				/*if (Lake[il].n_sd)*/ runoff = 0; /*!!*/
			}
			if (drainage[row][col].type == 'E') {
				/*Put into this outlet the rain from lake nodes draining here.*/
				for (int i=0; i<Lake[il].n; i++) {
					/*Polish code and fix possible error ChaoWang201912300824*/
					int irow, icol;
					irow = Lake[il].row[i];
					icol = Lake[il].col[i];
					if (drainage[irow][icol].dr_row == row && drainage[irow][icol].dr_col == col) {
						runoff += precipitation[irow][icol] * dx*dy; // Possible error fixed ChaoWang201912280816
					}
				}
			}
 
			total_rain += runoff;
			drainage[row][col].discharge += runoff;

			/*Remove evaporated lake water from outlets (evaporation in endorheic lakes is done below)*/
			/*
			Can this account for that some lake cells can become river cells if
			lake evap is larger than input discharge?

			If the evaporation (m/s) is calculated using 
			Orographic_Precipitation_Evaporation_conservative(), then
			atmospheric water balance is already calculated before calculate
			actual evaporation from land surface. However, actual evaporation
			from the lake can be smaller than the evaporation rate calculate
			from the atmospheric model. And actual ET from grid cells other than
			lakes are not recycled into atmospheric water balance calculation.
			This might serious influence precipitation and ET spatial
			distribution. ChaoWang202005301722
			Recycling the moisture using the previous time step actual ET
			preserves the water balance with a lagged time step.
			ChaoWang202008121128
			*/
			if (drainage[row][col].type == 'E') {
				float lake_evap=0, input_disch, factor;
				for (int i=0; i<Lake[il].n; i++) lake_evap += evaporation[Lake[il].row[i]][Lake[il].col[i]];
				lake_evap *= dx*dy;
				input_disch = Lake_Input_Discharge(il);
				if (input_disch) factor = MIN_2(1, lake_evap/input_disch);
				else factor = 0;
				*total_evap_water += drainage[row][col].discharge * factor;
				drainage[row][col].discharge -= drainage[row][col].discharge * factor;
				float factor2;
				if (input_disch) factor2 = MIN_2(1, input_disch/lake_evap);
				for (int i=0; i<Lake[il].n; i++)
					et_tot_ant[Lake[il].row[i]][Lake[il].col[i]] = evaporation[Lake[il].row[i]][Lake[il].col[i]] * factor2;
			}

			// When we reach a node (row, col), all nodes higher than this node
			// has been accounted. So now discharge at this node is the
			// cumulative discharge with ET and loss in upstream deducted.
			// Deduct riparian ET and channel infiltration for river node here.
			// ChaoWang202004221132

			// Note that now this is applied to all types of cells before
			// water transfer. Should this be applied to only river cells?
			// ChaoWang202005301727
			if (hydro_model == 4 && drainage[row][col].type == 'R'){
				float et_rh; // Riparian and hillslope evapotranspiration
				// Upgraded ET from riparian and hillslope area, ChaoWang202004241028
				et_rh = et_riparian_hillslope(drainage[row][col].discharge,dd,row,col);
				et_tot_ant[row][col] = et_rh;
				*total_evap_water += MIN_2(et_rh*dx*dy, drainage[row][col].discharge);
				drainage[row][col].discharge -= MIN_2(et_rh*dx*dy, drainage[row][col].discharge);
			}

			/*Underground seepage of part of the water to lower nodes*/
			if (permeability) {
				float rndi, rndj;
				int ru=25; //Half-width of the rectangle of underground flow calculation (in cells)
				int i_ini, i_end, i_inc, j_ini, j_end, j_inc;
				rndi=(((float) rand())/((float) RAND_MAX));
				rndj=(((float) rand())/((float) RAND_MAX));
//PRINT_ERROR(">>>>>>>>>>> %.3f %.3f", rndi, rndj)
				if (rndi >= .5) {
					i_ini=row-ru;
					i_end=row+ru;
					i_inc=+1;
				} 
				else {
					i_ini=row+ru;
					i_end=row-ru;
					i_inc=-1;
				}
				if (rndj >= .5) {
					j_ini=col-ru;
					j_end=col+ru;
					j_inc=+1;
				} 
				else {
					j_ini=col+ru;
					j_end=col-ru;
					j_inc=-1;
				}
				for (int i=i_ini; i>=i_ini && i<=i_end && drainage[row][col].discharge>0; i+=i_inc) 
					for (int j=j_ini; j>=j_ini && j<=j_end && drainage[row][col].discharge>0; j+=j_inc) 
						if (IN_DOMAIN(i, j)) if (i!=row || j!=col) {
							float dist, underground_water_flow;
							dist=sqrt((i-row)*(i-row)*dy*dy+(j-col)*(j-col)*dx*dx);
							/*Darcy's law (isotropic porous medium): fluid_velocity = perm/visc * pressure_diff/distance */
							/*Need to account for the 3D effect properly*/
							underground_water_flow = (topo[row][col]>topo[i][j])? MIN_2(drainage[row][col].discharge, dxy*dxy*dx*dy/dist/dist*permeability/viscwater*denswater*g*(topo[row][col]-topo[i][j])/dist) : 0;
							drainage[row][col].discharge -= underground_water_flow;
							drainage[i][j].discharge     += underground_water_flow;
							*total_underground_water     += underground_water_flow;
				}
			}

			/*Transfers water.*/
			if (IN_DOMAIN(drow, dcol)) {
				/*Remove evapotranspirated water from the rivers*/
				/*
				Evapotranspiration is now calculated by the upgraded
				ET model for riparian and hillslope area. The lost_rate
				can be used to represent infiltration loss during transfer.
				Otherwise set lost_rate = 0.
				ChaoWang202007281837
				*/ 
				*total_evap_water			 	+= drainage[row][col].discharge * MIN_2(lost_rate*dd, 1);
				drainage[row][col].discharge	-= drainage[row][col].discharge * MIN_2(lost_rate*dd, 1);
				// Transfers water
				switch (drainage[drow][dcol].type) {
				case 'L':
					/*Check: this shouldn't happen (a node transferring to a lake with a higher level).*/
					if (Lake[drainage[drow][dcol].lake].n_sd) if (IN_DOMAIN(drainage[drow][dcol].dr_row, drainage[drow][dcol].dr_col)) if (topo[drainage[drow][dcol].dr_row][drainage[drow][dcol].dr_col] > topo[row][col])
						PRINT_ERROR("[%d][%d] transferring water to lake in [%d][%d] is < than outlet [%d][%d]:  %.1f<%.1f.", row, col, drow, dcol, drainage[drow][dcol].dr_row, drainage[drow][dcol].dr_col, topo[row][col], topo[drainage[drow][dcol].dr_row][drainage[drow][dcol].dr_col]);
					/*Drain to the lake node*/
					drainage[drow][dcol].discharge += drainage[row][col].discharge;
					/*Drain also to its outlet if it has*/
					/*
					Note that the down gradient cell of a lake cell
					is set to the outlet of the lake. ChaoWang202007301642
					*/
					if (IN_DOMAIN(drainage[drow][dcol].dr_row, drainage[drow][dcol].dr_col))
						drainage[drainage[drow][dcol].dr_row][drainage[drow][dcol].dr_col].discharge += drainage[row][col].discharge;
					else {
						/*Do nothing, border outlet transfer will be calculated below*/
					}
					break;
				case 'R':
					if (drow==row && dcol==col) {
						PRINT_ERROR("\aI should never write this!.");
						*total_evap_water += drainage[row][col].discharge;
					}
					else
						drainage[drow][dcol].discharge += drainage[row][col].discharge;
					break;
				case 'E':
					/*Lake internal drainage is done above*/
					if (il != drainage[drow][dcol].lake)
						drainage[drow][dcol].discharge += drainage[row][col].discharge;
					break;
				default:
					PRINT_ERROR("[%d][%d] draining to [%d][%d] has missing drainage type.", row, col, drow, dcol);
					break;
				}
			}
			else {
				if (AT_BORDER(row,col)) {
					/*Transfers out of model.*/
					*total_lost_water += drainage[row][col].discharge;
				}
				else {
					/*Evaporates water from endorheic lake nodes*/
					*total_evap_water += drainage[row][col].discharge;
				}
			}
	}
	
	/*Calculate lake elevation and volume*/
	/*LAKES HAVE CHANGED HERE, IN Calculate_Discharge, BY EVAPORATION!*/
	for (int il=1; il<=nlakes; il++) {
		if (Lake[il].n_sd) {
			if (topo[Lake[il].row_sd[0]][Lake[il].col_sd[0]] < sea_level && AT_BORDER(Lake[il].row_sd[0], Lake[il].col_sd[0])) {
				Lake[il].alt = sea_level;
			}
			else
				Lake[il].alt = topo[Lake[il].row_sd[0]][Lake[il].col_sd[0]];
			}
		else {
			Lake[il].alt = topo[Lake[il].row[Lake[il].n-1]][Lake[il].col[Lake[il].n-1]];
		}
		Lake[il].vol = 0;
		for (int i=0; i<Lake[il].n; i++)  Lake[il].vol +=  (Lake[il].alt - topo[Lake[il].row[i]][Lake[il].col[i]]);
		Lake[il].vol *= dx*dy;
	}

	/*CHECKS:*/
	for (int il=1; il<=nlakes; il++) {
		float diff, lake_evap=0, max_evap=0;
		for (int i=0; i<Lake[il].n; i++) {lake_evap += evaporation[Lake[il].row[i]][Lake[il].col[i]]; max_evap=MAX_2(max_evap,evaporation[Lake[il].row[i]][Lake[il].col[i]]);}
		lake_evap *= dx*dy;
		/*Check: open-lake input discharge should be >= evaporation*surface, except perhaps for the Sea*/
		if (Lake[il].n_sd) {
		if (diff=(lake_evap - Lake_Input_Discharge(il))<0) {
			BOOL its_sea=0;
			IF_LAKE_IS_SEA(il) its_sea=1;
			if (!its_sea) 
			if (diff>total_rain/200) PRINT_WARNING("Lake %d (open; not sea; %d nodes) has less input %.2f m3/s than evap. %.2f m3/s.", il, Lake[il].n, Lake_Input_Discharge(il), lake_evap);
		}
		}
		/*Check: lake input discharge should be ~= evaporation*surface if lake is endorheic*/
		if (!Lake[il].n_sd && (fabs(lake_evap-Lake_Input_Discharge(il)) > 1.1*max_evap*dx*dy || verbose_level>=4)) {
		PRINT_WARNING("Calculate_Discharge: Endorh. lake %d (%d nodes) at [%d][%d] evaporates different %.2f m3/s than inputs %.2f m3/s.", 
				il, Lake[il].n, Lake[il].row[0], Lake[il].col[0], lake_evap, Lake_Input_Discharge(il) );
		}
		/*Check: the lake's registered altitude should be the same as the one of the last node (except in the sea)*/
		if (Lake[il].alt != topo[Lake[il].row[Lake[il].n-1]][Lake[il].col[Lake[il].n-1]]) {
			BOOL its_sea=0;
			IF_LAKE_IS_SEA(il) its_sea=1;
			if (!its_sea) {
				PRINT_ERROR("\aLake %d (not sea) should have the altitude of lake's last node %.2f m instead of %.2f m.", il, topo[Lake[il].row[Lake[il].n-1]][Lake[il].col[Lake[il].n-1]], Lake[il].alt);
			for (int i=0; i<Lake[il].n; i++) PRINT_WARNING("[%d][%d] elevation: %.2f", Lake[il].row[i], Lake[il].col[i], topo[Lake[il].row[i]][Lake[il].col[i]]);
			}
		}
		/*Check: the lake's registered elevation should be the maximum among lake nodes (except for the sea)*/
		{
		float max_elev=-1e9;
		for (int i=0; i<Lake[il].n; i++) max_elev = MAX_2(topo[Lake[il].row[i]][Lake[il].col[i]], max_elev);
		if (Lake[il].alt != max_elev) {
			BOOL its_sea=0;
			IF_LAKE_IS_SEA(il) its_sea=1;
			/*BUG: This check failed at Jenna West mac, giving repeated errors without apparent cause 2016-09-01*/
			if (!its_sea)
			PRINT_ERROR("Lake %d (not sea) should have the elevation of its highest node %.2f m instead of %.2f m.", il, max_elev, Lake[il].alt);
		}
		/*Check: all outlets should have the same elevation (except for the sea)*/
		for (int i=1; i<Lake[il].n_sd; i++) {
			if (topo[Lake[il].row_sd[i-1]][Lake[il].col_sd[i-1]] != topo[Lake[il].row_sd[i]][Lake[il].col_sd[i]]) {
				BOOL its_sea=0;
				IF_LAKE_IS_SEA(il) its_sea=1;
				if (!its_sea)
				PRINT_ERROR("\aLake %d (open but not sea) should have all saddles at same elevation but has %.2f m at [%d][%d] instead of %.2f m.", il, topo[Lake[il].row_sd[i-1]][Lake[il].col_sd[i-1]], Lake[il].row_sd[i-1], Lake[il].col_sd[i-1], topo[Lake[il].row_sd[i]][Lake[il].col_sd[i]]);
			}
		}
		}
	}

	return (1);
}




int constant_rate_eros (
	float **topo, float Keroseol, float Ksedim, float sea_level, int water_load, 
	float dt, float Time) 
{
	float Dh;

	/*
	  Adds background erosion and sea sedimentation
	*/

	if (!erosed_model) return (0);

	/*Calculate eros/sed*/
	for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
		/*EROSION proportional to elevation*/
		if (topo[i][j] > sea_level) {
			Dh = Keroseol * dt * (topo[i][j]-sea_level);
			Erode (THICK2SEDMASS(Dh), i, j);
		}
		/*SEDIMENTATION*/
		else if (water_load) {
			Dh = MIN_2(Ksedim*dt, sea_level-topo[i][j]);
				Sediment (THICK2SEDMASS(Dh), i, j);
		}
	}

}




int Define_Drainage_Net (struct GRIDNODE *sortcell) 
{
	/*
	Here the drainage net is defined by classifying the nodes in the domain 
	in 3 types:
	'R' river;  'L' lake/sea;  'E' lake/sea outlet/exit;

	This routine will define the lakes (these include topographic minima,
	flats,  i.e., lakes themself, sea and plains) at every time
	substep  of the fluvial erosion. The topography is asumed to be
	perfectly  sorted in 'sortcell'. 
	The seas are distinguished from proper lakes only because they are 
	the only lakes having outlets in the boundary and below sea level.
	Lakes occupy topographic minima and (before evaporation is calculated)
	always have an outlet where water exits. Sea is defined as one or more
	lakes including at least one node below sea level in the boundary. 
	This connection with the boundary is an exit/outlet. These outlets 
	below sea level, allow to differenciate sea-lakes from the rest of 
	normal lakes. Note that if a lake has no connection with the boundary 
	then it will either be a closed lake with no outlet or it will fill 
	above sea-level, looking for an outlet. 
	(depending on if ET> or < uphill discharge into the lake)

	The following algorithm is applied to every node, starting from the 
	lowest in ascending order. Initially nothing is known about the
	nodes,  therefore in the algorithm nothing is known about the nodes
	above the  current one. 
	I refer to a lake as 'defined' when an outlet for the overflowing has 
	already been found and so, the limits of the lake are well known.  I
	use 'adjacent' to refer to any of the 8 possible surrounding nodes of 
	the current node.
	I use the word 'outlet' for any point of the lake that transfers out of
	the  lake. This can be either a saddle of the topography or a node in
	the border below sea level (sea outlet), or the limit of a plain that 
	has a lower adjacent.

	BEFORE (for the nodes with topo <= sea_level): these nodes are treated 
		as lake nodes. They can form different lakes. 
		
	LOOP (for every node with a topo > sea_level):
	1. if has no undefined-lake adjacents
		1.1 if < than all the adjacents and it is not in the border, 
			then mark it as a new undefined lake.
		1.2 if = than at least one of the adjacents
			1.2.1 if > than one adjacent, then mark the node as a new 
				undefined lake and mark it as the outlet.
		1.2.2 if <= than all adjacents, then mark the node as a new 
			undefined lake.
	2. if has undefined lake adjacents 
		2.1. if <= than all the adjacents that are not undefined 
		lakes, then mark it as the adjacent undefined lake.
		2.2. if > than at least one adjacent that is not undefined 
			lake, then mark it as the adjacent undefined lake, mark it as 
			an outlet and mark the minimum adjacent that is not undefined 
			lake as the node to transfer. 
	3. if it's a lake labelled different than an undefined lake 
	  	adjacent, then unify both lakes adding the outlets. If one of 
	  	the lakes is defined then the resulting lake is too.
	4. if it's a lake in the border and it has not been defined as an outlet, 
		then mark it as an outlet.
	5. if the next node in the sortcell array is > than the present 
		node, then mark the lakes which outlets are at the present 
		height as defined.

	AFTER (checks): each lake has one or more outlets (all of them with 
		the same height) with a node next to them to which to transfer 
		water and seds. Lakes cannot be formed only with outlets.

	EXAMPLE in 1D assuming no evaporation:
		 22  1111  333 444444555 666666  66   <--lake_number
	 ELRRSSSSRRELLRELLLLLLLLRLLLLLE  LERR <--node_type
	  z	|					   #	  |	 |
	|			  #		##### #| ##  |
	|		  #  ###   #  ########| ### |
	|  ##	 ### #### ############| ####|
	|####	######################| ####|
	z=0	|----#-------------------------| ----|
	|	 ###					  |	 |
	|	   #					  |	 |
		 12120000--2212222122122-122212  22--  \
		 2121	  2111211111211 211111  11	| <-- applied rule
		 2 1				 2   2			 /

	The information about the lakes is stored in two structures: 'drainage' 
	(for every node in the grid) and 'Lake' (for every lake).
	*/


	int ro[NDERS], co[NDERS], total_lake_nodes;

	PRINT_DEBUG("Defining drainage network");

	/*Delete all lakes*/
	for (int il=nlakes; il>0; il--) Deallocate_Lake (il);
	for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
		/*Keep record of lake nodes from last erosion time-step, for evaporation*/
		lake_former_step[i][j]=drainage[i][j].lake; 
		drainage[i][j].lake = 0; 
		drainage[i][j].type = '-';
	}

	/*Create lake #0 wich will contain nothing*/
	Lake = calloc(1, sizeof(struct LAKE_INFO));
	
	
	/*Define lakes ascending in the topo grid.*/
	for (int isort=Nx*Ny-1; isort>=0; isort--) {
		int i, j;
		int undef_lake_adj, n_nonposders=0, n_zeroders=0, n_negders=0, n_posders=0;
		int imaxderneg=SIGNAL, imaxderneg_noundef=SIGNAL, n_negders_nonundef_lake=0; 
		float maxderneg=0, maxderneg_noundef=0;
		BOOL switch_change_in_next_height=NO, switch_outlet=NO;

		i = sortcell[isort].row;  j = sortcell[isort].col;
		ro[0]=i-1, ro[1]=i,   ro[2]=i+1, ro[3]=i,   ro[4]=i-1, ro[5]=i+1, ro[6]=i+1, ro[7]=i-1;
		co[0]=j,   co[1]=j+1, co[2]=j,   co[3]=j-1, co[4]=j+1, co[5]=j+1, co[6]=j-1, co[7]=j-1;

		/*Calculate derivates and look for undefined lakes in all directions: */
/*!!*/		for (int l=undef_lake_adj=0; l<NDERS; l++) {
			float dist, deriv;
			switch (l) {
				case 0: case 2: dist=dy;  break;   /*N,S*/
				case 1: case 3: dist=dx;  break;   /*E,W*/
				default:		dist=dxy; break;   /*others*/
			}
			if (IN_DOMAIN(ro[l],co[l])) {
				deriv = (topo[ro[l]][co[l]]-topo[i][j])/dist;
				if (drainage[ro[l]][co[l]].lake < 0) 
					/*deberia BUSCAR ALEATORIAMENTE!!*/
					undef_lake_adj = drainage[ro[l]][co[l]].lake;
				else
					if (deriv<0 && deriv<maxderneg_noundef) {imaxderneg_noundef=l; maxderneg_noundef=deriv;}
			}
			else {
				deriv = SIGNAL;
			}
			if (deriv != SIGNAL) {
				if (deriv<0 ) n_negders++;
				if (deriv>0 ) n_posders++;
				if (deriv<=0) n_nonposders++;
				if (deriv==0) n_zeroders++;
				if (deriv<0 && drainage[ro[l]][co[l]].lake >= 0) n_negders_nonundef_lake++;
				if (deriv<0 && deriv<maxderneg) {imaxderneg=l; maxderneg=deriv;}
			}
		}

		/*START IDENTIFYING THE NEW NODE.*/
		/*Sea lakes:*/
		if (topo[i][j]<=sea_level) {
			if (undef_lake_adj) {
				Add_Node_To_Lake(i,j, undef_lake_adj);
			}
			else {
				int i_lake;
				Add_Node_To_Lake(i,j, i_lake=New_Lake());
			}
		}
		/*Normal (non-sea) lakes:*/
		else {
			if (undef_lake_adj) {
				/*Add outlet to the undefined lake.*/
				if (n_negders_nonundef_lake) {
					Add_Node_To_Lake(i,j, undef_lake_adj);
					Add_Outlet_To_Lake(i,j,ro[imaxderneg_noundef],co[imaxderneg_noundef], undef_lake_adj);
					switch_outlet=YES;
				}
				/*Add node to the undefined lake.*/
				else {
					Add_Node_To_Lake(i,j, undef_lake_adj);
				}
			}
			else {
				/*Create a new lake if there is no way down/flat/out.*/
				if (!n_nonposders && NOT_AT_BORDER(i,j)) {
					Add_Node_To_Lake(i,j, New_Lake());
				}
				/*Create a new lake if there is a flat.*/
				if (n_zeroders) {
					if (n_negders) {
						int i_lake;
						Add_Node_To_Lake(i,j, i_lake=New_Lake());
						Add_Outlet_To_Lake(i,j, ro[imaxderneg], co[imaxderneg], i_lake);
						switch_outlet=YES;
					}
					else {
						Add_Node_To_Lake(i,j, New_Lake());
					}
				}
			}
		}
		/*Unify connected undefined lakes*/
		if (drainage[i][j].lake<0 && undef_lake_adj) {
			for (int l=0; l<NDERS; l++) {
				if (IN_DOMAIN(ro[l],co[l])) {
					int il = drainage[ro[l]][co[l]].lake;
					if (il<0 && il != drainage[i][j].lake) {
						Unify_Lakes(drainage[i][j].lake, il);
					}
				}
			}
		}
		/*If it's a lake node in the border, then it's an outlet*/
		if (drainage[i][j].lake && AT_BORDER(i,j) && !switch_outlet) {
			if (eros_bound_cond[BORDER_INDEX(i,j)] != 'c') 
				Add_Outlet_To_Lake(i,j,SIGNAL,SIGNAL,drainage[i][j].lake);
		}

		if (topo[i][j]>sea_level) {
			/*
			  If the altitude is going to change in the next node or
			  this is the last node, then mark as defined all the
			  open (exorheic, not endorheic) lakes that have the 
			  present altitude.
			*/
			if (isort>0)  if (topo[i][j] != topo[sortcell[isort-1].row][sortcell[isort-1].col]) 
				switch_change_in_next_height=YES;
			if (switch_change_in_next_height || isort==0) {
				for (int l=1; l<=nlakes; l++) {
					if (Lake[l].n_sd)
						if (topo[Lake[l].row_sd[0]][Lake[l].col_sd[0]] == topo[i][j]) {
							Define_Lake(l);
						}
				}
			}
		}
		else {
			/*
			  If this isort is the last under-sea-level node, or simply the
			  last (upper most) node, then define all 'sea-like' lakes.
			*/
			if (isort>0) {
				if (topo[sortcell[isort-1].row][sortcell[isort-1].col] > sea_level) {
				for (int l=1; l<=nlakes; l++) {
					if (Lake[l].n_sd) {
						Define_Lake(l);
					}
				}
				}
			}
			else {
				for (int l=1; l<=nlakes; l++) {
					if (Lake[l].n_sd) {
						Define_Lake(l);
					}
				}
			}
		}
		/*
		  Determines drainage of the non-lake (river) nodes.
		  Add transferring and other information to 'drainage'.
		*/
		if (!drainage[i][j].lake) {
			drainage[i][j].type = 'R';
			if (imaxderneg != SIGNAL) {
					/*Drain to the lowest neighbour*/
				drainage[i][j].dr_row = ro[imaxderneg];
				drainage[i][j].dr_col = co[imaxderneg];
			}
			else {
				drainage[i][j].dr_row = SIGNAL;
				drainage[i][j].dr_col = SIGNAL;
			}
		}
	}

	/*
	  Delete all lakes which nodes are all of them outlets. 
	  The outlet drainage is not mantained because in the borders does not work.
	*/
	for (int l=1; l<=nlakes; l++) {
		if (Lake[l].n == Lake[l].n_sd) {
			for (int m=0; m<Lake[l].n; m++) {
				/*
				  Determines drainage of the new non-lake (river) nodes.
				  Add transferring and other information to 'drainage'.
				*/
			/*!!next line unnecessary, since they are outlets with defined drainage*/
				Minimum_Neg_Slope (Lake[l].row[m], Lake[l].col[m],  &drainage[Lake[l].row[m]][Lake[l].col[m]].dr_row, &drainage[Lake[l].row[m]][Lake[l].col[m]].dr_col);
				drainage[Lake[l].row[m]][Lake[l].col[m]].lake = 0;
				drainage[Lake[l].row[m]][Lake[l].col[m]].type = 'R';
			}
			Deallocate_Lake(l);
			l--;
		}
	}

	/*
	  Determines in which case are the lake nodes: 
	  	'L' lake;  'E' outlet/exit of lake;
	  Add transferring and other information to 'drainage'.
	*/
	for (int l=1; l<=nlakes; l++) {
		register float 	dist2, mindist2, distx, disty, dxdivdy=dx/dy;
		register int 	imindist2=-1, lrow, lcol;
		/*Outlets*/
		for (int m=0; m<Lake[l].n_sd; m++) {
			lrow = Lake[l].row_sd[m];
			lcol = Lake[l].col_sd[m];
			drainage[lrow][lcol].type = 'E';
			/*All outlets should be 'defined' as lake node in 'drainage'*/
			if (drainage[lrow][lcol].lake != l) 
				PRINT_ERROR("[%d][%d] outlet 'Lake' %d and 'drainage' %d do not match.", lrow, lcol, l, drainage[lrow][lcol].lake);
			if (drainage[lrow][lcol].lake < 0) 
				PRINT_ERROR("[%d][%d] outlet was supposed to be a defined lake node.", lrow, lcol);
		}
		/*Lakes*/
		for (int m=0; m<Lake[l].n; m++) {
			float mindist2=1e24;
			lrow = Lake[l].row[m];
			lcol = Lake[l].col[m];
			/*All lakes should be 'defined' in 'drainage'*/
			if (drainage[lrow][lcol].lake < 0) {
				if (Lake[l].n_sd) PRINT_ERROR("[%d][%d] of lake %d (%d nodes) needed to be defined late.", lrow, lcol, l, Lake[(int) fabs((float) l)].n);
			}
			if (drainage[lrow][lcol].lake != l) 
				PRINT_ERROR("[%d][%d] of lake %d (%dth node of %d) and 'drainage' %d do not match in node [%d][%d].", lrow, lcol, l, m, Lake[l].n, drainage[lrow][lcol].lake, lrow, lcol);
			/*Lake nodes drain to the nearest outlet:*/
			for (int n=0; n<Lake[l].n_sd; n++) {
				distx = dxdivdy*(Lake[l].col_sd[n]-lcol);
				disty = (Lake[l].row_sd[n]-lrow);
				dist2 = distx*distx + disty*disty;
				if (dist2 < mindist2) {imindist2=n; mindist2=dist2;}
			}
			if (Lake[l].n_sd && drainage[lrow][lcol].type != 'E') {
				drainage[lrow][lcol].dr_row = Lake[l].row_sd[imindist2];
				drainage[lrow][lcol].dr_col = Lake[l].col_sd[imindist2];
				drainage[lrow][lcol].type = 'L';
				if (!drainage[lrow][lcol].lake) PRINT_ERROR("[%d][%d] should have an assigned lake.", lrow, lcol);
			}
			if (!Lake[l].n_sd) PRINT_ERROR("Lake %d has no exit before evaporation.", l);
		}
	}

	/*For the same altitude, put the outlets first in sortcell*/
	for (int isort=0; isort<Nx*Ny-1; isort++) {
		if (drainage[sortcell[isort].row][sortcell[isort].col].type != 'E') {
		float topoisort;
		topoisort = topo[sortcell[isort].row][sortcell[isort].col];
		for (int j=isort+1; j<Nx*Ny; j++) {
			float topoj;
			topoj = topo[sortcell[j].row][sortcell[j].col];
			if (topoj == topoisort) {
			if (drainage[sortcell[j].row][sortcell[j].col].type == 'E') {
					int auxrow, auxcol;
				auxrow=sortcell[j].row;
				auxcol=sortcell[j].col;
				sortcell[j].row=sortcell[isort].row;
				sortcell[j].col=sortcell[isort].col;
				sortcell[isort].row=auxrow;
				sortcell[isort].col=auxcol;
				break;
				}
			}
			else break;
		}
		}
	}

	/*CHECK RESULTS*/
	/*Check: Lake nodes 'defined' in 'drainage' should be as many as the total lake nodes.*/
	{
	int total_lake_nodes=0;
	for (int il=1; il<=nlakes; il++) total_lake_nodes += Lake[il].n;
	{
		int k=0;
		for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) if (drainage[i][j].lake) k++;
		if (k != total_lake_nodes) 
			PRINT_ERROR("%d lake nodes were expected rather than %d.", total_lake_nodes, k);
	}
	}
	/*Check: All lake nodes should be 'defined'*/
	for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
		if (drainage[i][j].lake<0) PRINT_ERROR("[%d][%d] belongs to undefined lake %d.", i, j, drainage[i][j].lake);
		if (drainage[i][j].lake) {
			if (Lake_Node_Number(i,j)<0) PRINT_ERROR("[%d][%d] should be in Lake %d structure. %d ", i, j, drainage[i][j].lake, Lake_Node_Number(i,j));
		}
	}
	/*Check: All outlets of a lake have the same height, except for the sea*/
	for (int il=1; il<=nlakes; il++) {
		float  sd_height;
		BOOL its_sea=0;
		IF_LAKE_IS_SEA(il) its_sea=1;
		if (!its_sea) {
		for (int m=1; m<Lake[il].n_sd; m++) {
			sd_height = topo[Lake[il].row_sd[m-1]][Lake[il].col_sd[m-1]];
			if (sd_height!=topo[Lake[il].row_sd[m]][Lake[il].col_sd[m]])
			PRINT_ERROR("lake %d has outlets %d and %d with different height.", il, m-1, m);
		}
		}
	}
	/*Check: Nodes of a lake cannot be repeated. Outlets of a lake cannot be repeated.*/
	/*Check: Drainage direction and lake transferring info should match.*/
	/*Check: Drainage type and lake info should match.*/
	for (int il=1; il<=nlakes; il++) {
		for (int m=0; m<Lake[il].n_sd; m++) {
			int i=Lake[il].row_sd[m], j=Lake[il].col_sd[m];
			if (drainage[i][j].type != 'E') 
				PRINT_ERROR("[%d][%d] should have type 'E' (outlet).", i, j);
		}
	}
	/*Check: All nodes transfer either to SIGNAL,SIGNAL or to a real node.*/
	for (int i=0; i<Ny; i++)  for (int j=0; j<Nx; j++) {
		if (drainage[i][j].dr_row < 0 && drainage[i][j].dr_row != SIGNAL || 
			drainage[i][j].dr_col < 0 && drainage[i][j].dr_col != SIGNAL) 
			PRINT_ERROR("[%d][%d] -->>  [%d][%d] \t'%c'", i,j,drainage[i][j].dr_row,drainage[i][j].dr_col, drainage[i][j].type);
	}
	/*Check: all outlets should have the same elevation (except for the sea)*/
	for (int il=1; il<=nlakes; il++) {
		for (int i=1; i<Lake[il].n_sd; i++) {
			if (topo[Lake[il].row_sd[i-1]][Lake[il].col_sd[i-1]] != topo[Lake[il].row_sd[i]][Lake[il].col_sd[i]]) {
				BOOL its_sea=0;
				IF_LAKE_IS_SEA(il) its_sea=1;
				if (!its_sea)
					PRINT_ERROR("\aLake %d (open but not sea) should have all saddles at same elevation but has %.1f m at [%d][%d] instead of %.1f m.", il, topo[Lake[il].row_sd[i-1]][Lake[il].col_sd[i-1]], Lake[il].row_sd[i-1], Lake[il].col_sd[i-1], topo[Lake[il].row_sd[i]][Lake[il].col_sd[i]]);
			}
		}
	}
	/*Check: all outlets should have the maximum elevation among lake nodes (except for the sea)*/
	for (int il=1; il<=nlakes; il++) {
		float max_elev=-1e9;
		for (int i=0; i<Lake[il].n; i++) max_elev = MAX_2(topo[Lake[il].row[i]][Lake[il].col[i]], max_elev);
		for (int i=0; i<Lake[il].n_sd; i++) {
			if (topo[Lake[il].row_sd[i]][Lake[il].col_sd[i]] < max_elev) {
				BOOL its_sea=0;
				IF_LAKE_IS_SEA(il) its_sea=1;
				if (!its_sea)
					PRINT_ERROR("\aLake %d (open but not sea) should have all saddles higher than all other nodes but %.1f m at [%d][%d] is lower than %.1f m.", il, topo[Lake[il].row_sd[i]][Lake[il].col_sd[i]], Lake[il].row_sd[i], Lake[il].col_sd[i], max_elev);
			}
		}
	}

	return(1);
}




int Diffusive_Eros (float Kerosdif, float dt, float dt_eros)
{
	float	**Dheros;
	int 	n_iters;
	
	/*
	  COMPUTES THE LOAD, HEIGHT & Block-THICKNESS CHANGES DUE TO 
	  SURFACE MASS DIFFUSION. This process approaches short scale 
	  transport processes.
	  Resolutions of 100x100 need dt_eros<=.01  My to converge.
	  Resolutions of 200x200 need dt_eros<=.005 My to converge.
	*/

	if (!erosed_model || !Kerosdif) return (0);
	
	PRINT_DEBUG("Calculating diffusive transport");

	calculate_topo(topo);

	Dheros = alloc_matrix(Ny, Nx);

	n_iters = MAX_2(floor(dt/dt_eros+.5), 1); 
	PRINT_INFO("n_iters=%3d", n_iters);

	for (int k=0; k<n_iters; k++) {
	 		 diffusion_2D(topo, Dheros, Nx, Ny, Kerosdif, dx, dy, dt/n_iters);

	 		 /*Adds results to the height and the next load Dq and removes material from the Blocks*/
	 		 for(int i=0; i<Ny; i++)  for(int j=0; j<Nx; j++)  {
	 			 if (Dheros[i][j]>0) {
	 				 Sediment ( Dheros[i][j]*dx*dy*denscrust, i, j);
	 			 }
	 			 if (Dheros[i][j]<0) {
	 				 Erode	(-Dheros[i][j]*dx*dy*denscrust, i, j);
	 			 }
	 		 }
	}

	free_matrix(Dheros, Ny);
	return (1);
}




int Fluvial_Transport(struct GRIDNODE *sortcell, float dt_st, int erosed_model, float *total_lost_sed_mass, int lake_instant_fill)
{
	/*
	  THIS ROUTINE COMPUTES THE TOPOGRAPHY AND LOAD VARIATIONS DUE
	  TO FLUVIAL EROSION/TRANSPORT/SEDIMENTATION.
	  The model assumes that the large scale erosion/sedimentation
	  in a point depends on the water volume crossing the cell
	  and the slope in that point.
	  >Input water comes only from the rain falling at each
	  cell. Water is transferred to the lowest sorrounding cell.
	  Water losts occur when streams reach the model boundary,
	  when they reach the sea and during the transfer from one to
	  the next cell.
	  >Eroded material is transported with water. Mass conservs.
	  Mass losts only occur at the domain boundaries.
	  >The criteria used from erosion/sedimentation is as follows:
	  Continent:
	  	  Sediments capacity proportional to slope and discharge.
	  	  eroded/sedimented mass proportional to
	  		  (capacity-tranported_sediment)*distance
	  	  Lake: Sedimentation of all the available material
	  		  until lake overflow.
	  Sea:
	  	  Sedimentation as in continent with discharge=0
	  	  Bathymetric minima as in continental lakes.
	*/

	int 	row, col, drow, dcol, co[NDERS], ro[NDERS];
	float	d_mass; 	/*Increment of suspended mass in this cell (positive means erosion).*/

	PRINT_DEBUG("Calculating fluvial_transport");
	/*
	  This bucle starts from the top point and
	  descends transferring the eroded mass
	*/
	if (erosed_model>=2) for (int isort=0; isort < Nx*Ny; isort++) {
		float minsorr_trib, maxsorr, minsorr, main_tribut_slope, main_tribut_disch, main_tribut_alt;
		row = sortcell[isort].row;  col = sortcell[isort].col;
		ro[0]=row-1, ro[1]=row,   ro[2]=row+1, ro[3]=row,	ro[4]=row-1, ro[5]=row+1, ro[6]=row+1, ro[7]=row-1;
		co[0]=col,   co[1]=col+1, co[2]=col,   co[3]=col-1, co[4]=col+1, co[5]=col+1, co[6]=col-1, co[7]=col-1;
		drow = drainage[row][col].dr_row;
		dcol = drainage[row][col].dr_col;

		/*Calculate max and min height in the 8 sorrounding points:*/
		maxsorr=minsorr=topo[row][col];
		for(int i=0; i<NDERS; i++) {
				if (IN_DOMAIN(ro[i],co[i])) {
					minsorr = MIN_2(minsorr, topo[ro[i]][co[i]]);
					maxsorr = MAX_2(maxsorr, topo[ro[i]][co[i]]);
				}
		}
		/*
		  Finds the height of the minimum sorrounding contributor
		  and the slope of the main contributor
		*/
		minsorr_trib=maxsorr;
		main_tribut_disch=0; main_tribut_slope=0; main_tribut_alt=0;
		for(int i=0; i<NDERS; i++) {
				if (IN_DOMAIN(ro[i],co[i])) {
					if (drainage[ro[i]][co[i]].dr_row == row && drainage[ro[i]][co[i]].dr_col == col) {
						if (drainage[ro[i]][co[i]].discharge>main_tribut_disch) {
							float dist;
							switch (i) {
								case 0: case 2: dist=dy;  break;   /*N,S*/
								case 1: case 3: dist=dx;  break;   /*E,W*/
								default:		dist=dxy; break;   /*others*/
							}
							main_tribut_disch = drainage[ro[i]][co[i]].discharge;
							main_tribut_slope = (topo[row][col]-topo[ro[i]][co[i]])/dist;
							main_tribut_alt   = topo[ro[i]][co[i]];
						}
						minsorr_trib = MIN_2(minsorr_trib, topo[ro[i]][co[i]]);
					}
				}
		}

		/*
		  Calculates the potential sediment mass increment in this cell d_mass (kg) due to 
		  river erosion in this cell (erosion or sedimentation): 
		*/
		switch (drainage[row][col].type) {
			float dist, slope, transp_capacity_eq;
			case 'L':
				/*Sedimentation will be done in Lake_Fill().*/
				d_mass  = 0;
				break;
			case 'R':
			case 'E':
				/*Calculate distance to the output node*/
				if (IN_DOMAIN(drow,dcol)) {
					int ild = drainage[drow][dcol].lake;
					dist = sqrt(dy*(drow-row)*dy*(drow-row) + dx*(dcol-col)*dx*(dcol-col));
					if (ild) slope = - (Lake[ild].alt	- topo[row][col]) / dist;
					else	 slope = - (topo[drow][dcol] - topo[row][col]) / dist;
		   		}
				else {
					dist = dxy;
					slope = main_tribut_slope;
				}

				switch (erosed_model) {
#define ERODED_ERODIBILITY   /*Takes a mean erodibility*/ float depth2average=10., dh, weight, totalweight=0, basedepth=0, erodibility_aux=0;\
					for (int i=numBlocks-1; i>=0; i--) {\
						basedepth+=Blocks[i].thick[row][col];\
						basedepth=MIN_2(basedepth,depth2average+.1);\
						weight=Blocks[i].thick[row][col]/(basedepth+1.); totalweight+=weight;\
						erodibility_aux+=weight*Blocks[i].erodibility; \
						/*PRINT_ERROR("xxx %.2e %.2e %.2e %.2e", Blocks[i].erodibility, erodibility_aux, basedepth, weight);*/\
						if (basedepth>=depth2average) break;\
					};\
					if (basedepth<=depth2average) {\
						weight=(depth2average-basedepth)/(depth2average+1); totalweight+=weight;\
						erodibility_aux+=weight*erodibility;\
						/*PRINT_ERROR("XXX %.2e %.2e %.2e %.2e", erodibility, erodibility_aux, basedepth, weight);*/\
					}\
					if (totalweight) erodibility_aux/=totalweight; if (!erodibility_aux) erodibility_aux=erodibility;\
					/*PRINT_ERROR("XXX erodibil: %.2e", erodibility_aux);*/
#define TRANSPORT_BOUNDARY_CONDITIONS \
					if (AT_BORDER(row,col)) {\
						switch (eros_bound_cond[BORDER_INDEX(row,col)]) {\
						case '1':\
						case '2':\
						case 'c':					break;\
						case '0':   transp_capacity_eq = 0;		break;\
						case '3':   transp_capacity_eq /= 2;		break;\
					}}
				  case 2:
					/*Beaumont et al. (1992) stream power law ('uncercapacity'):*/
					/*Transport capacity in equilibrium [kg/s]. Whipple & Tucker, 2002 conclude m'=n'=1*/
					transp_capacity_eq = K_river_cap * drainage[row][col].discharge * slope;	/*Eq. 16 of Tucker&Slingerland, 1996*/
					TRANSPORT_BOUNDARY_CONDITIONS;
					/*EROSION*/
					if (transp_capacity_eq >= drainage[row][col].masstr) {
						ERODED_ERODIBILITY;
						/*!!dxy instead of dist does not help to promote non-diagonal drainage (along x,y) (see cone_postectonic)*/
						d_mass  =  dist / erodibility_aux * (transp_capacity_eq - drainage[row][col].masstr) * dt_st;
					}
					/*SEDIMENTATION*/
					else {
						d_mass  =  dist / l_fluv_sedim   *  (transp_capacity_eq - drainage[row][col].masstr) * dt_st;
					}
					break;
				  case 3:
					/*Tucker & Slingerland (1996) hybrid stream power:*/
					transp_capacity_eq = K_river_cap * drainage[row][col].discharge * slope;	/*Eq. 16 of Tucker&Slingerland, 1996*/
					TRANSPORT_BOUNDARY_CONDITIONS;
					if (transp_capacity_eq >= drainage[row][col].masstr) {
						spl_m = 1/3;
						spl_n = 2/3;
						ERODED_ERODIBILITY;
						/*bedrock channel incision*/
						dh = erodibility_aux		/*Eq. 11 of T&S*/
							* pow((double)drainage[row][col].discharge, (double)spl_m)
							* pow((double)slope,			(double)spl_n)
							* dt_st;
						d_mass = THICK2SEDMASS(dh);
					}
					else{
						/*alluvial channel aggradation: sediment the excess*/
						d_mass =					/*Eqs. 18 & 10 of T&S*/
							(transp_capacity_eq - drainage[row][col].masstr)
							* dt_st;
					}
					break;
					  case 4:
					/*Modified stream power used by Davy's group (see Loget et al., 2006, Gibraltar), similar to Beaumont's and Kooi's:*/
					/*Transport capacity in equilibrium [kg/s].*/
					transp_capacity_eq = K_river_cap * pow(drainage[row][col].discharge, 1.5) * slope;
					TRANSPORT_BOUNDARY_CONDITIONS;
					/*EROSION*/
					if (transp_capacity_eq >= drainage[row][col].masstr) {
						ERODED_ERODIBILITY;
						d_mass  =  dist / erodibility_aux * (transp_capacity_eq - drainage[row][col].masstr) * dt_st;
					}
					/*SEDIMENTATION*/
					else {
						d_mass  =  dist / l_fluv_sedim   *  (transp_capacity_eq - drainage[row][col].masstr) * dt_st;
					}
					break;
				  case 5:
					/*Undercapacity of Beaumont incorporating width by van der Beek & Bishop, 2003 (described in Cowie et al., 2006), modifies Beaumont's*/
					/*Transport capacity in equilibrium [kg/s].*/
					transp_capacity_eq = K_river_cap * drainage[row][col].discharge * slope;		/*Eq. 16 of Tucker&Slingerland, 1996*/
					TRANSPORT_BOUNDARY_CONDITIONS;
					/*EROSION*/
					if (transp_capacity_eq >= drainage[row][col].masstr) {
						ERODED_ERODIBILITY;
						d_mass = dist / erodibility_aux / pow(drainage[row][col].discharge, .5) * (transp_capacity_eq - drainage[row][col].masstr) * dt_st;
					}
					/*SEDIMENTATION*/
					else {
						d_mass  =  dist / l_fluv_sedim * (transp_capacity_eq - drainage[row][col].masstr) * dt_st;
					}
					break;
				  case 6:
					/*Garcia-Castellanos & Villasenor (2011, Nature) basal shear stress approach:*/
					transp_capacity_eq = K_river_cap * drainage[row][col].discharge * slope;		/*Eq. 16 of Tucker&Slingerland, 1996*/
					TRANSPORT_BOUNDARY_CONDITIONS;
					if (transp_capacity_eq >= drainage[row][col].masstr) {
					float a=1.5, Kw=1.1, aw=0.5;
					spl_m = 3*a*(1-aw)/5;
					spl_n = 7*a/10;
						/*bedrock channel incision*/
					ERODED_ERODIBILITY;
						dh = erodibility_aux/secsperyr * pow(1020*g, a)
							* pow((double).05/Kw, (double) 3*a/5)
						* pow((double)drainage[row][col].discharge, (double)spl_m) 
						* pow((double)slope,			(double)spl_n)
							* dt_st;
					if (transp_capacity_eq) dh *= (transp_capacity_eq-drainage[row][col].masstr)/transp_capacity_eq;
						d_mass = THICK2SEDMASS(dh);
					}
					else{
						/*alluvial channel aggradation: sediment the excess*/
						d_mass = 
							dist / l_fluv_sedim * (transp_capacity_eq - drainage[row][col].masstr)
							* dt_st;
					}
					break;
				  case 7:
					/*Ferrier et al. (2013, Nature) unit stream power approach:*/
					transp_capacity_eq = K_river_cap * drainage[row][col].discharge * slope;		/*Eq. 16 of Tucker&Slingerland, 1996*/
					TRANSPORT_BOUNDARY_CONDITIONS;
					if (transp_capacity_eq >= drainage[row][col].masstr) {
						float a=1, Kw=1.1, aw=0.5;
						spl_m = (1-aw);
						spl_n = a;
						/*bedrock channel incision*/
						ERODED_ERODIBILITY;
						dh = erodibility_aux/secsperyr * pow(1020*g, a) / Kw 
							* pow((double)drainage[row][col].discharge, (double)spl_m) 
							* pow((double)slope, 			(double)spl_n) 
							* dt_st;
						if (transp_capacity_eq) dh *= (transp_capacity_eq-drainage[row][col].masstr)/transp_capacity_eq;
							d_mass = THICK2SEDMASS(dh);
					}
					else{
						/*alluvial channel aggradation: sediment the excess*/
						d_mass = 
							dist / l_fluv_sedim * (transp_capacity_eq - drainage[row][col].masstr)
							* dt_st;
					}
					break;
				  case 8:
					/*Berry et al. 2019 (in revision) basal shear stress similar to case 6, but with sediment scaling curve*/
					transp_capacity_eq = K_river_cap * drainage[row][col].discharge * slope; /*Eq. 16 of Tucker&Slingerland, 1996*/
	                TRANSPORT_BOUNDARY_CONDITIONS;
	                if (transp_capacity_eq >= drainage[row][col].masstr) {
		                float a=1.5, Kw=1.1, aw=0.5;
		                                
						/*bedrock channel incision, same as #6 with tau_c (critical shear stress) and pulled exp out of individual powers*/
						ERODED_ERODIBILITY;
						double in_eqn = 1020 * g * pow((double).05/Kw,(double) 3/5) *
							pow((double)drainage[row][col].discharge, (double)(3*(1-aw))/5) *
							pow((double)slope, (double)7/10) - (double) tau_c ;

						dh = erodibility_aux/secsperyr * pow((double) in_eqn, a) * dt_st;
						/*sediment scaling curve*/

						/*Sklar & Dietrich, 1994; Whipple & Tucker, 2002; Equation 11 in Berry et al., 2019*/
						if (transp_capacity_eq) 
							dh *= (double)(0.64)*
								(-4*pow(((double)drainage[row][col].masstr/(double)transp_capacity_eq),2)
								+3*((double)drainage[row][col].masstr/(double)transp_capacity_eq)+1);
		                d_mass = THICK2SEDMASS(dh);
					}
	                else{
	                	/*alluvial channel aggradation: sediment the excess*/
	                	d_mass = 
	        				dist / l_fluv_sedim * (transp_capacity_eq - drainage[row][col].masstr)
	                           * dt_st;
	                }
	                break;
       			  default:
       			  	break;
				}
				break;
			default:
			PRINT_ERROR("[%d][%d] has no defined drainage type.", row, col);
		}
		
		if (AT_BORDER(row,col)) if (eros_bound_cond[BORDER_INDEX(row,col)]=='2') d_mass = 0;
		
		/*Limit d_mass with the sorrounding topo*/
		switch (drainage[row][col].type) {
		  float Dhsed;
		  case 'L':
				/*Sedimentation will be done in Lake_Fill().*/
				d_mass = 0;
				break;
		  case 'R':
				/*RIVER EROSION/SEDIMENTATION*/
				Dhsed = -MASS2SEDTHICK(d_mass);
				/*Sedimentation*/			/*minsorr_trib??  main_tribut_alt??*/
				if (Dhsed>0) Dhsed = MIN_2(Dhsed, MAX_2(minsorr_trib - topo[row][col]-1, 0));
				/*Erosion*/
				if (Dhsed<0) Dhsed = MAX_2(Dhsed, MIN_2(minsorr	 - topo[row][col]+1, 0));
				/*limit*/
				d_mass = -THICK2SEDMASS(Dhsed);
				break;
		  case 'E':
				/*OUTLET RIVER-LIKE EROSION*/
			/*If not the sea*/
				if (topo[row][col]>sea_level || !AT_BORDER(row,col)) {
					Dhsed = -MASS2SEDTHICK(d_mass);
					if (Dhsed>0./*meters*/) {
					if (Dhsed/dt_st*secsperMa/1e3>.5) PRINT_DEBUG("[%d][%d] (topo=%.1f) outlet of lake (%d nodes) but deposits %.1f mm/yr.", row, col, topo[row][col], Lake[drainage[row][col].lake].n, Dhsed/dt_st*secsperMa/1e3); 
				Dhsed=0; /*!!*/
				}
					if (Dhsed<=0) Dhsed = MAX_2(Dhsed, MIN_2(minsorr-topo[row][col]+1, 0));
					d_mass = -THICK2SEDMASS(Dhsed);
				}
				/*Else, sea outlets (border nodes below sea_level) are already sedimented in Lake_Fill*/
				else
					d_mass = 0;
				break;
		  default:
				PRINT_ERROR("[%d][%d] has no defined drainage type.", row, col);
		}

		/*Adds results to the topo and the next load Dq and removes/adds material to the Blocks*/
		if (d_mass<0) {
				/*SEDIMENTATION, limit d_mass with the masstr in this node*/
				d_mass = MAX_2(d_mass, -drainage[row][col].masstr*dt_st);
				Sediment (-d_mass, row, col);
		}
		if (d_mass>0) {
				/*EROSION.*/
				Erode	( d_mass, row, col);
		}

		/*Adds the mass increment to the transferring mass contained in this cell: */
		drainage[row][col].masstr += d_mass/dt_st;
		/*Transfers suspended solid mass.*/
		if (IN_DOMAIN(drow,dcol)) {
		int ild = drainage[drow][dcol].lake;
		float hl;
		switch (drainage[drow][dcol].type) {
			case 'L':
			drainage[drow][dcol].masstr += drainage[row][col].masstr;
			/*If draining to an OPEN lake:*/
			if (Lake[ild].n_sd) {
				float diff; 
				/*Check: this can happen when a node transferring to a lake is eroded below the lake level or when the lake node was already deposited and became higher than the tributary node. See Lake_Fill at Dhsed=...*/
				if (IN_DOMAIN(drainage[drow][dcol].dr_row, drainage[drow][dcol].dr_col))  if (diff=(topo[drainage[drow][dcol].dr_row][drainage[drow][dcol].dr_col] - topo[row][col]) > 0)
					if (fabs(diff)>2 || verbose_level>=3) PRINT_ERROR("[%d][%d] transferring mass to lake in [%d][%d] is < than outlet [%d][%d] by %.1f m.", row, col, drow, dcol, drainage[drow][dcol].dr_row, drainage[drow][dcol].dr_col, diff);
				/*
				hl = topo[Lake[ild].row_sd[0]][Lake[ild].col_sd[0]] + 1;
				if (topo[Lake[ild].row_sd[0]][Lake[ild].col_sd[0]] < sea_level && AT_BORDER(Lake[ild].row_sd[0], Lake[ild].col_sd[0]))
					hl = sea_level;
				*/
				hl = topo[row][col];  /*MIN_2 (Lake[ild].alt+1., topo[row][col]-1.);??*/
				Lake_Fill (Lake, drow, dcol, hl, dt_st, lake_instant_fill);
				if (IN_DOMAIN(drainage[drow][dcol].dr_row, drainage[drow][dcol].dr_col))
						/*Next line commented in tAo!!*/
					drainage[drainage[drow][dcol].dr_row][drainage[drow][dcol].dr_col].masstr += drainage[drow][dcol].masstr;
				else
					*total_lost_sed_mass += drainage[row][col].masstr * dt_st;
			}
			/*If draining to an CLOSED (endorheic) lake:*/
			else {
				hl = topo[row][col];
				Lake_Fill (Lake, drow, dcol, hl, dt_st, lake_instant_fill);
				/*Check: should be no sediment left.*/
				if (drainage[drow][dcol].masstr > .1)
				PRINT_ERROR("[%d][%d] transferring to a closed lake %d in [%d][%d] returns %.1f kg/s.",
					row, col, ild, drow, dcol, drainage[drow][dcol].masstr);
			}
			/*Next line commented in tAo!!*/
			drainage[drow][dcol].masstr = 0;
			break;
			case 'R':
			case 'E':
			drainage[drow][dcol].masstr += drainage[row][col].masstr;
			break;
		 }
		}
		else {
			/*Transfers out of model.*/
			*total_lost_sed_mass += drainage[row][col].masstr * dt_st;
		}
	}
	return(1);
}



int Ice_Flow(float **ice_velx_sl, float **ice_vely_sl, float **ice_velx_df, float **ice_vely_df, float dt_st, float *total_ice_melt, float *total_ice_precip, float *total_lost_water, float *total_evap_water)
{
	/*
	  CALCULATES ICE FLOW AND ITS SEDIMENT TRANSPORT.
	  Formulation taken from Knap et al., 1996, J. of Glaciology (see also Tomkin & Braun, 2002, Am. J. Sci.; Braun et al., 2003).
	*/
	int	n_iters;
	float 	**dh, **dhl, **D_coeff, **icetopo, ice_vol=0, ice_def_vol_incr=0, 
			dt_ice=.5*secsperyr, 
			melt_temp_per_depth = 8.7e-4, melt_temp, 
			melt_temp_range = 2 /*K*/;

	if (!K_ice_eros) return(0);

	PRINT_DEBUG("Calculating ice flow");

   	n_iters = MAX_2(floor(dt_st/dt_ice+.5), 1);
	dt_ice = dt_st/n_iters;
	PRINT_INFO("n_iters=%3d", n_iters);

	icetopo = alloc_matrix(Ny, Nx);
	D_coeff = alloc_matrix(Ny, Nx);
	dh  = alloc_matrix(Ny, Nx);
	dhl = alloc_matrix(Ny, Nx);

	/*Time loop for the ice flow velocity field*/
	for (int iter=0; iter<n_iters; iter++) {
		PRINT_INFO("ice iter=%d/%d, dt_st=%.1f yr, dt_ice=%.3f yr, Time=%.6f Myr", iter, n_iters, dt_st/secsperyr, dt_ice/secsperyr, Time/secsperMa);
		for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
			icetopo[i][j] = topo[i][j] + ice_thickness[i][j];
		}
		/*Determine coefficients D_coeff and a temptative ice velocity field*/
		for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
			float 	kc = 1e5 /*[m]*/,
				A_ice_rheo = 2.5e-16/secsperyr /*Pa-3/s*/, /*See Knap et al., 1996; Values are identical to Tomkin's, just different units (seconds and years)*/
				A_sl	   = 1.9e-10/secsperyr /*Pa-3/s/m2*/,
				vel, vel_limit, curvtopoxx, curvtopoyy, curvtopoxy, curvtopograd,
				beta, gradicetopox, gradicetopoy, modgradicetopo, D_df, D_sl;
			int 	il, jl,
				n_ice_rheo=3;
			il=i; jl=j;
			if (il==0)	il = 1;		if (il==Ny-1) il = Ny-2;
			if (jl==0)	jl = 1;		if (jl==Nx-1) jl = Nx-2;
			/*gradient of bedrock topography and curvature of ice surface topography.*/
			gradicetopox = (icetopo[il][jl+1]-icetopo[il][jl-1]) /2/dx;
			gradicetopoy = (icetopo[il-1][jl]-icetopo[il+1][jl]) /2/dy;
			modgradicetopo = sqrt(gradicetopox*gradicetopox + gradicetopoy*gradicetopoy);
			curvtopoxx = (topo[il][jl-1] -2*topo[il][jl] +topo[il][jl+1])/dx/dx;
			curvtopoyy = (topo[il-1][jl] -2*topo[il][jl] +topo[il+1][jl])/dy/dy;
			curvtopoxy = (topo[il+1][jl+1] -topo[il-1][jl+1] -topo[il+1][jl-1] +topo[il-1][jl-1])/4/dx/dy;
			/*ice constriction factor depending on the topo curvature perpendicular to ice flow.*/
			if (modgradicetopo)
				curvtopograd =  pow(gradicetopoy/modgradicetopo,2)*curvtopoxx
						  + pow(gradicetopox/modgradicetopo,2)*curvtopoyy
						  + 2*gradicetopox*gradicetopoy/modgradicetopo/modgradicetopo*curvtopoxy;
			else	curvtopograd = 0;
			curvtopograd = MAX_2(curvtopograd, -.9/kc);
			beta = 1 / (1+kc*curvtopograd);
			/*
			  See Knap et al. (1996). My D's are everything in eqs. 2 and 3 except for the last 'grad(H+h)'.
			  Tomkin's thesis has mistakes, and there D includes the ice_thickness required to convert velocity into flow.
			*/
			D_df =  -2*A_ice_rheo/(n_ice_rheo+2) * beta*pow(densice*g*ice_thickness[i][j],n_ice_rheo) * pow(modgradicetopo,n_ice_rheo-1) * ice_thickness[i][j];
			D_sl =  -A_sl/.8					 * beta*pow(densice*g*ice_thickness[i][j],n_ice_rheo) * pow(modgradicetopo,n_ice_rheo-1) ;

			melt_temp = melt_temp_per_depth*ice_thickness[i][j];
			/*No slip if ice bottom is frozen (apply a gradual change around the melting temperature)*/
			if (TEMPERATURE_ICE(topo[i][j])<melt_temp-melt_temp_range) {
				D_sl = 0;
			}
			if (TEMPERATURE_ICE(topo[i][j])>=melt_temp-melt_temp_range && TEMPERATURE_ICE(topo[i][j])<=melt_temp+melt_temp_range) {
				D_sl = D_sl * (TEMPERATURE_ICE(topo[i][j]) - (melt_temp-melt_temp_range)) / (2*melt_temp_range);
			}
			if (TEMPERATURE_ICE(topo[i][j])>melt_temp+melt_temp_range) {
				D_sl = D_sl;
			}

			/*Limit velocities to a fraction of (dx+dy)/2*/
			vel_limit = 1.0*(dx+dy)/2/dt_ice;
			vel = sqrt(pow(D_df*gradicetopox, 2)+pow(D_df*gradicetopoy, 2));
			if (vel > vel_limit) {
				PRINT_WARNING("Limiting ice deformation velocity to %.2f", vel_limit*secsperyr);
				D_df /= (vel/vel_limit) ;
			}
			vel = sqrt(pow(D_sl*gradicetopox, 2)+pow(D_sl*gradicetopoy, 2));
			if (vel > vel_limit) {
				PRINT_DEBUG("Limiting ice slip velocity to %.2f", vel_limit*secsperyr);
				D_sl /= (vel/vel_limit) ;
			}
			D_coeff[i][j] = D_df + D_sl;
			/*Ice deformation and sliding velocities, both parallel to icetopo gradient*/
			/*These do not perfectly agree with the ice flux q calculated below*/
			ice_velx_sl[i][j] = D_sl * gradicetopox;
			ice_vely_sl[i][j] = D_sl * gradicetopoy;
			ice_velx_df[i][j] = D_df * gradicetopox;
			ice_vely_df[i][j] = D_df * gradicetopoy;
		}

		/*Calculate ice flow q (evaluated in an intermediate grid: q_ri is actually at i,j+.5) and the new ice thickness in the original grid*/
		for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
			float 	ice_thermal_diff=1.09e-6 /*m2/s*/,
				surf_heat_flow=54e-3 /*W/m2*/, ice_thermal_conduc=2.1 /*W/m/K*/,
				ablation_constant=.5/secsperyr /*m/s/C*/,
				ice_precip, ice_melt, surf_temp_grad, Ts, Tb,
				divergence, gradicetopox, gradicetopoy, divergence_q, divergence_q_d, divergence_qs,
				q_up, q_dw, q_lf, q_ri, q_ru, q_rd, q_lu, q_ld,
				qs_up, qs_dw, qs_lf, qs_ri, mindh, maxdh, mindhl, maxdhl,
				D_sl_over_D, D_df_over_D, ice_vel_sl, ice_vel_df;

			q_up=q_dw=q_lf=q_ri=0;  q_ru=q_rd=q_lu=q_ld=0;  qs_up=qs_dw=qs_lf=qs_ri=0;

			/*Ice flow q. Here comes the additional ice_thickness term to pass from velocity into mass flow*/
			if (j<Nx-1)	q_ri  = ( D_coeff[i][j+1]*ice_thickness[i][j+1] + D_coeff[i][j]*ice_thickness[i][j] )/2  *  (+icetopo[i][j+1] - icetopo[i][j]) / dx;
			if (j>0)	q_lf  = ( D_coeff[i][j-1]*ice_thickness[i][j-1] + D_coeff[i][j]*ice_thickness[i][j] )/2  *  (-icetopo[i][j-1] + icetopo[i][j]) / dx;
			if (i>0)	q_up  = ( D_coeff[i-1][j]*ice_thickness[i-1][j] + D_coeff[i][j]*ice_thickness[i][j] )/2  *  (+icetopo[i-1][j] - icetopo[i][j]) / dy;
			if (i<Ny-1)	q_dw  = ( D_coeff[i+1][j]*ice_thickness[i+1][j] + D_coeff[i][j]*ice_thickness[i][j] )/2  *  (-icetopo[i+1][j] + icetopo[i][j]) / dy;

			if (i>0	&& j<Nx-1)	q_ru  = ( D_coeff[i-1][j+1]*ice_thickness[i-1][j+1] + D_coeff[i][j]*ice_thickness[i][j] )/2  *  (+icetopo[i-1][j+1] - icetopo[i][j]) / dx;
			if (i<Ny-1 && j<Nx-1)	q_rd  = ( D_coeff[i+1][j+1]*ice_thickness[i+1][j+1] + D_coeff[i][j]*ice_thickness[i][j] )/2  *  (+icetopo[i+1][j+1] - icetopo[i][j]) / dx;
			if (i>0	&& j>0   )	q_lu  = ( D_coeff[i-1][j-1]*ice_thickness[i-1][j-1] + D_coeff[i][j]*ice_thickness[i][j] )/2  *  (-icetopo[i-1][j-1] + icetopo[i][j]) / dy;
			if (i<Ny-1 && j>0)	q_ld  = ( D_coeff[i+1][j-1]*ice_thickness[i+1][j-1] + D_coeff[i][j]*ice_thickness[i][j] )/2  *  (-icetopo[i+1][j-1] + icetopo[i][j]) / dy;

			/*Sedim. load flow qs.*/
			if (j<Nx-1)	qs_ri = ( D_coeff[i][j+1]*ice_sedm_load[i][j+1] + D_coeff[i][j]*ice_sedm_load[i][j] )/2  *  (+icetopo[i][j+1] - icetopo[i][j]) / dx;
			if (j>0)	qs_lf = ( D_coeff[i][j-1]*ice_sedm_load[i][j-1] + D_coeff[i][j]*ice_sedm_load[i][j] )/2  *  (-icetopo[i][j-1] + icetopo[i][j]) / dx;
			if (i>0)	qs_up = ( D_coeff[i-1][j]*ice_sedm_load[i-1][j] + D_coeff[i][j]*ice_sedm_load[i][j] )/2  *  (+icetopo[i-1][j] - icetopo[i][j]) / dy;
			if (i<Ny-1)	qs_dw = ( D_coeff[i+1][j]*ice_sedm_load[i+1][j] + D_coeff[i][j]*ice_sedm_load[i][j] )/2  *  (-icetopo[i+1][j] + icetopo[i][j]) / dy;

			/*actual ice velocity field*/
/*			D_sl_over_D=D_df_over_D=0;
			ice_vel_sl = sqrt(ice_velx_sl[i][j]*ice_velx_sl[i][j]+ice_vely_sl[i][j]*ice_vely_sl[i][j]);
			ice_vel_df = sqrt(ice_velx_df[i][j]*ice_velx_df[i][j]+ice_vely_df[i][j]*ice_vely_df[i][j]);
			if (ice_vel_sl+ice_vel_df) {
				D_sl_over_D = ice_vel_sl/(ice_vel_sl+ice_vel_df);
				D_df_over_D = ice_vel_df/(ice_vel_sl+ice_vel_df);
			}
			if (ice_thickness[i][j]) {
				ice_velx_sl[i][j] = D_sl_over_D * (q_ri+q_lf)/2/ice_thickness[i][j];
				ice_vely_sl[i][j] = D_sl_over_D * (q_up+q_dw)/2/ice_thickness[i][j];
				ice_velx_df[i][j] = D_df_over_D * (q_ri+q_lf)/2/ice_thickness[i][j];
				ice_vely_df[i][j] = D_df_over_D * (q_up+q_dw)/2/ice_thickness[i][j];
			}
			else {
				ice_velx_sl[i][j] = ice_vely_sl[i][j] = ice_velx_df[i][j] = ice_vely_df[i][j] = 0;
			}
*/
			/*Calculate divergence of flux q and update increments in ice thickness*/
			divergence_q   =  ( q_ri  - q_lf  ) / dx  +  ( q_up  - q_dw  ) / dy ;
			divergence_q_d =  ( q_ru  - q_ld  ) / dxy +  ( q_rd  - q_lu  ) / dxy ;
			divergence_qs  =  ( qs_ri - qs_lf ) / dx  +  ( qs_up - qs_dw ) / dy ;
			dh[i][j]  =  - (divergence_q*1 + divergence_q_d*.7)/1.7 * dt_ice;
			dhl[i][j] =  - divergence_qs * dt_ice;

			/*Limit the amount of thining/thickening to the sorrounding icetopo*/
			/*Limit the amount of sediment load to the sorrounding one*/
			mindh=0;  maxdh=-0;
			mindhl=0; maxdhl=-0;
			if (j>0) {
				mindh =MIN_2(mindh,  icetopo[i][j-1]-icetopo[i][j]); 			maxdh =MAX_2(maxdh,  icetopo[i][j-1]-icetopo[i][j]);
				mindhl=MIN_2(mindhl, ice_sedm_load[i][j-1]-ice_sedm_load[i][j]); 	maxdhl=MAX_2(maxdhl, ice_sedm_load[i][j-1]-ice_sedm_load[i][j]);
			}
			if (j<Nx-1) {
				mindh =MIN_2(mindh,  icetopo[i][j+1]-icetopo[i][j]); 			maxdh =MAX_2(maxdh,  icetopo[i][j+1]-icetopo[i][j]);
				mindhl=MIN_2(mindhl, ice_sedm_load[i][j+1]-ice_sedm_load[i][j]); 	maxdhl=MAX_2(maxdhl, ice_sedm_load[i][j+1]-ice_sedm_load[i][j]);
			}
			if (i<Ny-1) {
				mindh =MIN_2(mindh,  icetopo[i+1][j]-icetopo[i][j]); 			maxdh =MAX_2(maxdh,  icetopo[i+1][j]-icetopo[i][j]);
				mindhl=MIN_2(mindhl, ice_sedm_load[i+1][j]-ice_sedm_load[i][j]); 	maxdhl=MAX_2(maxdhl, ice_sedm_load[i+1][j]-ice_sedm_load[i][j]);
			}
			if (i>0) {
				mindh =MIN_2(mindh,  icetopo[i-1][j]-icetopo[i][j]);			maxdh =MAX_2(maxdh,  icetopo[i-1][j]-icetopo[i][j]);
				mindhl=MIN_2(mindhl, ice_sedm_load[i-1][j]-ice_sedm_load[i][j]);	maxdhl=MAX_2(maxdhl, ice_sedm_load[i-1][j]-ice_sedm_load[i][j]);
			}
	/*!!*/		dh[i][j]  = MAX_2(dh[i][j],  .45*mindh);	dh[i][j]  = MIN_2(dh[i][j],  .45*maxdh);
			dhl[i][j] = MAX_2(dhl[i][j], .45*mindhl);	dhl[i][j] = MIN_2(dhl[i][j], .45*maxdhl);

			/*Limit the amount of thickening to the available upflow ice thickness*/
			if (dh[i][j]>0 && ice_thickness[i][j]) {
				float maxdh, vel_df_abs;
				int il, jl;
				vel_df_abs = fabs(ice_velx_df[i][j])+fabs(ice_vely_df[i][j]);
				if (ice_velx_df[i][j]>0) 	{jl=MAX_2(j-1,0);   }
				else  			{jl=MIN_2(j+1,Nx-1);}
				if (ice_vely_df[i][j]>0) 	{il=MIN_2(i+1,Ny-1);}
				else  			{il=MAX_2(i-1,0);   }
				if (vel_df_abs) maxdh = (fabs(ice_velx_df[i][j])*ice_thickness[i][jl] + fabs(ice_vely_df[i][j])*ice_thickness[il][j]) / vel_df_abs;
				else maxdh = 1e23;
				dh[i][j]  = MIN_2(dh[i][j],  maxdh);
			}

			ice_def_vol_incr += dh[i][j];
			ice_vol += ice_thickness[i][j];


			if (dh[i][j] < -ice_thickness[i][j]) {PRINT_DEBUG("%d  %d   %f", i, j, dh[i][j]);}

			/*Limit the amount of thinning to the available ice thickness*/
			if (dh[i][j]  < -ice_thickness[i][j])  dh[i][j]  = -ice_thickness[i][j];
			if (dhl[i][j] < -ice_sedm_load[i][j])  dhl[i][j] = -ice_sedm_load[i][j];


			/*Ice temperature model to calculate melting*/
			ice_precip = precipitation_snow[i][j];
			*total_ice_precip += ice_precip*dx*dy / n_iters;
			surf_temp_grad = surf_heat_flow/ice_thermal_conduc  +  ice_thickness[i][j]*densice*g*sqrt(ice_velx_sl[i][j]*ice_velx_sl[i][j]+ice_vely_sl[i][j]*ice_vely_sl[i][j])/ice_thermal_conduc;
			Ts = TEMPERATURE_ICE(icetopo[i][j]); /*[K]*/
			if (ice_thickness[i][j] && ice_precip) {
				float sqrt_term;
				sqrt_term = sqrt(2*ice_thickness[i][j]*ice_thermal_diff/ice_precip);
				Tb = Ts + surf_temp_grad * sqrt(3.1415927)/2 * sqrt_term * erf(ice_thickness[i][j]/sqrt_term); /*[K]*/
			}
			else {
				Tb = Ts + surf_temp_grad * ice_thickness[i][j];
			}
			/*Ice melt at the base of the ice sheet, proportional to basal TEMPERATURE_ICE in centigrades*/
			melt_temp = melt_temp_per_depth*ice_thickness[i][j];
			ice_melt = ablation_constant * (Tb+melt_temp-TEMP_FREEZE_WATER);
			ice_melt = LIMIT(ice_melt, 0, ice_precip + (ice_thickness[i][j]+dh[i][j])/dt_ice);

			dh[i][j] +=  (ice_precip - ice_melt) * dt_ice;

			/*Transfer melt water to river network*/
			if (drainage[i][j].type == 'L') {
				if (IN_DOMAIN(drainage[i][j].dr_row, drainage[i][j].dr_col)) {
				drainage[drainage[i][j].dr_row][drainage[i][j].dr_col].discharge += ice_melt*dx*dy / n_iters;
				}
				else {
					if (AT_BORDER(i,j))
					*total_lost_water += ice_melt*dx*dy / n_iters;
				else 	*total_evap_water += ice_melt*dx*dy / n_iters;
				}
			}
			else {
				drainage[i][j].discharge += ice_melt*dx*dy / n_iters;
			}
			*total_ice_melt += ice_melt*dx*dy / n_iters;
		}
		/*Check ice mass balance*/
		{
			float ice_sed_vol_incr=0, ice_sed_vol_incr_corr=0, ice_sed_vol=0;  
			int numpositive=0;
			for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
				ice_sed_vol_incr += dhl[i][j];
				ice_sed_vol += ice_sedm_load[i][j];
			}
			for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
				if (ice_sedm_load[i][j]>ice_sed_vol_incr/Nx/Ny) numpositive++;
			}
			for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
				if (ice_sedm_load[i][j]>ice_sed_vol_incr/Nx/Ny) dhl[i][j] -= ice_sed_vol_incr/numpositive;
				ice_sed_vol_incr_corr += dhl[i][j];
			}
			ice_sed_vol *= dx*dy;
			ice_sed_vol_incr_corr *= dx*dy;
			ice_def_vol_incr *= dx*dy;
			ice_vol *= dx*dy;
			if (ice_sed_vol && fabs(ice_sed_vol_incr_corr)>1*ice_sed_vol/100) PRINT_ERROR("sediment unbalance: %.2e N (%+.2f %%) out of %.2e N", ice_sed_vol_incr_corr*denscrust*g, ice_sed_vol_incr_corr/ice_sed_vol*100, ice_sed_vol*denscrust*g);
			if (ice_vol && fabs(ice_def_vol_incr)>1*ice_vol/100) PRINT_ERROR("ice volume unbalance: %.2e m3 (%+.2f %%) out of %.2e m3", ice_def_vol_incr, ice_def_vol_incr/ice_vol*100, ice_vol);
		}
		for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
			/*Apply the results*/
			ice_thickness[i][j] += dh[i][j];
			ice_sedm_load[i][j] += dhl[i][j];
			/*For later isostatic compensation of ice load changes*/
			Dq[i][j] += g * dh[i][j] * (densice - densenv);
			/*Check that ice and seds have >0 thickness*/
			if (ice_thickness[i][j]<-1e-3 || ice_sedm_load[i][j]<-1e-1) PRINT_ERROR("[%d][%d] negative ice thickness\tice thickness=%8.2e m\tsed. thick.=%8.2e m", i, j, ice_thickness[i][j], ice_sedm_load[i][j]);
			ice_thickness[i][j] = MAX_2(ice_thickness[i][j], 0);
			ice_sedm_load[i][j] = MAX_2(ice_sedm_load[i][j], 0);
		}
		/*Check convergence*/
		/*{
			float max_abs_dh=0;
			for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
				max_abs_dh = MAX_2(max_abs_dh, fabs(dh[i][j]));
			}
			max_abs_dh /= (dt_ice/secsperyr);
			if (verbose_level>=3) fprintf(stdout, "\tmax.change: %.3f m/yr", max_abs_dh);
			if (max_abs_dh<1.) break;
		}*/
	}

	PRINT_DEBUG("END ice deformation");

	free_matrix(icetopo, Ny);
	free_matrix(D_coeff, Ny);
	free_matrix(dh, Ny);
	free_matrix(dhl, Ny);
	return (1);
}



int Ice_EroSed(float **ice_velx_sl, float **ice_vely_sl, float dt_ice, float *total_ice_eros, float *total_ice_sedim)
{
	/*GLACIAR INCISION AND DEPOSITION*/
	
	if (!K_ice_eros || !erosed_model) return(0);
	for (int i=0; i<Ny; i++) for (int j=0; j<Nx; j++) {
		/*ice incision*/
		if (ice_thickness[i][j]>0) {
			float dheros;
			dheros = K_ice_eros * sqrt(ice_velx_sl[i][j]*ice_velx_sl[i][j]+ice_vely_sl[i][j]*ice_vely_sl[i][j]) * dt_ice;
			Erode(dheros * dx*dy * denscrust, i, j);
			ice_sedm_load[i][j] += dheros;
			*total_ice_eros += dheros * dx*dy * denscrust;
		}
		/*deposit if few ice left*/
		{
			float deposit=0, max_sed_factor=.02;
			if (ice_sedm_load[i][j]>max_sed_factor*ice_thickness[i][j]) 
				deposit = ice_sedm_load[i][j]-max_sed_factor*ice_thickness[i][j];
			Sediment(deposit * dx*dy * denscrust, i, j);
			ice_sedm_load[i][j] -= deposit;
			*total_ice_sedim += deposit * dx*dy * denscrust;
			/*printf("\n%d %d %.2e m", i,j, *total_ice_sedim);*/
		}
	}

	return (1);
}


int Lake_Fill (
	struct LAKE_INFO *Lake, 
	int row, int col, 		/*Lake node receiving the sediments*/
	float hl, 			/*Limit of sedimentation heigh*/
	float dt_st, 
	int lake_instant_fill)
{
	/*
	  THIS ROUTINE TREATS THE PROBLEM OF LAKE SEDIMENT INFILLING.
	  The strategy is to call this routine when a lake node 
	  receives sediments from a river. Then, the sediments and water of
	  this node is equally distributed to all the lake neighbours,
	  depositing a part of the sediments which is limited by
	  the lake's elevation. The sediment transfer is iterated 
	  with the next neigbor nodes in the lake successively.
	  No sedimentation occurs in the outlets, which are eroded
	  as rivers.
	  row,col is the lake node receiving sediments.
	*/

	int	il, 
		n_total_done=0, n_done=0, 
		ro[NDERS], co[NDERS];
	float 	d_mass, d_mass_now, Dhsedmax, Dhsed, 
		l_fluv_sedim_aux=l_fluv_sedim, total_weight_distr=0;
	struct GRIDNODE *to_do, *done;
	float	**done_grid;

	if (!drainage[row][col].masstr) return(0);

	il = drainage[row][col].lake;

	/*Try to fill lakes (except sea) with sediment if lake_instant_fill*/
	if (lake_instant_fill) { 
		BOOL its_sea=0;
		IF_LAKE_IS_SEA(il) its_sea=1;
		if (!its_sea) {
			for (int i=0; i<Lake[il].n; i++) {
				Sediment (THICK2SEDMASS(Lake[il].alt - topo[Lake[il].row[i]][Lake[il].col[i]]), Lake[il].row[i], Lake[il].col[i]);
			}
		}
	}

	/*Sediments very efficiently if the lake is small and endorheic (!!)*/
	if (!Lake[il].n_sd && Lake[il].n<(Nx+Ny)/5) l_fluv_sedim_aux *= .01;

	to_do = (struct GRIDNODE *) calloc(Lake[il].n, sizeof(struct GRIDNODE));
	done  = (struct GRIDNODE *) calloc(Lake[il].n, sizeof(struct GRIDNODE));
	done_grid = (float **) calloc(Ny, sizeof(float *));
	for (int i=0; i<Ny; i++) done_grid[i] = (float *) calloc(Nx, sizeof(float));


	/*Sediment in the receiving node itself. Assuming null transport capacity.*/
	/*d_mass<0*/
	d_mass = dxy / l_fluv_sedim_aux * (0 - drainage[row][col].masstr) * dt_st;
	d_mass = MAX_2(d_mass, -drainage[row][col].masstr*dt_st);
	Dhsed = -MASS2SEDTHICK(d_mass);
	Dhsedmax = MAX_2(hl-topo[row][col], 0);
	Dhsed = MIN_2(Dhsed, Dhsedmax);
	d_mass = -THICK2SEDMASS(Dhsed);
	Sediment (-d_mass, row, col);
	drainage[row][col].masstr += d_mass/dt_st;
	n_done=n_total_done=1;
	done[0].row=row; done[0].col=col; 
	done_grid[row][col]=1;

	/*Propagate 1 cell per loop:*/
	for (;;) {
		int n_prop=1, n_to_do=0; 
		/*Count the n_to_do untreated lake nodes that are next to the n_done already treated lake nodes*/
		n_prop; n_to_do; total_weight_distr=0;
		for (int m=0; m<n_done; m++) {
			int i=done[m].row, j=done[m].col;
		ro[0]=i-1, ro[1]=i,   ro[2]=i+1, ro[3]=i,   ro[4]=i-1, ro[5]=i+1, ro[6]=i+1, ro[7]=i-1;
		co[0]=j,   co[1]=j+1, co[2]=j,   co[3]=j-1, co[4]=j+1, co[5]=j+1, co[6]=j-1, co[7]=j-1;
		for (int k=0; k<NDERS; k++) {
			/*if it's in the lake but not treated*/
			if (IN_DOMAIN(ro[k],co[k])) {
			if (drainage[ro[k]][co[k]].lake == il) {
				if (!done_grid[ro[k]][co[k]]) {
				to_do[n_to_do].row = ro[k];
				to_do[n_to_do].col = co[k];
				n_to_do++;
				}
				if (k<4) {total_weight_distr += 1;		done_grid[ro[k]][co[k]] += 1;}
				else	 {total_weight_distr += 1/1.4142; done_grid[ro[k]][co[k]] += 1/1.4142;}
			}
			}
		}
		}
		if (!n_to_do || drainage[row][col].masstr<.001) break;
		d_mass_now = dxy / l_fluv_sedim_aux * (0 - drainage[row][col].masstr) * dt_st;
		d_mass_now = MAX_2(d_mass_now, -drainage[row][col].masstr*dt_st);
		d_mass_now /= total_weight_distr;
		/*Sediment in the counted n_to_do untreated lake nodes*/
		n_done=0; n_prop++;
		for (int i=0; i<n_to_do; i++) {
		/*Limit d_mass with the lake height and with a small slope from the outlet towards the lake*/
		/*Removing the 1. in "MAX_2(hl+1.-(..." leads to formation of Mississippi-like levees in deltas*/
		Dhsedmax = MAX_2(hl+1.-(float)n_prop*(dx+dy)/2*.001-topo[to_do[i].row][to_do[i].col], 0);
		if (hl < topo[to_do[i].row][to_do[i].col]-3)
			/*This check is not robust: topo may have changed during sedimentation.*/
			PRINT_DEBUG("[%d][%d] (%.1f m) is %.1f m higher than level of its lake %d in Lake_Fill.",
				to_do[i].row, to_do[i].col, topo[to_do[i].row][to_do[i].col], topo[to_do[i].row][to_do[i].col]-hl, il);
		if (done_grid[to_do[i].row][to_do[i].col]<1/1.4142 || done_grid[to_do[i].row][to_do[i].col]>5.9) 
			PRINT_WARNING("[%d][%d] (%.1f m) has %.2f neighbours (or 'weight') to distribute sediments in Lake_Fill.",
				to_do[i].row, to_do[i].col, topo[to_do[i].row][to_do[i].col], done_grid[to_do[i].row][to_do[i].col]);
		Dhsed = -MASS2SEDTHICK(d_mass_now * done_grid[to_do[i].row][to_do[i].col]);
		Dhsed = MIN_2(Dhsed, Dhsedmax);
		d_mass = -THICK2SEDMASS(Dhsed);
		Sediment (-d_mass, to_do[i].row, to_do[i].col);
		drainage[row][col].masstr += d_mass/dt_st;
		done[i].row = to_do[i].row;
		done[i].col = to_do[i].col;
		n_total_done++;
		n_done++;
		}
		if (n_done != n_to_do) PRINT_ERROR("n_done (%d) != n_to_do (%d).", n_done, n_to_do);
	}

	/*
	  Deleting the '!drainage[row][col].masstr' break condition in the 
	  top of this routine (which is unnecessary), 
	  should be a good test to check the number of nodes of deposition, 
	  but it does not work!!.
	*/
	if (n_total_done != Lake[il].n-Lake[il].n_sd) PRINT_DEBUG("Little sediment left: %.2e; n_total_done (%d) != lakenodes-lakeoutlets (%d).", drainage[row][col].masstr, n_total_done, Lake[il].n-Lake[il].n_sd);

	/*If the lake is endorheic and there is still some transported mass left, then deposit it uniformly.*/
	if (!Lake[il].n_sd && drainage[row][col].masstr > .1) {
		d_mass = -drainage[row][col].masstr * dt_st;
		for (int i=0; i<Lake[il].n; i++) {
			Sediment (-d_mass/Lake[il].n, Lake[il].row[i], Lake[il].col[i]);
		}
		Dhsed = -MASS2SEDTHICK(d_mass) / Lake[il].n;
		if (verbose_level>=3 || (drainage[row][col].masstr>1 && Dhsed>10))
			PRINT_WARNING("filling endorh. lake %d (%d nds, %.1f m3/s), at %.2f,%.2f km in rough way: %.1f m.", 
				il, Lake[il].n, Lake_Input_Discharge(il), (col*dx+xmin)/1e3, (ymax-row*dy)/1e3, Dhsed);
		drainage[row][col].masstr = 0;
	}

	free(done);
	free(to_do);
	for (int i=0; i<Ny; i++) free(done_grid[i]);
	free(done_grid);

	return(1);
}




int Landslide_Transport (float critical_slope, float dt, float dt_eros)
{
	float	Dheros, dl;
	int 	n_iters;
	
	/*
	  COMPUTES THE LOAD, HEIGHT & Block-THICKNESS changes DUE TO 
	  LANDSLIDING. This process approaches short scale 
	  transport processes. Assumes that slope cannot exceed a critical 
	  slope.
	*/

	if (!erosed_model || !critical_slope) return (0);
	
	PRINT_DEBUG("Calculating Landslides");

	calculate_topo(topo);

	n_iters = MAX_2(floor(dt/dt_eros+.5), 1); 
	PRINT_INFO("n_iters=%3d", n_iters);

	for (int k=0; k<n_iters; k++) {
 		/*Adds results to the height and the next load Dq and removes material from the Blocks*/
 		for(int i=0; i<Ny; i++)  for(int j=0; j<Nx; j++)  {
			int ro[NDERS], co[NDERS], imaxslope, l;
		float maxdiff, dl;
		ro[0]=i-1, ro[1]=i,   ro[2]=i+1, ro[3]=i,   ro[4]=i-1, ro[5]=i+1, ro[6]=i+1, ro[7]=i-1;
		co[0]=j,   co[1]=j+1, co[2]=j,   co[3]=j-1, co[4]=j+1, co[5]=j+1, co[6]=j-1, co[7]=j-1;

		/*Calculate max slope in the 8 sorrounding directions*/
		imaxslope=-1; maxdiff=0;
		for(int l=0; l<NDERS; l++) {
			if (IN_DOMAIN(ro[l],co[l])) {
				if (maxdiff < topo[i][j]-topo[ro[l]][co[l]]) {
					maxdiff = topo[i][j] - topo[ro[l]][co[l]];
					imaxslope = l;
				}
			}
		}

		if (imaxslope>=0) {
				dl = sqrt(pow(dy*(ro[imaxslope]-i),2)+pow(dx*(co[imaxslope]-j),2));
				if (critical_slope < maxdiff/dl) {
				/*it should preserve mass instead of using denscrust!!*/
					Dheros = maxdiff/4; /*reduces slope by 1/2*/
				Erode	(Dheros*dx*dy*denscrust, i, j);
				Sediment (Dheros*dx*dy*denscrust, ro[imaxslope], co[imaxslope]);
				}
		}
		}
	}

	return (1);
}




int Sediment (double d_mass, int row, int col) 
{
	/*
	  Adds dh_sed to the highest Block.
	  d_mass > 0   (mass of deposited seds., subtracted from the river load, does not include the water deposited with the seds)
	  dh_sed > 0   (thickness of sediments deposited)
	*/

	float dh_sed;

	dh_sed = MASS2SEDTHICK(d_mass);
	if (dh_sed < -10) PRINT_WARNING("trying to sediment negative mass: %f m", dh_sed);
	/*Increment load, Blocks and topo*/
	Dq[row][col] +=  dh_sed * g * (denssedim-densenv);
	Blocks[numBlocks-1].thick[row][col] += dh_sed;
	topo[row][col] += dh_sed;
	/*record of eros/sed is performed in kg*/
	eros_now[row][col]   -= d_mass ;
	accumul_erosion[row][col] -= d_mass ;

	total_sed_mass += d_mass;
	return (1);
}


int Erode (double d_mass, int row, int col) 
{
	/*
	  Erode a certain amount of rock mass from the uppermost Blocks.
	  Increment load, Blocks and topo.
	  d_mass  > 0   (eroded mass)
	  dh_eros > 0   (thickness of material eroded)
	*/

	float dh_eros=0, dh_eros_Block, mass_per_m2=d_mass/dx/dy;

	for (int k=numBlocks-1; mass_per_m2>0 && k>=0; k--) {
		if (Blocks[k].density == denssedim) {
			dh_eros_Block = MIN_2(Blocks[k].thick[row][col], MASS2SEDTHICK(mass_per_m2)*dx*dy);
			mass_per_m2 -= THICK2SEDMASS(dh_eros_Block) / dx/dy;
			total_sed_mass -= THICK2SEDMASS(dh_eros_Block);
		}
		else {
			dh_eros_Block = MIN_2(Blocks[k].thick[row][col], fabs(mass_per_m2/Blocks[k].density));
			mass_per_m2 -= dh_eros_Block * Blocks[k].density;
			total_bedrock_eros_mass += dh_eros_Block * Blocks[k].density * dx*dy;
		}
		Blocks[k].thick[row][col] -= dh_eros_Block;
		dh_eros += dh_eros_Block;
		Dq[row][col] -=  g * dh_eros_Block * (Blocks[k].density - densenv);
	}
	/*Erode basement*/
	if (mass_per_m2>0) {
		Blocks_base[row][col] -= mass_per_m2/denscrust;
		dh_eros += mass_per_m2/denscrust;
		Dq[row][col] -=  g * mass_per_m2*(denscrust-densenv)/denscrust;
		total_bedrock_eros_mass +=  mass_per_m2 * dx*dy;
	}
	topo[row][col] -= dh_eros;
	/*record of eros/sed is performed in kg*/
	eros_now[row][col]   += d_mass;
	accumul_erosion[row][col] += d_mass;

	return (1);
}




int Damn_River_Node (
	int ia, int ja,  	/* previous (upper, higher) node of the river to be damned */
	int i,  int j)  	/* node of the river to which ia,ja must drain*/
{
	/*
		Damns the previous (upper) node ia,ja, belonging to the river 
		so that it drains to this i,j river node.
	*/

	/*Rise the 8 sorrounding damn nodes clockwise:*/
	Rise_Damn_Node (ia-1, ja-1, i, j);
	Rise_Damn_Node (ia,   ja-1, i, j);
	Rise_Damn_Node (ia+1, ja-1, i, j);
	Rise_Damn_Node (ia+1, ja,   i, j);
	Rise_Damn_Node (ia+1, ja+1, i, j);
	Rise_Damn_Node (ia,   ja+1, i, j);
	Rise_Damn_Node (ia-1, ja+1, i, j);
	Rise_Damn_Node (ia-1, ja,   i, j);
	
	return 1;
}


int Rise_Damn_Node (
	int iia, int jja,  	/* node to be rised, which is neighbour of the previous (upper) node of the river */
	int i,   int j)  	/* node of the river that should receive the drainage*/
{
	/*
	  Increases the altitude of one of the nodes sorrounding the previous node of the river.
	*/

	float	damn_height = .1;

	if (IN_DOMAIN(iia,jja) && IN_DOMAIN(i,j)) {
		if (iia !=i || jja !=j) {
			topo[iia][jja] = MAX_2(topo[iia][jja], topo[i][j]+damn_height);
		}
	}
	return (1);
}







/******************************************************************************/
/*=========================  NODE & LAKE MANAGEMENT  =========================*/




int Add_Node_To_Lake (int row, int col, int i_lake)
{
	/*	
	  Allocates memory for a new lake node.
	  New node is added at the end of the lake's node list.
	*/
	int i, il;

	il = fabs((float) i_lake);

	for (i=0; i<Lake[il].n; i++) 
		if (row==Lake[il].row[i] && col==Lake[il].col[i]) {
		  	PRINT_ERROR("[%d][%d] tried to repeat lake node.", row, col);
			return(0);
		}

	Lake[il].n++;
	Lake[il].row = realloc(Lake[il].row, Lake[il].n*sizeof(int));
	Lake[il].col = realloc(Lake[il].col, Lake[il].n*sizeof(int));
	Lake[il].row[Lake[il].n-1] = row;
	Lake[il].col[Lake[il].n-1] = col;

	/*
	  Add information to the drainage grid.
	  Use the same sign as the nodes in the lake already had.
	  Negative sign notes the undefined character.
	*/
	drainage[row][col].lake = -il;

	PRINT_DEBUGPLUS("ADD node [%d][%d] (%.2f m) to lake %d (%d nodes)", row, col, topo[row][col], il, Lake[il].n);

	return (1);
}



int Add_Outlet_To_Lake (int row_sd, int col_sd, int row_tr, int col_tr, int i_lake)
{
	/*	
	  Allocates memory for a new lake node.
	  New outlet is added at the end of the lake's outlet list.
	*/
	int i, il;

	il = fabs((float) i_lake);

	for (i=0; i<Lake[il].n_sd; i++) 
		if (row_sd==Lake[il].row_sd[i] && col_sd==Lake[il].col_sd[i]) {
		  	PRINT_WARNING("[%d][%d] tried to repeat lake outlet.", row_sd, col_sd);
			return(0);
		}

	Lake[il].n_sd++;
	Lake[il].row_sd = realloc(Lake[il].row_sd, Lake[il].n_sd*sizeof(int));
	Lake[il].col_sd = realloc(Lake[il].col_sd, Lake[il].n_sd*sizeof(int));
	Lake[il].row_sd[Lake[il].n_sd-1] = row_sd;
	Lake[il].col_sd[Lake[il].n_sd-1] = col_sd;
	drainage[row_sd][col_sd].dr_row = row_tr;
	drainage[row_sd][col_sd].dr_col = col_tr;

	PRINT_DEBUGPLUS("ADD outlet [%d][%d] (%.2f m) to lake %d (%d outlets)", row_sd, col_sd, topo[row_sd][col_sd], il, Lake[il].n_sd);

	return (1);
}




int Attempt_Delete_Node_From_Lake (int row, int col)
{
	/*
	  Divides tha lake if necessary when deleting node.
	  Returns the number of subdivided lakes (>=2, 0 means no subdivision).
	*/
	int 	i, j, k, il, n_neigh_same_lake=0, splitting=0, 
		ro[NDERS], co[NDERS];

	il = drainage[row][col].lake;
	if (!il) {PRINT_ERROR("[%d][%d] is not in a lake", row, col); return (0);}
	if (Lake[il].n<=1) return (0);
	/*Don't delete the node if there are still higher outlets in the lake*/
	if (Lake[il].n_sd) if (topo[Lake[il].row_sd[0]][Lake[il].col_sd[0]]>topo[row][col]) return (0);
	/*Don't delete sea nodes*/
	IF_LAKE_IS_SEA(il) return (0);

	PRINT_DEBUGPLUS("Deleting [%d][%d] from lake %d (%d nodes);  node_disch: %f m3/s", row,col, il, Lake[il].n,  drainage[row][col].discharge);

	/*Locate ammount of neighbours belonging to the same lake*/
	i = row;  j = col;
	ro[0]=i-1, ro[1]=i,   ro[2]=i+1, ro[3]=i,   ro[4]=i-1, ro[5]=i+1, ro[6]=i+1, ro[7]=i-1;
	co[0]=j,   co[1]=j+1, co[2]=j,   co[3]=j-1, co[4]=j+1, co[5]=j+1, co[6]=j-1, co[7]=j-1;
	for (k=0; k<NDERS; k++) {
		if (IN_DOMAIN(ro[k],co[k])) {
			if (drainage[ro[k]][co[k]].lake == il) n_neigh_same_lake++;
		}
	}

	if (n_neigh_same_lake<7) {
		splitting = Divide_Lake(row, col);
		if (splitting) {
			/*If the lake was subdivided then don't delete this node. If necessary, it was already done during splitting.*/
			if (drainage[row][col].lake<0) {
				Define_Lake(drainage[row][col].lake);
			}
			return (splitting);
		}
		else {
			/*The node is now a river. Remove it from lake.*/
			Delete_Node_From_Lake (row, col);
			return (splitting);
		}
	}
	else {
		/*The node is now a river. Remove it from lake.*/
		Delete_Node_From_Lake (row, col);
		return (splitting);
	}
}


int Deallocate_Lake (int i_lake)
{
	/*	
	  Frees memory of a lake
	*/

	int i, j, il;

	il = fabs((float) i_lake);

	PRINT_DEBUGPLUS("Deleting lake %d (of %d).", i_lake, nlakes);
	free(Lake[il].row);
	free(Lake[il].col);
	free(Lake[il].row_sd);
	free(Lake[il].col_sd);

	for (i=il+1; i<=nlakes; i++) {
		for (j=0; j<Lake[i].n; j++) 
			drainage[Lake[i].row[j]][Lake[i].col[j]].lake = 
				(i-1) * fabs((float) drainage[Lake[i].row[j]][Lake[i].col[j]].lake)/drainage[Lake[i].row[j]][Lake[i].col[j]].lake;
		Lake[i-1].n = Lake[i].n;
		Lake[i-1].n_sd = Lake[i].n_sd;
		Lake[i-1].row = Lake[i].row;
		Lake[i-1].col = Lake[i].col;
		Lake[i-1].row_sd = Lake[i].row_sd;
		Lake[i-1].col_sd = Lake[i].col_sd;
	}

	nlakes--;

	Lake = (struct LAKE_INFO *) realloc(Lake, (nlakes+1)*sizeof(struct LAKE_INFO));
	
	return(1);
}




int Define_Lake (int i_lake)
{
	/*	
	  Marks a lake as defined, i.e., all its nodes are already known.
	*/
	int i, il;

	il = fabs((float) i_lake);

	for (i=0; i<Lake[il].n; i++) {
		if ((int) fabs((float) drainage[Lake[il].row[i]][Lake[il].col[i]].lake) != il)
			PRINT_ERROR("lake # %d was expected instead of %d. [%d][%d]",
				il, (int) fabs((float) drainage[Lake[il].row[i]][Lake[il].col[i]].lake), Lake[il].row[i], Lake[il].col[i]);
		drainage[Lake[il].row[i]][Lake[il].col[i]].lake = il;
	}
	return(1);
}



int Delete_Node_From_Lake (int row, int col)
{
	/*
	  ACTUALLY REMOVES THE NODE FROM THE LAKE STRUCTURE 
	  Deallocates memory for a lake node.
	  The rest of nodes are shifted in the lake's node list.
	  Finds new drainage for this node.
	*/

	int 	i, j, k, il, ild, i_node, i_outlet;
	int	imaxderneg=SIGNAL, ro[NDERS], co[NDERS];
	int 	lrow, lcol;
	float	maxderneg=1e9, dxdivdy=dx/dy;

	il = drainage[row][col].lake;
	i_node = Lake_Node_Number(row, col);
	i_outlet = Lake_Outlet_Number(row,col);

	if (il<=0) PRINT_ERROR("[%d][%d] is not a lake (%d)", row, col, il);

	/*Calculate topography derivates (slopes)*/
	i = row;  j = col;
	ro[0]=i-1, ro[1]=i-1, ro[2]=i-1, ro[3]=i,   ro[4]=i,   ro[5]=i+1, ro[6]=i+1, ro[7]=i+1;
	co[0]=j-1, co[1]=j,   co[2]=j+1, co[3]=j-1, co[4]=j+1, co[5]=j-1, co[6]=j,   co[7]=j+1;
	for (k=0; k<NDERS; k++) {
		float dist, deriv;
		switch (k) {
			case 1: case 6: dist=dy;  break;   /*N,S*/
			case 3: case 4: dist=dx;  break;   /*E,W*/
			default:		dist=dxy; break;   /*others*/
		}
		if (IN_DOMAIN(ro[k],co[k])) {
			deriv = (topo[ro[k]][co[k]]-topo[i][j])/dist;
			if ((deriv<0 && deriv<maxderneg) || (deriv==0 && deriv<=maxderneg && drainage[ro[k]][co[k]].lake)) 
				{imaxderneg=k; maxderneg=deriv;}
		}
	}

	/*Determine new drainage by draining through the maximum slope*/
	if (IN_DOMAIN(drainage[row][col].dr_row, drainage[row][col].dr_col)) 
		ild = drainage[drainage[row][col].dr_row][drainage[row][col].dr_col].lake;
	else 	ild = 0;
	if (maxderneg==0 && (drainage[row][col].type=='E' || ild)) {
		/*Leave it the same*/
	}
	else {
		if (imaxderneg != SIGNAL) {
			drainage[row][col].dr_row = ro[imaxderneg];
			drainage[row][col].dr_col = co[imaxderneg];
		}
		else {
			drainage[row][col].dr_row = SIGNAL;
			drainage[row][col].dr_col = SIGNAL;
		}
	}
	drainage[row][col].type = 'R';
	drainage[row][col].lake = 0;
	/*Deallocates node memory from Lake structure*/
	Lake[il].n--;
	for (i=i_node; i<Lake[il].n; i++) {
		Lake[il].row[i] = Lake[il].row[i+1];
		Lake[il].col[i] = Lake[il].col[i+1];
	}
	Lake[il].row = realloc(Lake[il].row, Lake[il].n*sizeof(int));
	Lake[il].col = realloc(Lake[il].col, Lake[il].n*sizeof(int));
	/*Arrange everything if the node to be deleted is an outlet*/
	if (i_outlet>=0) {
		/*Remove and deallocate outlet*/
		Lake[il].n_sd--;
		for (i=i_outlet; i<Lake[il].n_sd; i++) {
			Lake[il].row_sd[i] = Lake[il].row_sd[i+1];
			Lake[il].col_sd[i] = Lake[il].col_sd[i+1];
		}
		Lake[il].row_sd = realloc(Lake[il].row_sd, Lake[il].n_sd*sizeof(int));
		Lake[il].col_sd = realloc(Lake[il].col_sd, Lake[il].n_sd*sizeof(int));
		/*Rearrange drainage in lake*/
		if (Lake[il].n_sd) {
			/*Lake nodes draining to this outlet should drain to the another (nearest) outlet:*/
			for (i=0; i<Lake[il].n; i++) {
				lrow = Lake[il].row[i]; lcol = Lake[il].col[i];
				if (drainage[lrow][lcol].dr_row == row && drainage[lrow][lcol].dr_col == col) {
					int imindist2=-1;
				float mindist2=1e24, distx, disty, dist2;
					for (k=0; k<Lake[il].n_sd; k++) {
					distx = (Lake[il].col_sd[k]-lcol) * dxdivdy;
					disty = (Lake[il].row_sd[k]-lrow);
					dist2 = distx*distx + disty*disty;
					if (dist2 < mindist2) {imindist2=k; mindist2=dist2;}
					}
					drainage[lrow][lcol].dr_row = Lake[il].row_sd[imindist2];
					drainage[lrow][lcol].dr_col = Lake[il].col_sd[imindist2];
					drainage[row][col].discharge -= drainage[lrow][lcol].discharge;
					drainage[drainage[lrow][lcol].dr_row][drainage[lrow][lcol].dr_col].discharge += drainage[lrow][lcol].discharge;
				}
				if (drainage[lrow][lcol].lake != il)
					PRINT_ERROR("'Lake' %d (%dth of %d) and 'drainage' %d don't match in node [%d][%d].", il, i, Lake[il].n, drainage[lrow][lcol].lake, lrow, lcol);
			}
		}
 		/*If this was the last outlet then change drainage of all lake nodes*/
		else {
			for (i=0; i<Lake[il].n; i++) {
				lrow = Lake[il].row[i]; lcol = Lake[il].col[i];
				drainage[lrow][lcol].dr_row = SIGNAL;
				drainage[lrow][lcol].dr_col = SIGNAL;
				drainage[row][col].discharge -= drainage[lrow][lcol].discharge;
			}
		}
	}

	/*Delete lakes with only outlets*/
	if (Lake[il].n == Lake[il].n_sd && Lake[il].n>0) {
		for (j=0; j<Lake[il].n_sd; j++) {
			drainage[Lake[il].row[j]][Lake[il].col[j]].lake = 0;
			drainage[Lake[il].row[j]][Lake[il].col[j]].type = 'R';
		}
		Deallocate_Lake(il);
	}

	if (drainage[row][col].lake<0) {
		PRINT_ERROR("[%d][%d]: lake %d", row, col, drainage[row][col].lake);
		Define_Lake(drainage[row][col].lake);
	}

	return (1);
}



int Divide_Lake (int row, int col /*lake node to be removed*/) 
{
	/*
	  This routine divides a lake if the node that is to be 
	  removed from the lake (by evaporation) is 
	  the only connection between two or more parts of the lake.
	*/

	register int 	i, j, k, il, i_node, i_outlet, 
		*new_lake_num, 
		imaxderneg, 
		local_num_lakes, 
		n_became_open, first_open, 
		first_endorheic, new_dr_row, new_dr_col;
	int 	ro[NDERS], co[NDERS];
	float 	maxderneg=0;

	il=drainage[row][col].lake;
	if (Lake[il].n<=2) return(0); /*Lake can't be split*/

	new_lake_num = calloc (Lake[il].n, sizeof(int));
	i_node = Lake_Node_Number(row, col);
	i_outlet = Lake_Outlet_Number(row, col);

	if (i_node<0) PRINT_ERROR("[%d][%d] should be a lake node!", row,col);

	/*Find the number of local lakes resulting from removing row,col from Lake*/
	/*Put a seed for the new lake numbering in a node different from row,col*/
	if (i_node==0)	new_lake_num[1]=1;
	else 		new_lake_num[0]=1;
	local_num_lakes=1;
	/*Check by pairs excluding row,col*/
	for (i=0; i<Lake[il].n; i++)  if (i!=i_node) {
		for (j=0; j<i; j++)  if (j!=i_node) {
		if (NEIGHBOURS(Lake[il].row[i],Lake[il].col[i], Lake[il].row[j],Lake[il].col[j])) {
			if (!new_lake_num[i]) new_lake_num[i] = new_lake_num[j];
			else if (new_lake_num[i] != new_lake_num[j]) {
				/*Unify two local lakes by removing new_lake_num[i]:*/
				int dying_lake_num=new_lake_num[i];
				int surviving_lake_num=new_lake_num[j];
				//if (surviving_lake_num>dying_lake_num) surviving_lake_num--;
				for (k=0; k<=i; k++) {
					if (new_lake_num[k]==dying_lake_num)
						new_lake_num[k]=surviving_lake_num;
					if (new_lake_num[k]>dying_lake_num)
						new_lake_num[k]--;
				}
				local_num_lakes --;
			}
		}
		}
		if (!new_lake_num[i]) {
			local_num_lakes ++;
			new_lake_num[i] = local_num_lakes;
		}
	}

	/*If the lake remains fully connected, then exit the routine*/
	if (local_num_lakes==1) {
		free(new_lake_num); 
		PRINT_DEBUGPLUS("Lake %d (%d nodes) does not split at [%d][%d]", il, Lake[il].n, row, col);
		return(0);
	}

	/*OTHERWISE THERE IS SPLITTING*/

	if (local_num_lakes>4) {PRINT_ERROR("\aLake %d (%d nodes) to be split in %d at [%d][%d]", il, Lake[il].n, local_num_lakes, row, col); /*exit(0);*/}

	/*Check: all lake nodes except row,col should belong to one of the new lakes*/
	for (i=j=0; i<Lake[il].n; i++) if (new_lake_num[i]) j++;
	if (j!=Lake[il].n-1) PRINT_ERROR("%d new lake nodes were expected rather than %d", Lake[il].n-1, j);

	/*Substract to row,col the drainage comming from the lake (only has sense for outlets)*/
	if (i_outlet>=0) {
		int drow, dcol;
		for (i=0; i<Lake[il].n; i++) {
			drow = drainage[Lake[il].row[i]][Lake[il].col[i]].dr_row;
			dcol = drainage[Lake[il].row[i]][Lake[il].col[i]].dr_col;
		if (drow==row && dcol==col) 
			drainage[row][col].discharge -= drainage[Lake[il].row[i]][Lake[il].col[i]].discharge;
		}
	}

	/*Distribute the nodes among the new lakes and delete the original lake*/
	for (i=1, k=0; i<=local_num_lakes; i++) {
		int new_lake;
		new_lake = New_Lake();
 		for (j=0; j<Lake[il].n; j++)
 		if (new_lake_num[j]==i) {
			k++;
			Add_Node_To_Lake   (Lake[il].row[j], Lake[il].col[j], new_lake);
			if (drainage[Lake[il].row[j]][Lake[il].col[j]].type=='E') {
				Add_Outlet_To_Lake (Lake[il].row[j], Lake[il].col[j], drainage[Lake[il].row[j]][Lake[il].col[j]].dr_row, drainage[Lake[il].row[j]][Lake[il].col[j]].dr_col, new_lake);
			}
 		}
	}
	if (k!=Lake[il].n-1) PRINT_ERROR("%d new-lake nodes were expected rather than %d", Lake[il].n-1, k);
	Deallocate_Lake(il);

	/*
	  Check opening of the new lakes after separation 
	  and determine the role of the node that is not a lake anymore.
	*/
	n_became_open=first_open=first_endorheic=0;
	for (i=nlakes-local_num_lakes+1; i<=nlakes;) {
		float lake_evap=0;
		Define_Lake(i);
		/*Substract in the outlets the water that came from nodes that now are in another lake*/
		/*for (j=0; j<Lake[i].n; j++) {
				drow = drainage[Lake[i].row[j]][Lake[i].col[j]].dr_row;
				dcol = drainage[Lake[i].row[j]][Lake[i].col[j]].dr_col;
			if (drainage[drow][dcol].lake!=i && drainage[drow][dcol].type=='E') 
				drainage[drow][dcol].discharge -= drainage[Lake[i].row[j]][Lake[i].col[j]].discharge;
		}*/
		for (j=0; j<Lake[i].n; j++) lake_evap += dx*dy * evaporation[Lake[i].row[j]][Lake[i].col[j]];
		PRINT_DEBUGPLUS("Inputwater, Evap: %f,%f  lake %d: n=%d", Lake_Input_Discharge(i), lake_evap, i, Lake[i].n);
			/*
			  If now it is open then the node must not be deleted, 
			  but it must be the outlet of the new merged open lakes
			  towards the endorheic lake(s).
			*/
			if (lake_evap < Lake_Input_Discharge(i)) {
			/*open sublake*/
				n_became_open++;
				if (n_became_open == 1) {
					first_open = i;					
				}
/*??, pero importa !!*/	else if (first_endorheic || i<nlakes) {
					/*If it is not the first open, then unify it with the first one. But at least one lake must remain endorheic.*/
					Unify_Lakes(first_open, i); 
					i--;
				}
			}
			else {
				/*At least there should be one endorheic lake.*/
				if (!first_endorheic) first_endorheic = i;
			}
			i++;
	}
	if (n_became_open>local_num_lakes) PRINT_ERROR("%d lakes became open among %d. At least lake %d remains endorheic.", n_became_open, local_num_lakes, first_endorheic);
	if (!first_endorheic) {
		first_endorheic = nlakes;
		PRINT_WARNING("no lake remained endorheic among %d. %d became open. Lake #%d is set endorheic", local_num_lakes, n_became_open, first_endorheic);
	}

	/*Find the maximum negative or null slope*/
	i = row;  j = col;   imaxderneg = new_dr_row = new_dr_col = SIGNAL;
	ro[0]=i-1, ro[1]=i-1, ro[2]=i-1, ro[3]=i,   ro[4]=i,   ro[5]=i+1, ro[6]=i+1, ro[7]=i+1;
	co[0]=j-1, co[1]=j,   co[2]=j+1, co[3]=j-1, co[4]=j+1, co[5]=j-1, co[6]=j,   co[7]=j+1;
	/*Calculate derivates*/
	for (k=0; k<NDERS; k++) {
		float dist, deriv;
		switch (k) {
			case 1: case 6: dist=dy;  break;   /*N,S*/
			case 3: case 4: dist=dx;  break;   /*E,W*/
			default:		dist=dxy; break;   /*others*/
		}
		if (IN_DOMAIN(ro[k],co[k])) {
			deriv = (topo[ro[k]][co[k]]-topo[i][j])/dist;
			/*I include deriv==0 because it can happen that the removed node is in a plane and needs to drain somewhere in that plane.*/
			if (deriv<=0 && deriv<=maxderneg && drainage[ro[k]][co[k]].lake!=first_open) {
				imaxderneg=k;
				maxderneg=deriv;
			}
		}
	}
	if (imaxderneg != SIGNAL) {
		new_dr_row = ro[imaxderneg];
		new_dr_col = co[imaxderneg];
	}
	else PRINT_ERROR("no available negative slope! first_open: %d", first_open);

	/*Change drainage in row,col*/
	if (n_became_open) {
		/*Keep it as a lake node: add it as outlet to one of the new open lakes.*/
		for (i=0; i<Lake[first_open].n; i++) {
			/*Drain the nodes of the new open lake towards this new outlet.*/
			drainage[Lake[first_open].row[i]][Lake[first_open].col[i]].dr_row = row;
			drainage[Lake[first_open].row[i]][Lake[first_open].col[i]].dr_col = col;
			/*The water of the new open lake will be transferred to this outlet inmediately after in Calculate_Discharge.*/
			drainage[row][col].discharge += MAX_2(0, drainage[Lake[first_open].row[i]][Lake[first_open].col[i]].discharge);
			PRINT_DEBUG("Open lake resulting from splitting at [%d][%d] (%f m3/s): [%d][%d]   %f m3/s", row,col, drainage[row][col].discharge, Lake[first_open].row[i], Lake[first_open].col[i], drainage[Lake[first_open].row[i]][Lake[first_open].col[i]].discharge);
		}
		Add_Node_To_Lake   (row, col, first_open);
		Add_Outlet_To_Lake (row, col, new_dr_row, new_dr_col, first_open);
		drainage[row][col].type = 'E';
		drainage[row][col].dr_row = new_dr_row;
		drainage[row][col].dr_col = new_dr_col;
	}
	else {
		/*No open sublakes => Not a lake node anymore: convert in river*/
		drainage[row][col].lake = 0;
		drainage[row][col].type = 'R';
		drainage[row][col].dr_row = new_dr_row;
		drainage[row][col].dr_col = new_dr_col;
	}
	/*Define all lakes and delete those having only outlets.*/
	/*Change drainage at the new endorheic lakes*/
	for (i=nlakes-local_num_lakes+1; i<=nlakes; i++) {
		Define_Lake(i); 
		if (Lake[i].n == Lake[i].n_sd) {
			for (j=0; j<Lake[i].n; j++) {
				drainage[Lake[i].row[j]][Lake[i].col[j]].lake = 0;
				drainage[Lake[i].row[j]][Lake[i].col[j]].type = 'R';
			}
			Deallocate_Lake(i);
		}
		if (!Lake[i].n_sd) {
			for (j=0; j<Lake[i].n; j++) {
				drainage[Lake[i].row[j]][Lake[i].col[j]].dr_row = SIGNAL;
				drainage[Lake[i].row[j]][Lake[i].col[j]].dr_col = SIGNAL;
			}
		}
	}

	free (new_lake_num);

	return(local_num_lakes);
}



float Lake_Input_Discharge (int ilake) 
{
	/*
	  The input discharge of a lake is the discharge received at its nodes from outside the lake.
	  The inputs from tributaries to the outlet(s) also contribute.
	  Also the rain in the whole lake does.
	*/

	int 	i;
	float 	input_discharge=0;

	for (i=0; i<Lake[ilake].n; i++) {
		if (drainage[Lake[ilake].row[i]][Lake[ilake].col[i]].type != 'E') 
			input_discharge += drainage[Lake[ilake].row[i]][Lake[ilake].col[i]].discharge;
		input_discharge += precipitation[Lake[ilake].row[i]][Lake[ilake].col[i]] * dx*dy;
	}
	/*Adds the discharge going directly to the outlet(s) from neigbour cells but not from the same lake*/
	for (i=0; i<Lake[ilake].n_sd; i++) {
		int ro[NDERS], co[NDERS], m, n, k;
		m = Lake[ilake].row_sd[i]; n = Lake[ilake].col_sd[i];
		ro[0]=m-1, ro[1]=m,   ro[2]=m+1, ro[3]=m,   ro[4]=m-1, ro[5]=m+1, ro[6]=m+1, ro[7]=m-1;
		co[0]=n,   co[1]=n+1, co[2]=n,   co[3]=n-1, co[4]=n+1, co[5]=n+1, co[6]=n-1, co[7]=n-1;
		for (k=0; k<NDERS; k++) {
			if (IN_DOMAIN(ro[k],co[k])) if (drainage[ro[k]][co[k]].dr_row == m && drainage[ro[k]][co[k]].dr_col == n && drainage[ro[k]][co[k]].lake != ilake) 
				input_discharge += drainage[ro[k]][co[k]].discharge; 
		}
	}
	return (input_discharge);
}



int Lake_Node_Number(int row, int col)
{
	int i, il, i_node=-1;

	/*
	  Returns the order number of the node in Lake structure (>=0).
	  Returns -1 if not in a lake or not in the spected lake.
	*/

	il = drainage[row][col].lake;
	if (il<=0) {PRINT_ERROR("[%d][%d] does not belong to a lake.", row, col); return(i_node);}
	for (i=0; i<Lake[il].n; i++) 
		if (Lake[il].row[i] == row && Lake[il].col[i] == col) 
			{i_node=i; break;}
	if (i_node==-1) {PRINT_ERROR("[%d][%d] is not in the expected lake.", row, col); return(i_node);}

	return(i_node);
}


int Lake_Outlet_Number (int row, int col)
{
	int i, il, i_outlet=-1;

	/*
	  Returns the order number of the outlet in Lake structure (>=0).
	  Returns -1 if not in a lake or not an outlet of its lake.
	*/

	il = drainage[row][col].lake;
	if (il<=0) {PRINT_ERROR("the potential outlet [%d][%d] is not in a lake.", row, col); return(i_outlet);}
	for (i=0; i<Lake[il].n_sd; i++) 
		if (Lake[il].row_sd[i] == row && Lake[il].col_sd[i] == col) 
			{i_outlet=i; break;}

	return(i_outlet);
}



float Minimum_Neg_Slope (int i, int j, int *dr_row, int *dr_col) 
{
	/*
	  Returns the maximum negative slope and its destiny node.
	  Returns 0, SIGNAL, SIGNAL if no negative derivate is found.
	*/
	int 	k, ro[NDERS], co[NDERS], iminderneg=SIGNAL;
	float 	minderneg=0;

	ro[0]=i-1, ro[1]=i,   ro[2]=i+1, ro[3]=i,   ro[4]=i-1, ro[5]=i+1, ro[6]=i+1, ro[7]=i-1;
	co[0]=j,   co[1]=j+1, co[2]=j,   co[3]=j-1, co[4]=j+1, co[5]=j+1, co[6]=j-1, co[7]=j-1;

	/*Calculate derivates*/
	for (k=0; k<NDERS; k++) {
		float dist, deriv;
		switch (k) {
			case 0: case 2: dist=dy;  break;   /*N,S*/
			case 1: case 3: dist=dx;  break;   /*E,W*/
			default:		dist=dxy; break;   /*others*/
		}
		if (IN_DOMAIN(ro[k],co[k])) {
			deriv = (topo[ro[k]][co[k]]-topo[i][j])/dist;
			if (deriv<0 && deriv<minderneg) {
				iminderneg=k;
				minderneg=deriv;
			}
		}
	}
	if (iminderneg != SIGNAL) {
		*dr_row = ro[iminderneg];
		*dr_col = co[iminderneg];
	}
	else {
		*dr_row = SIGNAL;
		*dr_col = SIGNAL;
	}

	return(minderneg);
}




int New_Lake ()
{
	/*	
	  Allocates memory for a new lake.
	  New lake is added at the end of the lake list.
	  Returns the number of the created lake.
	*/

	nlakes++;
	if (nlakes>Nx*Ny/4) PRINT_WARNING("Lots of lakes (%d)", nlakes);

	Lake = (struct LAKE_INFO *) realloc(Lake, (nlakes+1)*sizeof(struct LAKE_INFO));
	Lake[nlakes].n = 0;
	Lake[nlakes].n_sd = 0;
	Lake[nlakes].row = NULL;
	Lake[nlakes].col = NULL;
	Lake[nlakes].row_sd = NULL;
	Lake[nlakes].col_sd = NULL;

	return (nlakes);
}



int Unify_Lakes (int i_lake, int i_lake_to_delete)
{
	int i, j, k, il, ild;

	/*JOIN TWO LAKES DURING NODE IDENTIFICATION (also called from Calculate_Discharge)*/

	il = fabs((float) i_lake);
	ild = fabs((float) i_lake_to_delete);

	PRINT_DEBUGPLUS("Unifying lakes %d (%d nodes, %d exits) and %d (%d nodes, %d exits, this lake will be deleted) out of %d.", i_lake, Lake[il].n, Lake[il].n_sd, i_lake_to_delete, Lake[ild].n, Lake[ild].n_sd, nlakes);

	Lake[il].n	+= Lake[ild].n;
	Lake[il].n_sd += Lake[ild].n_sd;
	Lake[il].row	= realloc(Lake[il].row, Lake[il].n*sizeof(int));
	Lake[il].col	= realloc(Lake[il].col, Lake[il].n*sizeof(int));
	Lake[il].row_sd = realloc(Lake[il].row_sd, Lake[il].n_sd*sizeof(int));
	Lake[il].col_sd = realloc(Lake[il].col_sd, Lake[il].n_sd*sizeof(int));
	/*Resort nodes and saddles in increasing order of altitude, it's sometimes used!*/
	/*This algorithm assumes that both unifying lakes had already their nodes and exits sorted by elevation in the Lake structure*/
	for (i=0; i<Lake[ild].n; i++) {
		if (Lake[il].n!=Lake[ild].n) for (j=0; j<Lake[il].n-Lake[ild].n+i; j++) {
		if (topo[Lake[ild].row[i]][Lake[ild].col[i]] < topo[Lake[il].row[j]][Lake[il].col[j]]) {
			/*Shift upwards the lake nodes above this one to make romm for it*/
			for (k=Lake[il].n-Lake[ild].n+i; k>j; k--) {
				Lake[il].row[k] = Lake[il].row[k-1];
			Lake[il].col[k] = Lake[il].col[k-1];
			}
			/*Now transfer the deleted-lake node to that place*/
			Lake[il].row[j] = Lake[ild].row[i];
			Lake[il].col[j] = Lake[ild].col[i];
			break;
		}
		else if (j==Lake[il].n-Lake[ild].n+i-1) {
			Lake[il].row[j+1] = Lake[ild].row[i];
			Lake[il].col[j+1] = Lake[ild].col[i];			
		}
		} 
		else {
			Lake[il].row[i] = Lake[ild].row[i];
			Lake[il].col[i] = Lake[ild].col[i];
		}
	}
	for (i=0; i<Lake[ild].n_sd; i++) {
		if (Lake[il].n_sd!=Lake[ild].n_sd) for (j=0; j<Lake[il].n_sd-Lake[ild].n_sd+i; j++) {
		if (OUT_DOMAIN(Lake[il].row_sd[j], Lake[il].col_sd[j])) PRINT_DEBUG("\a$$$$$$$$$$$$$$$$$$$$ %d   %d  %d   [%d][%d]", il, j, Lake[il].n_sd, Lake[il].row_sd[j], Lake[il].col_sd[j]);
		if (topo[Lake[ild].row_sd[i]][Lake[ild].col_sd[i]] < topo[Lake[il].row_sd[j]][Lake[il].col_sd[j]]) {
			/*Shift upwards the lake nodes above this one to make room for it*/
			for (k=Lake[il].n_sd-Lake[ild].n_sd+i; k>j; k--) {
				Lake[il].row_sd[k] = Lake[il].row_sd[k-1];
			Lake[il].col_sd[k] = Lake[il].col_sd[k-1];
			}
			/*Now transfer the deleted-lake node to that place*/
			Lake[il].row_sd[j] = Lake[ild].row_sd[i];
			Lake[il].col_sd[j] = Lake[ild].col_sd[i];
			break;
		}
		else if (j==Lake[il].n_sd-Lake[ild].n_sd+i-1) {
			Lake[il].row_sd[j+1] = Lake[ild].row_sd[i];
			Lake[il].col_sd[j+1] = Lake[ild].col_sd[i];			
		}
		}
		else {
			Lake[il].row_sd[i] = Lake[ild].row_sd[i];
			Lake[il].col_sd[i] = Lake[ild].col_sd[i];
		}
	}

	/*Changes the associated drainage lake signal*/
	for (i=0; i<Lake[ild].n; i++) drainage[Lake[ild].row[i]][Lake[ild].col[i]].lake = i_lake;

	/*Check: nodes and outlets of the extended lake il should now be sorted, complete, and not repeated*/
	for (j=0; j<Lake[il].n; j++) {
		if (OUT_DOMAIN(Lake[il].row[j], Lake[il].col[j])) PRINT_ERROR("\a$@ lake:%d   %d/%d   [%d][%d]", il, j, Lake[il].n, Lake[il].row[j], Lake[il].col[j]);
		if (j>0) if (topo[Lake[il].row[j]][Lake[il].col[j]] < topo[Lake[il].row[j-1]][Lake[il].col[j-1]]) PRINT_ERROR("\a$#Lake nodes not well sorted:%d   %d/%d   [%d][%d]", il, j, Lake[il].n, Lake[il].row[j], Lake[il].col[j]);
		if (drainage[Lake[il].row[j]][Lake[il].col[j]].lake != i_lake) PRINT_ERROR("\a$$Lake drainage badly defined:%d   %d/%d   [%d][%d]", il, j, Lake[il].n, Lake[il].row[j], Lake[il].col[j]);
		for (i=0; i<j; i++) {if (Lake[il].row[i]==Lake[il].row[j] && Lake[il].col[i]==Lake[il].col[j]) PRINT_ERROR("\a$&Lake nodes repeated:%d   %d/%d   [%d][%d]", il, j, Lake[il].n, Lake[il].row[j], Lake[il].col[j]);}
	}
	for (j=0; j<Lake[il].n_sd; j++) {
		if (OUT_DOMAIN(Lake[il].row_sd[j], Lake[il].col_sd[j])) PRINT_ERROR("\a$* lake:%d   %d/%d   [%d][%d]", il, j, Lake[il].n_sd, Lake[il].row_sd[j], Lake[il].col_sd[j]);
		if (j>0) if (topo[Lake[il].row_sd[j]][Lake[il].col_sd[j]] < topo[Lake[il].row_sd[j-1]][Lake[il].col_sd[j-1]]) PRINT_ERROR("\a$#Lake nodes not well sorted:%d   %d/%d   [%d][%d]", il, j, Lake[il].n, Lake[il].row_sd[j], Lake[il].col_sd[j]);
		if (drainage[Lake[il].row_sd[j]][Lake[il].col_sd[j]].lake != i_lake) PRINT_ERROR("\a$$Lake outlet drainage badly defined:%d   %d/%d   [%d][%d]", il, j, Lake[il].n, Lake[il].row_sd[j], Lake[il].col_sd[j]);
		for (i=0; i<j; i++) {if (Lake[il].row_sd[i]==Lake[il].row_sd[j] && Lake[il].col_sd[i]==Lake[il].col_sd[j]) PRINT_ERROR("\a$&Lake outlets repeated:%d   %d/%d   [%d][%d]", il, j, Lake[il].n, Lake[il].row_sd[j], Lake[il].col_sd[j]);}
	}
	
	Deallocate_Lake(ild);

	return (1);
}




/******************************************************************************/
int Calculate_Precipitation_Evaporation ()
{
	/*
	  calculate the solid & liquid precipitation and the evaporation matrixes in m/s
	*/
	int row, col, i, j;

 	PRINT_DEBUG("Calculating precipitation");
	for (row=0; row<Ny; row++) for (col=0; col<Nx; col++) {precipitation[row][col]=0; precipitation_snow[row][col]=0; evaporation[row][col]=evaporation_ct;}
	switch (hydro_model) {
		case 1: {
		float altitude; int il;
		for (row=0; row<Ny; row++) for (col=0; col<Nx; col++) {
 			if (precipitation_file[row][col]>=0) {
 			precipitation[row][col] = precipitation_file[row][col];
 			}
 			else {
 			altitude = topo[row][col];
 			il=drainage[row][col].lake;
 			if (il) {
 				// Return the highest lake elevation. Lake nodes sorted in Unify_Lakes()
 				altitude = topo[Lake[il].row[Lake[il].n-1]][Lake[il].col[Lake[il].n-1]];
 				/*Sea*/
 				IF_LAKE_IS_SEA(il) altitude = sea_level;
 			}
 			precipitation[row][col] = MAX_2((rain+Krain*altitude),  0);
 			if (CXrain) precipitation[row][col] *= MAX_2 (0, 1 + (xmin+col*dx-(xmax+xmin)/2)/CXrain);
 			if (CYrain) precipitation[row][col] *= MAX_2 (0, 1 + (ymax-row*dy-(ymax+ymin)/2)/CYrain);
 			}
		}
		break;
		}
		case 2: {
		for (row=0; row<Ny; row++) for (col=0; col<Nx; col++) {
	 			precipitation[row][col] = Orographic_Precipitation_with_local_slope(row, col, Krain, windazimut);
		}
 		break;
   		}
			case 3: {
			Orographic_Precipitation_Evaporation_conservative (Krain, windazimut, relative_humidity);
		break;
		}
		case 4: {
			Orographic_Precipitation_Evaporation_conservative (Krain, windazimut, relative_humidity);
			
		}
	}

	/*Smooth out rain (based on upwind rain)*/
		if (hydro_model == 2 || hydro_model == 3 || hydro_model == 4) if (CXrain) {
			float windvel=Krain, windvelx, windvely /*[m/s]*/, DL=CXrain, factor, precip_here;
			int i, j;
		float **precipitation_aux;
		precipitation_aux = alloc_matrix(Ny, Nx);
			windvelx = windvel*sin(windazimut/180*3.1415927);
			windvely = windvel*cos(windazimut/180*3.1415927);
			factor = 2/DL/sqrt(3.1415927);
		for (row=0; row<Ny; row++) for (col=0; col<Nx; col++) {
			precip_here=0;
 		/*integrates upwind to smooth (see Roe et al., 2003, JGR)*/
 		if (fabs(windvelx)>fabs(windvely)) {
 			for (j=col; ; ) {
 			i  = row - (int) ((float)(j-col)*windvely/windvelx);
 			if (OUT_DOMAIN(i,j)) break;
 			precip_here += precipitation[row][col] * exp(-pow((j-col)*dx/DL, 2)-pow((i-row)*dy/DL, 2)) * sqrt(dx*dx+dy*dy*cos(windazimut/180*3.1415927)*cos(windazimut/180*3.1415927));
			j -= windvelx/fabs(windvelx);
			}
 		}
 		else {
 			for (i=row; ; ) {
 			j  = col - (int) ((float)(i-row)*windvelx/windvely);
 			if (OUT_DOMAIN(i,j)) break;
			precip_here += precipitation[row][col] * exp(-pow((j-col)*dx/DL, 2)-pow((i-row)*dy/DL, 2)) * sqrt(dx*dx*sin(windazimut/180*3.1415927)*sin(windazimut/180*3.1415927)+dy*dy);
 			i += windvely/fabs(windvely);
			}
 		}
 		precipitation_aux[row][col] = precip_here * factor;
		}
 		for (row=0; row<Ny; row++) for (col=0; col<Nx; col++) {precipitation[row][col]=precipitation_aux[row][col];}
		free_matrix(precipitation_aux, Ny);
	}

	if (hydro_model==4) land_surface_process();
	
	/*Separate snow and rain according to ground temperature*/
	if (K_ice_eros) {
		for (row=0; row<Ny; row++) for (col=0; col<Nx; col++) {
 		float precip_here, Ts, temp_gradual_change=3 /*C*/;
 		precip_here = precipitation[row][col];
		Ts =  TEMPERATURE_ICE(topo[row][col]+ice_thickness[row][col]) - TEMP_FREEZE_WATER;
 		if (Ts<-temp_gradual_change) {
 			precipitation[row][col] = 0;
  			precipitation_snow[row][col] = precip_here;
		}
		if (Ts>=-temp_gradual_change && Ts<=+temp_gradual_change) {
 			precipitation[row][col] = (Ts+temp_gradual_change)/(2*temp_gradual_change) * precip_here;
  			precipitation_snow[row][col] = (temp_gradual_change-Ts)/(2*temp_gradual_change) * precip_here;
		}
		if (Ts>+temp_gradual_change) {
 			precipitation[row][col] = precip_here;
 			precipitation_snow[row][col] = 0;
		}
 		}
	}
	
	{
		float total_rain_test=0, total_evap_test=0;
		for (i=0; i<Ny; i++) for (j=0; j<Nx; j++) {
			total_rain_test += precipitation[i][j]; total_evap_test += evaporation[i][j];
			if (fabs(precipitation[i][j]*secsperyr)>1e3 || fabs(evaporation[i][j]*secsperyr)>1e3 || isnan(precipitation[i][j]) || isnan(evaporation[i][j]) ) 
				PRINT_ERROR("\aPrecipitation: [%d][%d]  P,E = %.4f , %.4f m/yr", i, j, precipitation[i][j]*secsperyr, evaporation[i][j]*secsperyr);
		}
		PRINT_DEBUG("Total P, E = %.2e, %.2e m3/s", total_rain_test*dx*dy, total_evap_test*dx*dy);
	}

	return (1);
}


float Orographic_Precipitation_with_local_slope (int i, int j, float windvel, float windazimut)
{
	/*
	  Calculates precipitation at a given location.
	  Returns that precipitation in m/s. 
	  This is a 2D version of Roe et al., 2003, JGR. See also Masek et al., 1994, JGR.; Purves & Hulton, 2000, IJC.
	*/
	float 	es0=6.1078 /*mb*/, a=17.27, b=35.86 /*centigrades*/, 
		vapor_convergence, alpha0, alpha1=110 /*m yr-1 / (m s-1)*/, Ts, 
		slopex, slopey, esat, DX = 10e3 /*m*/, topoC;
		float	windvelx, windvely /*[m/s]*/;
	int 	il, li, lj;
		windvelx = windvel*sin(windazimut/180*3.1415927);
		windvely = windvel*cos(windazimut/180*3.1415927);

	/*Calculate the maximum between topogrpahy and water surface:*/
	topoC = topo[i][j];   
	if (il=drainage[i][j].lake) {
		/*Lake[il].alt doesn't work here, it's calculated later in the main loop!*/
		topoC = topo[Lake[il].row[Lake[il].n-1]][Lake[il].col[Lake[il].n-1]];
		/*Except for the sea*/
		IF_LAKE_IS_SEA(il) topoC = sea_level;
	}
	else topoC = MAX_2(topoC, sea_level);

	/*Surface (ground) temperature*/
	Ts = TEMPERATURE(topoC); 
	/*Tetens formula for Clausius-Clapeyron, giving the saturation vapor pressure in the surface*/
	esat = es0 * exp(a*(Ts-TEMP_FREEZE_WATER)/(Ts-b));
	li=i; lj=j;
	if (i<=0) 	li=1;
	if (i>=Ny-1) 	li=Ny-2;
	if (j<=0) 	lj=1;
	if (j>=Nx-1) 	lj=Nx-2;
	slopex = (topo[li][lj+1] - topo[li][lj-1]) / dx / 2;
	slopey = (topo[li-1][lj] - topo[li+1][lj]) / dy / 2;
	if (topoC < sea_level) slopex=slopey=0;
	alpha0 = rain / es0; /*arbitrary value so that vapor_convergence=rain at plains or in absence of wind, at 0 Centigrade*/
	alpha1 = 110 / es0;  /*m yr-1 / (m s-1)*/;
	/*steam budget*/
	vapor_convergence = (alpha0 + alpha1/secsperyr * (windvelx*slopex + windvely*slopey)) * esat;
	return ((vapor_convergence>0)? vapor_convergence : 0);
}



int Orographic_Precipitation_Evaporation_conservative (float windvel, float windazimut, float relative_humidity)
{
	/*
	  Returns precipitation and evaporation in m/s.
	  Inspired on Roe et al. (2003, JGR) and Masek et al. (1994, JGR). 
	  Modified by Garcia-Castellanos, 2007, EPSL. 
	  Improved water-conservative version described in Garcia-Castellanos & Jimenez-Munt, 2015.
	  It calculates the max. water content in every column as a function of temperature. 
	  Evaporation depends on wind velocity.
	*/
	int 	i, j, ib, jb, istart=0, iincr=0, jstart=0, jincr=0;
	float 	Wcol, Wmax, 	/*Precipitable Water content in a column, and maximum precipitable water content usually in the order of mm and cm*/
		z, dz=50, dtwind;
	float 	windvelx, windvely; /*[m/s]*/
	float 	**done;

	if (!windvel) return (0);
	windvelx = windvel*sin(windazimut/180*3.1415927);
	windvely = windvel*cos(windazimut/180*3.1415927);

	done = alloc_matrix(Ny, Nx);

	/*time spent by air in a cell*/
	if (fabs(windvelx)>fabs(windvely))	
		dtwind = dx/fabs(windvelx); 
	else
		dtwind = dy/fabs(windvely); 
	if (windvelx>0)	{jstart=0;	jincr=+1;} else {jstart=Nx-1; jincr=-1;}
	if (windvely<0)	{istart=0;	iincr=+1;} else {istart=Ny-1; iincr=-1;}
	
	/*Start from all upwind (windward) boundary cells*/
	/*vertical boundary*/
	// Start from upwind boundary and advect along wind direction
	for (ib=istart; ib>=0 && ib<Ny; ib+=iincr) {
		//Scans along vertical boundary
		/*Incoming water content. Precipitable water usually in the order of mm and cm*/
		Wcol = relative_humidity * max_water_in_air_colum(ib,jstart);
		/*Go downwind (leeward) from each cell in the vertical boundary*/
		if (fabs(windvelx)>fabs(windvely)) 
			for (j=jstart; j>=0 && j<Nx; j+=jincr) {
				// Advect along wind direction starting from each upwind boundary cell
			i = ib - rint((j-jstart)/windvelx*windvely);
			if OUT_DOMAIN(i,j) break;
			if (!done[i][j]) Precipitation_Evaporation_at_cell (i, j, &Wcol, windvel, dtwind);
			done[i][j]=1;
 			}
		else 
			for (i=ib; i>=0 && i<Ny; i+=iincr) {
			j = jstart - rint((i-ib)/windvely*windvelx);
			if OUT_DOMAIN(i,j) break;
			if (!done[i][j]) Precipitation_Evaporation_at_cell (i, j, &Wcol, windvel, dtwind);
			done[i][j]=1;
			}
	}
	/*horizontal boundary (DO NOT REPEAT i,j=0,0)!*/
	for (jb=jstart; jb>=0 && jb<Nx; jb+=jincr) {
		/*Incoming water content*/
		Wcol = relative_humidity * max_water_in_air_colum(istart,jb);
		/*Go downwind (leeward) from each cells in the horizontal boundary*/
		if (fabs(windvelx)>fabs(windvely)) 
			for (j=jb; j>=0 && j<Nx; j+=jincr) {
			i = istart - rint((j-jb)/windvelx*windvely);
			if OUT_DOMAIN(i,j) break;
			if (!done[i][j]) Precipitation_Evaporation_at_cell (i, j, &Wcol, windvel, dtwind);
 			done[i][j]=1;
			}
		else 
			for (i=istart; i>=0 && i<Ny; i+=iincr) {
			j = jb - rint((i-istart)/windvely*windvelx);
			if OUT_DOMAIN(i,j) break;
			if (!done[i][j]) Precipitation_Evaporation_at_cell (i, j, &Wcol, windvel, dtwind);
			done[i][j]=1;
			}
	}
	/*Check results*/
	for(i=0; i<Ny; i++)  for(j=0; j<Nx; j++) {
		if (!done[i][j]) PRINT_ERROR("[%d][%d] not done! ", i, j);
		if (fabs(precipitation[i][j]*secsperyr)>1e3 || fabs(evaporation[i][j]*secsperyr)>1e3 || isnan(precipitation[i][j]) || isnan(evaporation[i][j])) 
			PRINT_ERROR("\aPrecipitation: [%d][%d] P,E = %.4f , %.4f m/yr  %.2f h  windvel=%.2f m/s, %.2f m/yr, Wmax=%f m", i, j, precipitation[i][j]*secsperyr, evaporation[i][j]*secsperyr, dtwind/3600, windvel, rain*secsperyr, Wmax);
	}

	free_matrix(done, Ny);
	return (1);
}


int Precipitation_Evaporation_at_cell (int i, int j, float *Wcol, float windvel, float dtwind)
{
	float 	beta=.3;  	/*!! better beta = 0.3, pero en sumision de altiplano esta con 1*/
	float 	Wmax; 		/*Water content in a column, and maximum water content*/
	float 	et_recycle; // Total ET [m/s] over each grid cell from previous time step for moisture recycling
	/*Calculate P,E based on the water Wcol coming into this column from the upwind column*/
	
	/*Max. amount of water in the column to reach saturation:*/
 	Wmax=max_water_in_air_colum(i,j);
 	W_grid[i][j] = (*Wcol);
 	Wmax_grid[i][j] = Wmax;
	PRINT_DEBUG("[%d][%d]  rain=%.2e mm/yr, Wmax=%.2e  Wcol=%.2e m", i, j, rain*secsperyr*1e3, Wmax, *Wcol);
	if (Wmax>1e-5) {
			/*Precipitation is proportional to the quotient between the water Wcol coming into this column from the upwind column and the Wmax*/
			relHumidity[i][j] = (*Wcol)/Wmax;
			precipitation[i][j] = rain * (*Wcol) / Wmax;

			/*limit precipitation to at least 0*/
			precipitation[i][j] = MAX_2(0, precipitation[i][j]);
			/*limit precipitation to at least the excess water*/
			/*
			This assumption cannot be used for small grid size.
			Otherwise, very large local precipitation can occur.
			ChaoWang202008181649
			*/
			// precipitation[i][j] = MAX_2((*Wcol-Wmax)/dtwind, precipitation[i][j]);
	 		/*limit precipitation to at most the available water in column*/
   		precipitation[i][j] = MIN_2(*Wcol/dtwind, precipitation[i][j]);

   		/*Evaporation is proportional to water deficit and windvel*/
			evaporation[i][j] = evaporation_ct * (1+beta*windvel) * (Wmax-*Wcol)/Wmax;

			/*limit evaporation to at least 0*/
			evaporation[i][j] = MAX_2(0, evaporation[i][j]);
			/*
			Lake evaporation can be smaller than this rate if
			precipitation plus upstream discharge input to the lake
			is smaller than evaporation.
			This won't be an issue if hydro_model == 4 is used.
			ChaoWang202008072231
			*/
			et_recycle = evaporation[i][j];
		// Use calculated actual ET from previous time step land surface modeling
		// This is only used to contribute to ET recycle and
		// precipitation over downwind grid cell.
		// ChaoWang202007161114
		if (hydro_model == 4 && idt_eros > 0) et_recycle = MAX_2(0, et_tot_ant[i][j]);
			/*calculate change in water content in column (in m of water)*/
			*Wcol -= precipitation[i][j] * dtwind;
			if (lake_former_step[i][j]) {
			/*
			This is problematic because actual lake evaporation is limited
			by the amount of precipitation plus upstream discharge input to lake.
			ChaoWang202007301815
			*/
			// *Wcol += evaporation[i][j] * dtwind;
			}
		*Wcol += et_recycle * dtwind; // ChaoWang202007241726
	}
	else {
		precipitation[i][j] = 0;
		evaporation[i][j] = evaporation_ct * (1+beta*windvel);
	}
	// Previous time step lake evaporation
	// This will be overwritten by the current time step evaporation calculation
	// from the land surface model
	// if (hydro_model == 4) evaporation[i][j] = MAX_2(0, elk[i][j]); // ChaoWang202007241728
	if (fabs(precipitation[i][j]*secsperyr)>1e3 || fabs(evaporation[i][j]*secsperyr)>1e3 || isnan(precipitation[i][j]) || isnan(evaporation[i][j])) 
		PRINT_ERROR("\aPrecipitation: [%d][%d] P,E = %.4f , %.4f m/yr  %.2f h  windvel=%.2f m/s, %.2f m/yr, Wmax=%f m", i, j, precipitation[i][j]*secsperyr, evaporation[i][j]*secsperyr, dtwind/3600, windvel, rain*secsperyr, Wmax);
	return(1);
}


float max_water_in_air_colum (int i, int j)
{
	/*calculate maximum possible water content (saturation) in column i,j in m*/
	float 	es0=610.78 /*Pa*/, 
		L, Rv=461.5, /*J/kg/K*/
		esat, topoC;
	int 	il;
	float 	Wmax=0, 	/*Water content in a column, and maximum water content*/
		z, dz=50;

	/*Calculate the maximum between topogrpahy and water surface:*/
	topoC = topo[i][j];   
	if (il=drainage[i][j].lake) {
		/*Lake[il].alt doesn't work here, it's calculated later in the main loop!*/
		topoC = topo[Lake[il].row[Lake[il].n-1]][Lake[il].col[Lake[il].n-1]];
		/*Except for the sea*/
		IF_LAKE_IS_SEA(il) topoC = sea_level;
	}
	else topoC = MAX_2(topoC, sea_level);

	for (z=0; z<10000; z+=dz) {
		float temp_air;
		temp_air = TEMPERATURE_AIR(topoC, z);
		// Use temperature record if available
		// ChaoWang202007161123
		float Tmaa_zr;
		Tmaa_zr = T_mean_annual_file[idt_eros][1];
		if (Tmaa_zr>-9998){
			temp_air = TEMPERATURE_REFZ2AIR(Tmaa_zr,topoC,z);
		}
		L = 2.4995e6+(temp_air-TEMP_FREEZE_WATER)*2359;
		esat = es0*exp(L/Rv*(1/TEMP_FREEZE_WATER - 1/temp_air));
		Wmax += esat/temp_air/Rv/denswater*dz; /*Blocks: m of water*/
		if (isinf(Wmax)) PRINT_ERROR("Wmax is infinite. %d topoC=%.2f %.2f esat=%.2e temp_air=%.2e %.2e %.2e %.2e ", il, topoC, z, esat, temp_air, L, Rv, denswater);
		if (isinf(Wmax) && il) PRINT_ERROR("Lake.alt=%.2f m", Lake[il].alt);
	}
	if (Wmax<1e-5) PRINT_DEBUG("Water content in column only Wmax=%.2e m; topo=%.2f", Wmax, topoC);
	if (Wmax>1e-1) PRINT_WARNING("Water content in column is too high: Wmax=%.2e m", Wmax);
	return (Wmax);
}


// ET from grid cell with type R or E
// ChaoWang202004241004
float et_riparian_hillslope(float Qw,float dd,int row,int col){
	/*
	Qw: discharge at current cell
	dd: distance to down stream cell
	row, col: index of current cell
	*/
	float wc, wr, Ac, Ar, et;
	/*
	wc: channel width
	wr: riparian width
	Ac: channel plain view area
	Ar: riparian plain view area
	et: ET from riparian and channel areas
	*/
	const float kw = 1.1, aw = 0.5, kr = 16.0, ar = 0.9;
	// Regression coefficients from (Berry et al., 2019 G3)
	wc = width_channel(Qw, kw, aw);
	wr = width_riparian(wc, kr, ar);
	Ac = wc*dd;
	Ar = wr*dd;
	// eth and etr must have unit m/s, please double check
	et = eth[row][col]*(dy*dy-Ar)+etr[row][col]*Ar;
	// Why is ET from riparian and hillslope area deducted from river discharge?
	// This is water balance in long time scale. ChaoWang202005301744
	// River discharge in this model is actually all the runoff (water) from
	// each grid cell. ChaoWang202007281754
	if (et > Qw) et = Qw;
	return (et/dx/dy); // Unit: m/s
}

// Land surface processes for TISC-LSM
// ChaoWang202004211709
int land_surface_process(){
	float Tmaa_zr; // Mean annual temperature anomaly [degC] at reference elevation for the study region
	float Rmmt; // Annual temperature range [degC]
	float Pma_zr; // Mean annual precipitation [mm/yr] at reference elevation for the study region
	// Calculate mean annual evapotranspiration [m/s] for each erosion time step
	Tmaa_zr = T_mean_annual_file[idt_eros][1];
	Rmmt = T_mean_annual_file[idt_eros][2];
	Pma_zr = P_mean_annual_file[idt_eros][1];
	evapotranspiration_grid(Tmaa_zr, Rmmt, Pma_zr);
	return (1);
}


void evapotranspiration_grid(float Tmaa_zr, float Rmmt, float Pma_zr){
	int row, col, il, imon, imontot;
	const float rheight = 1000.0; // Reference elevation of mean annual temperature [m]
	float Tmaz; // Mean annual temperature at elevation z [degC];
	float Tmmz[12]; // Mean monthly temperature at elevation z [degC];
	float Tdc[12]; // Daily temperature variation [degC]
	// Idt, Sdt: Regression coefficients of daily temperature variation
	float Idt[12] = {22.5, 25.7, 28.8, 32.0, 33.7, 35.3,
					 37.0, 34.0, 31.0, 28.0, 26.2, 24.3};
	float Sdt[12] = {0.004, 0.005, 0.006, 0.007, 0.012, 0.018,
					 0.023, 0.019, 0.014, 0.010, 0.008, 0.006};
	float Tmzmax[12], Tmzmin[12]; // Maximum and minimum temperatures for each month [degC]
	float Pmaz; // Mean annual precipitation [m/s] at elevation z;
	// Rn: Solar radiation
	// pressure: air pressure
	// PET: potential ET
	float Rn, et_eq_0, et_pt_0, pressure, PET;
	// Kcln: the atmospheric clearness (turbidity) coefficient
	// Kcln ranges from less than 0.5 for extremely turbid,
	// dusty or polluted air to 1.0 for clean air.
	// K_rs: the adjustment coefficient for calculation of "measured" solar radiation
	// (typically 0.16 to 0.19)
	float T_avg, T_max, T_min, RHmean, windcell, Kcln, K_rs, cellalbedo, elev_cell,
		  slope, Lat_avg, Azimuth;
	/*
	elk: lake evaporation [L/T]
	etr: riparian evapotranspiration [L/T]
	eth: hillslope evapotranspiration [L/T]
	The unit originally in TISC is m/s
	*/
	// Set default values
	windcell = Krain; Kcln = 0.8; K_rs = 0.19; cellalbedo = 0.23;
	arcslope(Ny, Nx, dy, dx, topo, slope_grid);
	arcaspect(Ny, Nx, dy, dx, topo, azimuth_grid);
	for (row=0; row<Ny; row++)
		for (col=0; col<Nx; col++){
			et_eq[row][col] = 0; et_pt[row][col] = 0; et_a[row][col] = 0;
			elk[row][col] = 0; etr[row][col] = 0; eth[row][col] = 0;
			elev_cell = topo[row][col];
			il = drainage[row][col].lake;
			if (il) {
				elev_cell = topo[Lake[il].row[Lake[il].n-1]][Lake[il].col[Lake[il].n-1]];
				IF_LAKE_IS_SEA(il) elev_cell = sea_level;
			}
			
			Tmaz = TEMPERATURE_REFZ2Z(Tmaa_zr,elev_cell);
			// Should we use modelled cell precipitation here?
			// No. The daily temperature variation was regressed on this precipitation.
			Pmaz = PRECIPITATION_REFZ2Z(Pma_zr,elev_cell); // [m/s]
			Pmaz_grid[row][col] = Pmaz;
			// RHmean can be from precipitation model
			// RHmean = relHumidity[row][col];
			// RHmean = 0.5;
			// Initial Lat_avg can be stored in and read from *.ZINI file
			// Lat_avg for one cell can change due to tectonics
			// Temporally assume 111 km per latitude and y = 0 is at the Equator
			Lat_avg = 36 + (ymax-row*dy)/111E3;
			slope = slope_grid[row][col];
			Azimuth = azimuth_grid[row][col];
			for (imon=0; imon<12; imon++) {
				Tavg_mon[imon][row*Nx+col] = 0;
				Tmax_mon[imon][row*Nx+col] = 0;
				Tmin_mon[imon][row*Nx+col] = 0;
				RHmean_mon[imon][row*Nx+col] = 0;
				Rn_mon[imon][row*Nx+col] = 0;
				et_eq_mon[imon][row*Nx+col] = 0;
				et_pt_mon[imon][row*Nx+col] = 0;
				imontot = idt_eros*12+imon; // Counter of months along the erosion time step
				Tmmz[imon] = Tmaz + Rmmt*sin(2.0*pi/12.0*(imon+1.0+8.5));
				RHmean = 0.5 + 0.3*sin(2.0*pi/12.0*(imon+1.0+8.5));
				Tdc[imon] = 0.5*(Idt[imon] - Sdt[imon]*Pmaz*secsperyr*1000); // Daily temperature variation
				Tmzmax[imon] = Tmmz[imon] + Tdc[imon];
				Tmzmin[imon] = Tmmz[imon] - Tdc[imon];
				T_avg = Tmmz[imon]; T_max = Tmzmax[imon]; T_min = Tmzmin[imon];
				// T_avg = 20; T_max = 30; T_min = 10; // For testing only
				Rn = netSolarRadiation(DOY_month[imon], T_max, T_min,
							RHmean, windcell, Kcln, K_rs, cellalbedo, Lat_avg, 
							elev_cell, slope, Azimuth);
				et_eq_0 = evaporation_penman_equilibrium(Rn,T_avg,elev_cell);
				et_pt_0 = evapotranspiration_potential(DOY_month[imon],T_avg,Lat_avg);
				et_eq[row][col] = et_eq[row][col]+1.0/12.0*et_eq_0;
				et_pt[row][col] = et_pt[row][col]+1.0/12.0*et_pt_0;
				Tavg_mon[imon][row*Nx+col] = T_avg;
				Tmax_mon[imon][row*Nx+col] = T_max;
				Tmin_mon[imon][row*Nx+col] = T_min;
				RHmean_mon[imon][row*Nx+col] = RHmean;
				Rn_mon[imon][row*Nx+col] = Rn;
				et_eq_mon[imon][row*Nx+col] = et_eq_0;
				et_pt_mon[imon][row*Nx+col] = et_pt_0;
			}
			et_a[row][col] = evapotranspiration_actual(Pmaz, et_pt[row][col]); // precipitation[row][col]
			if (il){
				elk[row][col] = et_eq[row][col];
			}
			else {
				etr[row][col] = et_pt[row][col];
				eth[row][col] = et_a[row][col];
			}
			evaporation[row][col] = elk[row][col];
		}
}


// Equilibrium free water / lake evaporation
// from equation (6B3.10), p. 265, (Dingman, 2015)
// ChaoWang202004211100
// equation and units checked
float evaporation_penman_equilibrium(float Rn,float Tair,float elev){
	float slope_es, Psych, Pair, Epe;
	/*
	Rn: Net shortwave plus longwave solar radiation [MJ/m^2/day] (p. 261, Dingman, 2015)
	Tair: Air temperature [degC] (p. 254, Dingman, 2015)
	elev: Elevation [m]
	denswater: [kg/m^3]
	lheat: [MJ/kg] (p. 116, Dingman, 2015)
	*/
	slope_es = slope_es_fcn(Tair); // [kPa/degC] (p. 254, Dingman, 2015)
	Pair = Pair_fcn(elev); // Atmospheric pressure at elevation elev [kPa] (p. 256, Dingman, 2015)
	Psych = Psych_fcn(Pair); // Psychometric constant [kPa/degC] (eq 6.21, p. 256, Dingman, 2015)
	Epe = slope_es*Rn/(denswater*lheat*(slope_es+Psych)); // derived unit [m/day]
	return (Epe/24.0/3600.0); // [m/s]
}

// Potential evapotranspiration
// ChaoWang202004211630
// equation and units checked
float evapotranspiration_potential(int DOY,float Ta,float dLat){
	// Ta: mean daily temperature [degC] (p. 294, Dingman, 2015)
	// dLat: latitude [deg]
	float daylength, d_dec, rLat=dLat*pi/180.0, d_ws, peth;
	d_dec = 0.409*sin(2.0*pi*DOY/365.0-1.39);
	d_ws = acos(-tan(rLat)*tan(d_dec));
	daylength = 24/pi*d_ws; // [hr], (Allen et al 1998, p. 48, eq. 34), DPWM/RefET.cpp
	// Hamon (1963) estimated daily PET. (p. 294, Dingman, 2015)
	peth = 29.8*daylength*e0(Ta)/(Ta+273.2); // [mm/day] (p. 294, Dingman, 2015)
	return (peth/1000.0/24.0/3600.0); // [m/s]
}

// Actual evapotranspiration
// ChaoWang202004211648
// equation and units checked
float evapotranspiration_actual(float P, float PET){
	float w=2.0, et;
	// Budyko-type equation, equation (6.72), p. 298, (Dingman, 2015)
	et = P/pow(1+pow(P/PET,w),1.0/w); // same unit as P
	return (et);
}

// Channel width from discharge (eq. (5), Berry et al., 2019)
// ChaoWang202004231716
float width_channel(float Qw, float kw, float aw){
	/*
	Qw: stream discharge
	kw, aw: regression coefficients
	kw = 1.1, aw = 0.5 (used by Doe Han)
	*/
	return (kw*pow(Qw,aw));
}

// Riaprian width from channel width (eq. (13), Berry et al., 2019)
// ChaoWang202004231721
float width_riparian(float wc, float kr, float ar){
	/*
	wc: channel width
	kr, ar: regression coefficients
	kr = 16, ar = 0.9 (in Berry et al., 2019)
	*/
	return (kr*pow(wc,ar));
}


/*
 Aspect using ArcGIS algorithm
 https://desktop.arcgis.com/en/arcmap/10.3/tools/spatial-analyst-toolbox/how-aspect-works.htm
 ChaoWang202004301739
*/
void arcaspect(int nr, int nc, float dy, float dx, float **zz, float **asp){
    
    const float PI = 3.141592654;
    const int testflg = 0;
    float dzxe, dzye;
    int ir, ic;
    float dzx[nr][nc], dzy[nr][nc];
    arcdzxy(nr, nc, dy, dx, zz, dzx, dzy);
    // Calculate aspect
    for (ir=0; ir<nr; ir++)
        for (ic=0; ic<nc; ic++){
            dzxe = dzx[ir][ic];
            dzye = dzy[ir][ic];
            asp[ir][ic] = atan2(dzye,-dzxe)*180.0/PI;
            if (asp[ir][ic] < 0){
                asp[ir][ic] = 90.0 - asp[ir][ic];
            }
            else if (asp[ir][ic] > 90.0) {
                asp[ir][ic] = 360.0 - asp[ir][ic] + 90.0;
            }
            else {
                asp[ir][ic] = 90.0 - asp[ir][ic];
            }
        }
    // Print and check
    if (testflg) {
        printf("Aspect array:\n");
        for (ir=0; ir<nr; ir++){
            for (ic=0; ic<nc; ic++){
                printf("%.2f ", asp[ir][ic]);
            }
            printf("\n");
        }
    }
}


// Mean slope using ArcGIS algorithm
// https://desktop.arcgis.com/en/arcmap/10.3/tools/spatial-analyst-toolbox/how-slope-works.htm
// ChaoWang202004290928
void arcslope(int nr, int nc, float dy, float dx, float **zz, float **slp){
    
    const float PI = 3.141592654;
    const int testflg = 0;
    float dzxe, dzye, rise_run, slope_degrees;
    int ir, ic;
    float dzx[nr][nc], dzy[nr][nc];
    
    arcdzxy(nr, nc, dy, dx, zz, dzx, dzy);
    // Calculate mean slope
    for (ir=0; ir<nr; ir++)
        for (ic=0; ic<nc; ic++){
            dzxe = dzx[ir][ic];
            dzye = dzy[ir][ic];
            rise_run = sqrtf(dzxe*dzxe+dzye*dzye);
            slope_degrees = atan(rise_run)*180.0/PI;
            slp[ir][ic] = slope_degrees;
        }
    // Print and check
    if (testflg) {
        printf("Slope array:\n");
        for (ir=0; ir<nr; ir++){
            for (ic=0; ic<nc; ic++){
                printf("%.2f ", slp[ir][ic]);
            }
            printf("\n");
        }
    }
}


/*
 The rates of change in the x and y directions for the middle cell in a 3*3 window using ArcGIS algorithm
 https://pro.arcgis.com/en/pro-app/tool-reference/spatial-analyst/how-slope-works.htm
 ChaoWang202004301743
 */
void arcdzxy(int nr, int nc, float dy, float dx, float **zz, float dzx[nr][nc], float dzy[nr][nc]){
    
    const int testflg = 0;
    float zp[nr+2][nc+2];
    float za, zb, zc, zd, ze, zf, zg, zh, zi;
    int ir, ic;
    int ra=-1, rb=-1, rc=-1, rd=0, re=0, rf=0, rg=1, rh=1, ri=1;
    int ca=-1, cb=0, cc=1, cd=-1, ce=0, cf=1, cg=-1, ch=0, ci=1;
    // Pad array to avoid edge effects
    for (ir=1; ir<nr+1; ir++){
        zp[ir][0] = zz[ir-1][0];
        zp[ir][nc+1] = zz[ir-1][nc-1];
        for (ic=1; ic<nc+1; ic++){
            zp[ir][ic] = zz[ir-1][ic-1];
        }
    }
    for (ic=1; ic<nc+1; ic++){
        zp[0][ic] = zz[0][ic-1];
        zp[nr+1][ic] = zz[nr-1][ic-1];
    }
    zp[0][0] = zz[0][0];
    zp[0][nc+1] = zz[0][nc-1];
    zp[nr+1][0] = zz[nr-1][0];
    zp[nr+1][nc+1] = zz[nr-1][nc-1];
    // Calculate mean change rates on padded array
    for (ir=1; ir<nr+1; ir++)
        for (ic=1; ic<nc+1; ic++){
        za = zp[ir+ra][ic+ca];
        zb = zp[ir+rb][ic+cb];
        zc = zp[ir+rc][ic+cc];
        zd = zp[ir+rd][ic+cd];
        ze = zp[ir+re][ic+ce];
        zf = zp[ir+rf][ic+cf];
        zg = zp[ir+rg][ic+cg];
        zh = zp[ir+rh][ic+ch];
        zi = zp[ir+ri][ic+ci];
        dzx[ir-1][ic-1] = ((zc+2.0*zf+zi)-(za+2.0*zd+zg))/(8.0*dx);
        dzy[ir-1][ic-1] = ((zg+2.0*zh+zi)-(za+2.0*zb+zc))/(8.0*dy);
    }
    // Print and check
    if (testflg) {
        printf("Padded array:\n");
        for (ir=0; ir<nr+2; ir++){
            for (ic=0; ic<nc+2; ic++){
                printf("%.2f ", zp[ir][ic]);
            }
            printf("\n");
        }
    }
}





// Solar radiation calculation
// Code borrowed from DPWM
// ChaoWang202003102140

// Functions from Climate.cpp in DPWM

// Saturation vapour pressure [kPa] at the air temperature Tair, (FAO 56, p. 36, eq.11)
// equation and units checked
float e0(float Tair){
// Tair: air temperature [degC]
	return (0.6108 * exp(17.27 * Tair / (Tair + 237.3)));
}

// Slope of saturation vapour pressure curve (FAO-56 eq 13; p. 37)
float slope_es_fcn(float Tair){
	// Tair: air temperature [degC]
	// slope_es: slope of saturation vapour pressure curve at air temperature T [kPa/degC]
	float slope_es;
	slope_es = 4098.f*(0.6108f*exp((17.27f*Tair)/(Tair+237.3f)))/pow(Tair+237.3f,2.f);
	return (slope_es);
}

//Atmospheric pressure as a function of elevation (FAO-56 eq 7; p. 31)
float Pair_fcn(float elev){
	// Pair: atmospheric pressure [kPa]
	// elev: elevation above sea level [m]
	float Pair;
	Pair = 101.3f*pow((293.f - 0.0065f * elev)/293.f,5.26f);
	return (Pair);
}

//Psychrometric constant (FAO-56 eq 8; pp. 31-32)
float Psych_fcn(float Pair){
	// Pair: atmospheric pressure [kPa]
	const float lheatvp = 2.45; // latent heat of vaporization [MJ/kg],
	const float cp = 0.001013; //specific heat at a constant pressure [MJ/kg/degC]
	const float eratio = 0.622; //ratio of molecular weight of water vapour/dry air
	float dPsych; //psychrometric constant [kPa/degC]

	dPsych = (cp*Pair)/(eratio*lheatvp);
	return (dPsych);
}

// Net solar radiation at the incline (but horizontal projection) (MJ m^-2 day^-1)
float netSolarRadiation(int DOY,float T_max_cell,float T_min_cell,
	float RHmean,float windspeed,float Kcln,float K_rs,float albedo,
	float dLat,float elev_cell,float slope,float Azimuth){

	float dSolarConstant = 0.082f;	// MJ/(day·m2)
	float dStefan = 0.000000004903f; // Stefan-Boltzman constant (MJ·K^-4·m^-2·day^-1)
	float d_es;// saturation vapor pressure (kPa) [3-8]
	float d_ea;// Actual vapor pressure (kPa)
	float d_dr;// inverse square relative distance between earth and sun [23]
	float d_dec;// Declination of the earth [24]
	float d_ws;// sunset hour angle for a horizontal surface [26]
	float d_Rnl; // Net outgoing longwave radiation (MJ/day·m2) [39]
	float d_Rn;//Net radiation (MJ/day·m2) [40]
	float d_ea_general;//General, actual vapor pressure
	float Ra_hor;//Extraterrestrial radiation on a horizontal surface
	float sinB24;//sin of mean solar elevation over a 24-hr period weighted by extraterrestrial radiation
	float P_basin;//mean atmospheric pressure for the reference weather basin
	float dW;//precipitable water at the reference weather basin
	float KBo_hor;//24-hr transmissivity for beam radiation
	float KDo_hor;//24-hr transmissivity for diffuse radiation
	float Rsm_hor;//'measured' solar radiation on a horizontal surface
	float tsw_hor;//total short-wave transmissivity
	float Rb;//ratio of beam radiation
	float dLat_eff;//the effective latitude for a given slope and aspect
	float Rso_hor;//Clear sky solar radiation over the 24-hr period
	float Id_hor; // Diffuse component of measured Rsm_hor for a horizontal surface
	float Ir;//Reflected radiation component for the inclined surface
	float dRns;//Horizontal equivalent for net short wave radiation on the incline
	float df;//modulating function
	float Ib;//direct beam on the inclined surface having a specific slope-aspect combination
	float Ib_hor;//Direct beam radiation on the horizontal surface
	float At;// Aisotropic index
	float Id;//Diffuse component for the inclined surface
	float Rsm_inc;// Total radiation received by the inclined surface
	float KD_hor;
	float dA, dB, dC, d_wsr, d_wss;
	float KB_hor;
	float dArcCosTerm_pos;
	float dArcCosTerm_neg;
	float dRb_1, dRb_2, dRb_3, dRb_4, dRb_5, dRb_6;
	float Rs_equiv_hor;
	float rLat=dLat*pi/180.f;//(dLat in degree, rLat in radian)
	float rslope=slope*pi/180.f;//(slope in degree, rslope in radian)
	float dAzimuth=Azimuth*pi/180.f;//(Azimuth in degree, dAzimuth in radian)
	
	
    // Solar Radiation on Inclined Surface

	//Steps 1 though 18 are general for the basin *****************************

	// STEP 2	Calculate general, actual vapor pressure for use in the
	//			Penman-Monteith equation and for estimating precipitable water
	//			(W) over the application area
	d_ea_general = (e0(T_min_cell)+e0(T_max_cell))/2*RHmean/100; //Eq. 19 Allen et al 1998
	
	// STEP 3	Calculate the inverse square relative distance between earth
	//and sun
	d_dr = 1.f+0.033f*cos(2.f*pi*DOY/365.f);										// (Eq. 23, p46, FAO 56)

	// STEP 4	Calculate declination of the earth
	d_dec = 0.409f*sin(2.f*pi*DOY/365.f-1.39f);										//(Eq. 24, p46, FAO-56)

	// STEP 5	Calculate the sunset hour angle for a horizontal surface
	d_ws = acos(-tan(rLat)*tan(d_dec));												//(Eq. 25, p46, FAO-56)

	// STEP 6	Calculate extraterrestrial radiation on a horizontal surface
	//for a 24-hr period
	Ra_hor =(24.f*60.f/pi)*dSolarConstant*d_dr*(d_ws*sin(rLat)*sin(d_dec)+
		cos(rLat)*cos(d_dec)*sin(d_ws));											//(Eq. 21, p46, FAO-56)

	// STEP 7	Calculate sin of mean solar elevation over a 24-hr period 
	//weighted by extraterrestrial radiation

	sinB24 = sin(0.85f+0.3f*rLat*sin(2.f*pi*DOY/365.f-1.39f)-0.42f*pow(rLat,2.f));  //FAO-56 Equation 3-16, p. 227
	if(sinB24 < 0.001f)	sinB24 = 0.001f;											//FAO-56 Equation 3-16, p. 227
	
	// STEP 8	Calculate mean atmospheric pressure for the reference weather
	//basin using the average elevation of the basin
	// Should grid cell elevation be used ???
	P_basin = 101.3f*pow((293.f-0.0065f*elev_cell)/293.f,5.26f);			//FAO-56 Equation 3-4, page 224

	// STEP 9	Calculate precipitable water at the reference weather basin
	dW = 0.14f*d_ea_general*P_basin+2.1f;											//FAO-56 Equation 3-19, p. 227

	// STEP 10	Calculate 24-hr transmissivity for beam radiation
	KBo_hor = 0.98f*exp(((-0.00146f*P_basin)/(Kcln*sinB24))-0.075f*
		pow(dW/sinB24,0.4f));														//Eq D-2, ASCE

	// STEP 11	Calculate 24-hr transmissivity for diffuse radiation
	if(KBo_hor < 0.15f)
		KDo_hor = 0.18f+0.82f*KBo_hor;												//FAO-56 Equation 3-20
	else
		KDo_hor = 0.35f-0.36f*KBo_hor;												//FAO-56 Equation 3-20

	// STEP 12	Calculate clear sky solar radiation over the 24-hr period
	Rso_hor = (KBo_hor + KDo_hor)*Ra_hor;											//Eq. D-1, P. D-7, ASCE; (MJ m-2 d-1)

	// STEP 13	Estimate 'measured' solar radiation on a horizontal surface
	//(Hargreave's Method)
	Rsm_hor = K_rs*pow((T_max_cell - T_min_cell),0.5f)*Ra_hor;					// Eq. 50, p 60 FAO-56 (MJ m-2 d-1)
	// Allen (1997) recommends k_rs = 0.19 for high altitude
	// Measure solar radiation if available can be substituted here

	// Rsm_hor must be limited to <= Rso_hor
	if (Rsm_hor > Rso_hor) Rsm_hor = Rso_hor;

	// STEP 14	Calculate total short-wave transmissivity
	tsw_hor = Rsm_hor / Ra_hor;

	// STEP 15	Partition the atmospheric transmissivity from step 14 into its
	//diffusive and direct beam components.  The following was modified by
	//Trezza and Allen, 2006 from source to match YMP data
	if (tsw_hor >= 0.78f)
		KD_hor = 0.12f*tsw_hor;
	else if (tsw_hor <= 0.35f)
		KD_hor = tsw_hor - 0.249f*pow(tsw_hor,2.f);
	else
		KD_hor = 1.557f*tsw_hor - 1.84f*pow(tsw_hor,2.f);

	// STEP 16	Calculate actual direct beam transmissivity
	KB_hor = tsw_hor - KD_hor;														// (Allen, 1996, eq. 7)

	// STEP 17	Calculate direct beam radiation on the horizontal surface
	//based on the measured Rsm_hor
	Ib_hor = KB_hor*Ra_hor;															// (MJ m-2 d-1)

	// STEP 18	Calculate the diffuse component of measured Rsm_hor for a
	//horizontal surface
	Id_hor = KD_hor*Ra_hor;															// (MJ m-2 d-1)

	// SETP 19	If grid cell terrain albedo is not specified, set to default
	if (albedo < 0.f) albedo = 0.23f;

	// STEP 20 Calculate ratio of beam radiation Rb (Appendix A)on an incline
	//to the beam radiation on a horizontal plane (Rb)
	//		dSlope is surface slope where
	//			0 = horizontal
	//			pi/2 radians for vertical slope
	//				(dSlope is always positive for any aspect)
	//		dAzimuth is the surface aspect angle where
	//			0 = slopes oriented due south
	//			-pi/2 rad for slopes oriented due east
	//			+pi/2 rad for slopes oriented due west
	//			+/- pi rad for slopes oriented due north

	//Reverse direction from GIS that was given as 0 = north, pi = south
	// Need to be checked and adjusted for TISC !!! attention
	// ChaoWang202004031116
	if(dAzimuth < pi)
		dAzimuth = -(pi - dAzimuth);
	else
		dAzimuth = dAzimuth - pi;


	// STEP 20a	Calculate the effective latitude for a given slope and aspect
	//as described by Revfeim 1976 (eq 2, page 651)
	dLat_eff = asin(cos(rslope)*sin(rLat)+sin(rslope)*cos(rLat)*
		cos(dAzimuth+pi));

	// STEP 20b	Check whether surface receives any direct beam radiation
	//during the day
	if (dLat_eff - d_dec >= pi/2.f)
		Rb = 0.f;																// e.g. during winter on extreme northerly slopes
	else {
		// Set up for the solution of daily integration limits for
		//beam (direct) radiation using Duffie and Beckman (1991).
		// STEP 20c	Parameter A
		dA = cos(rslope)+tan(rLat)*cos(dAzimuth)*sin(rslope);					// Eq. 2.20.5g, p 117, Duffue and Beckman, 1991

		// STEP 20d Parameter B
		dB = cos(d_ws)*cos(rslope)+tan(d_dec)*sin(rslope)*cos(dAzimuth);		// Eq. 2.20.5h, p 117, Duffie and Beckman, 1991

		// STEP 20e	Parameter C
		dC = sin(rslope)*sin(dAzimuth)/cos(rLat);								//Eq. 2.20.5i, p 117, Duffie and Beckman, 1991

		// STEP 20	Calculate the 24-hour integration limits
		if (pow(dA,2.f)-pow(dB, 2.f)+pow(dC,2.f) <= 0.f) {
			//Check for negative values in square root
			d_wsr = -d_ws;
			d_wss = d_ws;
		}
		else {
		     // if no negative values in square root, proceed ...
			dArcCosTerm_pos = (dA*dB+dC*pow(pow(dA,2.f)-pow(dB,2.f)+
				pow(dC,2.f),0.5f)/(pow(dA,2.f)+pow(dC,2.f)));
			dArcCosTerm_neg = (dA*dB-dC*pow(pow(dA,2.f)-pow(dB,2.f)+
				pow(dC,2.f),0.5f)/(pow(dA,2.f)+pow(dC,2.f)));
			if ( (dArcCosTerm_pos < -1.f) || (dArcCosTerm_pos > 1.f))			// Check for out of bounds on arcos function for d_wsr
				d_wsr = -d_ws;
			else { // proceed with arcos function for d_wsr
				d_wsr = MIN_2(d_ws,acos(dArcCosTerm_pos));						// Eq 2.20.5e, Duffie and Beckman, 1991
				d_wsr = abs(d_wsr);
				if ( ((dA > 0) && (dB > 0)) || (dA >= dB))
					d_wsr = -d_wsr;
				else
				   d_wsr=d_wsr;
			}
			if ( (dArcCosTerm_neg < -1.f) || (dArcCosTerm_neg > 1.f))			// Check for out of bounds on arcos function d_wss
				d_wss = d_ws;
			else { // proceed with arcod function for d_wss
				d_wss = MIN_2(d_ws,acos(dArcCosTerm_neg));						// Eq 2.20.5, Duffie and Beckman, 1991
				d_wss = abs(d_wss);
				if ( ((dA > 0) && (dB > 0)) || (dA >= dB))
					d_wss = d_wss;
				else
					d_wss = -d_wss;
			}
			// Prevent negative values for Rb.
			if ((dA < dB) && (dAzimuth > 0))
				d_wsr = -d_wsr;
			if ((dA < dB) && (dAzimuth < 0))
				d_wss = -d_wss;
		}
		// STEP 20g	Calculate Rb
		dRb_1 = sin(d_dec)*sin(rLat)*cos(rslope)*(d_wss - d_wsr);
		dRb_2 = sin(d_dec)*cos(rLat)*sin(rslope)*cos(dAzimuth)*(d_wss - d_wsr);
		dRb_3 = cos(d_dec)*cos(rLat)*cos(rslope)*(sin(d_wss)-sin(d_wsr));
		dRb_4 = cos(d_dec)*sin(rLat)*sin(rslope)*cos(dAzimuth)*(sin(d_wss)
			-sin(d_wsr));
		dRb_5 = cos(d_dec)*sin(rslope)*sin(dAzimuth)*(cos(d_wss)-cos(d_wsr));
		dRb_6 = 2.f*(cos(rLat)*cos(d_dec)*sin(d_ws)+d_ws*sin(rLat)
			*sin(d_dec));
		Rb  = (dRb_1 - dRb_2 + dRb_3 + dRb_4 - dRb_5) / dRb_6;
	}

	// STEP 21	Calculate direct beam on the inclined surface having a specific
	//slope-aspect combination
	Ib = Ib_hor * Rb; // (MJ m-2 d-1)

	// STEP 22	Anisotropic index on page 11 of Reindel et al., 1990, and also
	//as in Eq. 2.16.3 in Duffie and Beckman, 1991
	At = KB_hor;

	// STEP 23	Calculate the modulating function f (as defined in Reindl et al,
	//page 11, and Eq. 2.16.6, Duffie and Beckman, 1991)
	df = pow(Ib_hor/Rsm_hor,0.5f);

	// STEP 24	Calculate the diffuse component for the inclined surface (Eq. 5,
	//page 11, Reindl et al, 1990, and Eq. 2.16.5, Duffie and Beckman, 1991)
	Id = Id_hor*((1.f-At)*((1.f+cos(rslope))/2.f)*(1.f+df*pow(sin(rslope/2.f),
		3.f))+At*Rb);															// (MJ m-2 d-1)

	// STEP 25	Calculate the reflected radiation component for the inclined
	//surface (Eq. 1, page 10, Reindl et al., 1990)
	Ir = Rsm_hor*albedo*((1.f-cos(rslope))/2.f);								//(MJ m-2 d-1)

	// STEP 26	Calculate the total radiation received by the inclined surface
	Rsm_inc = Ib + Id + Ir;														// (MJ m-2 d-1)
	
	// STEP 27	Reproject Rsm_inc to a horizontal projection (equivalent)
	Rs_equiv_hor = Rsm_inc / cos(rslope); // (MJ m-2 d-1)

	// STEP 28	Calculate mean satuation vapor pressure associated with cell
	//daily extreme temperatures
	d_es = (e0(T_max_cell) + e0(T_min_cell)) / 2.f;								//FAO-56 Eq. 12, p. 36 (kPa)

	// STEP 29	Limit actual vapor pressure of the grid cell to less than
	//or equal to d_es
	d_ea = MIN_2(d_ea_general,d_es); // (kPa)

	// STEP 33	Calculate the horizontal equivalent for net short wave
	//radiation on the incline
	dRns = (1.f - albedo)*Rs_equiv_hor;											// Eq. 38, p. 51, FAO-56

	// STEP 34	Calculate the net outgoing long wave radiation
	d_Rnl = dStefan*((pow(T_max_cell+273.16f,4.f)+pow(T_min_cell+273.16f,4.f))/2.f)*
		(0.34f-0.14f*pow(d_ea,0.5f))*(1.35f*MIN_2(Rsm_hor/Rso_hor,1.f)-0.35f);	// Eq. 39, p. 52, FAO-56

	// STEP 35	Calculate net radiation on the inclined surface projected
	//to a horizontal projection
	d_Rn = dRns - d_Rnl;														// Eq. 40, p. 53, FAO-56)
	d_Rn = MAX_2(d_Rn,0.f);														//Fix for high slope cells

	return (d_Rn);

}





