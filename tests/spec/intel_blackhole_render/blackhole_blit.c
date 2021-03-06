/*
 * Copyright © 2019 Intel Corporation
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


/** @file blackhole_blit.c
 *
 * Verifies that with GL_INTEL_black_render enabled, blit operations
 * have no effect.
 */

#include "piglit-util-gl.h"
#include "piglit-matrix.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

#if defined(PIGLIT_USE_OPENGL)
	config.supports_gl_core_version = 42;
#elif defined(PIGLIT_USE_OPENGL_ES2) || defined(PIGLIT_USE_OPENGL_ES3)
	config.supports_gl_es_version = 20;
#endif

	config.window_width = 400;
	config.window_height = 400;
	config.window_visual = PIGLIT_GL_VISUAL_RGBA | PIGLIT_GL_VISUAL_DOUBLE;

PIGLIT_GL_TEST_CONFIG_END

static enum piglit_result
run(void)
{
	float delta = 1.01 / piglit_width;
	GLuint prog, fb, fb_tex;

	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, piglit_winsys_fbo);
	glBindFramebufferEXT(GL_READ_FRAMEBUFFER, piglit_winsys_fbo);

	glClearColor(0.0, 0.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glGenFramebuffersEXT(1, &fb);
	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, fb);
	glBindFramebufferEXT(GL_READ_FRAMEBUFFER, fb);

	glCreateTextures(GL_TEXTURE_2D, 1, &fb_tex);
	glBindTexture(GL_TEXTURE_2D, fb_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
		     piglit_width, piglit_height, 0,
		     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2DEXT(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				  GL_TEXTURE_2D, fb_tex, 0);

	/* This atom should be disabled by default */
	if (glIsEnabled(GL_BLACKHOLE_RENDER_INTEL))
		return PIGLIT_FAIL;

	prog = piglit_build_simple_program(
#if defined(PIGLIT_USE_OPENGL)
		"#version 330\n"
		"in vec4 piglit_vertex;\n"
#elif defined(PIGLIT_USE_OPENGL_ES2) || defined(PIGLIT_USE_OPENGL_ES3)
		"#version 100\n"
		"attribute vec4 piglit_vertex;\n"
#endif
		"void main()\n"
		"{\n"
		"  gl_Position = piglit_vertex;\n"
		"}\n",
#if defined(PIGLIT_USE_OPENGL)
		"#version 330\n"
#elif defined(PIGLIT_USE_OPENGL_ES2) || defined(PIGLIT_USE_OPENGL_ES3)
		"#version 100\n"
#endif
		"void main()\n"
		"{\n"
		"  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
		"}\n");
	if (!prog)
		piglit_report_result(PIGLIT_FAIL);

	glViewport(0, 0, piglit_width, piglit_height);

	glClearColor(0.0, 0.0, 0.0, 0.0);

	glUseProgram(prog);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo;
	float vertices[3][2] = {
		{ -0.5, -1 + delta, },
		{ 0, 0.8, },
		{ 0.5, -1 + delta, },
	};
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);
	glEnableVertexAttribArray(0);

	glDisable(GL_BLACKHOLE_RENDER_INTEL);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	if(!piglit_check_gl_error(GL_NO_ERROR))
		return PIGLIT_FAIL;

	const float shader_expected[] = { 1.0, 0.0, 0.0, 1.0 };
	if (!piglit_probe_pixel_rgba(piglit_width / 2,
				     piglit_height / 2,
				     shader_expected))
		return PIGLIT_FAIL;

	piglit_present_results();

	glClearColor(0.0, 1.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	const float clear_expected[] = { 0.0, 1.0, 0.0, 1.0 };
	if (!piglit_probe_pixel_rgba(piglit_width / 2,
				     piglit_height / 2,
				     clear_expected))
		return PIGLIT_FAIL;

	piglit_present_results();

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fb);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, piglit_winsys_fbo);

	glEnable(GL_BLACKHOLE_RENDER_INTEL);

	if (!glIsEnabled(GL_BLACKHOLE_RENDER_INTEL))
		return PIGLIT_FAIL;

	glBlitFramebuffer(0, 0, piglit_width, piglit_height,
			  0, 0, piglit_width, piglit_height,
			  GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glDisable(GL_BLACKHOLE_RENDER_INTEL);

	if(!piglit_check_gl_error(GL_NO_ERROR))
		return PIGLIT_FAIL;

	glBindFramebuffer(GL_READ_FRAMEBUFFER, piglit_winsys_fbo);

	/* Blitting into piglit_winsys_fbo was dropped because of the
	 * blackhole state, expect the first clear color.
	 */
	const float blackhole_expected[] = { 0.0, 0.0, 1.0, 1.0 };
	if (!piglit_probe_pixel_rgba(piglit_width / 2,
				     piglit_height / 2,
				     blackhole_expected))
		return PIGLIT_FAIL;

	piglit_present_results();

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fb);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, piglit_winsys_fbo);

	glBlitFramebuffer(0, 0, piglit_width, piglit_height,
			  0, 0, piglit_width, piglit_height,
			  GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, piglit_winsys_fbo);

	/* Now the clear color that landed in fb should have been
	 * copied into piglit_winsys_fbo.
	 */
	if (!piglit_probe_pixel_rgba(piglit_width / 2,
				     piglit_height / 2,
				     clear_expected))
		return PIGLIT_FAIL;

	return PIGLIT_PASS;
}

enum piglit_result
piglit_display(void)
{
	enum piglit_result result = run();

	glDisable(GL_BLACKHOLE_RENDER_INTEL);

	return result;
}

void piglit_init(int argc, char **argv)
{
	piglit_require_extension("GL_INTEL_blackhole_render");
}
