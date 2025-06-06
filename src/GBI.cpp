#include <algorithm>
#include <assert.h>
#include <cctype>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "convert.h"
#include "N64.h"
#include "GLideN64.h"
#include "GBI.h"
#include "RDP.h"
#include "RSP.h"
#include "uCodes/F3D.h"
#include "uCodes/F3DEX.h"
#include "uCodes/F3DEX095.h"
#include "uCodes/F3DEX2.h"
#include "uCodes/F3DEX3.h"
#include "uCodes/L3D.h"
#include "uCodes/L3DEX.h"
#include "uCodes/L3DEX2.h"
#include "uCodes/S2DEX.h"
#include "uCodes/S2DEX2.h"
#include "uCodes/F3DAM.h"
#include "uCodes/F3DDKR.h"
#include "uCodes/F3DBETA.h"
#include "uCodes/F3DPD.h"
#include "uCodes/F3DSETA.h"
#include "uCodes/F3DGOLDEN.h"
#include "uCodes/F3DEX2CBFD.h"
#include "uCodes/F3DZEX2.h"
#include "uCodes/F3DTEXA.h"
#include "uCodes/F3DEX2ACCLAIM.h"
#include "uCodes/F3DFLX2.h"
#include "uCodes/F5Indi_Naboo.h"
#include "uCodes/F5Rogue.h"
#include "uCodes/ZSort.h"
#include "uCodes/ZSortBOSS.h"
#include "CRC.h"
#include "Log.h"
#include "DebugDump.h"
#include "Graphics/Context.h"
#include "Graphics/Parameters.h"

#include <set>
#include <sstream>

u32 last_good_ucode = (u32) -1;

struct SpecialMicrocodeInfo
{
	u32 type;
	bool NoN; // ucode does not use near clipping
	bool negativeY; // Y is inverted
	bool fast3DPerspNorm; // ucode is from Fast3D family and has G_PERSPNORMALIZE. See #1303
	bool legacyVertexPipeline;
	u32 crc;
};

static const
std::vector<SpecialMicrocodeInfo> specialMicrocodes =
{
    { S2DEX2,       false,  true,   false,  true,   0x02c399dd }, // Animal Forest
    { F3DEX095,     false,  false,  true,   true,   0x0ace4c3f }, // Mario Kart 64, F3DEX 0.95
    { F3D,          true,   false,  false,  true,   0x16c3a775 }, // AeroFighters
    { F3DEX2CBFD,   true,   true,   false,  true,   0x1b4ace88 }, // Conker's Bad Fur Day
    { F3DPD,        true,   true,   false,  true,   0x1c4f7869 }, // Perfect Dark
    { F3D,          false,  false,  true,   true,   0x1f24cc84 }, // Wayne Gretzky's 3D Hockey (U)
    { F5Indi_Naboo, false,  false,  false,  true,   0x23fef05f }, // SW Ep.1 Battle for Naboo
    { Turbo3D,      false,  true,   false,  true,   0x2bdcfc8a }, // Dark Rift, Turbo3D
    { F3DSETA,      false,  true,   true,   true,   0x2edee7be }, // RSP SW Version: 2.0D, 04-01-96
    { F3DGOLDEN,    true,   true,   false,  true,   0x302bca09 }, // RSP SW Version: 2.0G, 09-30-96 GoldenEye
    { F3D,          false,  false,  false,  true,   0x4AED6B3B }, // Vivid Dolls [ALECK64]
    { F3D,          true,   true,   true,   true,   0x54c558ba }, // RSP SW Version: 2.0D, 04-01-96 Pilot Wings, Blast Corps
    { ZSortBOSS,    false,  false,  false,  true,   0x553538cc }, // World Driver Championship
    { F3D,          false,  false,  true,   true,   0x55be9bad }, // RSP SW Version: 2.0D, 04-01-96, Mischief Makers, Mortal Combat Trilogy, J.League Live
    { F3DEX,        true,   true,   true,   true,   0x637b4b58 }, // RSP SW Version: 2.0D, 04-01-96 Power League
    { F5Indi_Naboo, false,  false,  false,  true,   0x6859bf8e }, // Indiana Jones
    { F3D,          false,  false,  true,   true,   0x6932365f }, // Super Mario 64
    { ZSortBOSS,    false,  false,  false,  true,   0x6a76f8dd }, // Stunt Racer
    { F3DDKR,       false,  true,   true,   true,   0x6e6fc893 }, // Diddy Kong Racing
    { ZSortBOSS,    false,  false,  false,  true,   0x75ed44cc }, // World Driver Championship, European
    { F3D,          true,   false,  true,   true,   0x77195a68 }, // Dark Rift
    { L3D,          true,   true,   true,   true,   0x771ce0c4 }, // RSP SW Version: 2.0D, 04-01-96 Blast Corps
    { F3D,          false,  false,  false,  true,   0x7d372819 }, // Pachinko nichi 365
    { F3DDKR,       false,  true,   true,   true,   0x8d91244f }, // Diddy Kong Racing
    { F3DBETA,      false,  true,   true,   true,   0x94c4c833 }, // Star Wars Shadows of Empire
    { S2DEX_1_05,   false,  true,   false,  true,   0x9df31081 }, // RSP Gfx ucode S2DEX  1.06 Yoshitaka Yasumoto Nintendo
    { T3DUX,        false,  true,   false,  true,   0xbad437f2 }, // T3DUX vers 0.83 for Toukon Road
    { F3DJFG,       false,  true,   true,   true,   0xbde9d1fb }, // Jet Force Gemini, Mickey
    { T3DUX,        false,  true,   false,  true,   0xd0a1aa3d }, // T3DUX vers 0.85 for Toukon Road 2
    { F3DBETA,      false,  true,   true,   true,   0xd17906e2 }, // RSP SW Version: 2.0D, 04-01-96, Wave Race (U)
    { F3DZEX2MM,    true,   true,   false,  true,   0xd39a0d4f }, // Animal Forest
    { F3D,          false,  false,  true,   true,   0xd3ab59b2 }, // Cruise'n USA
    { F5Rogue,      false,  false,  false,  true,   0xda51ccdb }, // Star Wars RS
    { F3D,          false,  false,  false,  true,   0xe01e14be }, // Eikou no Saint Andrews
    { F3DEX2ACCLAIM,true,   true,   false,  true,   0xe44df568 }, // Acclaim games: Turok2 & 3, Armories and South park
    { F3D,          false,  true,   false,  true,   0xe62a706d }, // Fast3D
	{ F3DEX095,     false,  false,  true,   true,   0xf89248e1 }, // Mario Kart 64, F3DLX 0.95
	{ F3D,          false,  true,   false,  false,  0xfff0637d }, // Caribbean Nights
};

u32 G_RDPHALF_1, G_RDPHALF_2, G_RDPHALF_CONT;
u32 G_SPNOOP;
u32 G_SETOTHERMODE_H, G_SETOTHERMODE_L;
u32 G_DL, G_ENDDL, G_CULLDL, G_BRANCH_Z, G_BRANCH_W;
u32 G_LOAD_UCODE;
u32 G_MOVEMEM, G_MOVEWORD;
u32 G_MTX, G_POPMTX;
u32 G_GEOMETRYMODE, G_SETGEOMETRYMODE, G_CLEARGEOMETRYMODE;
u32 G_TEXTURE;
u32 G_DMA_IO, G_DMA_DL, G_DMA_TRI, G_DMA_MTX, G_DMA_VTX, G_DMA_TEX_OFFSET, G_DMA_OFFSETS;
u32 G_SPECIAL_1, G_SPECIAL_2, G_SPECIAL_3;
u32 G_VTX, G_MODIFYVTX, G_VTXCOLORBASE;
u32 G_TRI1, G_TRI2, G_TRIX;
u32 G_QUAD, G_LINE3D;
u32 G_RESERVED0, G_RESERVED1, G_RESERVED2, G_RESERVED3;
u32 G_SPRITE2D_BASE;
u32 G_BG_1CYC, G_BG_COPY;
u32 G_OBJ_RECTANGLE, G_OBJ_SPRITE, G_OBJ_MOVEMEM;
u32 G_SELECT_DL, G_OBJ_RENDERMODE, G_OBJ_RECTANGLE_R;
u32 G_OBJ_LOADTXTR, G_OBJ_LDTX_SPRITE, G_OBJ_LDTX_RECT, G_OBJ_LDTX_RECT_R;
u32 G_RDPHALF_0;
u32 G_PERSPNORM;
u32 G_ZOBJ, G_ZRDPCMD, G_ZWAITSIGNAL, G_ZMTXCAT, G_ZMULT_MPMTX, G_ZLIGHTING;
u32 G_TRISTRIP, G_TRIFAN, G_LIGHTTORDP, G_RELSEGMENT;


u32 G_MTX_STACKSIZE;
u32 G_MTX_MODELVIEW;
u32 G_MTX_PROJECTION;
u32 G_MTX_MUL;
u32 G_MTX_LOAD;
u32 G_MTX_NOPUSH;
u32 G_MTX_PUSH;

u32 G_TEXTURE_ENABLE;
u32 G_SHADING_SMOOTH;
u32 G_CULL_FRONT;
u32 G_CULL_BACK;
u32 G_CULL_BOTH;
u32 G_CLIPPING;

u32 G_MV_VIEWPORT;

u32 G_MWO_aLIGHT_1, G_MWO_bLIGHT_1;
u32 G_MWO_aLIGHT_2, G_MWO_bLIGHT_2;
u32 G_MWO_aLIGHT_3, G_MWO_bLIGHT_3;
u32 G_MWO_aLIGHT_4, G_MWO_bLIGHT_4;
u32 G_MWO_aLIGHT_5, G_MWO_bLIGHT_5;
u32 G_MWO_aLIGHT_6, G_MWO_bLIGHT_6;
u32 G_MWO_aLIGHT_7, G_MWO_bLIGHT_7;
u32 G_MWO_aLIGHT_8, G_MWO_bLIGHT_8;

GBIInfo GBI;

void GBI_Unknown( u32 w0, u32 w1 )
{
	DebugMsg(DEBUG_NORMAL, "UNKNOWN GBI COMMAND 0x%02X\n", _SHIFTR(w0, 24, 8));
	LOG(LOG_ERROR, "UNKNOWN GBI COMMAND 0x%02X", _SHIFTR(w0, 24, 8));
}

void GBIInfo::init()
{
	m_hwlSupported = true;
	m_pCurrent = nullptr;
	_flushCommands();
}

void GBIInfo::destroy()
{
	m_pCurrent = nullptr;
	m_list.clear();
}

bool GBIInfo::isHWLSupported() const
{
	return m_hwlSupported;
}

void GBIInfo::setHWLSupported(bool _supported)
{
	m_hwlSupported = _supported;
}

void GBIInfo::_flushCommands()
{
	std::fill(std::begin(cmd), std::end(cmd), GBI_Unknown);
}

void GBIInfo::_makeCurrent(MicrocodeInfo * _pCurrent)
{
	if (_pCurrent->type == NONE) {
		LOG(LOG_ERROR, "[GLideN64]: error - unknown ucode!!!");
		return;
	}

	if ((_pCurrent->type == ZSortBOSS) && (!REG.SP_STATUS)) {
		assert(false && "ZSortBOSS ucode needs access to SP_STATUS register'n");
	}

	if (m_pCurrent == nullptr || (m_pCurrent->type != _pCurrent->type)) {
		m_pCurrent = _pCurrent;
		_flushCommands();

		RDP_Init();

		G_TRI1 = G_TRI2 = G_TRIX = G_QUAD = G_TRISTRIP = G_TRIFAN = -1; // For correct work of gSPFlushTriangles()
		gSP.clipRatio = 1U;

		switch (m_pCurrent->type) {
			case F3D:
			case Turbo3D:
				F3D_Init();
				m_hwlSupported = true;
			break;
			case F3DEX095:
				F3DEX095_Init();
				m_hwlSupported = true;
				gSP.clipRatio = m_pCurrent->Rej ? 2U : 1U;
				break;
			case F3DEX:
				F3DEX_Init();
				m_hwlSupported = true;
				gSP.clipRatio = m_pCurrent->Rej ? 2U : 1U;
			break;
			case F3DEX2:
				F3DEX2_Init();
				m_hwlSupported = true;
				gSP.clipRatio = 2U;
			break;
			case L3D:
				L3D_Init();
				m_hwlSupported = false;
			break;
			case L3DEX:
				L3DEX_Init();
				m_hwlSupported = false;
				gSP.clipRatio = m_pCurrent->Rej ? 2U : 1U;
			break;
			case L3DEX2:
				L3DEX2_Init();
				m_hwlSupported = false;
				gSP.clipRatio = 2U;
			break;
			case S2DEX_1_03:
				S2DEX_1_03_Init();
				m_hwlSupported = false;
				break;
			case S2DEX_1_05:
				S2DEX_1_05_Init();
				m_hwlSupported = false;
				break;
			case S2DEX_1_07:
				S2DEX_1_07_Init();
				m_hwlSupported = false;
				break;
			case S2DEX2:
				S2DEX2_Init();
				m_hwlSupported = false;
				break;
			case F3DDKR:
				F3DDKR_Init();
				m_hwlSupported = false;
			break;
			case F3DJFG:
				F3DJFG_Init();
				m_hwlSupported = false;
			break;
			case F3DBETA:
				F3DBETA_Init();
				m_hwlSupported = true;
			break;
			case F3DPD:
				F3DPD_Init();
				m_hwlSupported = true;
			break;
			case F3DAM:
				F3DAM_Init();
				m_hwlSupported = true;
			break;
			case ZSortp:
				ZSort_Init();
				m_hwlSupported = true;
			break;
			case F3DEX2CBFD:
				F3DEX2CBFD_Init();
				m_hwlSupported = false;
			break;
			case F3DSETA:
				F3DSETA_Init();
				m_hwlSupported = true;
			break;
			case F3DGOLDEN:
				F3DGOLDEN_Init();
				m_hwlSupported = true;
			break;
			case F3DZEX2OOT:
				F3DZEX2_Init();
				m_hwlSupported = true;
				gSP.clipRatio = 2U;
			break;
			case F3DZEX2MM:
				F3DZEX2_Init();
				m_hwlSupported = false;
				gSP.clipRatio = 2U;
			break;
			case F3DEX3:
				F3DEX3_Init();
				m_hwlSupported = false;
				break;
			case F3DTEXA:
				F3DTEXA_Init();
				m_hwlSupported = true;
			break;
			case T3DUX:
				F3D_Init();
				m_hwlSupported = false;
			break;
			case F3DEX2ACCLAIM:
				F3DEX2ACCLAIM_Init();
				m_hwlSupported = false;
				gSP.clipRatio = 2U;
			break;
			case F5Rogue:
				F5Rogue_Init();
				m_hwlSupported = false;
			break;
			case F3DFLX2:
				F3DFLX2_Init();
				m_hwlSupported = true;
				gSP.clipRatio = 2U;
			break;
			case ZSortBOSS:
				ZSortBOSS_Init();
				m_hwlSupported = true;
			break;
			case F5Indi_Naboo:
				F5Indi_Naboo_Init();
				m_hwlSupported = false;
				break;
		}
		if (m_pCurrent->NoN)
			gfxContext.setClampMode(graphics::ClampMode::NoNearPlaneClipping);
		else
			gfxContext.setClampMode(graphics::ClampMode::ClippingEnabled);
		if (m_pCurrent->fast3DPersp) {
			GBI_SetGBI(G_PERSPNORM, F3DBETA_PERSPNORM, F3DBETA_Perpnorm);
			GBI_SetGBI(G_RDPHALF_1, F3DBETA_RDPHALF_1, F3D_RDPHalf_1);
			GBI_SetGBI(G_RDPHALF_2, F3DBETA_RDPHALF_2, F3D_RDPHalf_2);
		}
	} else if (m_pCurrent->NoN != _pCurrent->NoN) {
		if (_pCurrent->NoN)
			gfxContext.setClampMode(graphics::ClampMode::NoNearPlaneClipping);
		else
			gfxContext.setClampMode(graphics::ClampMode::ClippingEnabled);
	}
	m_pCurrent = _pCurrent;
}

bool GBIInfo::_makeExistingMicrocodeCurrent(u32 uc_start, u32 uc_dstart, u32 uc_dsize)
{
	auto iter = std::find_if(m_list.begin(), m_list.end(), [=](const MicrocodeInfo& info) {
		return info.address == uc_start && info.dataAddress == uc_dstart && info.dataSize == uc_dsize;
	});

	if (iter == m_list.end())
		return false;

	_makeCurrent(&*iter);
	return true;
}

// based on musl libc
static inline int ascii_isupper(int c)
{
	return (unsigned)c - 'A' < 26;
}

static inline int ascii_tolower(int c)
{
	if (isupper(c)) return c | 32;
	return c;
}

void GBIInfo::loadMicrocode(u32 uc_start, u32 uc_dstart, u16 uc_dsize)
{
	if (_makeExistingMicrocodeCurrent(uc_start, uc_dstart, uc_dsize))
		return;

	m_list.emplace_front();
	MicrocodeInfo & current = m_list.front();
	current.address = uc_start;
	current.dataAddress = uc_dstart;
	current.dataSize = uc_dsize;
	current.type = NONE;

	// See if we can identify it by CRC
	const u32 uc_crc = CRC_Calculate_Strict( 0xFFFFFFFF, &RDRAM[uc_start & 0x1FFFFFFF], 4096 );
	SpecialMicrocodeInfo infoToSearch;
	infoToSearch.crc = uc_crc;
	auto it = std::lower_bound(specialMicrocodes.begin(), specialMicrocodes.end(), infoToSearch,
			[](const SpecialMicrocodeInfo & i, const SpecialMicrocodeInfo & j){ return i.crc < j.crc; });
	if (it != specialMicrocodes.end() && it->crc == uc_crc)	{
		const SpecialMicrocodeInfo & info = *it;
		current.type = info.type;
		current.NoN = info.NoN;
		current.negativeY = info.negativeY;
		current.fast3DPersp = info.fast3DPerspNorm;
		current.f3dex3.legacyVertexPipeline = info.legacyVertexPipeline;
		LOG(LOG_VERBOSE, "Load microcode type: %d crc: 0x%08x romname: %s", current.type, uc_crc, RSP.romname);
		_makeCurrent(&current);
		return;
	}

	// See if we can identify it by text
	char uc_data[2048];
	UnswapCopyWrap(RDRAM, uc_dstart & 0x1FFFFFFF, (u8*)uc_data, 0, 0x7FF, 2048);
	char uc_str[256];
	strcpy(uc_str, "Not Found");

	// Check for F3DEX3 microcode
	{
		static const char F3DEX3_NAME[] = "f3dex3";
		const char* probe = &uc_data[0x138];
		char name_buffer[sizeof(F3DEX3_NAME) - 1];
		memcpy(name_buffer, probe, sizeof(F3DEX3_NAME) - 1);
		std::transform(name_buffer, name_buffer + sizeof(F3DEX3_NAME) - 1, name_buffer, ascii_tolower);
		if (0 == memcmp(name_buffer, F3DEX3_NAME, sizeof(F3DEX3_NAME) - 1))
		{
			current.type = F3DEX3;
			current.NoN = true;
			current.negativeY = false;
			current.fast3DPersp = false;
			current.combineMatrices = false;

			std::set<std::string> features;
			{
				// 0x180 is absolutely an overkill but it is ok for now
				const char* name_end = (const char*)memchr(probe, ' ', 0x180);
				size_t name_len = name_end - probe;
				// It will look like F3DEX3_LVP_BrZ_NOC
				std::string feature;
				std::string name = std::string(probe, name_len);
				std::transform(name.begin(), name.end(), name.begin(), ascii_tolower);
				std::stringstream name_stream(name);
				while (std::getline(name_stream, feature, '_'))
				{
					features.emplace(std::move(feature));
				}
			}

			current.f3dex3.legacyVertexPipeline = features.find("lvp") != features.end();
			current.f3dex3.noOcclusionPlane = features.find("noc") != features.end();
			current.f3dex3.branchOnZ = features.find("brz") != features.end();

			LOG(LOG_VERBOSE, "Load microcode (%s) type: %d crc: 0x%08x romname: %s\n", uc_str, current.type, uc_crc, RSP.romname);
			_makeCurrent(&current);
			return;
		}
	}

	for (u32 i = 0; i < 2046; ++i) {
		if ((uc_data[i] == 'R') && (uc_data[i+1] == 'S') && (uc_data[i+2] == 'P')) {
			u32 j = 0;
			while (uc_data[i+j] > 0x0A) {
				uc_str[j] = uc_data[i+j];
				j++;
			}

			uc_str[j] = 0x00;

			int type = NONE;

			if (strncmp(&uc_str[4], "SW", 2) == 0) {
				type = F3D;
			} else if (strncmp(&uc_str[4], "Gfx", 3) == 0) {
				current.NoN = (strstr( uc_str + 4, ".NoN") != nullptr);
				current.Rej = (strstr(uc_str + 4, ".Rej") != nullptr);
				if (current.Rej)
					// For the Z direction, a reject box can be done with the far plane, but not with the near plane.
					current.NoN = true;

				if (strncmp( &uc_str[14], "F3D", 3 ) == 0) {
					if (uc_str[28] == '1' || strncmp(&uc_str[28], "0.95", 4) == 0 || strncmp(&uc_str[28], "0.96", 4) == 0)
						type = F3DEX;
					else if (uc_str[31] == '2') {
						type = F3DEX2;
						if (uc_str[35] == 'H')
							current.combineMatrices = true;
					}
					if (strncmp(&uc_str[14], "F3DFLX", 6) == 0) {
						type = F3DFLX2;
						current.NoN = true;
					} else if (strncmp(&uc_str[14], "F3DZEX", 6) == 0) {
						// Zelda games
						if (uc_str[34] == '6')
							type = F3DZEX2OOT;
						else
							type = F3DZEX2MM;
						current.combineMatrices = false;
					} else if (strncmp(&uc_str[14], "F3DTEX/A", 8) == 0)
						type = F3DTEXA;
					else if (strncmp(&uc_str[14], "F3DAM", 5) == 0)
						type = F3DAM;
					else if (strncmp(&uc_str[14], "F3DLX.Rej", 9) == 0) {
						current.NoN = true;
						current.cullBoth = false;
					} else if (strncmp(&uc_str[14], "F3DLP.Rej", 9) == 0) {
						current.texturePersp = false;
						current.NoN = true;
					}
				}
				else if (strncmp( &uc_str[14], "L3D", 3 ) == 0) {
					u32 t = 22;
					while (!std::isdigit(uc_str[t]) && t++ < j);
					if (uc_str[t] == '1')
						type = L3DEX;
					else if (uc_str[t] == '2')
						type = L3DEX2;
				}
				else if (strncmp( &uc_str[14], "S2D", 3 ) == 0) {
					u32 t = 20;
					while (!std::isdigit(uc_str[t]) && t++ < j);
					if (uc_str[t] == '1') {
						if (strncmp(&uc_str[21], "1.03", 4) == 0)
							type = S2DEX_1_03;
						else if (strncmp(&uc_str[21], "1.05", 4) == 0)
							type = S2DEX_1_05;
						else
							type = S2DEX_1_07;
					} else if (uc_str[t] == '2')
						type = S2DEX2;
					current.texturePersp = false;
				}
				else if (strncmp(&uc_str[14], "ZSortp", 6) == 0) {
					type = ZSortp;
				}
			}

			if (type != NONE) {
				current.type = type;
				LOG(LOG_VERBOSE, "Load microcode (%s) type: %d crc: 0x%08x romname: %s", uc_str, current.type, uc_crc, RSP.romname);
				_makeCurrent(&current);
				return;
			}

			break;
		}
	}

	assert(false && "unknown ucode!!!'n");
	_makeCurrent(&current);
}
