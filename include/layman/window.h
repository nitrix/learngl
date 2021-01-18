#ifndef LAYMAN_WINDOW_H
#define LAYMAN_WINDOW_H

#include <stdbool.h>

/**
 * @brief Creates a new window.
 *
 * @param[in] width The width of the window.
 * @param[in] height The height of the window.
 * @param[in] title The title of the window.
 *
 * @remark UTF-8 titles are supported.
 * @remark [Thread safety] This function must only be called from the main thread.
 * 
 * @return A pointer to the newly created window or NULL on error.
 */
struct layman_window *layman_window_create(int width, int height, const char *title);

/**
 * @brief Destroys a window.
 *
 * @param window A pointer to the window to destroy.
 *
 * @remark [Thread safety] This function must only be called from the main thread.
 * @remark [Reentrancy] This function must not be called from a callback.
 */
void layman_window_destroy(struct layman_window *window);

/**
 * @brief Closes a window.
 *
 * @param window A pointer to the window to close.
 *
 * @remark Nothing happens if the window was already closed.
 */
void layman_window_close(const struct layman_window *window);

/**
 * @brief A way to check whether a window has been closed.
 *
 * @param window A pointer to the window.
 *
 * @return true if the window has been closed or false otherwise.
*/
bool layman_window_closed(const struct layman_window *window);

/**
 * @brief Poll for input events.
 * 
 * The events are handled by callbacks that must have been previously configured.
 * In case they weren't configured, dummy callbacks are used as a fallback to discard the events.
 * 
 * @param window A pointer to the window.
 * @param custom Custom user pointer that is passed back to the user during event callbacks.
 */
void layman_window_poll_events(const struct layman_window *window, void *custom);

/**
 * @brief Refreshes the window.
 * 
 * Due to double-buffering, things drawn to the window aren't shown immediately.
 * A window refresh is necessary to switch between those the two (active/inactive) buffers.
 * 
 * @param window A pointer to the window.
 */
void layman_window_refresh(const struct layman_window *window);

// TODO: Document these.
void layman_window_use(const struct layman_window *window);
void layman_window_unuse(const struct layman_window *window);

#endif