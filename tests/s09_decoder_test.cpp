// S09: audio decoder tests. Hand-built ADPCM streams with expected PCM worked out from the
// algorithms by hand (derivations in the comments). No game assets.
//
//   Decompress_Frame               WIN32LIB/AUDIO/AUDUNCMP.CPP  (Westwood ADPCM, was AUDUNCMP.ASM)
//   sosCODECDecompressData         WIN32LIB/AUDIO/SOSCODEC.CPP  (IMA ADPCM, 16-bit mono fast path)
//   General_sosCODECDecompressData WIN32LIB/AUDIO/OLSOSDEC.CPP  (IMA ADPCM, any format)
#include "ra_test.h"

#include "soundint.h"

#include <vector>

// The original C++ decoder tables, CODE/DTABLE.CPP and CODE/ITABLE.CPP, for comparison.
namespace ref {
#include "DTABLE.CPP"
#include "ITABLE.CPP"
}

namespace {

_SOS_COMPRESS_INFO Make_Info(int bits, int channels)
{
    _SOS_COMPRESS_INFO info;
    std::memset(&info, 0x5A, sizeof(info));     // garbage, so init must set what matters
    info.wBitSize = (short)bits;
    info.wChannels = (short)channels;
    sosCODECInitStream(&info);
    return info;
}

short Get16(std::vector<unsigned char> const &v, size_t i)
{
    return (short)(unsigned short)(v[i * 2] | (v[i * 2 + 1] << 8));
}

// The algorithm of CODE/ADPCM.CPP with the CODE tables, but taking a sample count, as the
// asm did (ADPCM.CPP took a byte count; see the port note in SOSCODEC.CPP).
void Reference_Decode(unsigned char const *in, unsigned long samples, short *out, unsigned &index, long &sample)
{
    unsigned fastindex = index * 16;
    for (unsigned long i = 0; i < samples; i++) {
        unsigned token = (i & 1) ? (in[i >> 1] >> 4) : (in[i >> 1] & 0x0F);
        fastindex += token;
        sample += ref::DiffTable[fastindex];
        fastindex = ref::IndexTable[fastindex];
        if (sample > 32767L) sample = 32767L;
        if (sample < -32768L) sample = -32768L;
        out[i] = (short)sample;
    }
    index = fastindex / 16;
}

std::vector<unsigned char> Pseudo_Random_Bytes(size_t count, unsigned seed)
{
    std::vector<unsigned char> v(count);
    for (size_t i = 0; i < count; i++) {
        seed = seed * 1103515245u + 12345u;
        v[i] = (unsigned char)(seed >> 16);
    }
    return v;
}

} // namespace


// ---------------------------------------------------------------------------------------------
// IMA ADPCM (HMI SOS)
//
// Per nibble: step = StepTab[index]; diff = step>>3 (+step if bit 2) (+step>>1 if bit 1)
// (+step>>2 if bit 0); negate if bit 3; sample = clamp(sample + diff, -32768, 32767);
// index = clamp(index + IndexAdjust[nibble], 0, 88). Low nibble first. Start: index 0, sample 0.
// ---------------------------------------------------------------------------------------------

RA_TEST(sos_tables_match_code_adpcm_tables)
{
    // The fast decoder builds its tables from the step table; they must equal the tables that
    // came with CODE/ADPCM.CPP (89 step indexes x 16 tokens).
    RA_CHECK_EQ(89 * 16, (int)(sizeof(ref::DiffTable) / sizeof(ref::DiffTable[0])));
    RA_CHECK_EQ(89 * 16, (int)(sizeof(ref::IndexTable) / sizeof(ref::IndexTable[0])));
    long const *diff = SOS_Fast_Diff_Table();
    unsigned short const *index = SOS_Fast_Index_Table();
    int mismatches = 0;
    for (int i = 0; i < 89 * 16; i++) {
        if (diff[i] != ref::DiffTable[i] || index[i] != ref::IndexTable[i]) mismatches++;
    }
    RA_CHECK_EQ(0, mismatches);
}

RA_TEST(sos_init_stream_sets_both_channels)
{
    _SOS_COMPRESS_INFO info = Make_Info(16, 2);
    RA_CHECK_EQ(0, info.wIndex);
    RA_CHECK_EQ(7, info.wStep);
    RA_CHECK_EQ(0, info.dwPredicted);
    RA_CHECK_EQ(0, (long)info.dwSampleIndex);
    RA_CHECK_EQ(0, info.wIndex2);
    RA_CHECK_EQ(7, info.wStep2);
    RA_CHECK_EQ(0, info.dwPredicted2);
    RA_CHECK_EQ(0, (long)info.dwSampleIndex2);
}

// Input 0x07 0x8F (nibbles in order: 7, 0, F, 8):
//   7: step 7:  diff = 0 + 7 + 3 + 1 = 11            -> 11;  index 0+8 = 8  (step 16)
//   0: step 16: diff = 16>>3 = 2                     -> 13;  index 8-1 = 7  (step 14)
//   F: step 14: diff = -(1 + 14 + 7 + 3) = -25       -> -12; index 7+8 = 15 (step 31)
//   8: step 31: diff = -(31>>3) = -3                 -> -15; index 15-1 = 14 (step 28)
static unsigned char const ImaIn[2] = {0x07, 0x8F};
static short const ImaOut[4] = {11, 13, -12, -15};

RA_TEST(sos_fast_decoder_16bit_mono_vector)
{
    _SOS_COMPRESS_INFO info = Make_Info(16, 1);
    std::vector<unsigned char> out(8, 0xCC);
    info.lpSource = (char *)ImaIn;
    info.lpDest = (char *)&out[0];
    RA_CHECK_EQ(8, (long)sosCODECDecompressData(&info, 8));   // size counts OUTPUT bytes
    for (int i = 0; i < 4; i++) RA_CHECK_EQ(ImaOut[i], Get16(out, i));
    RA_CHECK_EQ(14, info.wIndex);
    RA_CHECK_EQ(28, info.wStep);
    RA_CHECK_EQ(-15, info.dwPredicted);
}

RA_TEST(sos_general_decoder_16bit_mono_vector)
{
    _SOS_COMPRESS_INFO info = Make_Info(16, 1);
    std::vector<unsigned char> out(8, 0xCC);
    info.lpSource = (char *)ImaIn;
    info.lpDest = (char *)&out[0];
    RA_CHECK_EQ(8, (long)General_sosCODECDecompressData(&info, 8));
    for (int i = 0; i < 4; i++) RA_CHECK_EQ(ImaOut[i], Get16(out, i));
    RA_CHECK_EQ(14, info.wIndex);
    RA_CHECK_EQ(28, info.wStep);
}

// Clamping, starting from sample 32760 at step index 88 (step 32767), input 0xF7:
//   7: diff = 4095 + 32767 + 16383 + 8191 = 61436 -> 32760 + 61436 clamps to 32767; index 88
//   F: diff = -61436                             -> 32767 - 61436 = -28669
RA_TEST(sos_decoders_clamp)
{
    unsigned char const in[1] = {0xF7};
    for (int general = 0; general < 2; general++) {
        _SOS_COMPRESS_INFO info = Make_Info(16, 1);
        info.dwPredicted = 32760;
        info.wIndex = 88;
        info.wStep = 32767;
        std::vector<unsigned char> out(4);
        info.lpSource = (char *)in;
        info.lpDest = (char *)&out[0];
        if (general) General_sosCODECDecompressData(&info, 4); else sosCODECDecompressData(&info, 4);
        RA_CHECK_EQ(32767, Get16(out, 0));
        RA_CHECK_EQ(-28669, Get16(out, 1));
        RA_CHECK_EQ(88, info.wIndex);
    }
}

// 8-bit output is the high byte of the 16-bit sample, made unsigned (xor 0x80):
// 32767 = 0x7FFF -> 0x7F ^ 0x80 = 0xFF; -28669 = 0x9003 -> 0x90 ^ 0x80 = 0x10.
RA_TEST(sos_general_decoder_8bit_mono)
{
    unsigned char const in[1] = {0xF7};
    _SOS_COMPRESS_INFO info = Make_Info(8, 1);
    info.dwPredicted = 32760;
    info.wIndex = 88;
    info.wStep = 32767;
    unsigned char out[2] = {0, 0};
    info.lpSource = (char *)in;
    info.lpDest = (char *)out;
    RA_CHECK_EQ(2, (long)General_sosCODECDecompressData(&info, 2));   // 8-bit: bytes = samples
    RA_CHECK_EQ(0xFF, out[0]);
    RA_CHECK_EQ(0x10, out[1]);
}

// Stereo: source bytes alternate left/right, each byte holds two samples of its channel.
// Source 07 70 8F 08: left bytes 07 8F -> 11 13 -12 -15 (as above).
// Right bytes 70 08 (nibbles 0, 7, 8, 0):
//   0: step 7:  diff 0          -> 0;  index 0 (clamped from -1)
//   7: step 7:  diff 11         -> 11; index 8  (step 16)
//   8: step 16: diff -2         -> 9;  index 7  (step 14)
//   0: step 14: diff 14>>3 = 1  -> 10; index 6
// Output interleaved L R L R ...
RA_TEST(sos_general_decoder_16bit_stereo)
{
    unsigned char const in[4] = {0x07, 0x70, 0x8F, 0x08};
    short const expected[8] = {11, 0, 13, 11, -12, 9, -15, 10};
    _SOS_COMPRESS_INFO info = Make_Info(16, 2);
    std::vector<unsigned char> out(16, 0xCC);
    info.lpSource = (char *)in;
    info.lpDest = (char *)&out[0];
    RA_CHECK_EQ(16, (long)General_sosCODECDecompressData(&info, 16));
    for (int i = 0; i < 8; i++) RA_CHECK_EQ(expected[i], Get16(out, i));
    RA_CHECK_EQ(14, info.wIndex);
    RA_CHECK_EQ(6, info.wIndex2);
    RA_CHECK_EQ(10, info.dwPredicted2);
}

RA_TEST(sos_zero_length_is_safe)
{
    _SOS_COMPRESS_INFO info = Make_Info(16, 1);
    info.lpSource = 0;
    info.lpDest = 0;
    RA_CHECK_EQ(0, (long)General_sosCODECDecompressData(&info, 0));   // the asm ran away here
    RA_CHECK_EQ(0, (long)sosCODECDecompressData(&info, 0));
}

RA_TEST(sos_fast_decoder_rejects_other_formats)
{
    _SOS_COMPRESS_INFO info = Make_Info(8, 1);
    unsigned char in[1] = {0x77};
    unsigned char out[2] = {0xCC, 0xCC};
    info.lpSource = (char *)in;
    info.lpDest = (char *)out;
    RA_CHECK_EQ(0, (long)sosCODECDecompressData(&info, 2));
    RA_CHECK_EQ(0xCC, out[0]);
}

// A long random stream: the fast decoder, the general decoder and the CODE/ADPCM.CPP
// reference must agree sample for sample, also when decoded in 512-byte frames as
// Sample_Copy does (the state carries over between calls).
RA_TEST(sos_fast_general_and_reference_agree)
{
    const unsigned long in_bytes = 4096;
    std::vector<unsigned char> in = Pseudo_Random_Bytes(in_bytes, 1996);
    const unsigned long samples = in_bytes * 2;

    std::vector<short> reference(samples);
    unsigned ref_index = 0;
    long ref_sample = 0;
    Reference_Decode(&in[0], samples, &reference[0], ref_index, ref_sample);

    for (int general = 0; general < 2; general++) {
        for (int framed = 0; framed < 2; framed++) {
            _SOS_COMPRESS_INFO info = Make_Info(16, 1);
            std::vector<unsigned char> out(samples * 2);
            unsigned long frame = framed ? 512 : in_bytes;
            for (unsigned long pos = 0; pos < in_bytes; pos += frame) {
                info.lpSource = (char *)&in[pos];
                info.lpDest = (char *)&out[pos * 4];
                if (general) General_sosCODECDecompressData(&info, frame * 4);
                else sosCODECDecompressData(&info, frame * 4);
            }
            int mismatches = 0;
            for (unsigned long i = 0; i < samples; i++) {
                if (Get16(out, i) != reference[i]) mismatches++;
            }
            RA_CHECK_EQ(0, mismatches);
            RA_CHECK_EQ((long)ref_index, (long)info.wIndex);
            RA_CHECK_EQ(ref_sample, info.dwPredicted);
        }
    }
}


// ---------------------------------------------------------------------------------------------
// Westwood ADPCM (Decompress_Frame). Code byte: command in bits 6-7, argument in bits 0-5.
// Previous sample starts at 0x80. 2/4-bit deltas clamp to 0..255; the 5-bit delta wraps.
// ---------------------------------------------------------------------------------------------

// 40 9F         4-bit, 1 byte: nibble F -> +8 = 0x88, nibble 9 -> +1 = 0x89
// 00 E4         2-bit, 1 byte: E4 = 11 10 01 00 -> deltas (low bits first) -2 -1 0 +1:
//               0x87 0x86 0x86 0x87
// A3            raw delta (bit 5 set), 5-bit +3: 0x8A
// BF            raw delta -1: 0x89
// 81 10 F0      raw run of 2 bytes: 0x10 0xF0
// C2            silence: 3 more of 0xF0
// 40 FF         4-bit: +8 +8 from 0xF0 = 0xF8, then 0x100 clamps to 0xFF
// AF            raw delta +15 from 0xFF wraps (8-bit add) to 0x0E
// 13 source bytes, 16 output bytes.
RA_TEST(westwood_adpcm_vector)
{
    unsigned char const in[13] = {0x40, 0x9F, 0x00, 0xE4, 0xA3, 0xBF, 0x81, 0x10, 0xF0, 0xC2, 0x40, 0xFF, 0xAF};
    unsigned char const expected[16] = {
        0x88, 0x89, 0x87, 0x86, 0x86, 0x87, 0x8A, 0x89, 0x10, 0xF0, 0xF0, 0xF0, 0xF0, 0xF8, 0xFF, 0x0E
    };
    unsigned char out[17];
    std::memset(out, 0xCC, sizeof(out));
    RA_CHECK_EQ(13, Decompress_Frame((void *)in, out, 16));   // returns source bytes read
    RA_CHECK_MEM_EQ(expected, out, 16);
    RA_CHECK_EQ(0xCC, out[16]);
}

// 80 01   raw run of 1: 0x01
// 00 00   2-bit: four -2 deltas from 0x01: 0x00 (clamped) 0x00 0x00 0x00
RA_TEST(westwood_adpcm_clamps_at_zero)
{
    unsigned char const in[4] = {0x80, 0x01, 0x00, 0x00};
    unsigned char const expected[5] = {0x01, 0x00, 0x00, 0x00, 0x00};
    unsigned char out[5];
    RA_CHECK_EQ(4, Decompress_Frame((void *)in, out, 5));
    RA_CHECK_MEM_EQ(expected, out, 5);
}

// A run longer than the output: the asm overran the buffer; the port stops writing at 'size'
// but reads the source exactly like the asm (C7 = silence x8, one code byte).
RA_TEST(westwood_adpcm_never_writes_past_size)
{
    unsigned char const in[1] = {0xC7};
    unsigned char out[8];
    std::memset(out, 0xCC, sizeof(out));
    RA_CHECK_EQ(1, Decompress_Frame((void *)in, out, 4));
    unsigned char const expected[8] = {0x80, 0x80, 0x80, 0x80, 0xCC, 0xCC, 0xCC, 0xCC};
    RA_CHECK_MEM_EQ(expected, out, 8);
}

RA_TEST(westwood_adpcm_null_arguments)
{
    unsigned char buf[4];
    RA_CHECK_EQ(0, Decompress_Frame(0, buf, 4));
    RA_CHECK_EQ(0, Decompress_Frame(buf, 0, 4));
    RA_CHECK_EQ(0, Decompress_Frame(buf, buf, 0));
}

RA_TEST_MAIN()
