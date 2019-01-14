#ifndef SOFTFM_FMDECODE_H
#define SOFTFM_FMDECODE_H

#include <cstdint>
#include <vector>

#include "Filter.h"
#include "SoftFM.h"

// Detect frequency by phase discrimination between successive samples.
class PhaseDiscriminator {
public:
  // Construct phase discriminator.
  // max_freq_dev :: Full scale frequency deviation relative to the
  //                 full sample frequency.
  PhaseDiscriminator(double max_freq_dev);

  // Process samples.
  // Output is a sequence of frequency estimates, scaled such that
  // output value +/- 1.0 represents the maximum frequency deviation.
  void process(const IQSampleVector &samples_in, SampleVector &samples_out);

private:
  const Sample m_freq_scale_factor;
  IQSample m_last1_sample;
  IQSample m_last2_sample;
};

class DiscriminatorEqualizer {
public:
  // Construct equalizer for phase discriminator.
  DiscriminatorEqualizer(double ifeq_static_gain, double ifeq_fit_factor);

  // process samples.
  // Output is a sequence of equalized output.
  void process(const SampleVector &samples_in, SampleVector &samples_out);

private:
  double m_static_gain;
  double m_fit_factor;
  double m_last1_sample;
};

// Phase-locked loop for stereo pilot.
class PilotPhaseLock {
public:
  // Expected pilot frequency (used for PPS events).
  static constexpr int pilot_frequency = 19000;

  // Timestamp event produced once every 19000 pilot periods.
  struct PpsEvent {
    std::uint64_t pps_index;
    std::uint64_t sample_index;
    double block_position;
  };

  // Construct phase-locked loop.
  // freq        :: 19 kHz center frequency relative to sample freq
  //                (0.5 is Nyquist)
  // bandwidth   :: bandwidth relative to sample frequency
  // minsignal   :: minimum pilot amplitude
  PilotPhaseLock(double freq, double bandwidth, double minsignal);

  // Process samples and extract 19 kHz pilot tone.
  // Generate phase-locked 38 kHz tone with unit amplitude.
  // pilot_shift :: true to shift pilot phase
  //             :: (use cos(2*x) instead of sin (2*x))
  //             :: (for multipath distortion detection)
  void process(SampleVector &samples_in, SampleVector &samples_out,
               bool pilot_shift);

  // Return true if the phase-locked loop is locked.
  bool locked() const { return m_lock_cnt >= m_lock_delay; }

  // Return detected amplitude of pilot signal.
  double get_pilot_level() const { return 2 * m_pilot_level; }

  // Return PPS events from the most recently processed block.
  std::vector<PpsEvent> get_pps_events() const { return m_pps_events; }

  // Return detected phase error of pilot signal.
  double get_phase_error() const { return m_loopfilter_x1; }

private:
  Sample m_minfreq, m_maxfreq;
  Sample m_phasor_b0, m_phasor_a1, m_phasor_a2;
  Sample m_phasor_i1, m_phasor_i2, m_phasor_q1, m_phasor_q2;
  Sample m_loopfilter_b0, m_loopfilter_b1;
  Sample m_loopfilter_x1;
  Sample m_freq, m_phase;
  Sample m_minsignal;
  Sample m_pilot_level;
  int m_lock_delay;
  int m_lock_cnt;
  int m_pilot_periods;
  std::uint64_t m_pps_cnt;
  std::uint64_t m_sample_cnt;
  std::vector<PpsEvent> m_pps_events;
};

// Complete decoder for FM broadcast signal.
class FmDecoder {
public:
  static constexpr double default_deemphasis_eu = 50; // Europe and Japan
  static constexpr double default_deemphasis_na = 75; // USA/Canada
  static constexpr double default_bandwidth_if = 96000;
  static constexpr double default_freq_dev = 75000;
  static constexpr double default_bandwidth_pcm = 15000;
  static constexpr double pilot_freq = 19000;
  static constexpr unsigned int finetuner_table_size = 256;

  // Construct FM decoder.
  // Stereo decoding always enabled.
  // sample_rate_if   :: IQ sample rate in Hz.
  // ifeq_static_gain :: IF DiscriminatorEqualizer static_gain
  // ifeq_fit_factor  :: IF DiscriminatorEqualizer fit_factor
  // tuning_offset    :: Frequency offset in Hz of radio station with respect
  //                     to receiver LO frequency (positive value means
  //                     station is at higher frequency than LO).
  // sample_rate_pcm  :: Audio sample rate.
  // deemphasis       :: Time constant of de-emphasis filter in microseconds
  //                     (50 us for broadcast FM, 0 to disable de-emphasis).
  // bandwidth_if     :: Half bandwidth of IF signal in Hz
  //                     (~ 100 kHz for broadcast FM)
  // freq_dev         :: Full scale carrier frequency deviation
  //                     (75 kHz for broadcast FM)
  // bandwidth_pcm    :: Half bandwidth of audio signal in Hz
  //                     (15 kHz for broadcast FM)
  // downsample       :: Downsampling factor to apply after FM demodulation.
  //                     Set to 1 to disable.
  // pilot_shift      :: True to shift pilot signal phase
  //                  :: (use cos(2*x) instead of sin (2*x))
  //                  :: (for multipath distortion detection)
  FmDecoder(double sample_rate_if, double ifeq_static_gain,
            double ifeq_fit_factor, double tuning_offset,
            double sample_rate_pcm, double deemphasis = default_deemphasis_eu,
            double bandwidth_if = default_bandwidth_if,
            double freq_dev = default_freq_dev,
            double bandwidth_pcm = default_bandwidth_pcm,
            unsigned int downsample = 1, bool pilot_shift = false);

  // Process IQ samples and return audio samples.
  //
  // If the decoder is set in stereo mode, samples for left and right
  // channels are interleaved in the output vector (even if no stereo
  // signal is detected). If the decoder is set in mono mode, the output
  // vector only contains samples for one channel.
  void process(const IQSampleVector &samples_in, SampleVector &audio);

  // Return true if a stereo signal is detected.
  bool stereo_detected() const { return m_stereo_detected; }

  // Return actual frequency offset in Hz with respect to receiver LO.
  double get_tuning_offset() const {
    double tuned =
        -m_tuning_shift * m_sample_rate_if / double(m_tuning_table_size);
    return tuned + m_baseband_mean * m_freq_dev;
  }

  // Return RMS IF level (where full scale IQ signal is 1.0).
  double get_if_level() const { return m_if_level; }

  // Return RMS baseband signal level (where nominal level is 0.707).
  double get_baseband_level() const { return m_baseband_level; }

  // Return amplitude of stereo pilot (nominal level is 0.1).
  double get_pilot_level() const { return m_pilotpll.get_pilot_level(); }

  // Return detected phase error of stereo pilot signal.
  double get_phase_error() const { return m_pilotpll.get_phase_error(); }

  // Return PPS events from the most recently processed block.
  std::vector<PilotPhaseLock::PpsEvent> get_pps_events() const {
    return m_pilotpll.get_pps_events();
  }

private:
  // Demodulate stereo L-R signal.
  void demod_stereo(const SampleVector &samples_baseband,
                    SampleVector &samples_stereo);

  // Duplicate mono signal in left/right channels.
  void mono_to_left_right(const SampleVector &samples_mono,
                          SampleVector &audio);

  // Extract left/right channels from mono/stereo signals.
  void stereo_to_left_right(const SampleVector &samples_mono,
                            const SampleVector &samples_stereo,
                            SampleVector &audio);

  // Fill zero signal in left/right channels.
  // (samples_mono used for the size determination only)
  void zero_to_left_right(const SampleVector &samples_mono,
                          SampleVector &audio);

  // Data members.
  const double m_sample_rate_if;
  const double m_sample_rate_baseband;
  const int m_tuning_table_size;
  const int m_tuning_shift;
  const double m_freq_dev;
  const unsigned int m_downsample;
  const bool m_pilot_shift;
  bool m_stereo_detected;
  double m_if_level;
  double m_baseband_mean;
  double m_baseband_level;

  IQSampleVector m_buf_iftuned;
  IQSampleVector m_buf_iffiltered;
  SampleVector m_buf_baseband;
  SampleVector m_buf_baseband_raw;
  SampleVector m_buf_mono;
  SampleVector m_buf_rawstereo;
  SampleVector m_buf_stereo;

  FineTuner m_finetuner;
  LowPassFilterFirIQ m_iffilter;
  PhaseDiscriminator m_phasedisc;
  DiscriminatorEqualizer m_disceq;
  DownsampleFilter m_resample_baseband;
  PilotPhaseLock m_pilotpll;
  DownsampleFilter m_resample_mono;
  DownsampleFilter m_resample_stereo;
  HighPassFilterIir m_dcblock_mono;
  HighPassFilterIir m_dcblock_stereo;
  LowPassFilterRC m_deemph_mono;
  LowPassFilterRC m_deemph_stereo;
};

#endif
