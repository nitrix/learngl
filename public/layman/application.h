#ifndef LAYMAN_APPLICATION_H
#define LAYMAN_APPLICATION_H

#include "environment.h"
#include "renderer.h"
#include "scene.h"
#include "window.h"

struct layman_application {
	struct layman_window *window;
	struct layman_renderer *renderer;
	struct layman_camera *camera;
	struct layman_scene *scene;
	struct layman_environment *environment;
};

struct layman_application *layman_application_create(int width, int height, const char *title, bool fullscreen);
void layman_application_destroy(struct layman_application *);

void layman_application_run(struct layman_application *app);

#endif