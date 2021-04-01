#include "cimgui.h"
#include "environment.h"
#include "glad/glad.h"
#include "incbin.h"
#include "mesh.h"
#include "renderer.h"
#include "scene.h"
#include "shader.h"
#include "ui.h"
#include "utils.h"
#include <stdlib.h>

#define RENDERER_FOV 45.0f
#define RENDERER_PLANE_FAR 1000.0f
#define RENDERER_PLANE_NEAR 0.1f

INCBIN(shaders_skybox_main_vert, "../shaders/skybox/main.vert");
INCBIN(shaders_skybox_main_frag, "../shaders/skybox/main.frag");

struct renderer *renderer_create(const struct window *window) {
	struct renderer *renderer = malloc(sizeof *renderer);
	if (!renderer) {
		return NULL;
	}

	// TODO: Dynamic dimensions; what happens when the window gets resized?
	unsigned int width, height;
	window_framebuffer_size(window, &width, &height);
	renderer->viewport_width = width;
	renderer->viewport_height = height;

	// TODO: Should provide means of modifying these.
	renderer->fov = RENDERER_FOV;
	renderer->far_plane = RENDERER_PLANE_FAR;
	renderer->near_plane = RENDERER_PLANE_NEAR;
	renderer->start_time = glfwGetTime();
	renderer->exposure = 1;

	renderer->window = window;
	renderer->wireframe = false;

	renderer->ui = ui_create(renderer);
	if (!renderer->ui) {
		free(renderer);
		return NULL;
	}

	renderer->fps_history_total_count = 0;
	renderer->fps_history_highest = 0;
	for (size_t i = 0; i < FPS_HISTORY_MAX_COUNT; i++) {
		renderer->fps_history[i] = 0;
	}

	return renderer;
}

void renderer_destroy(struct renderer *renderer) {
	if (!renderer) {
		return;
	}

	ui_destroy(renderer->ui);
	free(renderer);
}

void renderer_switch(const struct renderer *new) {
	thread_local static const struct renderer *current;

	if (current == new) {
		return;
	}

	current = new;

	if (!new) {
		return;
	}

	// Resize the viewport, go back to the default framebuffer and clear color for rendering.
	glViewport(0, 0, new->viewport_width, new->viewport_height);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0, 0, 0, 1); // Black.

	// FIXME: Back face culling.
	// glEnable(GL_CULL_FACE);
	// glCullFace(GL_BACK);
	// glFrontFace(GL_CCW);
	// glDisable(GL_CULL_FACE);

	// Depth testing.
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// Multisampling.
	glEnable(GL_MULTISAMPLE);

	// Wireframe.
	if (new->wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// Necessary to avoid the seams of the cubemap being visible.
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

static void render_mesh(struct renderer *renderer, const struct camera *camera, const struct scene *scene, const struct entity *entity, const struct mesh *mesh) {
	static GLint viewProjectionMatrixLocation = -1;
	static GLint modelMatrixLocation = -1;
	static GLint normalMatrixLocation = -1;
	static GLint exposureLocation = -1;

	mesh_switch(mesh);

	// Uniforms.
	shader_bind_uniform_environment(mesh->shader, scene->environment);
	shader_bind_uniform_material(mesh->shader, mesh->material);
	shader_bind_uniform_camera(mesh->shader, camera);
	shader_bind_uniform_lights(mesh->shader, scene->lights, scene->lights_count);

	// FIXME: Horrible, please don't do this every frames!
	if (viewProjectionMatrixLocation == -1) {
		viewProjectionMatrixLocation = glGetUniformLocation(mesh->shader->program_id, "u_ViewProjectionMatrix");
		modelMatrixLocation = glGetUniformLocation(mesh->shader->program_id, "u_ModelMatrix");
		normalMatrixLocation = glGetUniformLocation(mesh->shader->program_id, "u_NormalMatrix");
		exposureLocation = glGetUniformLocation(mesh->shader->program_id, "u_Exposure");
	}

	// TODO: More uniforms, tidy this up.
	// TODO: Should all move into the model file and stuff.

	mat4 view_matrix, projection_matrix, view_projection_matrix;
	glm_lookat((float *) camera->translation, (vec3) { 0, 0, 0}, (vec3) { 0, 1, 0}, view_matrix);
	glm_perspective_default(renderer->viewport_width / renderer->viewport_height, projection_matrix);
	glm_perspective(glm_rad(renderer->fov), renderer->viewport_width / renderer->viewport_height, renderer->near_plane, renderer->far_plane, projection_matrix);

	// glm_rotate_z(view_matrix, camera->rotation[2], view_matrix);
	// glm_rotate_y(view_matrix, camera->rotation[1], view_matrix);
	// glm_rotate_x(view_matrix, camera->rotation[0], view_matrix);

	glm_mat4_mul(projection_matrix, view_matrix, view_projection_matrix);
	glUniformMatrix4fv(viewProjectionMatrixLocation, 1, false, view_projection_matrix[0]);

	// FIXME: Should we add the model initial transforms to this too? Or maybe they should just be copied to entities when they're created.
	mat4 model_matrix = GLM_MAT4_IDENTITY_INIT;

	// Translation, rotation, scale.
	glm_translate(model_matrix, (float *) entity->translation);
	glm_quat_rotate(model_matrix, entity->rotation, model_matrix);
	glm_scale(model_matrix, (vec3) { entity->scale, entity->scale, entity->scale});

	glUniformMatrix4fv(modelMatrixLocation, 1, false, model_matrix[0]);
	glUniformMatrix4fv(normalMatrixLocation, 1, false, model_matrix[0]);
	glUniform1f(exposureLocation, renderer->exposure);

	// Render.
	// FIXME: Support more than just unsigned shorts.
	glDrawElements(GL_TRIANGLES, mesh->indices_count, GL_UNSIGNED_SHORT, NULL);
}

static void render_skybox(const struct renderer *renderer, const struct camera *camera, const struct scene *scene) {
	static struct shader *skybox_shader = NULL;
	static GLint environment_map_location = 0;
	static GLint projection_location = 0;
	static GLint view_location = 0;

	// FIXME: All this shouldn't be here.
	if (skybox_shader == NULL) {
		skybox_shader = shader_load_from_memory(shaders_skybox_main_vert_data, shaders_skybox_main_vert_size, shaders_skybox_main_frag_data, shaders_skybox_main_frag_size, NULL, 0);
		if (skybox_shader == NULL) {
			fprintf(stderr, "Unable to load skybox shader\n");
			exit(EXIT_FAILURE); // TODO: Handle failure more gracefully.
		}

		shader_switch(skybox_shader);

		environment_map_location = glGetUniformLocation(skybox_shader->program_id, "environmentMap");
		projection_location = glGetUniformLocation(skybox_shader->program_id, "projection");
		view_location = glGetUniformLocation(skybox_shader->program_id, "view");
	}

	shader_switch(skybox_shader);
	texture_switch(scene->environment->cubemap);
	glUniform1i(environment_map_location, scene->environment->cubemap->kind);
	mat4 projection_matrix = GLM_MAT4_IDENTITY_INIT;
	// glm_perspective_default(renderer->viewport_width / renderer->viewport_height, projection);
	glm_perspective(glm_rad(renderer->fov), renderer->viewport_width / renderer->viewport_height, renderer->near_plane, renderer->far_plane, projection_matrix);
	glUniformMatrix4fv(projection_location, 1, false, projection_matrix[0]);
	mat4 view = GLM_MAT4_IDENTITY_INIT;
	glm_lookat((float *) camera->translation, (vec3) { 0, 0, -1}, (vec3) { 0, 1, 0}, view);
	glm_rotate_y(view, camera->rotation[1], view);
	glUniformMatrix4fv(view_location, 1, false, view[0]);

	// glDisable(GL_CULL_FACE);
	renderCube();
	// glEnable(GL_CULL_FACE);
	mesh_switch(NULL);
}

void track_fps(struct renderer *renderer) {
	static int fps_count = 0;
	static double previous_time = 0;

	double current_time = window_elapsed(renderer->window);
	if (previous_time == 0) {
		previous_time = current_time;
	}

	fps_count++;

	if (current_time - previous_time > 1) {
		previous_time = current_time;

		renderer->fps_history_total_count++;

		size_t index = (renderer->fps_history_total_count - 1) % FPS_HISTORY_MAX_COUNT;
		renderer->fps_history[index] = fps_count;

		if (fps_count > renderer->fps_history_highest) {
			renderer->fps_history_highest = fps_count;
		}

		fps_count = 0;
	}
}

void renderer_render(struct renderer *renderer, const struct camera *camera, const struct scene *scene) {
	renderer_switch(renderer);
	environment_switch(scene->environment);

	// Clear the screen.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Render all entities.
	for (size_t i = 0; i < scene->entity_count; i++) {
		const struct entity *entity = scene->entities[i];

		// Render all meshes.
		for (size_t i = 0; i < entity->model->meshes_count; i++) {
			struct mesh *mesh = entity->model->meshes[i];

			// Render mesh.
			render_mesh(renderer, camera, scene, entity, mesh);
		}
	}

	// Render the skybox.
	// This is done last so that only the fragments that aren't hiding it gets computed.
	// The shader is written such that the depth buffer is always 1.0 (the furtest away).
	render_skybox(renderer, camera, scene);

	// Track FPS.
	track_fps(renderer);

	// Render the UI.
	ui_render(renderer->ui);

	// Swap front and back buffers.
	glfwSwapBuffers(renderer->window->glfw_window);
}

void renderer_wireframe(struct renderer *renderer, bool enabled) {
	renderer->wireframe = enabled;
	renderer_switch(NULL);
}