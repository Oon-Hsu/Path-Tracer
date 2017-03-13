#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


#if defined(WIN32) || defined(_WINDOWS)
#include <Windows.h>
#include <gl/GL.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#endif

#include "PathTracer.h"
#include "Ray.h"
#include "Scene.h"
#include "Camera.h"
#include "perlin.h"

PathTracer::PathTracer()
{
	m_buffHeight = m_buffWidth = 0.0;
	m_renderCount = 0;
	SetTraceLevel(0); //Changed to 0 from 5
	m_traceflag = (TraceFlags)(TRACE_PATH | TRACE_GLOSS);
}

PathTracer::PathTracer(int Width, int Height)
{
	m_buffWidth = Width;
	m_buffHeight = Height;
	m_renderCount = 0;
	SetTraceLevel(0);

	m_framebuffer = new Framebuffer(Width, Height);
	m_traceflag = (TraceFlags)(TRACE_PATH);
}

PathTracer::~PathTracer()
{
	delete m_framebuffer;
}

void PathTracer::DoPathTrace(Scene* pScene)
{
	Camera* cam = pScene->GetSceneCamera();

	Vector3 camRightVector = cam->GetRightVector();
	Vector3 camUpVector = cam->GetUpVector();
	Vector3 camViewVector = cam->GetViewVector();
	Vector3 centre = cam->GetViewCentre();
	Vector3 camPosition = cam->GetPosition();

	double sceneWidth = pScene->GetSceneWidth();
	double sceneHeight = pScene->GetSceneHeight();

	double pixelDX = sceneWidth / m_buffWidth;
	double pixelDY = sceneHeight / m_buffHeight;

	int total = m_buffHeight*m_buffWidth;
	int done_count = 0;

	int spp = 1000; //number of samples

	Vector3 start;
	start[0] = centre[0] - ((sceneWidth * camRightVector[0])
		+(sceneHeight * camUpVector[0])) / 2.0;
	start[1] = centre[1] - ((sceneWidth * camRightVector[1])
		+ (sceneHeight * camUpVector[1])) / 2.0;
	start[2] = centre[2] - ((sceneWidth * camRightVector[2])
		+ (sceneHeight * camUpVector[2])) / 2.0;

	if (m_renderCount == 0)
	{
		fprintf(stdout, "Path Trace Start.\n");

		clock_t timer_start = clock(); //Timer
		Colour colour;
	
#pragma omp parallel for schedule (dynamic, 17) private(colour)
		for (int i = 0; i < m_buffHeight; i += 1)
		{
			for (int j = 0; j < m_buffWidth; j += 1)
			{
				//Allow passage of multiple rays per pixel
				for (int counter = 0; counter < spp; counter++)
				{
					Vector3 pixel;
					pixel[0] = start[0] + (i + 0.5) * camUpVector[0] * pixelDY
						+ (j + 0.5) * camRightVector[0] * pixelDX;
					pixel[1] = start[1] + (i + 0.5) * camUpVector[1] * pixelDY
						+ (j + 0.5) * camRightVector[1] * pixelDX;
					pixel[2] = start[2] + (i + 0.5) * camUpVector[2] * pixelDY
						+ (j + 0.5) * camRightVector[2] * pixelDX;

					Ray viewray;

					viewray.SetRay(camPosition, (pixel - camPosition).Normalise());
					//Accumulate colour, opposed to resetting after each iteration
					colour = colour + this->Radiance(pScene, viewray, 0);
				}

				colour = colour * (1. / spp);
				m_framebuffer->WriteRGBToFramebuffer(colour, j, i);
			}
		}

		clock_t timer_stop = clock();
		double time = double((timer_stop - timer_start) / CLOCKS_PER_SEC);

		fprintf(stdout,"%f Path Tracing Complete.\n", time);
		m_renderCount++;
	}
}

Colour PathTracer::Radiance(Scene* pScene, Ray& ray, int tracelevel)
{
	Colour outcolour;
	RayHitResult result;
	//Find closest objects
	result = pScene->IntersectByRay(ray);

	if (result.data)
	{
		Primitive* prim = (Primitive*)result.data;
		Material* mat = prim->GetMaterial();

		Colour diffuse = mat->GetDiffuseColour();
		Colour emittance = mat->GetEmissiveColour();

		Vector3 intersection = result.point;
		Vector3 normal = result.normal;

		double p = GetMax(diffuse);

		//Roulette
		if (++tracelevel > 5)
		{
			if (GetRandomNumber() < p)
			{
				diffuse = diffuse * (1 / p);
			}
			else
			{
				return emittance;
			}
		}

		Vector3 direction = SampleDiffuse(normal);

		Ray ray_diffuse;
		Ray ray_reflective;

		//Make the walls visible
		ray_diffuse.SetRay(intersection + direction * 0.0001f, direction);

		//F8: Make the balls shiny
		if (m_traceflag & TRACE_GLOSS)
		{
			if (((Primitive*)result.data)->m_primtype == Primitive::PRIMTYPE_Box || ((Primitive*)result.data)->m_primtype == Primitive::PRIMTYPE_Sphere)
			{
				Vector3 reflection = (ray.GetRay().Reflect(normal)).Normalise();
				ray_reflective.SetRay(intersection + reflection * 0.001f, reflection);
				outcolour = emittance + diffuse * Radiance(pScene, ray_reflective, tracelevel);
			}
			//Make sure the plane doesn't reflect
			if (((Primitive*)result.data)->m_primtype == Primitive::PRIMTYPE_Plane)
			{
				outcolour = emittance + diffuse * Radiance(pScene, ray_diffuse, tracelevel);
			}
		}
		//F7: Less shiny
		else
		{
			outcolour = emittance + diffuse * Radiance(pScene, ray_diffuse, tracelevel);
		}

		return outcolour;
	}
}