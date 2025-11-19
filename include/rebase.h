#ifndef REBASE_H
#define REBASE_H

/**
 * @brief Runs the guided, menu-driven interactive rebase process.
 * This function orchestrates the history rewriting using a simple terminal UI.
 * * @param target_ref The reference (branch name or SHA) to rebase onto.
 * @return 0 on success, -1 on failure.
 */
int do_rebase_interactive(const char *target_ref);

#endif // REBASE_H
