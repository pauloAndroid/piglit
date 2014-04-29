// BEGIN_COPYRIGHT -*- glean -*-
//
// Copyright (C) 2001  Allen Akin   All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the
// Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
// KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL ALLEN AKIN BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
// OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// END_COPYRIGHT

// treadpix.cpp:  implementation of ReadPixels tests

#include <cmath>
#ifdef __CYGWIN__
#undef log2
#endif
#include <iomanip>
#include <cstdlib>
#include <algorithm>
#include "misc.h"
#include "rand.h"
#include "treadpix.h"


namespace GLEAN {

void
ReadPixSanityTest::checkRGBA(ReadPixSanityResult& r, Window& w) {
	DrawingSurfaceConfig& config = *r.config;
	RandomBitsDouble rRand(config.r, 1066);
	RandomBitsDouble gRand(config.g, 1492);
	RandomBitsDouble bRand(config.b, 1776);
	RandomBitsDouble aRand((config.a? config.a: 1), 1789);
	int thresh = 1;

	r.passRGBA = true;
	r.errRGBA = 0.0;
	for (int i = 0; i < 100 && r.passRGBA; ++i) {
		// Generate a random color and use it to clear the color buffer:
		float expected[4];
		expected[0] = rRand.next();
		expected[1] = gRand.next();
		expected[2] = bRand.next();
		expected[3] = aRand.next();
		glClearColor(expected[0],expected[1],expected[2],expected[3]);
		glClear(GL_COLOR_BUFFER_BIT);

		// If the color buffer doesn't have an alpha channel, then
		// the spec requires the readback value to be 1.0:
		if (!config.a)
			expected[3] = 1.0;

		// Read the buffer:
		GLfloat buf[READPIX_SANITY_WIN_SIZE][READPIX_SANITY_WIN_SIZE][4];
		glReadPixels(0, 0, READPIX_SANITY_WIN_SIZE,
			READPIX_SANITY_WIN_SIZE, GL_RGBA, GL_FLOAT, buf);

		// Now compute the error for each pixel, and record the
		// worst one we find:
		for (int y = 0; y < READPIX_SANITY_WIN_SIZE; ++y)
			for (int x = 0; x < READPIX_SANITY_WIN_SIZE; ++x) {
				GLfloat dr = abs(buf[y][x][0] - expected[0]);
				GLfloat dg = abs(buf[y][x][1] - expected[1]);
				GLfloat db = abs(buf[y][x][2] - expected[2]);
				GLfloat da = abs(buf[y][x][3] - expected[3]);
				double err =
				    max(ErrorBits(dr, config.r),
				    max(ErrorBits(dg, config.g),
				    max(ErrorBits(db, config.b),
					ErrorBits(da,
					   config.a? config.a: thresh+1))));
					   // The "thresh+1" fudge above is
					   // needed to force the error to
					   // be greater than the threshold
					   // in the case where there is no
					   // alpha channel.  Without it the
					   // error would be just equal to
					   // the threshold, and the test
					   // would spuriously pass.
				if (err > r.errRGBA) {
					r.xRGBA = x;
					r.yRGBA = y;
					r.errRGBA = err;
					for (int j = 0; j < 4; ++j) {
						r.expectedRGBA[j] = expected[j];
						r.actualRGBA[j] = buf[y][x][j];
					}
				}
			}

		if (r.errRGBA > thresh)
			r.passRGBA = false;
		w.swap();
	}
} // ReadPixSanityTest::checkRGBA

void
ReadPixSanityTest::checkDepth(ReadPixSanityResult& r, Window& w) {
	DrawingSurfaceConfig& config = *r.config;
	RandomDouble dRand(35798);
	int thresh = 1;

	r.passDepth = true;
	r.errDepth = 0.0;
	for (int i = 0; i < 100 && r.passDepth; ++i) {
		// Generate a random depth and use it to clear the depth buffer:
		GLdouble expected = dRand.next();
		glClearDepth(expected);
		glClear(GL_DEPTH_BUFFER_BIT);

		// Because glReadPixels won't return data of type GLdouble,
		// there's no straightforward portable way to deal with
		// integer depth buffers that are deeper than 32 bits or
		// floating-point depth buffers that have higher precision
		// than a GLfloat.  Since this is just a sanity check, we'll
		// use integer readback and settle for 32 bits at best.
		GLuint buf[READPIX_SANITY_WIN_SIZE][READPIX_SANITY_WIN_SIZE];
		glReadPixels(0, 0, READPIX_SANITY_WIN_SIZE,
			READPIX_SANITY_WIN_SIZE, GL_DEPTH_COMPONENT,
			GL_UNSIGNED_INT, buf);

		// Now compute the error for each pixel, and record the
		// worst one we find:
		for (int y = 0; y < READPIX_SANITY_WIN_SIZE; ++y)
			for (int x = 0; x < READPIX_SANITY_WIN_SIZE; ++x) {
				GLfloat dd = abs(buf[y][x]/4294967295.0
					- expected);
				double err = ErrorBits(dd, config.z);
				if (err > r.errDepth) {
					r.xDepth = x;
					r.yDepth = y;
					r.errDepth = err;
					r.expectedDepth = expected;
					r.actualDepth = buf[y][x]/4294967295.0;
				}
			}

		if (r.errDepth > thresh)
			r.passDepth = false;
		w.swap();
	}
} // ReadPixSanityTest::checkDepth

void
ReadPixSanityTest::checkStencil(ReadPixSanityResult& r, Window& w) {
	DrawingSurfaceConfig& config = *r.config;
	RandomBits sRand(config.s, 10101);

	r.passStencil = true;
	for (int i = 0; i < 100 && r.passStencil; ++i) {
		GLuint expected = sRand.next();
		glClearStencil(expected);
		glClear(GL_STENCIL_BUFFER_BIT);

		GLuint buf[READPIX_SANITY_WIN_SIZE][READPIX_SANITY_WIN_SIZE];
		glReadPixels(0, 0, READPIX_SANITY_WIN_SIZE,
			READPIX_SANITY_WIN_SIZE, GL_STENCIL_INDEX,
			GL_UNSIGNED_INT, buf);

		for (int y = 0; y < READPIX_SANITY_WIN_SIZE && r.passStencil;
		    ++y)
			for (int x = 0; x < READPIX_SANITY_WIN_SIZE; ++x)
				if (buf[y][x] != expected) {
					r.passStencil = false;
					r.xStencil = x;
					r.yStencil = y;
					r.expectedStencil = expected;
					r.actualStencil = buf[y][x];
					break;
				}

		w.swap();
	}
} // ReadPixSanityTest::checkStencil

void
ReadPixSanityTest::checkIndex(ReadPixSanityResult& r, Window& w) {
	DrawingSurfaceConfig& config = *r.config;
	RandomBits iRand(config.bufSize, 2);

	r.passIndex = true;
	for (int i = 0; i < 100 && r.passIndex; ++i) {
		GLuint expected = iRand.next();
		glClearIndex(expected);
		glClear(GL_COLOR_BUFFER_BIT);

		GLuint buf[READPIX_SANITY_WIN_SIZE][READPIX_SANITY_WIN_SIZE];
		glReadPixels(0, 0, READPIX_SANITY_WIN_SIZE,
			READPIX_SANITY_WIN_SIZE, GL_COLOR_INDEX,
			GL_UNSIGNED_INT, buf);

		for (int y = 0; y < READPIX_SANITY_WIN_SIZE && r.passIndex; ++y)
			for (int x = 0; x < READPIX_SANITY_WIN_SIZE; ++x)
				if (buf[y][x] != expected) {
					r.passIndex = false;
					r.xIndex = x;
					r.yIndex = y;
					r.expectedIndex = expected;
					r.actualIndex = buf[y][x];
					break;
				}

		w.swap();
	}
} // ReadPixSanityTest::checkIndex

///////////////////////////////////////////////////////////////////////////////
// runOne:  Run a single test case
///////////////////////////////////////////////////////////////////////////////
void
ReadPixSanityTest::runOne(ReadPixSanityResult& r, GLEAN::Window& w) {

	// Many (if not most) other tests need to read the contents of
	// the framebuffer to determine if the correct image has been
	// drawn.  Obviously this is a waste of time if the basic
	// functionality of glReadPixels isn't working.
	//
	// This test does a "sanity" check of glReadPixels.  Using as
	// little of the GL as practicable, it writes a random value
	// in the framebuffer, reads it, and compares the value read
	// with the value written.

	glPixelStorei(GL_PACK_SWAP_BYTES, GL_FALSE);
	glPixelStorei(GL_PACK_LSB_FIRST, GL_FALSE);
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_SKIP_ROWS, 0);
	glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
	glPixelTransferi(GL_MAP_STENCIL, GL_FALSE);
	glPixelTransferi(GL_INDEX_SHIFT, 0);
	glPixelTransferi(GL_INDEX_OFFSET, 0);
	glPixelTransferf(GL_RED_SCALE, 1.0);
	glPixelTransferf(GL_GREEN_SCALE, 1.0);
	glPixelTransferf(GL_BLUE_SCALE, 1.0);
	glPixelTransferf(GL_ALPHA_SCALE, 1.0);
	glPixelTransferf(GL_DEPTH_SCALE, 1.0);
	glPixelTransferf(GL_RED_BIAS, 0.0);
	glPixelTransferf(GL_GREEN_BIAS, 0.0);
	glPixelTransferf(GL_BLUE_BIAS, 0.0);
	glPixelTransferf(GL_ALPHA_BIAS, 0.0);
	glPixelTransferf(GL_DEPTH_BIAS, 0.0);

	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DITHER);

	glIndexMask(~0);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glStencilMask(~0);

	if (r.config->canRGBA)
		checkRGBA(r, w);
	if (r.config->z)
		checkDepth(r, w);
	if (r.config->s)
		checkStencil(r, w);

	r.pass = r.passRGBA & r.passDepth & r.passStencil & r.passIndex;
} // ReadPixSanityTest::runOne

void
ReadPixSanityTest::logOne(ReadPixSanityResult& r) {
	logPassFail(r);
	logConcise(r);

	if (!r.passRGBA) {
		env->log << "\tRGB(A) worst-case error was "
			<< r.errRGBA << " bits at ("
			<< r.xRGBA << ", " << r.yRGBA << ")\n";
		env->log << "\t\texpected ("
			<< r.expectedRGBA[0] << ", "
			<< r.expectedRGBA[1] << ", "
			<< r.expectedRGBA[2] << ", "
			<< r.expectedRGBA[3] << ")\n\t\tgot ("
			<< r.actualRGBA[0] << ", "
			<< r.actualRGBA[1] << ", "
			<< r.actualRGBA[2] << ", "
			<< r.actualRGBA[3] << ")\n"
			;
	}
	if (!r.passDepth) {
		env->log << "\tDepth worst-case error was "
			<< r.errDepth << " bits at ("
			<< r.xDepth << ", " << r.yDepth << ")\n";
		env->log << "\t\texpected "
			<< r.expectedDepth
			<< "; got "
			<< r.actualDepth
			<< ".\n"
			;
	}
	if (!r.passStencil) {
		env->log << "\tStencil expected "
			<< r.expectedStencil
			<< "; got "
			<< r.actualStencil
			<< ".\n"
			;
	}
	if (!r.passIndex) {
		env->log << "\tIndex expected "
			<< r.expectedIndex
			<< "; got "
			<< r.actualIndex
			<< ".\n"
			;
	}
	if (env->options.verbosity) {
		if (r.config->canRGBA)
			env->log << "\tRGBA largest readback error was "
				<< r.errRGBA
				<< " bits\n";
		if (r.config->z)
			env->log << "\tDepth largest readback error was "
				<< r.errDepth
				<< " bits\n";
	}
} // ReadPixSanityTest::logOne

///////////////////////////////////////////////////////////////////////////////
// The test object itself:
///////////////////////////////////////////////////////////////////////////////
ReadPixSanityTest
readPixSanityTest("readPixSanity", "1",

	"This test performs a sanity check of glReadPixels, using as\n"
	"few other portions of the GL as possible.  If this test fails,\n"
	"it may be pointless to run other tests, since so many of them\n"
	"depend on reading the contents of the framebuffer to determine\n"
	"if they pass.\n"
	"\n"
	"The test works by using glClear to fill the framebuffer with a\n"
	"randomly-chosen value, reading the contents of the\n"
	"framebuffer, and comparing the actual contents with the\n"
	"expected contents.  RGB, RGBA, color index, stencil, and depth\n"
	"buffers (whichever are applicable to the current rendering\n"
	"context) are checked.  The test passes if the actual contents\n"
	"are within 1 LSB of the expected contents.\n"

	);
}; // namespace GLEAN



////////////////////////////////////////////////////////////////////////////////
// ExactRGBATest
//	Verifies that unsigned RGBA values written to a framebuffer with
//	sufficient depth are not altered by the OpenGL implementation.
////////////////////////////////////////////////////////////////////////////////

namespace {

// Mac OS header file AssertMacros.h defines check as a macro. 
#ifdef check
#undef check
#endif

template<class T>
void
check(GLEAN::ExactRGBAResult::Flavor& r, GLEAN::DrawingSurfaceConfig& config,
    GLenum type, int roundingMode) {
	unsigned size = EXACT_RGBA_WIN_SIZE - 2;
	unsigned nPixels = size * size;
	unsigned nComponents = 4 * nPixels;
	T* expected = new T[nComponents];
	T* actual = new T[nComponents];
	GLEAN::RandomBits rand(32, 1929);
	unsigned x;
	unsigned y;
	T* p;
	T* q;

	// Draw random colors into the window, recording the raw
	// color data in the array "expected":
	p = expected;
	for (y = 0; y < size; ++y)
		for (x = 0; x < size; ++x) {
			p[0] = rand.next();	// r
			p[1] = rand.next();	// g
			p[2] = rand.next();	// b
			p[3] = rand.next();	// a
			switch (type) {
				case GL_UNSIGNED_BYTE:
					glColor4ubv(reinterpret_cast<GLubyte*>
					    (p));
					break;
				case GL_UNSIGNED_SHORT:
					glColor4usv(reinterpret_cast<GLushort*>
					    (p));
					break;
				case GL_UNSIGNED_INT:
					glColor4uiv(reinterpret_cast<GLuint*>
					    (p));
					break;
			}
			glBegin(GL_QUADS);
				glVertex2i(x + 1, y + 1);
				glVertex2i(x + 2, y + 1);
				glVertex2i(x + 2, y + 2);
				glVertex2i(x + 1, y + 2);
			glEnd();
			p += 4;
		}

	// Read the relevant contents of the window into the array
	// "actual":
	glReadPixels(1, 1, size, size, GL_RGBA, type, actual);

	// Find masks that select only the high-order bits that should
	// be common to both the host representation and the framebuffer
	// representation:
	int hostBits;
	switch (type) {
		case GL_UNSIGNED_BYTE:
			hostBits = 8;
			break;
		case GL_UNSIGNED_SHORT:
			hostBits = 16;
			break;
		case GL_UNSIGNED_INT:
			hostBits = 32;
			break;
	}
	T Mask[4];
	Mask[0] = static_cast<T>(-1) << (hostBits - min(hostBits, config.r));
	Mask[1] = static_cast<T>(-1) << (hostBits - min(hostBits, config.g));
	Mask[2] = static_cast<T>(-1) << (hostBits - min(hostBits, config.b));
	Mask[3] = static_cast<T>(-1) << (hostBits - min(hostBits, config.a));
	// Patch up arithmetic for RGB drawing surfaces.  All other nasty cases
	// are eliminated by the drawing surface filter, which requires
	// nonzero R, G, and B.
	if (Mask[0] == static_cast<T>(-1))
		Mask[0] = 0;

	// Compare masked actual and expected values, and record the
	// worst-case error location and magnitude.
	r.err = 0;
	p = expected;
	q = actual;
	for (y = 0; y < size; ++y)
		for (x = 0; x < size; ++x) {
			T e[4] = { p[0], p[1], p[2], p[3] };
			T a[4] = { q[0], q[1], q[2], q[3] };
			for (unsigned i = 0; i < 4; ++i) {
				if (roundingMode == 0) {
					// Follow the OpenGL spec to the letter
					e[i] = e[i] & Mask[i];
					a[i] = a[i] & Mask[i];
				}
				GLuint err =
					max(e[i], a[i]) - min(e[i], a[i]);
				if (roundingMode == 1) {
					// Do the sane thing
					if (err < ~Mask[i] / 2)
						err = 0;
				}
				if (err > r.err) {
					r.x = x;
					r.y = y;
					r.err = err;
					for (unsigned j = 0; j < 4; ++j) {
						r.expected[j] = e[j];
						r.actual[j] = a[j];
					}
				}
			}
			p += 4;
			q += 4;
		}

	// We only pass if the maximum error was zero.
	r.pass = (r.err == 0);

	delete[] expected;
	delete[] actual;
}

};


namespace GLEAN {


///////////////////////////////////////////////////////////////////////////////
// runOne:  Run a single test case
///////////////////////////////////////////////////////////////////////////////
void
ExactRGBATest::runOne(ExactRGBAResult& r, GLEAN::Window& w) {

	// Many other tests depend on the ability of the OpenGL
	// implementation to store fixed-point RGBA values in the
	// framebuffer, and to read back exactly the value that
	// was stored.  The OpenGL spec guarantees that this will work
	// under certain conditions, which are spelled out in section
	// 2.13.9 in the 1.2.1 version of the spec:
	//
	//	Suppose that lighting is disabled, the color associated
	//	with a vertex has not been clipped, and one of
	//	[gl]Colorub, [gl]Colorus, or [gl]Colorui was used to
	//	specify that color.  When these conditions are
	//	satisfied, an RGBA component must convert to a value
	//	that matches the component as specified in the Color
	//	command:  if m [the number of bits in the framebuffer
	//	color channel] is less than the number of bits b with
	//	which the component was specified, then the converted
	//	value must equal the most significant m bits of the
	//	specified value; otherwise, the most significant b bits
	//	of the converted value must equal the specified value.
	//
	// This test attempts to verify that behavior.


	// Don't bother running if the ReadPixels sanity test for this
	// display surface configuration failed:
	vector<ReadPixSanityResult*>::const_iterator rpsRes;
	for (rpsRes = readPixSanityTest.results.begin();
	    rpsRes != readPixSanityTest.results.end();
	    ++rpsRes)
		if ((*rpsRes)->config == r.config)
			break;
	if (rpsRes == readPixSanityTest.results.end() || !(*rpsRes)->pass) {
		r.skipped = true;
		r.pass = false;
		return;
	}

	// Hack: Make hardware driver tests feasible
	// (One can debate the sanity of the spec requirement cited above,
	// anyway. On the one hand, we don't want to remember which API call
	// was used to set the color. This means that glColoru[b|s|i] must
	// perform some conversion. The default language of the spec mandates
	// that the maximum integer value always correspond to 1.0, which
	// mandates rounding up of discarded bits. However, the language
	// above mandates simply discarding those bits.)
	int roundingMode = 0;
	const char* s = getenv("GLEAN_EXACTRGBA_ROUNDING");
	if (s) {
		roundingMode = atoi(s);
		env->log << "Note: Rounding mode changed to " << roundingMode << "\n";
	}

	// Much of this state should already be set, if the defaults are
	// implemented correctly.  We repeat the setting here in order
	// to insure reasonable results when there are bugs.

	GLUtils::useScreenCoords(EXACT_RGBA_WIN_SIZE, EXACT_RGBA_WIN_SIZE);

	glDisable(GL_LIGHTING);

	glFrontFace(GL_CCW);

	glDisable(GL_COLOR_MATERIAL);

	glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_3D);

	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
	glDisable(GL_CLIP_PLANE4);
	glDisable(GL_CLIP_PLANE5);

	glDisable(GL_FOG);

	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_DITHER);
	glDisable(GL_COLOR_LOGIC_OP);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_CULL_FACE);
	glDisable(GL_POLYGON_STIPPLE);
	glDisable(GL_POLYGON_OFFSET_FILL);

	glShadeModel(GL_FLAT);

	glPixelStorei(GL_PACK_SWAP_BYTES, GL_FALSE);
	glPixelStorei(GL_PACK_LSB_FIRST, GL_FALSE);
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_SKIP_ROWS, 0);
	glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
	glPixelTransferi(GL_MAP_STENCIL, GL_FALSE);
	glPixelTransferi(GL_INDEX_SHIFT, 0);
	glPixelTransferi(GL_INDEX_OFFSET, 0);
	glPixelTransferf(GL_RED_SCALE, 1.0);
	glPixelTransferf(GL_GREEN_SCALE, 1.0);
	glPixelTransferf(GL_BLUE_SCALE, 1.0);
	glPixelTransferf(GL_ALPHA_SCALE, 1.0);
	glPixelTransferf(GL_DEPTH_SCALE, 1.0);
	glPixelTransferf(GL_RED_BIAS, 0.0);
	glPixelTransferf(GL_GREEN_BIAS, 0.0);
	glPixelTransferf(GL_BLUE_BIAS, 0.0);
	glPixelTransferf(GL_ALPHA_BIAS, 0.0);
	glPixelTransferf(GL_DEPTH_BIAS, 0.0);

	check<GLubyte>(r.ub, *(r.config), GL_UNSIGNED_BYTE, roundingMode);
	w.swap();
	check<GLushort>(r.us, *(r.config), GL_UNSIGNED_SHORT, roundingMode);
	w.swap();
	check<GLuint>(r.ui, *(r.config), GL_UNSIGNED_INT, roundingMode);
	w.swap();
	r.pass = r.ub.pass && r.us.pass && r.ui.pass;
	r.skipped = false;
} // ExactRGBATest::runOne

void
ExactRGBATest::logFlavor(const char* label, const ExactRGBAResult::Flavor& r) {
	if (!r.pass) {
		env->log << "\t"
			<< label
			<< " worst-case error was 0x"
			<< hex
			<< r.err << " at ("
			<< dec
			<< r.x << ", " << r.y << ")\n";
		env->log << "\t\texpected (0x"
			<< hex
			<< r.expected[0] << ", 0x"
			<< r.expected[1] << ", 0x"
			<< r.expected[2] << ", 0x"
			<< r.expected[3] << ")\n\t\tgot (0x"
			<< r.actual[0] << ", 0x"
			<< r.actual[1] << ", 0x"
			<< r.actual[2] << ", 0x"
			<< r.actual[3] << ")\n"
			<< dec
			;
	}
} // ExactRGBATest::logFlavor

void
ExactRGBATest::logOne(ExactRGBAResult& r) {
	if (r.skipped) {
		env->log << name << ":  NOTE ";
		logConcise(r);
		env->log << "\tTest skipped; prerequisite test "
			 << readPixSanityTest.name
			 << " failed or was not run\n";
		return;
	}

	logPassFail(r);
	logConcise(r);

	logFlavor("Unsigned byte ", r.ub);
	logFlavor("Unsigned short", r.us);
	logFlavor("Unsigned int  ", r.ui);
} // ExactRGBATest::logOne

///////////////////////////////////////////////////////////////////////////////
// The test object itself:
///////////////////////////////////////////////////////////////////////////////
Test* exactRGBATestPrereqs[] = {&readPixSanityTest, 0};
ExactRGBATest
exactRGBATest("exactRGBA", "rgb", exactRGBATestPrereqs,

	"The OpenGL specification requires that under certain conditions\n"
	"(e.g. lighting disabled, no clipping, no dithering, etc.) colors\n"
	"specified as unsigned integers are represented *exactly* in the\n"
	"framebuffer (up to the number of bits common to both the\n"
	"original color and the framebuffer color channel).  Several glean\n"
	"tests depend on this behavior, so this test is a prerequisite for\n"
	"them.\n"
	"\n"
	"This test works by drawing many small quadrilaterals whose\n"
	"colors are specified by glColorub, glColorus, and glColorui;\n"
	"reading back the resulting image; and comparing the colors read\n"
	"back to the colors written.  The high-order bits shared by the\n"
	"source representation of the colors and the framebuffer\n"
	"representation of the colors must agree exactly for the test to\n"
	"pass.\n"

	);


} // namespace GLEAN
