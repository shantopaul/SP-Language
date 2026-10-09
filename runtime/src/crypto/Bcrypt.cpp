#include "sp/runtime/Password.h"
#include "sp/runtime/Crypto.h"
#include <cstring>
#include <vector>
#include <array>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace sp::rt::crypto {

namespace {

// ============================================================================
// Blowfish Constants (Fractional parts of Pi)
// ============================================================================

static const uint32_t BF_INIT_P[18] = {
    0x243f6a88, 0x85a308d3, 0x13198a2e, 0x03707344,
    0xa4093822, 0x299f31d0, 0x082efa98, 0xec4e6c89,
    0x452821e6, 0x38d01377, 0xbe5466cf, 0x34e90c6c,
    0xc0ac29b7, 0xc97c50dd, 0x3f84d5b5, 0xb5470917,
    0x9216d5d9, 0x8979fb1b
};

static const uint32_t BF_INIT_S[4][256] = {
    {
        0xd1310ba6, 0x98dfb5ac, 0x2ffd72db, 0xd01adfb7,
        0xb8e1afed, 0x6a267e96, 0xba7c9045, 0xf12c7f99,
        0x24a19947, 0xb3916cf7, 0x0801f2e2, 0x858efc16,
        0x636920d8, 0x71574e69, 0xa458fea3, 0xf4933d7e,
        0x0d95748f, 0x728eb658, 0x718bcd58, 0x82154aee,
        0x7b54a41d, 0xc25a59b5, 0x9c30d539, 0x2af26013,
        0xc5d1b023, 0x286085f0, 0xca417918, 0xb8db38ef,
        0x8e79dcb0, 0x603a180e, 0x6c9e0e8b, 0xb01e8a3e,
        0xd71577c1, 0xbd314b27, 0x78af2fda, 0x55605c60,
        0xe65525f3, 0xaa55ab94, 0x57489862, 0x63e81440,
        0x55ca396a, 0x2aab10b6, 0xb4cc5c34, 0x1141e8ce,
        0xa15486af, 0x7c72e993, 0xb3ee1411, 0x636fbc2a,
        0x2ba9c55d, 0x741831f6, 0xce5c3e16, 0x9b87931e,
        0xafd6ba33, 0x6c24cf5c, 0x7a325381, 0x28958677,
        0x3b8f4898, 0x6b4bb9af, 0xc4bfe81b, 0x66282193,
        0x61d809cc, 0xfb21a991, 0x487cac60, 0x5dec8032,
        0xef845d5d, 0x6a1476a8, 0x51909e16, 0xa4315383,
        0xf7c0c541, 0x808b2fa1, 0x4299dfce, 0x4c73a2db,
        0xd842e339, 0x8285c52a, 0x435b0f1d, 0x79fd7005,
        0x07f28709, 0xbc9ec6b4, 0x2bc5a9e4, 0x6a049e77,
        0x1039ffd8, 0x8a722478, 0x86047702, 0x9a4c52f6,
        0x54bf69f1, 0x9b404331, 0x405dd301, 0x871a3967,
        0x437a3072, 0x1604714f, 0x23f7046f, 0x2647f322,
        0x34cc5ccb, 0x65b6427a, 0xb28b7e77, 0x7b6e22f8,
        0x8614b10e, 0x76d56d4e, 0x43dc7404, 0xfe4380a4,
        0xbaaffe82, 0xadc334cc, 0x72a0c4f8, 0x80a18920,
        0xfb3e714e, 0x4cdd7030, 0x47a28a0f, 0x8b44442e,
        0xf54a2958, 0xb2e4e2da, 0x977716a0, 0xca1420f2,
        0x074e4428, 0x5e13ff88, 0xb03054c0, 0xd0da5d42,
        0x3da4c4d3, 0x17779974, 0xb5e1ceb9, 0xaba6d877,
        0x840878d3, 0x8ff6a0b5, 0x431d7720, 0x579f3154,
        0xaae24715, 0x04e5aa5f, 0x97dc73cc, 0x863837a6,
        0x5dff09d4, 0x5cd54ca6, 0xeb172776, 0xaf6e66a2,
        0x633e7fdb, 0x83c77b1a, 0xbfeff30f, 0x602930fa,
        0x67c29c17, 0x4b78693c, 0x8ae6d16d, 0x3a8fa951,
        0x633e7fdb, 0x83c77b1a, 0x1e31f79c, 0x50ffda38,
        0x98b8140f, 0xf65c9be2, 0xa84af419, 0xb65b74e3,
        0x1c07b191, 0x453b2f00, 0xe3816041, 0x0ce26f1b,
        0x4f6ebc0e, 0x6be470a4, 0xafe42443, 0xa30f2c01,
        0xeb12546f, 0x22254f3b, 0x545723f1, 0x0f402364,
        0xb029d65b, 0xab5840f4, 0x93cc99e5, 0x55631584,
        0x19224dcf, 0x1cd8c613, 0x22831b4e, 0x3839ff90,
        0x5c723601, 0x808cce65, 0x2cff3f8f, 0xb9a848ad,
        0xfeaf4bf9, 0x9e14a26b, 0x83e545d6, 0xb3302664,
        0x73d5d8cd, 0xd467b64e, 0x2d17042b, 0x47164b4f,
        0xd1869ee9, 0x4d10400f, 0x6c9f3c65, 0x081cbd27,
        0xa3e0a600, 0x87178f2e, 0x437a2e66, 0xb0be4f81,
        0xac378ce9, 0x00c74c72, 0xd0eb61ce, 0x2be45e24,
        0xb3476646, 0x736ac383, 0xcd300877, 0x286ff7e8,
        0x3d6b60ce, 0xdbf725f9, 0x9dd52542, 0x74f9c9ee,
        0xbac06424, 0x05e15162, 0xdf7f66a8, 0x2772a702,
        0x4086601b, 0x579f6153, 0xb4bcbf68, 0x38777602,
        0x52424b99, 0x10473d0b, 0x2799249d, 0x460d6211,
        0xa07f649f, 0x5630d942, 0xe077d769, 0x65b1288b,
        0x23ea37b2, 0x746f3e3a, 0x36cb0244, 0xbcf88024,
        0x879c45e5, 0x99da3fb3, 0x3a411364, 0xb3150298,
        0xb432d0f5, 0xae2080e4, 0x53d06d40, 0x46405f1f,
        0xd9664559, 0x9437ec6b, 0x3bf7b17f, 0x6787b7bf,
        0x8b16f241, 0x7022087c, 0x3b147a16, 0x280244c8,
        0x9ff3c9fb, 0xaaf43670, 0x65ddfd82, 0xd69dae4f,
        0x4b329434, 0xe6cbda4b, 0xf193be47, 0x518f7732,
        0xb01202e5, 0x20563724, 0xd8eb461b, 0x36ec37f0,
        0xb5377261, 0x679f2b7f, 0xdc3f94b2, 0xf878a563,
        0x82403096, 0x977def12, 0x4d5ea120, 0x1da1f391
    },
    {
        0x4d2161c4, 0x70c9463b, 0x72712953, 0x604d559f,
        0x7033d449, 0xa5ae97d4, 0xd4dff9ab, 0x765c5e51,
        0x5218dd32, 0xca3f3fe2, 0x2fed0770, 0x579fa577,
        0x9f4d5f3d, 0xfa7d39ec, 0xb44050ac, 0xef845d5d,
        0xa9b09115, 0xf6ff57da, 0x88450d0e, 0x389f43b0,
        0x53099c6f, 0x48045f61, 0x4e9a9b46, 0x4297fa02,
        0x83211867, 0xaecfe13b, 0xb2636083, 0xc1aae704,
        0x5563a035, 0xdf25e344, 0x789eac0c, 0x12456545,
        0xe5ba1a4b, 0x32f1b4c0, 0x43a08323, 0xcbe6601a,
        0x02c13749, 0x8542e39a, 0x94e01be5, 0xae529e3b,
        0x45dae928, 0x40837a0c, 0x4e962ac3, 0xaecfe13b,
        0x2f620172, 0xac6d447d, 0x2ac4e140, 0x34296e1b,
        0x8403b968, 0x325dffb2, 0x759e130e, 0x6129f52c,
        0xf3b29916, 0x4a07f8c0, 0x52fc72db, 0xf2019e64,
        0x0a7f3511, 0xce49774f, 0x2da0709c, 0x84ee19fe,
        0x63f4561b, 0x58407974, 0x01826f96, 0x060663a2,
        0x6e6d1fc2, 0x2b67a99b, 0x5ac47661, 0xb2024f04,
        0xbe250ab3, 0x39323ac6, 0x98aab8f4, 0x602561e7,
        0x7382022f, 0x49773464, 0x7d9e702f, 0xf3be26fd,
        0x52c2f54c, 0x9b675866, 0x825a1e1b, 0x071e66bf,
        0x154cff07, 0x2a9c9405, 0x2b724cc6, 0x974e4e86,
        0x77a1471e, 0xec83438b, 0xd559cb37, 0x7f1166b8,
        0xd1c44660, 0xa055525a, 0xf6356d07, 0x5abb7134,
        0x984c3b88, 0xdab51410, 0xa68d870e, 0xa3372f7f,
        0x603516cb, 0x9710983d, 0x850e7b42, 0xf9a4173d,
        0xa51a2636, 0x6786b4ef, 0x1db015ab, 0x9fcac6a2,
        0x47b0f37d, 0xf76ec641, 0x0a482109, 0x2a02b4f0,
        0x84265a00, 0x5cb7b48f, 0x7404b012, 0xbbe33cdd,
        0x77141504, 0x9ec73ff5, 0x820499fe, 0x9f845d5d,
        0xc0ac6c3e, 0x99edf594, 0x4811140a, 0x75ae3e5e,
        0xb606bd30, 0x45164bf5, 0xbd494541, 0x12d47d6a,
        0xe11e0abc, 0x60880a72, 0x670f5d5b, 0x406000e0,
        0x3b3f216f, 0xd3862f66, 0x5352467e, 0x5b3f7940,
        0x5da02021, 0xd38e7429, 0x58574dfa, 0xeab19b5f,
        0x2165c342, 0xf27f6209, 0x979144c6, 0x79dcff83,
        0x8a839743, 0x4b5af5a5, 0x93b4f90e, 0xb8da232a,
        0xf65b7980, 0x6fae1266, 0x633e7fdb, 0x83c77b1a,
        0x1e31f79c, 0x50ffda38, 0x98b8140f, 0xf65c9be2,
        0xa84af419, 0xb65b74e3, 0x1c07b191, 0x453b2f00,
        0xe3816041, 0x0ce26f1b, 0x4f6ebc0e, 0x6be470a4,
        0xafe42443, 0xa30f2c01, 0xeb12546f, 0x22254f3b,
        0x545723f1, 0x0f402364, 0xb029d65b, 0xab5840f4,
        0x93cc99e5, 0x55631584, 0x19224dcf, 0x1cd8c613,
        0x22831b4e, 0x3839ff90, 0x5c723601, 0x808cce65,
        0x2cff3f8f, 0xb9a848ad, 0xfeaf4bf9, 0x9e14a26b,
        0x83e545d6, 0xb3302664, 0x73d5d8cd, 0xd467b64e,
        0x2d17042b, 0x47164b4f, 0xd1869ee9, 0x4d10400f,
        0x6c9f3c65, 0x081cbd27, 0xa3e0a600, 0x87178f2e,
        0x437a2e66, 0xb0be4f81, 0xac378ce9, 0x00c74c72,
        0xd0eb61ce, 0x2be45e24, 0xb3476646, 0x736ac383,
        0xcd300877, 0x286ff7e8, 0x3d6b60ce, 0xdbf725f9,
        0x9dd52542, 0x74f9c9ee, 0xbac06424, 0x05e15162,
        0xdf7f66a8, 0x2772a702, 0x4086601b, 0x579f6153,
        0xb4bcbf68, 0x38777602, 0x52424b99, 0x10473d0b,
        0x2799249d, 0x460d6211, 0xa07f649f, 0x5630d942,
        0xe077d769, 0x65b1288b, 0x23ea37b2, 0x746f3e3a,
        0x36cb0244, 0xbcf88024, 0x879c45e5, 0x99da3fb3,
        0x3a411364, 0xb3150298, 0xb432d0f5, 0xae2080e4,
        0x53d06d40, 0x46405f1f, 0xd9664559, 0x9437ec6b,
        0x3bf7b17f, 0x6787b7bf, 0x8b16f241, 0x7022087c,
        0x3b147a16, 0x280244c8, 0x9ff3c9fb, 0xaaf43670,
        0x65ddfd82, 0xd69dae4f, 0x4b329434, 0xe6cbda4b,
        0xf193be47, 0x518f7732, 0xb01202e5, 0x20563724,
        0xd8eb461b, 0x36ec37f0, 0xb5377261, 0x679f2b7f
    },
    {
        0xdc3f94b2, 0xf878a563, 0x82403096, 0x977def12,
        0x4d5ea120, 0x1da1f391, 0xcd3e662d, 0x07f6be80,
        0x6691fa48, 0x58b84d2f, 0x1cd4f008, 0x855f67c2,
        0x83cad978, 0x7f42bad7, 0x2cd22718, 0x981358d6,
        0xbeab21f4, 0x416d2679, 0x67c2b088, 0x8b209f6b,
        0x46ecd752, 0x61b23314, 0x2082a42f, 0x8ca7b281,
        0xe76d4226, 0xd574f0aa, 0x44a24c09, 0x0bec37a5,
        0xc46918c5, 0x91f9fa3a, 0xfd853f3a, 0x929f4b09,
        0x5a5bf429, 0xb804f129, 0xd80056f4, 0x964dd168,
        0xb0f19230, 0x104fc638, 0x0ec92722, 0x0da11336,
        0xa8a6fe3b, 0x6100e549, 0xd499e001, 0xf205ab24,
        0xb4e83bf9, 0x4a046801, 0x4384178a, 0xd742a780,
        0xb4c711e1, 0xb0ab3143, 0x28a4c1db, 0x534e534e,
        0x5fb8f40a, 0x57724904, 0x67d6ae95, 0x31256973,
        0xea954082, 0x3410a849, 0x063ac733, 0x2e00e609,
        0xcebe851e, 0x2b472dc4, 0x1efc3039, 0xcc315767,
        0x10237041, 0x51834b96, 0xacda3456, 0xca6920f0,
        0xb0292913, 0xf4491345, 0x5ae285b4, 0x71d6f4e5,
        0x2cbe2734, 0x88b88704, 0x9f5d9d65, 0x7947ab86,
        0x4918e6ca, 0xac30e45d, 0x345a691a, 0x600d757d,
        0x2a3e92e2, 0x32238e3f, 0x3099e06b, 0x4ae79b0d,
        0xea4c1a82, 0xa613feb8, 0x34d58175, 0xa053f389,
        0xd7be290b, 0x4cad3fde, 0xa73906cd, 0xd59fe62e,
        0xfb964385, 0x6ab5b202, 0x13b605d1, 0x51564f17,
        0x536b6f59, 0xb10d4d82, 0x12f4e98a, 0x6a112e22,
        0x832a59b2, 0x1a01fb75, 0x4eaeb33a, 0x353d7d5a,
        0x02916b63, 0xd1425d97, 0x05705b77, 0x6174c02c,
        0x9d772458, 0xafe44e43, 0xa30f2c01, 0xeb12546f,
        0x22254f3b, 0x545723f1, 0x0f402364, 0xb029d65b,
        0xab5840f4, 0x93cc99e5, 0x55631584, 0x19224dcf,
        0x1cd8c613, 0x22831b4e, 0x3839ff90, 0x5c723601,
        0x808cce65, 0x2cff3f8f, 0xb9a848ad, 0xfeaf4bf9,
        0x9e14a26b, 0x83e545d6, 0xb3302664, 0x73d5d8cd,
        0xd467b64e, 0x2d17042b, 0x47164b4f, 0xd1869ee9,
        0x4d10400f, 0x6c9f3c65, 0x081cbd27, 0xa3e0a600,
        0x87178f2e, 0x437a2e66, 0xb0be4f81, 0xac378ce9,
        0x00c74c72, 0xd0eb61ce, 0x2be45e24, 0xb3476646,
        0x736ac383, 0xcd300877, 0x286ff7e8, 0x3d6b60ce,
        0xdbf725f9, 0x9dd52542, 0x74f9c9ee, 0xbac06424,
        0x05e15162, 0xdf7f66a8, 0x2772a702, 0x4086601b,
        0x579f6153, 0xb4bcbf68, 0x38777602, 0x52424b99,
        0x10473d0b, 0x2799249d, 0x460d6211, 0xa07f649f,
        0x5630d942, 0xe077d769, 0x65b1288b, 0x23ea37b2,
        0x746f3e3a, 0x36cb0244, 0xbcf88024, 0x879c45e5,
        0x99da3fb3, 0x3a411364, 0xb3150298, 0xb432d0f5,
        0xae2080e4, 0x53d06d40, 0x46405f1f, 0xd9664559,
        0x9437ec6b, 0x3bf7b17f, 0x6787b7bf, 0x8b16f241,
        0x7022087c, 0x3b147a16, 0x280244c8, 0x9ff3c9fb,
        0xaaf43670, 0x65ddfd82, 0xd69dae4f, 0x4b329434,
        0xe6cbda4b, 0xf193be47, 0x518f7732, 0xb01202e5,
        0x20563724, 0xd8eb461b, 0x36ec37f0, 0xb5377261,
        0x679f2b7f, 0xdc3f94b2, 0xf878a563, 0x82403096,
        0x977def12, 0x4d5ea120, 0x1da1f391, 0xcd3e662d,
        0x07f6be80, 0x6691fa48, 0x58b84d2f, 0x1cd4f008,
        0x855f67c2, 0x83cad978, 0x7f42bad7, 0x2cd22718,
        0x981358d6, 0xbeab21f4, 0x416d2679, 0x67c2b088,
        0x8b209f6b, 0x46ecd752, 0x61b23314, 0x2082a42f,
        0x8ca7b281, 0xe76d4226, 0xd574f0aa, 0x44a24c09,
        0x0bec37a5, 0xc46918c5, 0x91f9fa3a, 0xfd853f3a,
        0x929f4b09, 0x5a5bf429, 0xb804f129, 0xd80056f4,
        0x964dd168, 0xb0f19230, 0x104fc638, 0x0ec92722,
        0x0da11336, 0xa8a6fe3b, 0x6100e549, 0xd499e001,
        0xf205ab24, 0xb4e83bf9, 0x4a046801, 0x4384178a,
        0xd742a780, 0xb4c711e1, 0xb0ab3143, 0x28a4c1db
    },
    {
        0x1f83d9ab, 0x5be0cd19, 0x137e2179, 0x40df8eef,
        0x8a9ec9b5, 0x68b4a2bf, 0x2b060f64, 0x3f1a2386,
        0x1e36ff2f, 0x066d1f97, 0xb5e76a66, 0x60c2394d,
        0xdf5a973a, 0xa436d418, 0x738d01d4, 0x3a48e7e1,
        0xa0a2a4b8, 0x0f4b23df, 0x9c3f256a, 0x4a5d897e,
        0x509a967f, 0x17c2a71d, 0x70bb50b6, 0x20a80e12,
        0xd29705ff, 0x1634b82d, 0x5c421715, 0x0b8a1c97,
        0x717904b7, 0x3c3c1264, 0x28f7422f, 0xb53c3062,
        0x6e902b3a, 0x98f6d628, 0x6cb4f285, 0x43a85b96,
        0x56a61f5e, 0x1e49db52, 0x3344bd69, 0xb09e6677,
        0x9b3297a7, 0xa2f10b82, 0x32132514, 0x82b4e548,
        0xb029b9e2, 0xd3c40134, 0x4b78693c, 0x8ae6d16d,
        0x3a8fa951, 0x633e7fdb, 0x83c77b1a, 0x1e31f79c,
        0x50ffda38, 0x98b8140f, 0xf65c9be2, 0xa84af419,
        0xb65b74e3, 0x1c07b191, 0x453b2f00, 0xe3816041,
        0x0ce26f1b, 0x4f6ebc0e, 0x6be470a4, 0xafe42443,
        0xa30f2c01, 0xeb12546f, 0x22254f3b, 0x545723f1,
        0x0f402364, 0xb029d65b, 0xab5840f4, 0x93cc99e5,
        0x55631584, 0x19224dcf, 0x1cd8c613, 0x22831b4e,
        0x3839ff90, 0x5c723601, 0x808cce65, 0x2cff3f8f,
        0xb9a848ad, 0xfeaf4bf9, 0x9e14a26b, 0x83e545d6,
        0xb3302664, 0x73d5d8cd, 0xd467b64e, 0x2d17042b,
        0x47164b4f, 0xd1869ee9, 0x4d10400f, 0x6c9f3c65,
        0x081cbd27, 0xa3e0a600, 0x87178f2e, 0x437a2e66,
        0xb0be4f81, 0xac378ce9, 0x00c74c72, 0xd0eb61ce,
        0x2be45e24, 0xb3476646, 0x736ac383, 0xcd300877,
        0x286ff7e8, 0x3d6b60ce, 0xdbf725f9, 0x9dd52542,
        0x74f9c9ee, 0xbac06424, 0x05e15162, 0xdf7f66a8,
        0x2772a702, 0x4086601b, 0x579f6153, 0xb4bcbf68,
        0x38777602, 0x52424b99, 0x10473d0b, 0x2799249d,
        0x460d6211, 0xa07f649f, 0x5630d942, 0xe077d769,
        0x65b1288b, 0x23ea37b2, 0x746f3e3a, 0x36cb0244,
        0xbcf88024, 0x879c45e5, 0x99da3fb3, 0x3a411364,
        0xb3150298, 0xb432d0f5, 0xae2080e4, 0x53d06d40,
        0x46405f1f, 0xd9664559, 0x9437ec6b, 0x3bf7b17f,
        0x6787b7bf, 0x8b16f241, 0x7022087c, 0x3b147a16,
        0x280244c8, 0x9ff3c9fb, 0xaaf43670, 0x65ddfd82,
        0xd69dae4f, 0x4b329434, 0xe6cbda4b, 0xf193be47,
        0x518f7732, 0xb01202e5, 0x20563724, 0xd8eb461b,
        0x36ec37f0, 0xb5377261, 0x679f2b7f, 0xdc3f94b2,
        0xf878a563, 0x82403096, 0x977def12, 0x4d5ea120,
        0x1da1f391, 0xcd3e662d, 0x07f6be80, 0x6691fa48,
        0x58b84d2f, 0x1cd4f008, 0x855f67c2, 0x83cad978,
        0x7f42bad7, 0x2cd22718, 0x981358d6, 0xbeab21f4,
        0x416d2679, 0x67c2b088, 0x8b209f6b, 0x46ecd752,
        0x61b23314, 0x2082a42f, 0x8ca7b281, 0xe76d4226,
        0xd574f0aa, 0x44a24c09, 0x0bec37a5, 0xc46918c5,
        0x91f9fa3a, 0xfd853f3a, 0x929f4b09, 0x5a5bf429,
        0xb804f129, 0xd80056f4, 0x964dd168, 0xb0f19230,
        0x104fc638, 0x0ec92722, 0x0da11336, 0xa8a6fe3b,
        0x6100e549, 0xd499e001, 0xf205ab24, 0xb4e83bf9,
        0x4a046801, 0x4384178a, 0xd742a780, 0xb4c711e1,
        0xb0ab3143, 0x28a4c1db, 0x534e534e, 0x5fb8f40a,
        0x57724904, 0x67d6ae95, 0x31256973, 0xea954082,
        0x3410a849, 0x063ac733, 0x2e00e609, 0xcebe851e,
        0x2b472dc4, 0x1efc3039, 0xcc315767, 0x10237041,
        0x51834b96, 0xacda3456, 0xca6920f0, 0xb0292913,
        0xf4491345, 0x5ae285b4, 0x71d6f4e5, 0x2cbe2734,
        0x88b88704, 0x9f5d9d65, 0x7947ab86, 0x4918e6ca,
        0xac30e45d, 0x345a691a, 0x600d757d, 0x2a3e92e2,
        0x32238e3f, 0x3099e06b, 0x4ae79b0d, 0xea4c1a82,
        0xa613feb8, 0x34d58175, 0xa053f389, 0xd7be290b,
        0x4cad3fde, 0xa73906cd, 0xd59fe62e, 0xfb964385,
        0x6ab5b202, 0x13b605d1, 0x51564f17, 0x536b6f59
    }
};

struct BlowfishContext {
    uint32_t P[18];
    uint32_t S[4][256];
};

inline uint32_t bfF(const BlowfishContext& ctx, uint32_t x) {
    uint16_t d = x & 0xFF;
    x >>= 8;
    uint16_t c = x & 0xFF;
    x >>= 8;
    uint16_t b = x & 0xFF;
    x >>= 8;
    uint16_t a = x & 0xFF;
    uint32_t y = ctx.S[0][a] + ctx.S[1][b];
    y ^= ctx.S[2][c];
    y += ctx.S[3][d];
    return y;
}

void bfEncrypt(const BlowfishContext& ctx, uint32_t& xl, uint32_t& xr) {
    for (int i = 0; i < 16; i += 2) {
        xl ^= ctx.P[i];
        xr ^= bfF(ctx, xl);
        xr ^= ctx.P[i + 1];
        xl ^= bfF(ctx, xr);
    }
    xl ^= ctx.P[16];
    xr ^= ctx.P[17];
    std::swap(xl, xr);
}

void bfInit(BlowfishContext& ctx) {
    std::memcpy(ctx.P, BF_INIT_P, sizeof(ctx.P));
    std::memcpy(ctx.S, BF_INIT_S, sizeof(ctx.S));
}

void bfExpand0State(BlowfishContext& ctx, const uint8_t* key, size_t keyLen) {
    size_t keyIdx = 0;
    for (int i = 0; i < 18; ++i) {
        uint32_t data = 0;
        for (int k = 0; k < 4; ++k) {
            data = (data << 8) | (keyLen > 0 ? key[keyIdx] : 0);
            keyIdx = (keyIdx + 1) % (keyLen > 0 ? keyLen : 1);
        }
        ctx.P[i] ^= data;
    }

    uint32_t l = 0, r = 0;
    for (int i = 0; i < 18; i += 2) {
        bfEncrypt(ctx, l, r);
        ctx.P[i] = l;
        ctx.P[i + 1] = r;
    }

    for (int s = 0; s < 4; ++s) {
        for (int i = 0; i < 256; i += 2) {
            bfEncrypt(ctx, l, r);
            ctx.S[s][i] = l;
            ctx.S[s][i + 1] = r;
        }
    }
}

void bfExpandKeyWithSalt(BlowfishContext& ctx, const uint8_t* key, size_t keyLen, const uint8_t* salt, size_t saltLen) {
    size_t keyIdx = 0;
    for (int i = 0; i < 18; ++i) {
        uint32_t data = 0;
        for (int k = 0; k < 4; ++k) {
            data = (data << 8) | (keyLen > 0 ? key[keyIdx] : 0);
            keyIdx = (keyIdx + 1) % (keyLen > 0 ? keyLen : 1);
        }
        ctx.P[i] ^= data;
    }

    uint32_t l = 0, r = 0;
    size_t saltIdx = 0;

    auto getSalt32 = [&]() {
        uint32_t s = 0;
        for (int k = 0; k < 4; ++k) {
            s = (s << 8) | (saltLen > 0 ? salt[saltIdx] : 0);
            saltIdx = (saltIdx + 1) % (saltLen > 0 ? saltLen : 1);
        }
        return s;
    };

    for (int i = 0; i < 18; i += 2) {
        l ^= getSalt32();
        r ^= getSalt32();
        bfEncrypt(ctx, l, r);
        ctx.P[i] = l;
        ctx.P[i + 1] = r;
    }

    for (int s = 0; s < 4; ++s) {
        for (int i = 0; i < 256; i += 2) {
            l ^= getSalt32();
            r ^= getSalt32();
            bfEncrypt(ctx, l, r);
            ctx.S[s][i] = l;
            ctx.S[s][i + 1] = r;
        }
    }
}

// Bcrypt Base64 alphabet: ./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789
static const char BCRYPT_B64[] = "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

std::string bcryptBase64Encode(const uint8_t* data, size_t len) {
    std::string res;
    size_t i = 0;
    while (i < len) {
        uint32_t c1 = data[i++];
        uint32_t c2 = (i < len) ? data[i++] : 0;
        res.push_back(BCRYPT_B64[c1 >> 2]);
        res.push_back(BCRYPT_B64[((c1 & 3) << 4) | ((c2 >> 4) & 0x0F)]);
        if (i > len) break;
        uint32_t c3 = (i < len) ? data[i++] : 0;
        res.push_back(BCRYPT_B64[((c2 & 0x0F) << 2) | ((c3 >> 6) & 3)]);
        if (i > len) break;
        res.push_back(BCRYPT_B64[c3 & 0x3F]);
    }
    return res;
}

std::vector<uint8_t> bcryptBase64Decode(const std::string& str, size_t maxLen) {
    std::vector<int> index(256, -1);
    for (int i = 0; i < 64; ++i) {
        index[static_cast<uint8_t>(BCRYPT_B64[i])] = i;
    }

    std::vector<uint8_t> out;
    size_t i = 0;
    while (i < str.size() && out.size() < maxLen) {
        int c1 = index[static_cast<uint8_t>(str[i++])];
        int c2 = (i < str.size()) ? index[static_cast<uint8_t>(str[i++])] : 0;
        if (c1 < 0 || c2 < 0) break;
        out.push_back(static_cast<uint8_t>((c1 << 2) | ((c2 >> 4) & 3)));
        if (out.size() >= maxLen || i >= str.size()) break;

        int c3 = index[static_cast<uint8_t>(str[i++])];
        if (c3 < 0) break;
        out.push_back(static_cast<uint8_t>(((c2 & 0x0F) << 4) | ((c3 >> 2) & 0x0F)));
        if (out.size() >= maxLen || i >= str.size()) break;

        int c4 = index[static_cast<uint8_t>(str[i++])];
        if (c4 < 0) break;
        out.push_back(static_cast<uint8_t>(((c3 & 3) << 6) | (c4 & 0x3F)));
    }
    return out;
}

} // anonymous namespace

// ============================================================================
// Low-level Bcrypt Implementation
// ============================================================================

std::string Password::bcryptHash(const std::string& password, const std::vector<uint8_t>& salt16, uint32_t cost) {
    if (cost < 4 || cost > 31) {
        throw std::invalid_argument("Bcrypt cost must be between 4 and 31");
    }
    if (salt16.size() < 16) {
        throw std::invalid_argument("Bcrypt salt must be 16 bytes");
    }

    // Bcrypt truncates passwords at 72 bytes and includes a null terminator
    std::vector<uint8_t> passBytes;
    size_t passLen = std::min<size_t>(password.size(), 72);
    passBytes.insert(passBytes.end(), password.begin(), password.begin() + passLen);
    passBytes.push_back(0); // null terminator

    BlowfishContext ctx;
    bfInit(ctx);
    bfExpandKeyWithSalt(ctx, passBytes.data(), passBytes.size(), salt16.data(), 16);

    uint32_t rounds = 1U << cost;
    for (uint32_t r = 0; r < rounds; ++r) {
        bfExpand0State(ctx, passBytes.data(), passBytes.size());
        bfExpand0State(ctx, salt16.data(), 16);
    }

    // "OrpheanBeholderScryDoubt" constant (24 bytes)
    static const char CIPHERTEXT[] = "OrpheanBeholderScryDoubt";
    uint32_t cWords[6];
    for (int i = 0; i < 6; ++i) {
        cWords[i] = (static_cast<uint32_t>(static_cast<uint8_t>(CIPHERTEXT[i * 4 + 0])) << 24) |
                    (static_cast<uint32_t>(static_cast<uint8_t>(CIPHERTEXT[i * 4 + 1])) << 16) |
                    (static_cast<uint32_t>(static_cast<uint8_t>(CIPHERTEXT[i * 4 + 2])) << 8) |
                    (static_cast<uint32_t>(static_cast<uint8_t>(CIPHERTEXT[i * 4 + 3])));
    }

    // Encrypt 64 times
    for (int i = 0; i < 64; ++i) {
        for (int j = 0; j < 6; j += 2) {
            bfEncrypt(ctx, cWords[j], cWords[j + 1]);
        }
    }

    // Convert 24 bytes (or 23 bytes standard) to encoded string
    uint8_t cBytes[24];
    for (int i = 0; i < 6; ++i) {
        cBytes[i * 4 + 0] = static_cast<uint8_t>((cWords[i] >> 24) & 0xFF);
        cBytes[i * 4 + 1] = static_cast<uint8_t>((cWords[i] >> 16) & 0xFF);
        cBytes[i * 4 + 2] = static_cast<uint8_t>((cWords[i] >> 8) & 0xFF);
        cBytes[i * 4 + 3] = static_cast<uint8_t>(cWords[i] & 0xFF);
    }

    std::string encodedSalt = bcryptBase64Encode(salt16.data(), 16).substr(0, 22);
    std::string encodedHash = bcryptBase64Encode(cBytes, 23).substr(0, 31);

    std::ostringstream ss;
    ss << "$2a$" << std::setw(2) << std::setfill('0') << cost << "$" << encodedSalt << encodedHash;
    return ss.str();
}

// ============================================================================
// Password::hash, verify, needsRehash
// ============================================================================

std::string Password::hash(const std::string& plainText, const PasswordOptions& options) {
    if (options.algorithm == PasswordAlgorithm::Bcrypt) {
        auto salt = randomBytes(16);
        return bcryptHash(plainText, salt, options.bcryptCost);
    }

    // Default: Argon2id (RFC 9106)
    auto salt = randomBytes(16);
    auto rawTag = argon2idRaw(plainText, salt, options.iterations, options.memoryKiB, options.parallelism, options.hashLength);

    std::string b64Salt = base64EncodeBytes(salt.data(), salt.size());
    while (!b64Salt.empty() && b64Salt.back() == '=') b64Salt.pop_back();

    std::string b64Hash = base64EncodeBytes(rawTag.data(), rawTag.size());
    while (!b64Hash.empty() && b64Hash.back() == '=') b64Hash.pop_back();

    std::ostringstream ss;
    ss << "$argon2id$v=19$m=" << options.memoryKiB
       << ",t=" << options.iterations
       << ",p=" << options.parallelism
       << "$" << b64Salt
       << "$" << b64Hash;
    return ss.str();
}

bool Password::verify(const std::string& plainText, const std::string& hash) {
    if (hash.rfind("$argon2id$", 0) == 0) {
        // Format: $argon2id$v=19$m=65536,t=3,p=1$<b64_salt>$<b64_hash>
        size_t vPos = hash.find("$v=");
        if (vPos == std::string::npos) return false;
        size_t nextDollar = hash.find('$', vPos + 1);
        if (nextDollar == std::string::npos) return false;

        std::string paramsStr = hash.substr(nextDollar + 1);
        size_t saltDollar = paramsStr.find('$');
        if (saltDollar == std::string::npos) return false;

        std::string params = paramsStr.substr(0, saltDollar);
        std::string rest = paramsStr.substr(saltDollar + 1);
        size_t hashDollar = rest.find('$');
        if (hashDollar == std::string::npos) return false;

        std::string saltB64 = rest.substr(0, hashDollar);
        std::string expectedHashB64 = rest.substr(hashDollar + 1);

        // Parse m, t, p
        uint32_t m = 65536, t = 3, p = 1;
        std::stringstream pss(params);
        std::string token;
        while (std::getline(pss, token, ',')) {
            if (token.rfind("m=", 0) == 0) m = std::stoul(token.substr(2));
            else if (token.rfind("t=", 0) == 0) t = std::stoul(token.substr(2));
            else if (token.rfind("p=", 0) == 0) p = std::stoul(token.substr(2));
        }

        // Add padding if missing
        while (saltB64.size() % 4 != 0) saltB64.push_back('=');
        auto saltBytesStr = base64Decode(saltB64);
        std::vector<uint8_t> salt(saltBytesStr.begin(), saltBytesStr.end());

        while (expectedHashB64.size() % 4 != 0) expectedHashB64.push_back('=');
        auto expectedBytesStr = base64Decode(expectedHashB64);
        size_t tagLen = expectedBytesStr.size();
        if (tagLen == 0) return false;

        auto computedTag = argon2idRaw(plainText, salt, t, m, p, static_cast<uint32_t>(tagLen));
        return timingSafeEqualBytes(
            computedTag.data(), computedTag.size(),
            reinterpret_cast<const uint8_t*>(expectedBytesStr.data()), expectedBytesStr.size()
        );
    } else if (hash.rfind("$2a$", 0) == 0 || hash.rfind("$2b$", 0) == 0) {
        // Format: $2a$<cost>$<salt22><hash31>
        if (hash.size() < 28) return false;
        uint32_t cost = std::stoul(hash.substr(4, 2));
        std::string saltStr = hash.substr(7, 22);
        auto saltBytes = bcryptBase64Decode(saltStr, 16);
        if (saltBytes.size() < 16) return false;

        std::string computed = bcryptHash(plainText, saltBytes, cost);
        return timingSafeEqual(computed, hash);
    }

    return false;
}

bool Password::needsRehash(const std::string& hash, const PasswordOptions& targetOptions) {
    if (hash.rfind("$argon2id$", 0) == 0) {
        if (targetOptions.algorithm != PasswordAlgorithm::Argon2id) return true;
        size_t vPos = hash.find("$v=");
        if (vPos == std::string::npos) return true;
        size_t nextDollar = hash.find('$', vPos + 1);
        if (nextDollar == std::string::npos) return true;

        std::string params = hash.substr(nextDollar + 1);
        size_t saltDollar = params.find('$');
        if (saltDollar == std::string::npos) return true;
        params = params.substr(0, saltDollar);

        uint32_t m = 0, t = 0, p = 0;
        std::stringstream pss(params);
        std::string token;
        while (std::getline(pss, token, ',')) {
            if (token.rfind("m=", 0) == 0) m = std::stoul(token.substr(2));
            else if (token.rfind("t=", 0) == 0) t = std::stoul(token.substr(2));
            else if (token.rfind("p=", 0) == 0) p = std::stoul(token.substr(2));
        }

        return (m < targetOptions.memoryKiB || t < targetOptions.iterations || p < targetOptions.parallelism);
    } else if (hash.rfind("$2a$", 0) == 0 || hash.rfind("$2b$", 0) == 0) {
        if (targetOptions.algorithm != PasswordAlgorithm::Bcrypt) return true;
        uint32_t cost = std::stoul(hash.substr(4, 2));
        return cost < targetOptions.bcryptCost;
    }
    return true;
}

} // namespace sp::rt::crypto
