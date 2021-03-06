#
/*
 *    Copyright (C) 2008, 2009, 2010
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef	__FM_PROCESSOR
#define	__FM_PROCESSOR

#include	<sndfile.h>
#include	<vector>
#include	"fm-constants.h"
#include	"fir-filters.h"
#include	"fft-filters.h"
#include	"sincos.h"
#include	"pllC.h"
#include	"fm-levels.h"
#include	"ringbuffer.h"
#include	"oscillator.h"
#include	"resampler.h"
#include	"rds-groupdecoder.h"

#define SCAN_BLOCK_SIZE 1024

/** Callback type for scanning
 * \param frequency The frequency on which a station has been found, in Hz
 * \param userdata A pointer provided to the scan function
 */
typedef void (*StationCallback) (int32_t frequency, void *userdata);

class		rigInterface;
class		virtualInput;
class		RadioInterface;
class		fm_Demodulator;
class		rdsDecoder;
class		audioSink;
class		newConverter;

class	fmProcessor {
public:
			fmProcessor (
				     virtualInput	*,
	                             RadioInterface *,
	                             audioSink *,
	                             int32_t,	// inputRate
	                             int32_t,	// fmrate
	                             int32_t,	// audioRate,
	                             int16_t,	// threshold scanning
				     ClearCallback = 0,	// rds station label clear callback
				     StringCallback = 0,	// rds station label change callback
				     StringCallback = 0,	// rds station label complete callback
				     ClearCallback = 0,	// rds radio text clear callback
				     StringCallback = 0,	// rds radio text change callback
				     StringCallback = 0,	// rds radio text complete callback
				     void * = 0);	// rds callbacks userdata
	        	~fmProcessor (void);
        void		start		(void);
	void		stop		(void);		// stop the processor
	void		setfmMode	(uint8_t);
	void		setFMdecoder	(int8_t);
	void		setSoundMode	(uint8_t);
	void		setDeemphasis	(int16_t);
	void		setVolume	(int16_t);
	void		setLFcutoff	(int32_t);
	void		setAttenuation	(int16_t);
	void		setfmRdsSelector (int8_t);
	void		resetRds	(void);
	void		set_LocalOscillator	(int32_t);
	void		set_squelchMode	(bool);
	void		setInputMode	(uint8_t);

	bool		ok			(void);
	DSPFLOAT	get_pilotStrength	(void);
	DSPFLOAT	get_rdsStrength		(void);
	DSPFLOAT	get_noiseStrength	(void);
	DSPFLOAT	get_dcComponent		(void);
	bool		isLocked		(void);
	void		startScanning		(StationCallback callback, void *userdata,
						 int16_t thresHold = -1);
	void		stopScanning		(void);
	bool		isScanning		(void);
	const char *	nameofDecoder	(void);

	enum Channels {
	   S_STEREO		= 0,
	   S_LEFT		= 1,
	   S_RIGHT		= 2,
	   S_LEFTplusRIGHT	= 0103,
	   S_LEFTminusRIGHT	= 0104
	};
	enum Mode {
	   FM_STEREO	= 0,
	   FM_MONO	= 1
	};

	void		set_squelchValue	(int16_t);

private:
	/** This holds data on a station found while scanning */
	struct StationData {
	  int32_t frequency;
	  float   snr;
	  int     count;
	};
	typedef std::vector<StationData> StationDataList;

        static void *   c_run (void * userdata);
	void		run		(void);
        pthread_t       threadId;
	virtualInput	*myRig;
	RadioInterface	*myRadioInterface;
	audioSink	*theSink;
	int32_t		inputRate;
	int32_t		fmRate;
	int32_t		workingRate;
	int32_t		audioRate;
	uint8_t		inputMode;
	int16_t		thresHold;
	pthread_mutex_t scanLock;
	void		lockScan();
	void		unlockScan();
	/** Run the scan check for a single sample */
	bool            checkStation(DSPCOMPLEX);
	/** Add a found station to the list of found frequencies */
	void            addStation(float);
	/** Locate the central frequency of those found and issue
	 * the station callback, finishing the scan */
	void            finishScan();
	bool		scanning;
	common_fft	*scan_fft;
	int32_t		scanPointer;
	StationDataList stations;
	StationCallback	scanCallback;
	void *		scanUserdata;
	DSPFLOAT	getSignal	(DSPCOMPLEX *, int32_t);
	DSPFLOAT	getNoise	(DSPCOMPLEX *, int32_t);
	bool		squelchOn;
	
	void		sendSampletoOutput	(DSPCOMPLEX);
	DecimatingFIR	*fmBandfilter;
	Oscillator	*localOscillator;
	newConverter	*theConverter;
	int32_t		lo_frequency;
	bool		running;
	SinCos		*mySinCos;
	LowPassFIR	*fmFilter;
	int32_t		fmBandwidth;
	int32_t		fmFilterDegree;
	bool		newFilter;

	int16_t		old_squelchValue;
	int16_t		squelchValue;
	int32_t		decimatingScale;

	int32_t		myCount;
	int16_t		Gain;

	rdsDecoder	*myRdsDecoder;

	void		stereo	(DSPCOMPLEX *, DSPCOMPLEX *, DSPFLOAT *);
	void		mono	(DSPCOMPLEX *, DSPCOMPLEX *, DSPFLOAT *);
	fftFilter	*pilotBandFilter;
	fftFilter	*rdsBandFilter;
//	fftFilter	*rdsLowPassFilter;
	DecimatingFIR	*rdsLowPassFilter;
	HilbertFilter	*rdsHilbertFilter;

	fmLevels	*fm_Levels;
	DSPFLOAT	pilotDelay;
	DSPCOMPLEX	audioGainCorrection	(DSPCOMPLEX);
	DSPFLOAT	Volume;
	DSPFLOAT	audioGain;
	int32_t		max_freq_deviation;
	int32_t		norm_freq_deviation;
	DSPFLOAT	omega_demod;
	LowPassFIR	*fmAudioFilter;

	int16_t		balance;
	DSPFLOAT	leftChannel;
	DSPFLOAT	rightChannel;
	uint8_t		fmModus;
	uint8_t		selector;
	DSPFLOAT	peakLevel;
	int32_t		peakLevelcnt;
	fm_Demodulator	*TheDemodulator;

	int8_t		rdsModus;

	int8_t		viewSelector;
	pllC		*rds_plldecoder;
	DSPFLOAT	K_FM;

	DSPFLOAT	xkm1;
	DSPFLOAT	ykm1;
	DSPFLOAT	alpha;
	class	pilotRecovery {
	   private:
	      int32_t	Rate_in;
	      DSPFLOAT	pilot_OscillatorPhase;
	      DSPFLOAT	pilot_oldValue;
	      DSPFLOAT	omega;
	      DSPFLOAT	gain;
	      SinCos	*mySinCos;
	      DSPFLOAT	pilot_Lock;
	      bool	pll_isLocked;
	      DSPFLOAT	quadRef;
	      DSPFLOAT	accumulator;
	      int32_t	count;
	   public:
	      pilotRecovery (int32_t	Rate_in,
	                     DSPFLOAT	omega,
	                     DSPFLOAT	gain,
	                     SinCos	*mySinCos) {
	         this	-> Rate_in	= Rate_in;
	         this	-> omega	= omega;
	         this	-> gain		= gain;
	         this	-> mySinCos	= mySinCos;
	         pll_isLocked		= false;
	         pilot_Lock		= 0;
	         pilot_oldValue		= 0;
	         pilot_OscillatorPhase	= 0;
	      }

	      ~pilotRecovery (void) {
	      }

	      bool	isLocked (void) {
	         return pll_isLocked;
	      }

	      DSPFLOAT	getPilotPhase	(DSPFLOAT pilot) {
	      DSPFLOAT	OscillatorValue =
	                  mySinCos -> getCos (pilot_OscillatorPhase);
	      DSPFLOAT	PhaseError	= pilot * OscillatorValue;
	      DSPFLOAT	currentPhase;
	         pilot_OscillatorPhase += PhaseError * gain;
	         currentPhase		= PI_Constrain (pilot_OscillatorPhase);

	         pilot_OscillatorPhase =
	                   PI_Constrain (pilot_OscillatorPhase + omega);
	         
	         quadRef	= (OscillatorValue - pilot_oldValue) / omega;
//	         quadRef	= PI_Constrain (quadRef);
	         pilot_oldValue	= OscillatorValue;
	         pilot_Lock	= 1.0 / 30 * (- quadRef * pilot) +
	                          pilot_Lock * (1.0 - (1.0 / 30)); 
	         pll_isLocked	= pilot_Lock > 0.1;
	         return currentPhase;
	      }
	};
	      
	pilotRecovery	*pilotRecover;

	//signals:
	void		setPLLisLocked		(bool);
	void		hfBufferLoaded		(int, int);
	void		lfBufferLoaded		(int);
	void		showStrength		(int, int, int, bool, float);
	void		scanresult		(void);
};

#endif

