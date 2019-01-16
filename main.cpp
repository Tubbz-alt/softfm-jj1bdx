// SoftFM - Software decoder for FM broadcast radio with RTL-SDR
//
// Copyright (C) 2013, Joris van Rantwijk.
// Copyright (C) 2015 Edouard Griffiths, F4EXB
// Copyright (C) 2018 Kenji Rikitake, JJ1BDX
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, see http://www.gnu.org/licenses/gpl-2.0.html

#include <algorithm>
#include <atomic>
#include <climits>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <memory>
#include <sys/time.h>
#include <thread>
#include <unistd.h>

#include "AudioOutput.h"
#include "DataBuffer.h"
#include "FmDecode.h"
#include "MovingAverage.h"
#include "RtlSdrSource.h"
#include "SoftFM.h"
#include "util.h"

// Flag is set on SIGINT / SIGTERM.
static std::atomic_bool stop_flag(false);

// Simple linear gain adjustment.
void adjust_gain(SampleVector &samples, double gain) {
  for (unsigned int i = 0, n = samples.size(); i < n; i++) {
    samples[i] *= gain;
  }
}

// Read data from source device and put it in a buffer.
// This code runs in a separate thread.
// The RTL-SDR library is not capable of buffering large amounts of data.
// Running this in a background thread ensures that the time between calls
// to RtlSdrSource::get_samples() is very short.
void read_source_data(RtlSdrSource *rtlsdr, DataBuffer<IQSample> *buf) {
  IQSampleVector iqsamples;

  while (!stop_flag.load()) {

    if (!rtlsdr->get_samples(iqsamples)) {
      fprintf(stderr, "ERROR: RtlSdr: %s\n", rtlsdr->error().c_str());
      exit(1);
    }

    buf->push(move(iqsamples));
  }

  buf->push_end();
}

// Get data from output buffer and write to output stream.
// This code runs in a separate thread.
void write_output_data(AudioOutput *output, DataBuffer<Sample> *buf,
                       unsigned int buf_minfill) {
  while (!stop_flag.load()) {

    if (buf->queued_samples() == 0) {
      // The buffer is empty. Perhaps the output stream is consuming
      // samples faster than we can produce them. Wait until the buffer
      // is back at its nominal level to make sure this does not happen
      // too often.
      buf->wait_buffer_fill(buf_minfill);
    }

    if (buf->pull_end_reached()) {
      // Reached end of stream.
      break;
    }

    // Get samples from buffer and write to output.
    SampleVector samples = buf->pull();
    output->write(samples);
    if (!(*output)) {
      fprintf(stderr, "ERROR: AudioOutput: %s\n", output->error().c_str());
    }
  }
}

// Handle Ctrl-C and SIGTERM.
static void handle_sigterm(int sig) {
  stop_flag.store(true);

  std::string msg = "\nGot signal ";
  msg += strsignal(sig);
  msg += ", stopping ...\n";

  const char *s = msg.c_str();
  ssize_t size = write(STDERR_FILENO, s, strlen(s));
  size++; // dummy
}

void usage() {
  fprintf(
      stderr,
      "Usage: softfm -f freq [options]\n"
      "  -f freq       Frequency of radio station in Hz\n"
      "  -d devidx     RTL-SDR device index, 'list' to show device list "
      "(default 0)\n"
      "  -g gain       Set LNA gain in dB, or 'auto' (default auto)\n"
      "  -a            Enable RTL AGC mode (default disabled)\n"
      "  -r pcmrate    Audio sample rate in Hz (default 48000)\n"
      "  -R filename   Write audio data as raw S16_LE samples\n"
      "                use filename '-' to write to stdout\n"
      "                (default output mode)\n"
      "  -W filename   Write audio data to .WAV file\n"
      "  -T filename   Write pulse-per-second timestamps\n"
      "                use filename '-' to write to stdout\n"
      "  -b seconds    Set audio buffer size in seconds\n"
      "  -q            Set quiet mode\n"
      "  -X            Shift pilot phase (for Quadrature Multipath Monitor)\n"
      "  -U            Set deemphasis to 75 microseconds (default: 50)\n"
      "  -L            Set if sample rate to 240kHz (default: 960kHz)\n"
      "\n");
}

void badarg(const char *label) {
  usage();
  fprintf(stderr, "ERROR: Invalid argument for %s\n", label);
  exit(1);
}

bool parse_int(const char *s, int &v, bool allow_unit = false) {
  char *endp;
  long t = strtol(s, &endp, 10);
  if (endp == s)
    return false;
  if (allow_unit && *endp == 'k' && t > INT_MIN / 1000 && t < INT_MAX / 1000) {
    t *= 1000;
    endp++;
  }
  if (*endp != '\0' || t < INT_MIN || t > INT_MAX)
    return false;
  v = t;
  return true;
}

// Return Unix time stamp in seconds.
double get_time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + 1.0e-6 * tv.tv_usec;
}

int main(int argc, char **argv) {
  double freq = -1;
  int devidx = 0;
  int lnagain = INT_MIN;
  bool agcmode = false;
  double ifrate = 960000;
  int pcmrate = 48000;
  enum OutputMode { MODE_RAW, MODE_WAV };
  OutputMode outmode = MODE_RAW;
  bool quietmode = false;
  std::string filename;
  std::string ppsfilename;
  FILE *ppsfile = NULL;
  double bufsecs = -1;
  bool pilot_shift = false;
  double if_level_max = 0;
  double if_level_min = 10;
  bool deemphasis_na = false;
  bool low_iffreq = false;
  double ifeq_static_gain = 1.0;
  double ifeq_fit_factor = 0.0;

  fprintf(stderr, "softfm-jj1bdx Version 0.2.3, final\n");
  fprintf(stderr,
          "SoftFM - Software decoder for FM broadcast radio with RTL-SDR\n");

  const struct option longopts[] = {
      {"freq", 1, NULL, 'f'},  {"dev", 1, NULL, 'd'},
      {"gain", 1, NULL, 'g'},  {"pcmrate", 1, NULL, 'r'},
      {"agc", 0, NULL, 'a'},   {"raw", 1, NULL, 'R'},
      {"wav", 1, NULL, 'W'},   {"play", 2, NULL, 'P'},
      {"pps", 1, NULL, 'T'},   {"buffer", 1, NULL, 'b'},
      {"quiet", 1, NULL, 'q'}, {"pilotshift", 0, NULL, 'X'},
      {"usa", 0, NULL, 'U'},   {"lowif", 0, NULL, 'L'},
      {NULL, 0, NULL, 0}};

  int c, longindex;
  while ((c = getopt_long(argc, argv, "f:d:g:r:R:W:P::T:b:aqXUL", longopts,
                          &longindex)) >= 0) {
    switch (c) {
    case 'f':
      if (!parse_dbl(optarg, freq) || freq <= 0) {
        badarg("-f");
      }
      break;
    case 'd':
      if (!parse_int(optarg, devidx))
        devidx = -1;
      break;
    case 'g':
      if (strcasecmp(optarg, "auto") == 0) {
        lnagain = INT_MIN;
      } else if (strcasecmp(optarg, "list") == 0) {
        lnagain = INT_MIN + 1;
      } else {
        double tmpgain;
        if (!parse_dbl(optarg, tmpgain)) {
          badarg("-g");
        }
        long int tmpgain2 = lrint(tmpgain * 10);
        if (tmpgain2 <= INT_MIN || tmpgain2 >= INT_MAX) {
          badarg("-g");
        }
        lnagain = tmpgain2;
      }
      break;
    case 'r':
      if (!parse_int(optarg, pcmrate, true) || pcmrate < 1) {
        badarg("-r");
      }
      break;
    case 'R':
      outmode = MODE_RAW;
      filename = optarg;
      break;
    case 'W':
      outmode = MODE_WAV;
      filename = optarg;
      break;
    case 'T':
      ppsfilename = optarg;
      break;
    case 'b':
      if (!parse_dbl(optarg, bufsecs) || bufsecs < 0) {
        badarg("-b");
      }
      break;
    case 'a':
      agcmode = true;
      break;
    case 'q':
      quietmode = true;
      break;
    case 'X':
      pilot_shift = true;
      break;
    case 'U':
      deemphasis_na = true;
      break;
    case 'L':
      low_iffreq = true;
      break;
    default:
      usage();
      fprintf(stderr, "ERROR: Invalid command line options\n");
      exit(1);
    }
  }

  if (optind < argc) {
    usage();
    fprintf(stderr, "ERROR: Unexpected command line options\n");
    exit(1);
  }

  std::vector<std::string> devnames = RtlSdrSource::get_device_names();
  if (devidx < 0 || (unsigned int)devidx >= devnames.size()) {
    if (devidx != -1) {
      fprintf(stderr, "ERROR: invalid device index %d\n", devidx);
    }
    if (!quietmode) {
      fprintf(stderr, "Found %u devices:\n", (unsigned int)devnames.size());
      for (unsigned int i = 0; i < devnames.size(); i++) {
        fprintf(stderr, "%2u: %s\n", i, devnames[i].c_str());
      }
    }
    exit(1);
  }
  if (!quietmode) {
    fprintf(stderr, "using device %d: %s\n", devidx, devnames[devidx].c_str());
  }

  if (freq <= 0) {
    usage();
    fprintf(stderr, "ERROR: Specify a tuning frequency\n");
    exit(1);
  }

  if (low_iffreq) {
    ifrate = 240000;
    ifeq_static_gain = 1.47112063;
    ifeq_fit_factor = 0.48567701;
  } else {
    ifrate = 960000;
    ifeq_static_gain = 1.3412962;
    ifeq_fit_factor = 0.34135089;
  }

  // Catch Ctrl-C and SIGTERM
  struct sigaction sigact;
  sigact.sa_handler = handle_sigterm;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = SA_RESETHAND;
  if (sigaction(SIGINT, &sigact, NULL) < 0) {
    fprintf(stderr, "WARNING: can not install SIGINT handler (%s)\n",
            strerror(errno));
  }
  if (sigaction(SIGTERM, &sigact, NULL) < 0) {
    fprintf(stderr, "WARNING: can not install SIGTERM handler (%s)\n",
            strerror(errno));
  }

  // Intentionally tune at a higher frequency to avoid DC offset.
  double tuner_freq = freq + 0.2 * ifrate;

  // Open RTL-SDR device.
  RtlSdrSource rtlsdr(devidx);
  if (!rtlsdr) {
    fprintf(stderr, "ERROR: RtlSdr: %s\n", rtlsdr.error().c_str());
    exit(1);
  }

  // Check LNA gain.
  if (lnagain != INT_MIN) {
    std::vector<int> gains = rtlsdr.get_tuner_gains();
    if (find(gains.begin(), gains.end(), lnagain) == gains.end()) {
      if (lnagain != INT_MIN + 1) {
        fprintf(stderr, "ERROR: LNA gain %.1f dB not supported by tuner\n",
                lnagain * 0.1);
      }
      fprintf(stderr, "Supported LNA gains: ");
      for (int g : gains) {
        fprintf(stderr, " %.1f dB ", 0.1 * g);
      }
      fprintf(stderr, "\n");
      exit(1);
    }
  }

  // Configure RTL-SDR device and start streaming.
  rtlsdr.configure(ifrate, tuner_freq, lnagain,
                   RtlSdrSource::default_block_length, agcmode);
  if (!rtlsdr) {
    fprintf(stderr, "ERROR: RtlSdr: %s\n", rtlsdr.error().c_str());
    exit(1);
  }

  tuner_freq = rtlsdr.get_frequency();
  ifrate = rtlsdr.get_sample_rate();

  if (!quietmode) {
    fprintf(stderr, "First tuned for:   %.6f MHz\n", freq * 1.0e-6);
    fprintf(stderr, "device tuned for:  %.6f MHz\n", tuner_freq * 1.0e-6);
    if (lnagain == INT_MIN) {
      fprintf(stderr, "LNA gain:          auto\n");
    } else {
      fprintf(stderr, "LNA gain:          %.1f dB\n",
              0.1 * rtlsdr.get_tuner_gain());
    }
    fprintf(stderr, "IF sample rate:    %.0f Hz\n", ifrate);
    fprintf(stderr, "RTL AGC mode:      %s\n",
            agcmode ? "enabled" : "disabled");
  }

  double delta_if = tuner_freq - freq;
  MovingAverage<float> ppm_average(40, 0.0f);

  // Create source data queue.
  DataBuffer<IQSample> source_buffer;

  // Start reading from device in separate thread.
  std::thread source_thread(read_source_data, &rtlsdr, &source_buffer);

  // We can downsample to the (default_bandwidth_if * 2) * 1.1
  // without loss of information.
  // This will speed up later processing stages.
  double downsample_target = FmDecoder::default_bandwidth_if * 2.2;
  unsigned int downsample = std::max(1, int(ifrate / downsample_target));

  // Prevent aliasing at very low output sample rates.
  double default_bandwidth_pcm = FmDecoder::default_bandwidth_pcm;
  double bandwidth_pcm = std::min(default_bandwidth_pcm, 0.45 * pcmrate);
  double deemphasis = deemphasis_na ? 75.0 : 50.0;

  if (!quietmode) {
    fprintf(stderr, "if -> baseband:    %u (downsampled by)\n", downsample);
    fprintf(stderr, "audio sample rate: %u Hz\n", pcmrate);
    fprintf(stderr, "audio bandwidth:   %.3f kHz\n", bandwidth_pcm * 1.0e-3);
    fprintf(stderr, "deemphasis:        %.1f microseconds\n", deemphasis);
  }

  // Prepare decoder.
  FmDecoder fm(ifrate,                          // sample_rate_if
               ifeq_static_gain,                // ifeq_static_gain
               ifeq_fit_factor,                 // ifeq_fit_factor
               freq - tuner_freq,               // tuning_offset
               pcmrate,                         // sample_rate_pcm
               deemphasis,                      // deemphasis,
               FmDecoder::default_bandwidth_if, // bandwidth_if
               FmDecoder::default_freq_dev,     // freq_dev
               bandwidth_pcm,                   // bandwidth_pcm
               downsample,                      // downsample
               pilot_shift);                    // pilot_shift

  // Calculate number of samples in audio buffer.
  unsigned int outputbuf_samples = 0;
  if (bufsecs < 0 && (outmode == MODE_RAW && filename == "-")) {
    // Set default buffer to 1 second for interactive output streams.
    outputbuf_samples = pcmrate;
  } else if (bufsecs > 0) {
    // Calculate nr of samples for configured buffer length.
    outputbuf_samples = (unsigned int)(bufsecs * pcmrate);
  }
  if (outputbuf_samples > 0 && !quietmode) {
    fprintf(stderr, "output buffer:     %.1f seconds\n",
            outputbuf_samples / double(pcmrate));
  }

  // Open PPS file.
  if (!ppsfilename.empty()) {
    if (ppsfilename == "-") {
      if (!quietmode) {
        fprintf(stderr, "writing pulse-per-second markers to stdout\n");
      }
      ppsfile = stdout;
    } else {
      if (!quietmode) {
        fprintf(stderr, "writing pulse-per-second markers to '%s'\n",
                ppsfilename.c_str());
      }
      ppsfile = fopen(ppsfilename.c_str(), "w");
      if (ppsfile == NULL) {
        fprintf(stderr, "ERROR: can not open '%s' (%s)\n", ppsfilename.c_str(),
                strerror(errno));
        exit(1);
      }
    }
    fprintf(ppsfile, "#pps_index sample_index   unix_time\n");
    fflush(ppsfile);
  }

  // Prepare output writer.
  std::unique_ptr<AudioOutput> audio_output;
  switch (outmode) {
  case MODE_RAW:
    if (!quietmode) {
      fprintf(stderr, "writing raw 16-bit audio samples to '%s'\n",
              filename.c_str());
    }
    audio_output.reset(new RawAudioOutput(filename));
    break;
  case MODE_WAV:
    if (!quietmode) {
      fprintf(stderr, "writing audio samples to '%s'\n", filename.c_str());
    }
    audio_output.reset(new WavAudioOutput(filename, pcmrate));
    break;
  }

  if (!(*audio_output)) {
    fprintf(stderr, "ERROR: AudioOutput: %s\n", audio_output->error().c_str());
    exit(1);
  }

  // If buffering enabled, start background output thread.
  DataBuffer<Sample> output_buffer;
  std::thread output_thread;
  if (outputbuf_samples > 0) {
    const unsigned int nchannel = 2;
    output_thread = std::thread(write_output_data, audio_output.get(),
                                &output_buffer, outputbuf_samples * nchannel);
  }

  SampleVector audiosamples;
  bool inbuf_length_warning = false;
  bool got_stereo = false;

  double block_time = get_time();

  // Main loop.
  for (unsigned int block = 0; !stop_flag.load(); block++) {

    // Check for overflow of source buffer.
    if (!inbuf_length_warning && source_buffer.queued_samples() > 10 * ifrate) {
      fprintf(stderr, "\nWARNING: Input buffer is growing (system too slow)\n");
      inbuf_length_warning = true;
    }

    // Pull next block from source buffer.
    IQSampleVector iqsamples = source_buffer.pull();
    if (iqsamples.empty())
      break;

    double prev_block_time = block_time;
    block_time = get_time();

    // Decode FM signal.
    fm.process(iqsamples, audiosamples);

    // Set nominal audio volume.
    adjust_gain(audiosamples, 0.5);

    // The minus factor is to show the ppm correction to make and not the one
    // made
    ppm_average.feed(((fm.get_tuning_offset() + delta_if) / tuner_freq) *
                     -1.0e6);

    // Write PPS markers.
    if (ppsfile != NULL) {
      for (const PilotPhaseLock::PpsEvent &ev : fm.get_pps_events()) {
        double ts = prev_block_time;
        ts += ev.block_position * (block_time - prev_block_time);
        fprintf(ppsfile, "%8s %14s %18.6f\n",
                std::to_string(ev.pps_index).c_str(),
                std::to_string(ev.sample_index).c_str(), ts);
        fflush(ppsfile);
      }
    }

    // Throw away first block. It is noisy because IF filters
    // are still starting up.
    if (block > 0) {
      if (outputbuf_samples > 0) {
        // Buffered write.
        output_buffer.push(move(audiosamples));
      } else {
        // Direct write.
        audio_output->write(audiosamples);
      }
    }

    // Show statistics.
    if (!quietmode) {

      // Estimate D/U ratio, skip first 10 blocks.
      double if_level = fm.get_if_level();
      double du_ratio = 2;
      if_level_max = std::max(if_level_max, if_level);
      if_level_min = std::min(if_level_min, if_level);
      if (block > 10) {
        double ratio = if_level_max / if_level_min;
        du_ratio = (ratio + 1) / (ratio - 1);
      }

      fprintf(stderr,
              "\rblk=%6d:f=%8.4fMHz:ppm=%+6.2f:IF=%+6.2fdBpp:"
              "DU=%6.2fdB:BB=%+5.1fdB",
              block, (tuner_freq + fm.get_tuning_offset()) * 1.0e-6,
              ppm_average.average(), 20 * log10(if_level), 20 * log10(du_ratio),
              20 * log10(fm.get_baseband_level()) + 3.01);
      if (outputbuf_samples > 0) {
        const unsigned int nchannel = 2;
        size_t buflen = output_buffer.queued_samples();
        fprintf(stderr, ":buf=%.1fs ", buflen / nchannel / double(pcmrate));
      }
      fflush(stderr);

      // Show stereo status.
      if (fm.stereo_detected() != got_stereo) {
        got_stereo = fm.stereo_detected();
        if (got_stereo) {
          fprintf(stderr, "\ngot stereo signal (pilot level = %f)\n",
                  fm.get_pilot_level());
        } else {
          fprintf(stderr, "\nlost stereo signal\n");
        }
      }
    }
  }

  // Join background threads.
  source_thread.join();
  if (outputbuf_samples > 0) {
    output_buffer.push_end();
    output_thread.join();
  }

  // No cleanup needed; everything handled by destructors.

  return 0;
}

// end
