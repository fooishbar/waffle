// Copyright 2012 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "glx_platform.h"

#include <dlfcn.h>
#include <stdlib.h>

#include <waffle/native.h>
#include <waffle/waffle_enum.h>
#include <waffle/core/wcore_error.h>

#include "glx_config.h"
#include "glx_context.h"
#include "glx_display.h"
#include "glx_gl_misc.h"
#include "glx_priv_types.h"
#include "glx_window.h"

static const struct native_dispatch glx_dispatch = {
    .display_connect = glx_display_connect,
    .display_disconnect = glx_display_disconnect,
    .config_choose = glx_config_choose,
    .config_destroy = glx_config_destroy,
    .context_create = glx_context_create,
    .context_destroy = glx_context_destroy,
    .window_create = glx_window_create,
    .window_destroy = glx_window_destroy,
    .window_swap_buffers = glx_window_swap_buffers,
    .make_current = glx_make_current,
    .get_proc_address = glx_get_proc_address,
    .dlsym_gl = glx_dlsym_gl,
};

union native_platform*
glx_platform_create(
        int gl_api,
        const struct native_dispatch **dispatch)
{
    union native_platform *self;
    NATIVE_ALLOC(self, glx);
    if (!self) {
        wcore_error(WAFFLE_OUT_OF_MEMORY);
        return NULL;
    }

    self->glx->gl_api = gl_api;

    switch (gl_api) {
        case WAFFLE_OPENGL:
            self->glx->libgl_name = "libGL.so";
            break;
        case WAFFLE_OPENGL_ES1:
        case WAFFLE_OPENGL_ES2:
            wcore_errorf(WAFFLE_BAD_ATTRIBUTE,
                         "GLX does not yet support %s",
                         waffle_enum_to_string(gl_api));
            goto error;
        default:
            wcore_error_internal("gl_api has bad value 0x%x", gl_api);
            goto error;
    }

    self->glx->libgl = dlopen(self->glx->libgl_name, RTLD_LAZY);
    if (!self->glx->libgl) {
        wcore_errorf(WAFFLE_UNKNOWN_ERROR,
                      "dlopen(\"%s\") failed", self->glx->libgl_name);
        goto error;
    }

    *dispatch = &glx_dispatch;
    return self;

error:
    WCORE_ERROR_DISABLED({
        glx_platform_destroy(self);
    });
    return NULL;
}

bool
glx_platform_destroy(union native_platform *self)
{
    int error = 0;

    if (!self)
        return true;

    if (self->glx->libgl) {
        error |= dlclose(self->glx->libgl);
        if (error) {
            wcore_errorf(WAFFLE_UNKNOWN_ERROR, "dlclose() failed on \"%s\"",
                         self->glx->libgl_name);
        }
    }

    free(self);
    return !error;
}