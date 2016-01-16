#include <hge.h>
#include <hgefont.h>
#include "stdheaderes.h"
#include <Windows.h>

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 1000

HGE *hge = 0;
hgeFont* fnt = 0;
				
class Cunit;
class Cnode;

unsigned int LenRecentRoute = -1;
int LenRoute = 0;

inline float dotdistance(float x1, float y1, float x2, float y2)
{
	float xx = (x1 - x2);
	xx = xx * xx;;
	float yy = (y1 - y2);
	yy = yy * yy;
	return sqrt(xx + yy);
}

class Cpos
{
public :
	float x;
	float y;
public :
	Cpos()
	{
		x = 0;
		y = 0;
	}
	bool operator==(Cpos a)
	{
		if (x != a.x) return false;
		if (y != a.y) return false;
		return true;
	}
	bool operator!=(Cpos a)
	{
		if (x != a.x) return true;
		if (y != a.y) return true;
		return false;
	}
};

class Cline
{
public :
	Cpos start;
	Cpos end;
};

class Cnode
{
public:
	Cunit* opposite;
	float length;
public:
	Cnode()
	{
		length = -1;
	}
};

Cpos FindNodalPoint(Cline a, Cline b)
{
	Cpos Return;
	float d1 = ((a.start.y) - (a.end.y)) / ((a.start.x) - (a.end.x));
	float d2 = ((b.start.y) - (b.end.y)) / ((b.start.x) - (b.end.x));
	Return.x = (a.start.x * a.end.y) - a.start.y - (b.start.x * b.end.y) + b.start.y;
	Return.x /= (d1 - d2);
	Return.y = (Return.x * d1) - (a.start.x * a.end.y) + a.start.y;

	if (a.start.x < Return.x && Return.x < a.end.x && b.start.x < Return.x && Return.x < b.end.x)
		return Return;
	else
	{
		Return.x = -1;
		return Return;
	}
}

class Cunit
{
public :
	vector<Cnode> nodes;
	Cpos pos;
	int code;
	bool flag;
public :
	Cunit()
	{
		flag = false;
		code = 0;
		nodes.reserve(100);
	}
	void FindRoute(vector<Cunit*> &result, vector<Cunit*> &vect, Cunit* target)
	{
		vector<Cunit*>::iterator it_vec;
		vect.push_back(this);
		if (this == target)
		{
			if (LenRoute < LenRecentRoute)
			{
				LenRecentRoute = LenRoute;
				result = vect;
			}
			vect.pop_back();
			return;
		}
		else
		{
			for (vector<Cnode>::iterator it_cn = nodes.begin(); it_cn != nodes.end(); ++it_cn)  // 모든 노드에서
			{
				it_vec = find(vect.begin(), vect.end(), it_cn->opposite);	   //루트와 겹치는게 없으면
				if (it_vec == vect.end())
				{
					LenRoute += it_cn->length;
					it_cn->opposite->FindRoute(result, vect, target);
					LenRoute -= it_cn->length;
				}
			}		// 노드가 전부 겹치면
			vect.pop_back();
			return;	  //return false
		}
	}
	bool operator==(Cunit a)
	{
		if (pos != a.pos) return false;
		if (code != a.code) return false;
		if (flag != a.flag) return false;
	}
};

vector<Cunit*> RecentRouteResult;

class Cmap
{
public :
	vector<Cunit> units;
public :
	Cmap()
	{
		units.reserve(100);
	}
	inline void FindRouteA(vector<Cunit*> &vect, int from, int to)
	{
		RecentRouteResult.clear();
		vect.clear();
		LenRecentRoute = -1;
		units[from].FindRoute(RecentRouteResult, vect,&units[to]);
	}
	void NewUnit(Cpos pos)
	{
		Cunit unitbuf;
		unitbuf.pos = pos;
		unitbuf.code = units.size();
		units.push_back(unitbuf);
	}
	void connect(int a, int b)
	{
		Cnode nodetemp;
		Cunit* ap = &units[a];
		Cunit* bp = &units[b];
		if (a == b) return;
		nodetemp.length = dotdistance(ap->pos.x, ap->pos.y, bp->pos.x, bp->pos.y);
		
		nodetemp.opposite = bp;
		ap->nodes.push_back(nodetemp);

		nodetemp.opposite = ap;
		bp->nodes.push_back(nodetemp);
	}
};

Cmap mainmap;

hgeQuad qdCross;
hgeQuad qdBackground;

int focusunit = -1;
int mouseonunit = -1;
int findfocusunit = -1;

bool saveattached = false;

vector<Cunit*> routevec;

vector<Cpos> dots;

Cpos mousepos;

inline void setquadcoor(float _x, float _y, float width, float height, hgeQuad &quad)
{
	quad.v[0].x = _x;
	quad.v[1].x = _x + width;
	quad.v[2].x = _x + width;
	quad.v[3].x = _x;
	quad.v[0].y = _y;
	quad.v[1].y = _y;
	quad.v[2].y = _y + height;
	quad.v[3].y = _y + height;
}

inline void setquadtcoor(float _x, float _y, float width, float height, hgeQuad &quad)
{
	quad.v[0].tx = _x;
	quad.v[1].tx = _x + width;
	quad.v[2].tx = _x + width;
	quad.v[3].tx = _x;
	quad.v[0].ty = _y;
	quad.v[1].ty = _y;
	quad.v[2].ty = _y + height;
	quad.v[3].ty = _y + height;
}

void SaveDots()
{
	ofstream ofs;
	ofs.open("dots.txt");

	ofs << dots.size() << endl;

	for (vector<Cpos>::iterator it = dots.begin(); it != dots.end(); ++it)
	{
		ofs << it->x << " " << it->y << endl;
	}

	ofs.close();
}

void LoadDots()
{
	ifstream ifs;
	ifs.open("dots.txt");
	Cpos posbuf;

	int num;
	ifs >> num;
	for (int i = 0; i < num; ++i)
	{
		ifs >> posbuf.x >> posbuf.y;
		dots.push_back(posbuf);
	}

	ifs.close();
}

void CheckMouseonUnit()
{
	for (vector<Cunit>::iterator it_unit = mainmap.units.begin(); it_unit != mainmap.units.end(); ++it_unit)
	{
		if (it_unit->pos.x - 15 < mousepos.x && mousepos.x < it_unit->pos.x + 15 && it_unit->pos.y - 15 < mousepos.y && mousepos.y < it_unit->pos.y + 15)
		{
			mouseonunit = it_unit->code;
		}
	}
}

void OnLbuttonDown()
{
	if (mouseonunit == -1)
	{
		if (focusunit == -1)
		{
			mainmap.NewUnit(mousepos);
			focusunit = mainmap.units.size() - 1;
		}
		else
		{
			mainmap.NewUnit(mousepos);
			mainmap.connect(focusunit, mainmap.units.size() - 1);
			focusunit = -1;
		}
	}
	else
	{
		if (focusunit == -1)
		{
			focusunit = mouseonunit;
		}
		else
		{
			mainmap.connect(focusunit, mouseonunit);
			focusunit = -1;
		}
	}
}

void OnRbuttonDown()
{
	if (mouseonunit != -1)
	{
		if (findfocusunit == -1)
		{
			findfocusunit = mouseonunit;
		}
		else
		{
			mainmap.FindRouteA(routevec, findfocusunit, mouseonunit);
			findfocusunit = -1;
		}
	}
}

void OnCbuttonDown()
{
	mainmap.units.clear();
	RecentRouteResult.clear();
	dots.clear();
}

void AttachDots()
{
	for (vector<Cpos>::iterator it = dots.begin(); it != dots.end(); ++it)
	{
	// 	mousepos = (*it);
	// 	CheckMouseonUnit();
		OnLbuttonDown();
	}
}

bool FrameFunc()
{
	float dt = hge->Timer_GetDelta();
	static bool flag_lbutton;
	static bool flag_rbutton;
	static bool flag_cbutton;
	hge->Input_GetMousePos(&mousepos.x, &mousepos.y);
	mouseonunit = -1;
	if (!saveattached)
	{
		AttachDots();
		saveattached = true;
	}
	if (hge->Input_GetKeyState(HGEK_C))
	{
		if (!flag_cbutton)
		{
			flag_cbutton = true;
			OnCbuttonDown();
		}
	}
	else flag_cbutton = false;
	CheckMouseonUnit();
	if (hge->Input_GetKeyState(HGEK_LBUTTON))
	{
		if (!flag_lbutton)
		{
			flag_lbutton = true;
			dots.push_back(mousepos);
			OnLbuttonDown();
		}
	}
	else flag_lbutton = false;

	if (hge->Input_GetKeyState(HGEK_RBUTTON))
	{
		if (!flag_rbutton)
		{
			flag_rbutton = true;
			OnRbuttonDown();
		}
	}
	else flag_rbutton = false;	

	return false;
}

bool RenderFunc()
{
	hge->Gfx_BeginScene();
	hge->Gfx_Clear(0);
	Cpos posbuf;
	Cunit* unitbuf;
	fnt->printf(0, 0, HGETEXT_LEFT, "focus : %d\nmouse : %d\nfindfocus : %d", focusunit, mouseonunit, findfocusunit);
	for (vector<Cunit>::iterator it = mainmap.units.begin(); it != mainmap.units.end(); ++it)
	{
		setquadcoor(it->pos.x-15, it->pos.y-15, 30, 30, qdCross);
		hge->Gfx_RenderQuad(&qdCross);
		fnt->printf(it->pos.x, it->pos.y, HGETEXT_LEFT, "%d", it->code);
		for (vector<Cnode>::iterator it_nod = it->nodes.begin(); it_nod != it->nodes.end(); ++it_nod)
		{
			hge->Gfx_RenderLine(it->pos.x, it->pos.y, it_nod->opposite->pos.x, it_nod->opposite->pos.y, 0xFFFF0000);
		}
	}
	hge->Input_GetMousePos(&posbuf.x, &posbuf.y);
	setquadcoor(posbuf.x, posbuf.y, 10, 10, qdCross);
	hge->Gfx_RenderQuad(&qdCross);

	if (!RecentRouteResult.empty())
	{
		posbuf = (*RecentRouteResult.begin())->pos;
		for (vector<Cunit*>::iterator it = RecentRouteResult.begin() + 1; it != RecentRouteResult.end(); ++it)
		{
			unitbuf = (*it);
			hge->Gfx_RenderLine(posbuf.x, posbuf.y, unitbuf->pos.x, unitbuf->pos.y, 0xFF0000FF);
			posbuf = unitbuf->pos;
		}
	}		 
																						
	hge->Gfx_EndScene();
	return false;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	hge = hgeCreate(HGE_VERSION);

	hge->System_SetState(HGE_LOGFILE, "hge_tut03.log");
	hge->System_SetState(HGE_FRAMEFUNC, FrameFunc);
	hge->System_SetState(HGE_RENDERFUNC, RenderFunc);
	hge->System_SetState(HGE_TITLE, "HGE Tutorial 03 - Using helper classes");
	hge->System_SetState(HGE_FPS, 100);
	hge->System_SetState(HGE_WINDOWED, true);
	hge->System_SetState(HGE_SCREENWIDTH, SCREEN_WIDTH);
	hge->System_SetState(HGE_SCREENHEIGHT, SCREEN_HEIGHT);
	hge->System_SetState(HGE_SCREENBPP, 32);

	hge->System_SetState(HGE_SHOWSPLASH, false); // Starting splash delete

	if (hge->System_Initiate()) {
		LoadDots();
		routevec.reserve(100);
		// Load Resources
		qdCross.tex = hge->Texture_Load("particles.png");
		qdBackground.tex = hge->Texture_Load("background.png");
		fnt = new hgeFont("font1.fnt");
		if (!qdCross.tex)
		{
			MessageBox(NULL, "Can't load MENU.WAV or PARTICLES.PNG", "Error", MB_OK | MB_ICONERROR | MB_APPLMODAL);
			hge->System_Shutdown();
			hge->Release();
			return 0;
		}
		qdCross.blend = BLEND_ALPHAADD | BLEND_COLORMUL | BLEND_ZWRITE;
		qdBackground.blend = BLEND_ALPHAADD | BLEND_COLORMUL | BLEND_ZWRITE;
		for (int i = 0; i<4; i++)
		{
			// Set up z-coordinate of vertices
			qdCross.v[i].z = 0.5f;
			// Set up color. The format of DWORD col is 0xAARRGGBB
			qdCross.v[i].col = 0xFFFFA000;
		}
		for (int i = 0; i<4; i++)
		{
			// Set up z-coordinate of vertices
			qdBackground.v[i].z = 0.5f;
			// Set up color. The format of DWORD col is 0xAARRGGBB
			qdBackground.v[i].col = 0xFFFFA000;
		}

		// Set up quad's texture coordinates.
		// 0,0 means top left corner and 1,1 -
		// bottom right corner of the texture.
		qdCross.v[0].tx = 96.0 / 128.0; qdCross.v[0].ty = 64.0 / 128.0;
		qdCross.v[1].tx = 128.0 / 128.0; qdCross.v[1].ty = 64.0 / 128.0;
		qdCross.v[2].tx = 128.0 / 128.0; qdCross.v[2].ty = 96.0 / 128.0;
		qdCross.v[3].tx = 96.0 / 128.0; qdCross.v[3].ty = 96.0 / 128.0;
		setquadtcoor(0, 0, 1, 1, qdBackground);

		if (0) MessageBox(0, "Resource Load Error", "Error", MB_OK); // Resource Loading Check

		hge->System_Start(); // HGE Routine Start

		SaveDots();

		delete fnt;
		hge->Texture_Free(qdCross.tex);
		hge->Texture_Free(qdBackground.tex);
		// Delete Resources

	}
	// Clean up and shut down
	hge->System_Shutdown();
	hge->Release();

	return 0;
}
