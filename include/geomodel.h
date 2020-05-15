/*
		GENERAL INCLUDE FILE FOR GEOPHYSICAL MODELS
*/

#include "universal.h"			/*Most general definitions and types*/


#define	ET2RIG(x)	(E*pow(x,3.)/(12*(1.-nu*nu)))		/*Converts Elastic Thickness into Rigidity*/
#define	RIG2ET(x)	(pow((x)/E*12.*(1.-nu*nu), 1./3.))	/*Converts Rigidity into Elastic Thickness*/

int	grav_anom_type, 	/*Gravity anomaly type [m]: 0 means no gravity calculation; 1 means Bouguer; 2 means free-Air*/
	isost_model,		/*Type of isostasy / lithospheric plate*/
	water_load; 		/*1 if sea presence has been required*/

float
	Te_default,				/*Default elastic plate thicknes [m]*/
	crust_thick_default, 	/*Default thickness for crust [m]*/
	upper_crust_thick_default,	/*Default thickness for upper crust [m]*/
	densasthen, 			/*Mantle density [kg/m3]*/
	densmantle, 			/*Mantle density [kg/m3]*/
	denscrust, 				/*Crust density [kg/m3]*/
	densinfill, 			/*Density of infilling (under the zero deflection level) material when avistopoest=SI [kg/m3]*/
	denssedim, 				/*Sediment density [kg/m3]*/		
	densenv, 				/*Density of medium over the plate (usually air ==> zero) [kg/m3]*/
	sea_level=0, 			/*Sea level height with respect to the model zero level [m]*/
	temp_sea_level, 		/*temperature at se level [C]*/
	Time=0, 				/*Present Time of the model running [s]*/
	Timeini=0, Timefinal=0, /*Initial and final Times [s]*/
	dt=0, 					/*Time increment [s]*/
	dt_eros, 				/*Approximate dt for surface processes.*/
	// Initial and final year and month for the current time step
	yearini_ts = 0, yearfinal_ts = 0, monthini_ts = 0, monthfinal_ts = 0,
	Timetotal,				// Total time duration [Myr]
	tau ;					/*Viscoelastic relaxation Time [s]*/

int 
	N_dt, idt=0 			// Number and counter of tectonic time steps
	N_dt_eros, idt_eros=0,	// Number and counter of surface process time steps
	// Day of year of the middle days of the 12 months in each year
	DOY_month[12] = {15,46,74,105,135,166,196,227,258,288,319,349};

char
	projectname[MAXLONFICH],/*The project name is also the root of current model file names*/
	title[MAXLONLINE];		/*Title with main current model characteristics*/


BOOL
	switch_geograph_coor;	/*YES if x-y are geographycal coordinates*/
