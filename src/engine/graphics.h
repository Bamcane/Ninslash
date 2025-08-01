

#ifndef ENGINE_GRAPHICS_H
#define ENGINE_GRAPHICS_H

#include "kernel.h"

enum RenderBuffers
{
	RENDERBUFFER_TILES,
	RENDERBUFFER_SPLATTER,
	RENDERBUFFER_BLOOD,
	RENDERBUFFER_ACID,
	RENDERBUFFER_LIGHT,
	RENDERBUFFER_MENU,
	NUM_RENDERBUFFERS
};

enum Shaders
{
	SHADER_PLAYER,
	SHADER_BALL,
	SHADER_ELECTRIC,
	SHADER_DEATHRAY,
	SHADER_COLORSWAP,
	SHADER_SPAWN,
	SHADER_DAMAGE,
	SHADER_SHIELD,
	SHADER_INVISIBILITY,
	SHADER_RAGE,
	SHADER_FUEL,
	SHADER_BLOOD,
	SHADER_ACID,
	SHADER_GRAYSCALE,
	SHADER_MENU,
	NUM_SHADERS
};


class CImageInfo
{
public:
	enum
	{
		FORMAT_AUTO=-1,
		FORMAT_RGB=0,
		FORMAT_RGBA=1,
		FORMAT_ALPHA=2,
	};

	/* Variable: width
		Contains the width of the image */
	int m_Width;

	/* Variable: height
		Contains the height of the image */
	int m_Height;

	/* Variable: format
		Contains the format of the image. See <Image Formats> for more information. */
	int m_Format;

	/* Variable: data
		Pointer to the image data. */
	void *m_pData;
};

/*
	Structure: CVideoMode
*/
class CVideoMode
{
public:
	int m_Width, m_Height;
	int m_Red, m_Green, m_Blue;
	bool operator<(const CVideoMode &Other) { return Other.m_Width < m_Width; }
};

class IGraphics : public IInterface
{
	MACRO_INTERFACE("graphics", 0)
protected:
	int m_ScreenWidth;
	int m_ScreenHeight;
	int m_DesktopScreenWidth;
	int m_DesktopScreenHeight;
public:
	/* Constants: Texture Loading Flags
		TEXLOAD_NORESAMPLE - Prevents the texture from any resampling
	*/
	enum
	{
		TEXLOAD_NORESAMPLE = 1,
		TEXLOAD_NOMIPMAPS = 2,
	};

	class CTextureHandle
	{
		friend class IGraphics;
		int m_Id;
	public:
		CTextureHandle()
		: m_Id(-1)
		{}

		bool IsValid() const { return Id() >= 0; }
		int Id() const { return m_Id; }
	};
	int ScreenWidth() const { return m_ScreenWidth; }
	int ScreenHeight() const { return m_ScreenHeight; }
	float ScreenAspect() const { return (float)ScreenWidth()/(float)ScreenHeight(); }

	virtual void Clear(float r, float g, float b) = 0;
	virtual void ClearBufferTexture() = 0;

	virtual void ClipEnable(int x, int y, int w, int h) = 0;
	virtual void ClipDisable() = 0;

	virtual void MapScreen(float TopLeftX, float TopLeftY, float BottomRightX, float BottomRightY) = 0;
	virtual void GetScreen(float *pTopLeftX, float *pTopLeftY, float *pBottomRightX, float *pBottomRightY) = 0;

	// TODO: These should perhaps not be virtuals
	virtual void BlendNone() = 0;
	virtual void BlendNormal() = 0;
	virtual void BlendAdditive() = 0;
	virtual void BlendBuffer() = 0;
	virtual void BlendLight() = 0;
	virtual void WrapNormal() = 0;
	virtual void WrapClamp() = 0;
	virtual int MemoryUsage() const = 0;

	virtual void CreateTextureBuffer(int Width, int Height) = 0;

	virtual void LoadShaders() = 0;
	virtual void ShaderBegin(int Shade, float Intensity = 1.0f, float ColorSwap = 0.0f, float WeaponCharge = 0.0f) = 0;
	virtual void PlayerShaderBegin(float colorG, float colorB, float Charge = 0.0f, float Visibility = 1.0f, float Electro = 0.0f, float Damage = 0.0f, float Deathray = 0.0f) = 0;
	virtual void BallShaderBegin(float Speed, float Speed2) = 0;
	virtual void ShaderEnd() = 0;
	
	virtual void CameraToShaders(int ScreenWidth, int ScreenHeight, int CameraX, int CameraY) = 0;
	
	virtual int LoadPNG(CImageInfo *pImg, const char *pFilename, int StorageType) = 0;
	virtual int UnloadTexture(int Index) = 0;
	virtual int LoadTextureRaw(int Width, int Height, int Format, const void *pData, int StoreFormat, int Flags) = 0;
	virtual int LoadTexture(const char *pFilename, int StorageType, int StoreFormat, int Flags) = 0;
	virtual int LoadTextureRawSub(int TextureID, int x, int y, int Width, int Height, int Format, const void *pData) = 0;
	virtual void TextureSet(int TextureID, int BufferTexture = -1) = 0;
	void TextureClear() { TextureSet(-1); }
	
	virtual void RenderToScreen() = 0;
	virtual void RenderToTexture(int RenderBuffer) = 0;

	struct CLineItem
	{
		float m_X0, m_Y0, m_X1, m_Y1;
		CLineItem() {}
		CLineItem(float x0, float y0, float x1, float y1) : m_X0(x0), m_Y0(y0), m_X1(x1), m_Y1(y1) {}
	};
	virtual void LinesBegin() = 0;
	virtual void LinesEnd() = 0;
	virtual void LinesDraw(const CLineItem *pArray, int Num) = 0;

	virtual void QuadsBegin() = 0;
	virtual void QuadsEnd() = 0;
	virtual void QuadsSetRotation(float Angle) = 0;
	virtual void QuadsSetSubset(float TopLeftY, float TopLeftV, float BottomRightU, float BottomRightV, bool FreeForm = false) = 0;
	virtual void QuadsSetSubsetFree(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3) = 0;

	struct CQuadItem
	{
		float m_X, m_Y, m_Width, m_Height;
		CQuadItem() {}
		CQuadItem(float x, float y, float w, float h) : m_X(x), m_Y(y), m_Width(w), m_Height(h) {}
	};
	virtual void QuadsDraw(CQuadItem *pArray, int Num) = 0;
	virtual void QuadsDrawTL(const CQuadItem *pArray, int Num) = 0;

	struct CFreeformItem
	{
		float m_X0, m_Y0, m_X1, m_Y1, m_X2, m_Y2, m_X3, m_Y3;
		CFreeformItem() {}
		CFreeformItem(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3)
			: m_X0(x0), m_Y0(y0), m_X1(x1), m_Y1(y1), m_X2(x2), m_Y2(y2), m_X3(x3), m_Y3(y3) {}
	};
	virtual void QuadsDrawFreeform(const CFreeformItem *pArray, int Num) = 0;
	virtual void QuadsText(float x, float y, float Size, const char *pText) = 0;

	virtual void TrianglesBegin() = 0;
	virtual void TrianglesEnd() = 0;
	
	struct CTriangleItem
	{
		float m_X0, m_Y0, m_X1, m_Y1, m_X2, m_Y2;
		CTriangleItem() {}
		CTriangleItem(float x0, float y0, float x1, float y1, float x2, float y2) : m_X0(x0), m_Y0(y0), m_X1(x1), m_Y1(y1), m_X2(x2), m_Y2(y2) {}
	};
	virtual void TrianglesDraw(const CTriangleItem *pArray, int Num) = 0;

	struct CColorVertex
	{
		int m_Index;
		float m_R, m_G, m_B, m_A;
		CColorVertex() {}
		CColorVertex(int i, float r, float g, float b, float a) : m_Index(i), m_R(r), m_G(g), m_B(b), m_A(a) {}
	};
	virtual void SetColorVertex(const CColorVertex *pArray, int Num) = 0;
	virtual void SetColor(float r, float g, float b, float a) = 0;

	virtual void TakeScreenshot(const char *pFilename) = 0;
	virtual int GetVideoModes(CVideoMode *pModes, int MaxModes, int screen) = 0;
	virtual int GetNumScreens() = 0;

	virtual void Swap() = 0;

	// syncronization
	virtual void InsertSignal(class semaphore *pSemaphore) = 0;
	virtual bool IsIdle() = 0;
	virtual void WaitForIdle() = 0;
	
protected:
	inline CTextureHandle CreateTextureHandle(int Index)
	{
		CTextureHandle Tex;
		Tex.m_Id = Index;
		return Tex;
	}
};

class IEngineGraphics : public IGraphics
{
	MACRO_INTERFACE("enginegraphics", 0)
public:
	virtual int Init() = 0;
	virtual void Shutdown() = 0;

	virtual void Minimize() = 0;
	virtual void Maximize() = 0;

    virtual void GrabWindow(bool grab) = 0;
	virtual void WarpMouse(int x, int y) = 0;
	virtual int WindowActive() = 0;
	virtual int WindowOpen() = 0;
	virtual void *GetWindowHandle() = 0;
};

extern IEngineGraphics *CreateEngineGraphics();
extern IEngineGraphics *CreateEngineGraphicsThreaded();

#endif
