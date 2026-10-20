/*
FxSound
Copyright (C) 2025  FxSound LLC
Contributors:
	www.theremino.com (2025)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
*  Public defines for the DfxDsp class
*/
#ifndef _DFX_DSP_H_
#define _DFX_DSP_H_

#include <string>
#include <array>
#include <vector>
#include <cstdint>
#include "InputAudioFeatures.h"

// Parse preset values without applying them to the current DSP.
struct DfxPresetSnapshot {
    std::array<float,5> effects {};
    std::vector<float> eqFrequencies;
    std::vector<float> eqGains;
};
struct DfxPreset {
	std::wstring full_path;
	std::wstring name;
};

class DfxDspPrivate;
class DfxDsp
{
public:
	enum Effect { Fidelity = 0, Ambience = 1, Surround = 2, DynamicBoost = 3, Bass = 4, NumEffects = 5 };

	DfxDsp();
	~DfxDsp();

	int setSignalFormat(int i_bps, int i_nch, int i_srate, int i_valid_bits);
	int processAudio(short int *si_input_samples, short int *si_output_samples, int i_num_sample_sets, int i_check_for_duplicate_buffers);
	int loadPreset(std::wstring preset_file_full_path);
    bool readPresetSnapshot(std::wstring path,DfxPresetSnapshot& output);
	int savePreset(std::wstring preset_name, std::wstring preset_file_full_path);
	int exportPreset(std::wstring preset_source_file_full_path, std::wstring preset_name, std::wstring preset_export_path);
	void eqOn(bool on);
	int getNumEqBands();
	float getBalance();
	void setBalance(float gain_db);
	float getNormalization();
	void setNormalization(float gain_db);
	float getVolumeLeveling();
	void setVolumeLeveling(float gain_db);
	float getMasterGain();
	void setMasterGain(float gain_db);
	float getFilterQ();
	void setFilterQ(float q_multiplier);
	void setNumBands(int num_bands);
    float getEqBandFrequency(int band_num);
    void setEqBandFrequency(int band_num, float freq);
    void getEqBandFrequencyRange(int band_num, float* min_freq, float* max_freq);
	float getEqBandBoostCut(int band_num);
	void setEqBandBoostCut(int band_num, float boost);
    void setApex10MSpatialPreview(float strength);
	void powerOn(bool on);
	bool isPowerOn();
	float getEffectValue(Effect effect);
	void setEffectValue(Effect effect, float value);
	DfxPreset getPresetInfo(std::wstring preset_file_full_path);
	unsigned long getTotalAudioProcessedTime();
	void resetTotalAudioProcessedTime();
    void getSpectrumBandValues(float* rp_band_values, int i_array_size);
    InputAudioFeatures getInputAudioFeatures();
    bool copyDjWindow(std::array<float,128000>& out,uint64_t& sampleNumber) const;
    void enableDj(bool enabled);
    void configureDj(float amount,float spacing,bool stereo);
    void setDjScene(float music,float speech);
    void invalidateDjScene();
    void setDjVoiceGate(bool safe);
    uint32_t djEffectsCount() const;
    int djLastEffect() const;
    bool copyYamNetWindow(std::array<float,15600>& out,uint64_t& sampleNumber) const;
    bool copyApexWindow(std::array<float,48000>& out,uint64_t& sampleNumber) const;

private:
	DfxDspPrivate *data_;
};

#endif
