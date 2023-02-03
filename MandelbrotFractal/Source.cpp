#include <SDL.h>
#include <thread>
#include <complex>
#include <direct.h>


using namespace std;

#define WIDTH 2560
#define HEIGHT 1440

#define START_POS -0.5
#define START_ZOOM (WIDTH * 0.25296875L) - 200

#define BAIL_OUT 2.0L
#define ZOOM_FACTOR 4L

const int threadCount = thread::hardware_concurrency() + 20;	// number of render threads, better to have slightly more than your actual CPU logical cores

void renderPart(int index, long double zoom, complex<long double> center, SDL_Surface* surface) {
	int x, y, n;
	int maxiter = (WIDTH / 2) * 0.06L * log10(zoom);	// change the multiplication value to adjust how precision scales with zoom
	complex<long double> z, c;
	long double C;
	int flips = threadCount;

	for (y = index * (HEIGHT / flips); y < (index + 1) * (HEIGHT / flips); ++y) {
		for (x = 0; x < WIDTH; ++x) {
			z = c = real(center) + ((x - (WIDTH / 2)) / zoom) + ((imag(center) + ((y - (HEIGHT / 2)) / zoom)) * complex<long double>(0.0L, 1.0L));

			#define X real(z)
			#define Y imag(z)

			// condition to stay within bounds
			if ((pow(X - 0.25L, 2) + pow(Y, 2)) * (pow(X, 2) + (X / 2L) + pow(Y, 2) - 0.1875L) < pow(Y, 2) / 4L || pow(X + 1.0L, 2) + pow(Y, 2) < 0.0625L) {
				n = maxiter;
			}
			else {
				for (n = 0; n <= maxiter && abs(z) < BAIL_OUT; ++n) {
					z = pow(z, 2) + c;
				}
			}

			// pixel coloring for points within and outside of the set
			C = n - log2l(logl(abs(z)) / 0.69314718055994530942L);
			((Uint32*)surface->pixels)[(y * surface->w) + x] = (n >= maxiter) ? 0 : SDL_MapRGB(surface->format, (1 + sin(C * 0.07L + 5)) * 127.0L, (1 + cos(C * 0.05L)) * 127.0L, (1 + sin(C * 0.05L)) * 127.0L);
		}
	}
}

void drawMandelbrotMultithreaded(SDL_Window* window, SDL_Surface* surface, complex<long double> center, long double zoom) {
	//chrono::steady_clock::time_point start = chrono::high_resolution_clock::now(), end;
	
	int flips = threadCount;	// number of splits of the screen, each thread renders a part of the screen
	thread threads[HEIGHT];

	for (int f = 0; f < flips; ++f) {
		threads[f] = thread(renderPart, f, zoom, center, surface);
	}

	for (int f = 0; f < flips; ++f) {
		threads[f].join();
	}

	SDL_UpdateWindowSurface(window);

	/*end = chrono::high_resolution_clock::now();
	int mills = chrono::duration<double, milli>(end - start).count();
	ostringstream time;
	time << "Time spent: ";
	time << mills;
	time << " ms\n";
	OutputDebugString(time.str().c_str());*/
}

int main(int argc, char* argv[]) {
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window* window;
	SDL_Event event;

	window = SDL_CreateWindow("SDL Mandelbrot",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		WIDTH,
		HEIGHT,
		SDL_WINDOW_VULKAN);		// you can also use opengl or metal

	SDL_Surface* surface = SDL_GetWindowSurface(window);

	complex<long double> center = START_POS;
	long double zoom = START_ZOOM;
	bool autozoom = true;

	if (autozoom) {
		center = complex<long double>(-1.315180982097868, 0.073481649996795);	// location to zoom at, you can search for other interesting locations on the internet
	}

	_mkdir("images");

	drawMandelbrotMultithreaded(window, surface, center, zoom);
	SDL_SaveBMP(surface, "images/sc0.bmp");

	int iterations = 0;
	while (true) {
		SDL_PollEvent(&event);

		switch (event.type) {
		case SDL_QUIT:
			exit(0);
			break;
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == ' ') {	// spacebar to reset current view to starting position and zoom
				center = START_POS;
				zoom = START_ZOOM;
				drawMandelbrotMultithreaded(window, surface, center, zoom);
			}
			else if (event.key.keysym.sym == SDLK_ESCAPE) {		// escape to exit app
				exit(0);
			}
			else if (event.key.keysym.sym == 'a') {		// "a" to switch autozoom on/off 
				autozoom = !autozoom;
			}
			break;
		case SDL_MOUSEBUTTONDOWN:	// zoom by zoom factor to the mouse location when left click
			center = real(center) + ((event.button.x - (WIDTH / 2)) / zoom) + ((imag(center) + ((event.button.y - (HEIGHT / 2)) / zoom)) * complex<long double>(0.0L, 1.0L));

			if (event.button.button == 1) {
				zoom *= ZOOM_FACTOR + log10(zoom);
			}
			else if (event.button.button == 3) {
				zoom /= ZOOM_FACTOR;
			}

			drawMandelbrotMultithreaded(window, surface, center, zoom);
		}

		if (autozoom) {		// automatically zoom and save images as bmp
			zoom *= 1.01;	// adjust for different rate of autozoom
			drawMandelbrotMultithreaded(window, surface, center, zoom);
			iterations++;
			string file = "images/sc";
			file += to_string(iterations);
			file += ".bmp";
			SDL_SaveBMP(surface, file.c_str());
		}
	}


	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}