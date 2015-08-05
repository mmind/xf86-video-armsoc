/*
 * Copyright © 2013 ARM Limited.
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "../drmmode_driver.h"
#include <stddef.h>
#include <xf86drmMode.h>
#include <xf86drm.h>
#include <sys/ioctl.h>

/* Following ioctls should be included from libdrm rockchip_drm.h but
 * libdrm doesn't install this correctly so for now they are here.
 */
/*struct drm_rockchip_plane_set_zpos {
	__u32 plane_id;
	__s32 zpos;
};
#define DRM_ROCKCHIP_PLANE_SET_ZPOS 0x06
#define DRM_IOCTL_ROCKCHIP_PLANE_SET_ZPOS DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_ROCKCHIP_PLANE_SET_ZPOS, struct drm_rockchip_plane_set_zpos)
*/

struct drm_rockchip_gem_create {
	uint64_t size;
	uint32_t flags;
	uint32_t handle;
};

#define DRM_ROCKCHIP_GEM_CREATE 0x00
#define DRM_IOCTL_ROCKCHIP_GEM_CREATE DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_ROCKCHIP_GEM_CREATE, struct drm_rockchip_gem_create)

/* Cursor dimensions
 * Technically we probably don't have any size limit.. since we
 * are just using an overlay... but xserver will always create
 * cursor images in the max size, so don't use width/height values
 * that are too big
 */
#define CURSORW  (64)
#define CURSORH  (64)

/*
 * Padding added down each side of cursor image. This is a workaround for a bug
 * causing corruption when the cursor reaches the screen edges.
 */
#define CURSORPAD (0)

#define ALIGN(val, align)	(((val) + (align) - 1) & ~((align) - 1))

static int create_custom_gem(int fd, struct armsoc_create_gem *create_gem)
{
	struct drm_rockchip_gem_create create_rockchip;
	int ret;
	unsigned int pitch;

	/* make pitch a multiple of 64 bytes for best performance */
	pitch = ALIGN(create_gem->width * ((create_gem->bpp + 7) / 8), 64);
	memset(&create_rockchip, 0, sizeof(create_rockchip));
	create_rockchip.size = create_gem->height * pitch;

	assert((create_gem->buf_type == ARMSOC_BO_SCANOUT) ||
			(create_gem->buf_type == ARMSOC_BO_NON_SCANOUT));

	ret = drmIoctl(fd, DRM_IOCTL_ROCKCHIP_GEM_CREATE, &create_rockchip);
	if (ret)
		return ret;

	/* Convert custom create_rockchip to generic create_gem */
	create_gem->handle = create_rockchip.handle;
	create_gem->pitch = pitch;
	create_gem->size = create_rockchip.size;

	return 0;
}

struct drmmode_interface rockchip_interface = {
	"rockchip"            /* name of drm driver */,
	1                     /* use_page_flip_events */,
	1                     /* use_early_display */,
	CURSORW               /* cursor width */,
	CURSORH               /* cursor_height */,
	CURSORPAD             /* cursor padding */,
	HWCURSOR_API_PLANE    /* cursor_api */,
	NULL                  /* init_plane_for_cursor */,
	1                     /* vblank_query_supported */,
	create_custom_gem     /* create_custom_gem */,
};
