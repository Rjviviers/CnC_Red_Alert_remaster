// S09: software mixer tests with synthetic PCM (WIN32LIB/AUDIO/audio_mixer.cpp).
#include "ra_test.h"

#include "audio_mixer.h"

#include <vector>

namespace {

// A voice source: plays a byte buffer once, in chunks of at most 'chunk' bytes per fill call.
struct Source {
    std::vector<unsigned char> data;
    size_t pos;
    long chunk;
    int fills;
    Source() : pos(0), chunk(1 << 30), fills(0) {}
};

Source Sources[AudioMixerClass::MAX_VOICES];

long Fill(void *, int voice, void *dest, long size)
{
    Source &s = Sources[voice];
    s.fills++;
    long left = (long)(s.data.size() - s.pos);
    long n = size < left ? size : left;
    if (n > s.chunk) n = s.chunk;
    if (n > 0) std::memcpy(dest, &s.data[s.pos], (size_t)n);
    s.pos += (size_t)n;
    return n;
}

void Reset_Sources()
{
    for (int i = 0; i < AudioMixerClass::MAX_VOICES; i++) Sources[i] = Source();
}

void Set16(Source &s, std::vector<short> const &samples)
{
    s.data.resize(samples.size() * 2);
    for (size_t i = 0; i < samples.size(); i++) {
        s.data[i * 2] = (unsigned char)(samples[i] & 0xFF);
        s.data[i * 2 + 1] = (unsigned char)((unsigned short)samples[i] >> 8);
    }
    s.pos = 0;
}

int Ticks = 0;
void Count_Tick(void *) { Ticks++; }

} // namespace

RA_TEST(mixer_unity_passthrough_same_rate)
{
    Reset_Sources();
    AudioMixerClass mixer;
    mixer.Configure(22050, 1);
    mixer.Set_Fill_Callback(Fill, 0);
    std::vector<short> pcm;
    for (int i = 0; i < 1000; i++) pcm.push_back((short)(i * 37 - 18000));
    Set16(Sources[0], pcm);

    RA_CHECK(mixer.Start_Voice(0, 22050, 1, 16, AudioMixerClass::UNITY_GAIN));
    std::vector<short> out(1200, 1234);
    mixer.Mix(&out[0], 1200);
    int mismatches = 0;
    for (int i = 0; i < 1000; i++) if (out[i] != pcm[i]) mismatches++;
    RA_CHECK_EQ(0, mismatches);
    for (int i = 1000; i < 1200; i++) RA_CHECK_EQ(0, out[i]);   // silence after the end
    RA_CHECK(!mixer.Is_Voice_Playing(0));
    RA_CHECK_EQ(1200, (long)mixer.Frames_Mixed());
}

RA_TEST(mixer_handles_tiny_and_odd_fills)
{
    // Fill returns 3 bytes at a time: frames split across fills must still come out right.
    Reset_Sources();
    AudioMixerClass mixer;
    mixer.Configure(11025, 1);
    mixer.Set_Fill_Callback(Fill, 0);
    std::vector<short> pcm;
    for (int i = 0; i < 64; i++) pcm.push_back((short)(i * 500 - 16000));
    Set16(Sources[2], pcm);
    Sources[2].chunk = 3;
    mixer.Start_Voice(2, 11025, 1, 16, AudioMixerClass::UNITY_GAIN);
    std::vector<short> out(64);
    mixer.Mix(&out[0], 64);
    RA_CHECK_MEM_EQ(&pcm[0], &out[0], 64 * sizeof(short));
}

RA_TEST(mixer_8bit_unsigned_and_stereo_to_mono)
{
    Reset_Sources();
    AudioMixerClass mixer;
    mixer.Configure(8000, 1);
    mixer.Set_Fill_Callback(Fill, 0);

    // 8-bit unsigned: 0x80 = 0, 0xFF = 127*256, 0x00 = -128*256.
    Sources[0].data.push_back(0x80);
    Sources[0].data.push_back(0xFF);
    Sources[0].data.push_back(0x00);
    mixer.Start_Voice(0, 8000, 1, 8, AudioMixerClass::UNITY_GAIN);
    short out[3];
    mixer.Mix(out, 3);
    RA_CHECK_EQ(0, out[0]);
    RA_CHECK_EQ(32512, out[1]);
    RA_CHECK_EQ(-32768, out[2]);

    // 16-bit stereo to mono output: (L + R) / 2.
    std::vector<short> st;
    st.push_back(1000); st.push_back(3000);
    st.push_back(-500); st.push_back(-1500);
    Set16(Sources[1], st);
    mixer.Start_Voice(1, 8000, 2, 16, AudioMixerClass::UNITY_GAIN);
    short out2[2];
    mixer.Mix(out2, 2);
    RA_CHECK_EQ(2000, out2[0]);
    RA_CHECK_EQ(-1000, out2[1]);
}

RA_TEST(mixer_stereo_output_and_reverse)
{
    Reset_Sources();
    AudioMixerClass mixer;
    mixer.Set_Fill_Callback(Fill, 0);
    std::vector<short> st;
    st.push_back(100); st.push_back(-200);
    Set16(Sources[0], st);
    Set16(Sources[1], std::vector<short>(1, 300));

    mixer.Configure(8000, 2);
    mixer.Start_Voice(0, 8000, 2, 16, AudioMixerClass::UNITY_GAIN);
    short out[2];
    mixer.Mix(out, 1);
    RA_CHECK_EQ(100, out[0]);
    RA_CHECK_EQ(-200, out[1]);

    Sources[0].pos = 0;
    mixer.Configure(8000, 2, true);       // reverse_channels
    mixer.Start_Voice(0, 8000, 2, 16, AudioMixerClass::UNITY_GAIN);
    mixer.Start_Voice(1, 8000, 1, 16, AudioMixerClass::UNITY_GAIN);   // mono goes to both sides
    mixer.Mix(out, 1);
    RA_CHECK_EQ(-200 + 300, out[0]);
    RA_CHECK_EQ(100 + 300, out[1]);
}

RA_TEST(mixer_sums_voices_and_clips)
{
    Reset_Sources();
    AudioMixerClass mixer;
    mixer.Configure(22050, 1);
    mixer.Set_Fill_Callback(Fill, 0);
    Set16(Sources[0], std::vector<short>(4, 20000));
    Set16(Sources[1], std::vector<short>(4, 20000));
    Set16(Sources[2], std::vector<short>(4, -30000));
    Set16(Sources[3], std::vector<short>(4, -30000));
    mixer.Start_Voice(0, 22050, 1, 16, AudioMixerClass::UNITY_GAIN);
    mixer.Start_Voice(1, 22050, 1, 16, AudioMixerClass::UNITY_GAIN);
    short out[4];
    mixer.Mix(out, 2);
    RA_CHECK_EQ(32767, out[0]);              // 40000 clips
    mixer.Start_Voice(2, 22050, 1, 16, AudioMixerClass::UNITY_GAIN);
    mixer.Start_Voice(3, 22050, 1, 16, AudioMixerClass::UNITY_GAIN);
    mixer.Mix(out, 2);
    RA_CHECK_EQ(-20000, out[0]);             // 20000 + 20000 - 30000 - 30000
    mixer.Stop_Voice(2);
    mixer.Stop_Voice(3);
    mixer.Stop_Voice(0);
    mixer.Stop_Voice(1);
    Set16(Sources[2], std::vector<short>(4, -30000));
    Set16(Sources[3], std::vector<short>(4, -30000));
    mixer.Start_Voice(2, 22050, 1, 16, AudioMixerClass::UNITY_GAIN);
    mixer.Start_Voice(3, 22050, 1, 16, AudioMixerClass::UNITY_GAIN);
    mixer.Mix(out, 1);
    RA_CHECK_EQ(-32768, out[0]);             // -60000 clips
}

RA_TEST(mixer_voice_and_master_gain)
{
    Reset_Sources();
    AudioMixerClass mixer;
    mixer.Configure(22050, 1);
    mixer.Set_Fill_Callback(Fill, 0);
    Set16(Sources[0], std::vector<short>(8, 10000));
    mixer.Start_Voice(0, 22050, 1, 16, AudioMixerClass::UNITY_GAIN / 2);
    short out[2];
    mixer.Mix(out, 1);
    RA_CHECK_EQ(5000, out[0]);               // voice gain 0.5
    RA_CHECK_EQ(AudioMixerClass::UNITY_GAIN / 2, mixer.Voice_Gain(0));
    mixer.Set_Voice_Gain(0, AudioMixerClass::UNITY_GAIN / 4);
    mixer.Mix(out, 1);
    RA_CHECK_EQ(2500, out[0]);
    mixer.Set_Master_Gain(AudioMixerClass::UNITY_GAIN / 2);
    mixer.Mix(out, 1);
    RA_CHECK_EQ(1250, out[0]);
    mixer.Set_Voice_Gain(0, 0);
    mixer.Mix(out, 1);
    RA_CHECK_EQ(0, out[0]);
    RA_CHECK(mixer.Is_Voice_Playing(0));     // silent, but still playing
}

// Upsampling 11025 -> 22050 Hz: step = 0.5 source frames per output frame, so the output is
// each source sample followed by the midpoint to the next one (linear interpolation).
// Source 0, 1000, 3000 -> 0, 500, 1000, 2000, 3000, then the last sample held for the half
// step that remains (3000), then the voice ends.
RA_TEST(mixer_resamples_linearly)
{
    Reset_Sources();
    AudioMixerClass mixer;
    mixer.Configure(22050, 1);
    mixer.Set_Fill_Callback(Fill, 0);
    std::vector<short> pcm;
    pcm.push_back(0); pcm.push_back(1000); pcm.push_back(3000);
    Set16(Sources[0], pcm);
    mixer.Start_Voice(0, 11025, 1, 16, AudioMixerClass::UNITY_GAIN);
    short out[8];
    mixer.Mix(out, 8);
    short const expected[8] = {0, 500, 1000, 2000, 3000, 3000, 0, 0};
    RA_CHECK_MEM_EQ(expected, out, sizeof(expected));
    RA_CHECK(!mixer.Is_Voice_Playing(0));

    // Downsampling 44100 -> 22050 Hz takes every other sample.
    pcm.clear();
    for (int i = 0; i < 8; i++) pcm.push_back((short)(i * 100));
    Set16(Sources[1], pcm);
    mixer.Start_Voice(1, 44100, 1, 16, AudioMixerClass::UNITY_GAIN);
    short down[5];
    mixer.Mix(down, 5);
    short const expected_down[5] = {0, 200, 400, 600, 0};
    RA_CHECK_MEM_EQ(expected_down, down, sizeof(expected_down));
}

RA_TEST(mixer_stop_and_restart_voice)
{
    Reset_Sources();
    AudioMixerClass mixer;
    mixer.Configure(22050, 1);
    mixer.Set_Fill_Callback(Fill, 0);
    Set16(Sources[0], std::vector<short>(100, 777));
    mixer.Start_Voice(0, 22050, 1, 16, AudioMixerClass::UNITY_GAIN);
    short out[4];
    mixer.Mix(out, 2);
    RA_CHECK_EQ(777, out[1]);
    mixer.Stop_Voice(0);
    RA_CHECK(!mixer.Is_Voice_Playing(0));
    mixer.Mix(out, 2);
    RA_CHECK_EQ(0, out[0]);
    RA_CHECK(!mixer.Start_Voice(0, 0, 1, 16, AudioMixerClass::UNITY_GAIN));      // bad rate
    RA_CHECK(!mixer.Start_Voice(0, 22050, 3, 16, AudioMixerClass::UNITY_GAIN));  // bad channels
    RA_CHECK(!mixer.Start_Voice(AudioMixerClass::MAX_VOICES, 22050, 1, 16, 0));  // bad voice
}

RA_TEST(mixer_empty_voice_ends_immediately)
{
    Reset_Sources();
    AudioMixerClass mixer;
    mixer.Configure(22050, 1);
    mixer.Set_Fill_Callback(Fill, 0);
    mixer.Start_Voice(5, 22050, 1, 16, AudioMixerClass::UNITY_GAIN);
    RA_CHECK(mixer.Is_Voice_Playing(5));        // until the first mix asks for data
    short out[4];
    mixer.Mix(out, 4);
    RA_CHECK(!mixer.Is_Voice_Playing(5));
    RA_CHECK_EQ(1, Sources[5].fills);
}

// The tick fires every rate/hz output frames, however the mix calls are split.
RA_TEST(mixer_tick_rate)
{
    AudioMixerClass mixer;
    mixer.Configure(22000, 1);
    Ticks = 0;
    mixer.Set_Tick_Callback(Count_Tick, 0, 40);   // every 550 frames
    std::vector<short> out(22000);
    mixer.Mix(&out[0], 549);
    RA_CHECK_EQ(0, Ticks);
    mixer.Mix(&out[0], 1);
    RA_CHECK_EQ(1, Ticks);
    for (int i = 0; i < 39; i++) mixer.Mix(&out[0], 550);   // whole seconds total
    RA_CHECK_EQ(40, Ticks);
    mixer.Mix(&out[0], 22000);
    RA_CHECK_EQ(80, Ticks);
}

RA_TEST_MAIN()
