#ifndef LAYMAN_SHADER_H
#define LAYMAN_SHADER_H

// TODO: Documentation.
struct layman_shader *layman_shader_load_from_file(const char *vertex_filepath, const char *fragment_filepath);

// TODO: Documentation.
void layman_shader_destroy(struct layman_shader *shader);

// TODO: Documentation.
void layman_shader_use(const struct layman_shader *shader);

// TODO: Documentation.
void layman_shader_unuse(const struct layman_shader *shader);

#endif
