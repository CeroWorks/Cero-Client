#ifndef CERO_LAUNCH_CERO_AGENT_H
#define CERO_LAUNCH_CERO_AGENT_H

#include <stddef.h>

/* Extracts the embedded CeroClient-MC.jar agent to
 * client_dir/agent/CeroClient-MC.jar, filling out_path. Returns 1 if the
 * agent was extracted (vanilla launch otherwise), 0 if unavailable. */
int extract_cero_agent(const char* client_dir, char* out_path, size_t out_path_sz);

#endif /* CERO_LAUNCH_CERO_AGENT_H */
