// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#include <math.h>

#include <base/system.h>
#include <base/math.h>

#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/shared/engine.h>

#include "skins.h"

CSkins::CSkins()
{
	m_NumSkins = 0;
}

void CSkins::SkinScan(const char *pName, int IsDir, void *pUser)
{
	CSkins *pSelf = (CSkins *)pUser;
	int l = str_length(pName);
	if(l < 4 || IsDir || pSelf->m_NumSkins == MAX_SKINS)
		return;
	if(str_comp(pName+l-4, ".png") != 0)
		return;
		
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "skins/%s", pName);
	CImageInfo Info;
	if(!pSelf->Graphics()->LoadPNG(&Info, aBuf))
	{
		dbg_msg("game", "failed to load skin from %s", pName);
		return;
	}
	
	pSelf->m_aSkins[pSelf->m_NumSkins].m_OrgTexture = pSelf->Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);
	
	int BodySize = 96; // body size
	unsigned char *d = (unsigned char *)Info.m_pData;
	int Pitch = Info.m_Width*4;

	// dig out blood color
	{
		int aColors[3] = {0};
		for(int y = 0; y < BodySize; y++)
			for(int x = 0; x < BodySize; x++)
			{
				if(d[y*Pitch+x*4+3] > 128)
				{
					aColors[0] += d[y*Pitch+x*4+0];
					aColors[1] += d[y*Pitch+x*4+1];
					aColors[2] += d[y*Pitch+x*4+2];
				}
			}
			
		pSelf->m_aSkins[pSelf->m_NumSkins].m_BloodColor = normalize(vec3(aColors[0], aColors[1], aColors[2]));
	}
	
	// create colorless version
	int Step = Info.m_Format == CImageInfo::FORMAT_RGBA ? 4 : 3;

	// make the texture gray scale
	for(int i = 0; i < Info.m_Width*Info.m_Height; i++)
	{
		int v = (d[i*Step]+d[i*Step+1]+d[i*Step+2])/3;
		d[i*Step] = v;
		d[i*Step+1] = v;
		d[i*Step+2] = v;
	}

	
	if(1)
	{
		int Freq[256] = {0};
		int OrgWeight = 0;
		int NewWeight = 192;
		
		// find most common frequence
		for(int y = 0; y < BodySize; y++)
			for(int x = 0; x < BodySize; x++)
			{
				if(d[y*Pitch+x*4+3] > 128)
					Freq[d[y*Pitch+x*4]]++;
			}
		
		for(int i = 1; i < 256; i++)
		{
			if(Freq[OrgWeight] < Freq[i])
				OrgWeight = i;
		}

		// reorder
		int InvOrgWeight = 255-OrgWeight;
		int InvNewWeight = 255-NewWeight;
		for(int y = 0; y < BodySize; y++)
			for(int x = 0; x < BodySize; x++)
			{
				int v = d[y*Pitch+x*4];
				if(v <= OrgWeight)
					v = (int)(((v/(float)OrgWeight) * NewWeight));
				else
					v = (int)(((v-OrgWeight)/(float)InvOrgWeight)*InvNewWeight + NewWeight);
				d[y*Pitch+x*4] = v;
				d[y*Pitch+x*4+1] = v;
				d[y*Pitch+x*4+2] = v;
			}
	}
	
	pSelf->m_aSkins[pSelf->m_NumSkins].m_ColorTexture = pSelf->Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);
	mem_free(Info.m_pData);

	// set skin data	
	str_copy(pSelf->m_aSkins[pSelf->m_NumSkins].m_aName, pName, min((int)sizeof(pSelf->m_aSkins[pSelf->m_NumSkins].m_aName),l-3));
	dbg_msg("game", "load skin %s", pSelf->m_aSkins[pSelf->m_NumSkins].m_aName);
	pSelf->m_NumSkins++;
}


void CSkins::Init()
{
	// load skins
	m_NumSkins = 0;
	Storage()->ListDirectory(IStorage::TYPE_ALL, "skins", SkinScan, this);
}

int CSkins::Num()
{
	return m_NumSkins;	
}

const CSkins::CSkin *CSkins::Get(int Index)
{
	return &m_aSkins[Index%m_NumSkins];
}

int CSkins::Find(const char *pName)
{
	for(int i = 0; i < m_NumSkins; i++)
	{
		if(str_comp(m_aSkins[i].m_aName, pName) == 0)
			return i;
	}
	return -1;
}

// these converter functions were nicked from some random internet pages
static float HueToRgb(float v1, float v2, float h)
{
   if(h < 0) h += 1;
   if(h > 1) h -= 1;
   if((6 * h) < 1) return v1 + ( v2 - v1 ) * 6 * h;
   if((2 * h) < 1) return v2;
   if((3 * h) < 2) return v1 + ( v2 - v1 ) * ((2.0f/3.0f) - h) * 6;
   return v1;
}

static vec3 HslToRgb(vec3 in)
{
	float v1, v2;
	vec3 Out;

	if(in.s == 0)
	{
		Out.r = in.l;
		Out.g = in.l;
		Out.b = in.l;
	}
	else
	{
		if(in.l < 0.5f) 
			v2 = in.l * (1 + in.s);
		else           
			v2 = (in.l+in.s) - (in.s*in.l);

		v1 = 2 * in.l - v2;

		Out.r = HueToRgb(v1, v2, in.h + (1.0f/3.0f));
		Out.g = HueToRgb(v1, v2, in.h);
		Out.b = HueToRgb(v1, v2, in.h - (1.0f/3.0f));
	} 

	return Out;
}

vec4 CSkins::GetColor(int v)
{
	vec3 r = HslToRgb(vec3(((v>>16)&0xff)/255.0f, ((v>>8)&0xff)/255.0f, 0.5f+(v&0xff)/255.0f*0.5f));
	return vec4(r.r, r.g, r.b, 1.0f);
}
