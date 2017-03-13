#pragma once

#include "Material.h"
#include "Ray.h"
#include "Scene.h"
#include "Framebuffer.h"

#define M_PI 3.14159265358979323846

class PathTracer
{
	private:
		Framebuffer		*m_framebuffer;
		int				m_buffWidth;
		int				m_buffHeight;
		int				m_renderCount;
		int				m_traceLevel;

		Colour Radiance(Scene* pScene, Ray& ray, int depth);

	public:

		enum TraceFlags
		{
			TRACE_NULL = 0,
			TRACE_PATH = 0x1,
			TRACE_GLOSS = 0x1 << 1,
		};

		TraceFlags m_traceflag;			

		PathTracer();
		PathTracer(int width, int height);
		~PathTracer();

		inline void SetTraceLevel(int level)	
		{
			m_traceLevel = level;
		}

		inline void ResetRenderCount()
		{
			m_renderCount = 0;
		}

		inline Framebuffer *GetFramebuffer() const
		{
			return m_framebuffer;
		}

		//My Functions
		inline double GetMax(Colour colour)
		{
			double x = colour[0];
			double y = colour[1];
			double z = colour[2];

			if (x > y && x > z)
			{
				return x;
			}
			if (y > z)
			{
				return y;
			}
			else
			{
				return z;
			}
		}
		
		inline double GetRandomNumber()
		{
			return (double)rand() / (double)RAND_MAX;
		}

		inline Vector3 SampleDiffuse(Vector3 normal)
		{
			double radius_1 = 2 * M_PI * GetRandomNumber();
			double radius_2 = GetRandomNumber();
			double radius_2s = sqrt(radius_2);

			Vector3 u = ((fabs(normal[0]) > 0.1 ? Vector3(0, 1, 0) : Vector3(1, 0, 0)).CrossProduct(normal)).Normalise();
			Vector3 v = normal.CrossProduct(u);
			Vector3 d = (u*cos(radius_1)*radius_2s + v*sin(radius_1)*radius_2s + normal*sqrt(1 - radius_2)).Normalise();

			return d;
		}

		void DoPathTrace(Scene* pScene);
};

