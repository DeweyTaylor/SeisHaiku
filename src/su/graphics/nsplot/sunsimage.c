/* SUNSIMAGE: $Revision: 1.3 $ ; $Date: 92/10/28 08:34:43 $		*/

/*----------------------------------------------------------------------
 * Copyright (c) Colorado School of Mines, 1990.
 * All rights reserved.
 *
 * This code is part of SU.  SU stands for Seismic Unix, a processing line
 * developed at the Colorado School of Mines, partially based on Stanford
 * Exploration Project (SEP) software.  Inquiries should be addressed to:
 *
 *  Jack K. Cohen, Center for Wave Phenomena, Colorado School of Mines,
 *  Golden, CO 80401  (jkc@dix.mines.colorado)
 *----------------------------------------------------------------------
 */

#include "su.h"
#include "segy.h"

/*********************** self documentation *****************************/
char *sdoc[] = {
" 									",
" SUNSIMAGE - NextStep IMAGE plot of a segy data set 			",
" 									",
" sunsimage <stdin >postscript file [optional parameters]		",
" 							        	",
" Optional parameters: 							",
" 							        	",
" d1 is the sampling interval in the fast dimension.  If not getparred	",
" then for seismic time domain data d1=tr.dt/10^6 if set, else 0.004. 	",
" For other types of data d1=tr.d1 if set, else 1.0			",
" 							        	",
" d2 is the sampling interval in the slow dimension.  If not getparred	",
" d2=tr.d2 if set, else 1.0						",
" 							        	",
" f1 is the first sample in the fast dimension.  If not getparred	",
" then for seismic time domain data f1=tr.delrt/10^3 if set, else 0.0.	",
" For other types of data f1=tr.d1 if set else 0.0	 		",
" 							        	",
" f2 is the first sample in the slow dimension.  If not getparred	",
" f2=tr.f2 if set, else tr.tracr if set, else tr.tracl if set,		",
" else 1.0 for seismic time domain data, else 0.0 			",
" 							        	",
" Note that for seismic time domain data, the \"fast dimension\" is	",
" time and the \"slow dimension\" is usually trace number or range.	",
" Also note that \"foreign\" data tapes may have something unexpected	",
" in the d2,f2 fields, use segyclean to clear these if you can afford	",
" the processing time or use d2= f2= to over-ride the header values if	",
" not.									",
" 							        	",
" See the nsimage selfdoc for the remaining parameters.			",
" 							        	",
NULL};
/**************** end self doc *******************************************/

/* Credits:
 *
 *	CWP: Dave (nsimage), Jack (su tee shirt)
 *
 */


segy tr;



main(int argc, char **argv)
{
	char plotcmd[BUFSIZ];	/* build command for popen	 	*/
	float *trbuf;		/* trace buffer			 	*/
	FILE *datafp;		/* fp for trace data file		*/
	FILE *plotfp;		/* fp for plot data			*/
	float d1;		/* time/depth sample rate 		*/
	float d2;		/* trace/dx sample rate 		*/
	float f1;		/* tmin/zmin				*/
	float f2;		/* tracemin/xmin	 		*/
	int nt;			/* number of samples on trace		*/
	int ntr;		/* number of traces			*/
	Bool seismic;		/* is this seismic data?		*/


	/* Initialize */
	initargs(argc, argv);
	requestdoc(1);
	

	/* Get info from first trace */
	if (!gettr(&tr)) err("can't get first trace");
	seismic = (tr.trid == 0 || tr.trid == TREAL);
	nt = tr.ns;

	if (!getparfloat("d1", &d1)) {
		if (seismic) {
			if (tr.dt) {
				d1 = (float) tr.dt / 1000000.0;
			} else {
				d1 = 0.004;
				warn("tr.dt not set, assuming dt=0.004");
			}
		} else { /* non-seismic data */
			if (tr.d1) {
				d1 = tr.d1;
			} else {
				d1 = 1.0;
				warn("tr.d1 not set, assuming d1=1.0");
			}
		}
	}

	if (!getparfloat("d2", &d2)) d2 = (tr.d2) ? tr.d2 : 1.0;

	if (!getparfloat("f1", &f1)) {
		if (seismic) {
			f1 = (tr.delrt) ? (float) tr.delrt/1000.0 : 0.0;
		} else {
			f1 = (tr.f1) ? tr.f1 : 0.0;
		}
	}

	if (!getparfloat("f2", &f2)) {
		if      (tr.f2)     f2 = tr.f2;
		else if (tr.tracr)  f2 = (float) tr.tracr;
		else if (tr.tracl)  f2 = (float) tr.tracl;
		else if (seismic)   f2 = 1.0;
		else 		    f2 = 0.0;
	}


	/* Allocate trace buffer */
	trbuf = ealloc1float(nt);


	/* Create temporary "file" to hold data */
	datafp = etmpfile();


	/* Loop over input traces & put them into the nsdata file */
	ntr = 0;
	do {
		++ntr;
		efwrite(tr.data, FSIZE, nt, datafp);
	} while (gettr(&tr));


	/* System call to nsimage */
	sprintf(plotcmd, "nsimage n1=%d n2=%d d1=%f d2=%f f1=%f f2=%f",
			   nt, ntr, d1, d2, f1, f2);

	for (--argc, ++argv; argc; --argc, ++argv) {
		if (strncmp(*argv, "d1=", 3) && /* skip those already set */
		    strncmp(*argv, "d2=", 3) &&
		    strncmp(*argv, "f1=", 3) &&
		    strncmp(*argv, "f2=", 3)) {
		    
			strcat(plotcmd, " ");   /* put a space between args */
			strcat(plotcmd, "\"");  /* user quotes are stripped */
			strcat(plotcmd, *argv); /* add the arg */
			strcat(plotcmd, "\"");  /* user quotes are stripped */
		}
	}


	/* Open pipe; read data to buf; write buf to plot program */
	plotfp = epopen(plotcmd, "w");
	rewind(datafp);
	{ register int itr;
		for (itr = 0; itr < ntr; ++itr) {
			efread (trbuf, FSIZE, nt, datafp);
			efwrite(trbuf, FSIZE, nt, plotfp);
		}
	}


	/* Clean up */
	epclose(plotfp);
	efclose(datafp);


	return EXIT_SUCCESS;
}
