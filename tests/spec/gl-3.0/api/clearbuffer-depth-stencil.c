/* Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * \file clearbuffer-depth.c
 * Verify clearing depth buffers with glClearBufferfv
 *
 * This test works by generating several framebuffer objects and attempting to
 * clear the depth buffer of those FBOs by calling \c glClearBufferfv.
 *
 *     - An FBO with only a color attachment.  This should not generate an
 *       error, but the color data should not be modified.
 *
 *     - An FBO with only a depth attachment.
 *
 *     - An FBO with a depth attachment and a color attachment.  The color
 *       data should not be modified.
 *
 *     - An FBO with a depth attachment and a stencil attachment.  The stencil
 *       data should not be modified.
 *
 *     - An FBO with a packed depth/stencil attachment.  The stencil data
 *       should not be modified.
 *
 * In each case, \c glClearBufferfv is called more than once. Each call uses a
 * different clear value.  This ensures that the test doesn't erroneously pass
 * because the depth buffer was already filled with the clear color.
 *
 * \author Ian Romanick
 */
#include "piglit-util-gl.h"
#include "clearbuffer-common.h"

void piglit_init(int argc, char **argv)
{
	static const struct {
		bool color;
		bool stencil;
		bool depth;
		bool packed;
	} test_vectors[] = {
		{ true,  false, false, false },
		{ false, true,  false, false },
		{ true,  true,  false, false },
		{ false, false, true,  false },
		{ true,  false, true,  false },
		{ false, true,  true,  false },
		{ true,  true,  true,  false },
		{ false, true,  true,  true },
		{ true,  true,  true,  true },
	};

	static const int first_s    = 0x01;
	static const int second_s   = 0xfe;
	static const float first_d  = 0.5;
	static const float second_d = 0.8;
	static const float third_d = -5;
	static const float third_d_clamped = 0;

	unsigned i;
	bool pass = true;

	piglit_require_gl_version(30);

	for (i = 0; i < ARRAY_SIZE(test_vectors); i++) {
		GLenum err;
		GLuint fb = generate_simple_fbo(test_vectors[i].color,
						test_vectors[i].stencil,
						test_vectors[i].depth,
						test_vectors[i].packed);

		if (fb == 0) {
			if (!piglit_automatic) {
				printf("Skipping framebuffer %s color, "
				       "%s depth, and "
				       "%s stencil (%s).\n",
				       test_vectors[i].color
				       ? "with" : "without",
				       test_vectors[i].depth
				       ? "with" : "without",
				       test_vectors[i].stencil
				       ? "with" : "without",
				       test_vectors[i].packed
				       ? "packed" : "separate");
			}

			continue;
		}

		if (!piglit_automatic) {
			printf("Trying framebuffer %s color, "
			       "%s depth and "
			       "%s stencil (%s)...\n",
			       test_vectors[i].color ? "with" : "without",
			       test_vectors[i].depth ? "with" : "without",
			       test_vectors[i].stencil ? "with" : "without",
			       test_vectors[i].packed ? "packed" : "separate");
		}

		/* The GL spec says nothing about generating an error for
		 * clearing a buffer that does not exist.  Certainly glClear
		 * does not.
		 */
		glClearBufferfi(GL_DEPTH_STENCIL, 0, first_d, first_s);
		err = glGetError();
		if (err != GL_NO_ERROR) {
			fprintf(stderr,
				"First call to glClearBufferfi erroneously "
				"generated a GL error (%s, 0x%04x)\n",
				piglit_get_gl_error_name(err), err);
			pass = false;
		}

		pass = simple_probe(test_vectors[i].color,
				    default_color,
				    test_vectors[i].stencil,
				    first_s,
				    test_vectors[i].depth,
				    first_d)
			&& pass;

		glClearBufferfi(GL_DEPTH_STENCIL, 0, second_d, second_s);
		err = glGetError();
		if (err != GL_NO_ERROR) {
			fprintf(stderr,
				"Second call to glClearBufferfi erroneously "
				"generated a GL error (%s, 0x%04x)\n",
				piglit_get_gl_error_name(err), err);
			pass = false;
		}

		pass = simple_probe(test_vectors[i].color,
				    default_color,
				    test_vectors[i].stencil,
				    second_s,
				    test_vectors[i].depth,
				    second_d)
			&& pass;

		/* From the OpenGL 3.0 spec,
		 * Section 4.2.3 "Clearing the Buffers":
		 *
		 *    depth and stencil are the values to clear the depth and
		 *    stencil buffers to, respectively. Clamping and type
		 *    conversion for fixed-point depth buffers are performed
		 *    in the same fashion as for ClearDepth.
		 *
		 * generate_simple_fbo generates a fixed-point depth buffer,
		 * so the clamping applies.
		 */
		glClearBufferfi(GL_DEPTH_STENCIL, 0, third_d, default_stencil);
		err = glGetError();
		if (err != GL_NO_ERROR) {
			fprintf(stderr,
				"Third call to glClearBufferfi erroneously "
				"generated a GL error (%s, 0x%04x)\n",
				piglit_get_gl_error_name(err), err);
			pass = false;
		}

		pass = simple_probe(test_vectors[i].color,
				    default_color,
				    test_vectors[i].stencil,
				    default_stencil,
				    test_vectors[i].depth,
				    third_d_clamped)
			&& pass;

		glDeleteFramebuffers(1, &fb);
		pass = piglit_check_gl_error(GL_NO_ERROR) && pass;
	}

	piglit_report_result(pass ? PIGLIT_PASS : PIGLIT_FAIL);
}
